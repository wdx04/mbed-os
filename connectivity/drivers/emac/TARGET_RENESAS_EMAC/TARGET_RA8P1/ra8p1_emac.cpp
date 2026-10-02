/* RA8P1 EMAC driver for the RT-Thread RA8P1 Titan Mini (and other RA8P1
 * boards with the same wiring). The RA8P1 gigabit Ethernet MAC is the RMAC
 * on top of the layer-3 switch (ESW) IP, driven through the FSP r_rmac /
 * r_layer3_switch / r_rmac_phy drivers. The board's PHY is a Realtek
 * RTL8211F-CG connected over RGMII on Ethernet channel 1 (MDIO address 1).
 *
 * Ported from the RT-Thread Titan Mini driver_eth example flow:
 *  - RX: zerocopy R_RMAC_Read returns driver-owned buffers; copied into
 *    lwIP pbufs and recycled with R_RMAC_BufferRelease.
 *  - TX: private pool of non-cacheable DMA buffers, recycled on
 *    ETHER_EVENT_TX_COMPLETE through R_RMAC_TxStatusGet.
 *  - All DMA-visible memory (descriptors, buffer nodes, buffers) lives in
 *    the .ram_nocache section (MPU non-cacheable range, see RA8P1.ld).
 *
 * Pins (Titan Mini schematic):
 *   P2_6  RGMII1_RX_CTL      P9_5  RGMII1_RXC
 *   P3_4  RGMII1_TXD3        P9_6  RGMII1_RXD0
 *   P3_5  RGMII1_TXD2        P9_7  RGMII1_RXD1
 *   P3_6  RGMII1_TXD1        P9_8  RGMII1_RXD2
 *   P3_7  RGMII1_TXD0        P9_9  RGMII1_RXD3
 *   P3_9  RGMII1_TXC         P12_11 ET0_MDC
 *   P3_10 RGMII1_TX_CTL      P12_12 ET0_MDIO
 *   P7_7  PHY reset (GPIO, active low, net ET1_RESET)
 */
#include "ra8p1_emac.h"
#include "r_ioport.h"
#include "rtos/ThisThread.h"
#include "rtos/Kernel.h"
#include <string.h>
#include <chrono>

using namespace std::chrono_literals;

extern "C" void ra8p1_emac_queues_init(void);

/* ------------------------------------------------------------------ */
/* RTL8211F PHY target hooks (adapted from the official RT-Thread BSP) */

extern "C" void rmac_phy_target_rtl8211_initialize(rmac_phy_instance_ctrl_t *phydev)
{
#define RTL_8211F_PAGE_SELECT 0x1F
#define RTL_8211F_EEELCR_ADDR 0x11
#define RTL_8211F_LED_PAGE    0xD04
#define RTL_8211F_LCR_ADDR    0x10

    uint32_t val1, val2 = 0;

    /* switch to led page */
    R_RMAC_PHY_Write(phydev, RTL_8211F_PAGE_SELECT, RTL_8211F_LED_PAGE);

    /* led1(green) Link 10/100/1000M, led2(yellow) Link 10/100/1000M+Active */
    R_RMAC_PHY_Read(phydev, RTL_8211F_LCR_ADDR, &val1);
    val1 |= (1 << 5);
    val1 |= (1 << 8);
    val1 &= (~(1 << 9));
    val1 |= (1 << 10);
    val1 |= (1 << 11);
    R_RMAC_PHY_Write(phydev, RTL_8211F_LCR_ADDR, val1);

    /* disable the EEE LED function so led1 stays on when linked */
    R_RMAC_PHY_Read(phydev, RTL_8211F_EEELCR_ADDR, &val2);
    val2 &= (~(1 << 2));
    R_RMAC_PHY_Write(phydev, RTL_8211F_EEELCR_ADDR, val2);

    /* switch back to page0 */
    R_RMAC_PHY_Write(phydev, RTL_8211F_PAGE_SELECT, 0xa42);

#undef RTL_8211F_PAGE_SELECT
#undef RTL_8211F_EEELCR_ADDR
#undef RTL_8211F_LED_PAGE
#undef RTL_8211F_LCR_ADDR
}

extern "C" bool rmac_phy_target_rtl8211_is_support_link_partner_ability(
    rmac_phy_instance_ctrl_t *p_instance_ctrl, uint32_t line_speed_duplex)
{
    (void)p_instance_ctrl;
    (void)line_speed_duplex;
    /* This PHY-LSI supports half and full duplex mode. */
    return true;
}

/* ------------------------------------------------------------------ */
/* DMA-visible memory, all in the non-cacheable RAM region             */

#define NOCACHE __attribute__((section(".ram_nocache"), aligned(32)))

static layer3_switch_ts_reception_process_descriptor_t
    s_ts_descriptors[8] NOCACHE;
static layer3_switch_descriptor_t
    s_tx_descriptors[2][RA8P1_ETH_TX_DESC_PER_QUEUE] NOCACHE;
static layer3_switch_descriptor_t
    s_rx_descriptors[2][RA8P1_ETH_RX_DESC_PER_QUEUE] NOCACHE;
static rmac_buffer_node_t
    s_buffer_nodes[RA8P1_ETH_RX_BUFFERS] NOCACHE;
static uint8_t
    s_ether_buffers[RA8P1_ETH_RX_BUFFERS][RA8P1_ETH_BUFFER_SIZE] NOCACHE;
static uint8_t
    s_tx_pool[RA8P1_ETH_TX_POOL][RA8P1_ETH_BUFFER_SIZE] NOCACHE;

static uint8_t *s_pp_ether_buffers[RA8P1_ETH_RX_BUFFERS];

/* Queue info lists are written back by the driver, keep them writable */
static rmac_queue_info_t s_ts_queue[1];
static rmac_queue_info_t s_tx_queue_list[2];
static rmac_queue_info_t s_rx_queue_list[2];

/* ------------------------------------------------------------------ */
/* FSP instances                                                       */

rmac_instance_ctrl_t g_ra8p1_ether0_ctrl;
rmac_phy_instance_ctrl_t g_ra8p1_rmac_phy0_ctrl;
rmac_phy_instance_ctrl_t g_ra8p1_rmac_phy1_ctrl;
/* The layer3 switch control block contains the LINKFIX descriptor table that
 * the GWCA DMA reads over AXI (GWDCBAC0/1 point here). It must therefore be
 * non-cacheable, or the GWCA sees stale RAM and follows a null descriptor
 * (AXI error GWEIS0.AES, all queues stall). */
layer3_switch_instance_ctrl_t g_ra8p1_layer3_switch0_ctrl NOCACHE;

static uint8_t s_mac_address_port0[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
static uint8_t s_mac_address_port1[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
static uint8_t s_mac_address[6]       = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

static layer3_switch_l3_filter_t s_l3_filter_list[10];
static layer3_switch_cbs_cfg_t s_cbs_cfg_zero;

/* On the Titan Mini the single RTL8211F (MDIO address 1) is reached over
 * the ET0 management bus (RMAC0). The layer-3 switch routes its RGMII data
 * through port 1. The FSP models this with two PHY driver instances that
 * share the same PHY: instance 0 configures the MDIO/RMAC0 side (LSI slot
 * 0), instance 1 configures the RGMII/port-1 side (LSI slot 1). Both PHY
 * cfgs therefore use channel 0 (the management channel), exactly like the
 * official generated configuration. */
static const ether_phy_lsi_cfg_t s_phy_lsi_cfg0 =
{
    .address = 1,
    .type = ETHER_PHY_LSI_TYPE_CUSTOM,
};

static const ether_phy_lsi_cfg_t s_phy_lsi_cfg1 =
{
    .address = 1,
    .type = ETHER_PHY_LSI_TYPE_CUSTOM,
};

static const rmac_phy_extended_cfg_t s_phy0_extended_cfg =
{
    .p_target_init = rmac_phy_target_rtl8211_initialize,
    .p_target_link_partner_ability_get = rmac_phy_target_rtl8211_is_support_link_partner_ability,
    .frame_format = RMAC_PHY_FRAME_FORMAT_MDIO,
    .mdc_clock_rate = 2500000,
    .mdio_hold_time = 0,
    .mdio_capture_time = 0,
    .p_phy_lsi_cfg_list = { &s_phy_lsi_cfg0, NULL },
    .default_phy_lsi_cfg_index = 0,
    .frame_preemption_enable = false,
    .frame_preemption_verification_interval = 10,
    .easi_irq =
    {
        FSP_INVALID_VECTOR,
        FSP_INVALID_VECTOR,
    },
    .easi_ipl =
    {
        BSP_IRQ_DISABLED,
        BSP_IRQ_DISABLED,
    },
    .p_callback = NULL,
    .p_context = NULL,
};

static const rmac_phy_extended_cfg_t s_phy1_extended_cfg =
{
    .p_target_init = rmac_phy_target_rtl8211_initialize,
    .p_target_link_partner_ability_get = rmac_phy_target_rtl8211_is_support_link_partner_ability,
    .frame_format = RMAC_PHY_FRAME_FORMAT_MDIO,
    .mdc_clock_rate = 2500000,
    .mdio_hold_time = 0,
    .mdio_capture_time = 0,
    .p_phy_lsi_cfg_list = { NULL, &s_phy_lsi_cfg1 },
    .default_phy_lsi_cfg_index = 1,
    .frame_preemption_enable = false,
    .frame_preemption_verification_interval = 10,
    .easi_irq =
    {
        FSP_INVALID_VECTOR,
        FSP_INVALID_VECTOR,
    },
    .easi_ipl =
    {
        BSP_IRQ_DISABLED,
        BSP_IRQ_DISABLED,
    },
    .p_callback = NULL,
    .p_context = NULL,
};

static const ether_phy_cfg_t s_phy0_cfg =
{
    .channel = 0,
    .phy_reset_wait_time = 0x00020000,
    .mii_bit_access_wait_time = 0,
    .flow_control = ETHER_PHY_FLOW_CONTROL_DISABLE,
    .mii_type = ETHER_PHY_MII_TYPE_RGMII,
    .p_context = NULL,
    .p_extend = &s_phy0_extended_cfg,
};

static const ether_phy_cfg_t s_phy1_cfg =
{
    .channel = 0,
    .phy_reset_wait_time = 0x00020000,
    .mii_bit_access_wait_time = 0,
    .flow_control = ETHER_PHY_FLOW_CONTROL_DISABLE,
    .mii_type = ETHER_PHY_MII_TYPE_RGMII,
    .p_context = NULL,
    .p_extend = &s_phy1_extended_cfg,
};

const ether_phy_instance_t g_ra8p1_rmac_phy0 =
{
    .p_ctrl = &g_ra8p1_rmac_phy0_ctrl,
    .p_cfg = &s_phy0_cfg,
    .p_api = &g_ether_phy_on_rmac_phy,
};

const ether_phy_instance_t g_ra8p1_rmac_phy1 =
{
    .p_ctrl = &g_ra8p1_rmac_phy1_ctrl,
    .p_cfg = &s_phy1_cfg,
    .p_api = &g_ether_phy_on_rmac_phy,
};

static layer3_switch_port_cfg_t s_port0_cfg =
{
    .p_cbs_cfg = &s_cbs_cfg_zero,
    .forwarding_to_cpu_enable = true,
    .p_mac_address = s_mac_address_port0,
    .p_callback = NULL,
    .p_callback_memory = NULL,
    .p_context = NULL,
};

static layer3_switch_port_cfg_t s_port1_cfg =
{
    .p_cbs_cfg = &s_cbs_cfg_zero,
    .forwarding_to_cpu_enable = true,
    .p_mac_address = s_mac_address_port1,
    .p_callback = NULL,
    .p_callback_memory = NULL,
    .p_context = NULL,
};

static const layer3_switch_extended_cfg_t s_switch_extended_cfg =
{
    /* Titan Mini has a single PHY on port 1 (RGMII1); port 0 is unused. */
    .p_ether_phy_instances = { &g_ra8p1_rmac_phy0, &g_ra8p1_rmac_phy1 },
    .p_gptp_instance = NULL,
    .fowarding_target_port_masks =
    {
        (LAYER3_SWITCH_PORT_BITMASK_PORT2 | 0U),
        (LAYER3_SWITCH_PORT_BITMASK_PORT2 | 0U),
        (0U),
    },
    .p_mac_addresses =
    {
        s_mac_address_port0,
        s_mac_address_port1,
    },
    .ipv_queue_depth_list =
    {
        { 64, 64, 64, 64, 64, 64, 64, 64 },
        { 64, 64, 64, 64, 64, 64, 64, 64 },
    },
    .ipv_queue_preemptable_bitmask =
    {
        (0U),
        (0U),
    },
    .frame_preemption_fragment_size =
    {
        LAYER3_SWITCH_PREEMPTABLE_FRAME_FRAGMENT_SIZE_64BYTE,
        LAYER3_SWITCH_PREEMPTABLE_FRAME_FRAGMENT_SIZE_64BYTE,
    },
    .l3_filter_list = s_l3_filter_list,
    .l3_filter_list_length = 10,
    .p_port_cfg_list = { &s_port0_cfg, &s_port1_cfg },
    .etha_error_irq_port_0 = VECTOR_NUMBER_ETHER_EAEI0,
    .etha_error_irq_port_1 = VECTOR_NUMBER_ETHER_EAEI1,
    .etha_error_ipl_port_0 = (12),
    .etha_error_ipl_port_1 = (12),
    .gptp_timer_numbers = {0, 1},
};

static const ether_switch_cfg_t s_switch_cfg =
{
    .channel = 0,
    .irq = VECTOR_NUMBER_ETHER_GWDI0,
    .ipl = (12),
    .p_callback = NULL,
    .p_context = NULL,
    .p_extend = &s_switch_extended_cfg,
};

const ether_switch_instance_t g_ra8p1_layer3_switch0 =
{
    .p_ctrl = &g_ra8p1_layer3_switch0_ctrl,
    .p_cfg = &s_switch_cfg,
    .p_api = &g_ether_switch_on_layer3_switch,
};

static const rmac_extended_cfg_t s_rmac_extended_cfg =
{
    .p_ether_switch = &g_ra8p1_layer3_switch0,
    .tx_queue_num = 2,
    .rx_queue_num = 2,
    .p_ts_queue = s_ts_queue,
    .p_tx_queue_list = s_tx_queue_list,
    .p_rx_queue_list = s_rx_queue_list,
    .rmpi_irq = FSP_INVALID_VECTOR,
    .rmpi_ipl = BSP_IRQ_DISABLED,
    .p_buffer_node_list = s_buffer_nodes,
    .buffer_node_num = RA8P1_ETH_RX_BUFFERS,
    .transmission_descriptor_format = RMAC_TRANSMISSION_DESCRIPTOR_FORMAT_DIRECT,
};

static const ether_cfg_t s_ether_cfg =
{
    .channel = 1,
    .zerocopy = ETHER_ZEROCOPY_ENABLE,
    .multicast = ETHER_MULTICAST_ENABLE,
    .promiscuous = ETHER_PROMISCUOUS_DISABLE,
    .flow_control = ETHER_FLOW_CONTROL_DISABLE,
    .padding = ETHER_PADDING_DISABLE,
    .padding_offset = 0,
    .broadcast_filter = 0,
    .p_mac_address = s_mac_address,
    .num_tx_descriptors = RA8P1_ETH_TX_DESC_PER_QUEUE,
    .num_rx_descriptors = RA8P1_ETH_RX_DESC_PER_QUEUE,
    .pp_ether_buffers = s_pp_ether_buffers,
    .ether_buffer_size = RA8P1_ETH_BUFFER_SIZE,
    .irq = FSP_INVALID_VECTOR,
    .interrupt_priority = 12,
    .p_callback = RA8P1_EMAC::ether_isr_cb,
    .p_context = NULL,
    .p_extend = &s_rmac_extended_cfg,
};

const ether_instance_t g_ra8p1_ether0 =
{
    .p_ctrl = &g_ra8p1_ether0_ctrl,
    .p_cfg = &s_ether_cfg,
    .p_api = &g_ether_on_rmac,
};

/* ------------------------------------------------------------------ */
/* TX buffer pool                                                      */

static volatile uint32_t s_tx_busy_mask;
static uint32_t s_tx_alloc_index;

static void tx_reclaim_completed()
{
    uint8_t *released = NULL;
    while (R_RMAC_TxStatusGet(&g_ra8p1_ether0_ctrl, &released) == FSP_SUCCESS)
    {
        uintptr_t base = (uintptr_t)&s_tx_pool[0][0];
        uintptr_t addr = (uintptr_t)released;
        uintptr_t offset = addr - base;
        if (addr >= base && offset < sizeof(s_tx_pool) &&
                (offset % RA8P1_ETH_BUFFER_SIZE) == 0)
        {
            s_tx_busy_mask &= ~(1U << (offset / RA8P1_ETH_BUFFER_SIZE));
        }
    }
}

static uint8_t *tx_acquire()
{
    tx_reclaim_completed();
    for (uint32_t count = 0; count < RA8P1_ETH_TX_POOL; count++)
    {
        uint32_t idx = (s_tx_alloc_index + count) % RA8P1_ETH_TX_POOL;
        if ((s_tx_busy_mask & (1U << idx)) == 0)
        {
            s_tx_busy_mask |= (1U << idx);
            s_tx_alloc_index = (idx + 1) % RA8P1_ETH_TX_POOL;
            return s_tx_pool[idx];
        }
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/* EMAC implementation                                                 */

RA8P1_EMAC::RA8P1_EMAC()
    : _memory_manager(nullptr), _rx_sem(0, 0xFFFFFFFF), _rx_thread(nullptr)
{
    _link_state = false;
    _powered_up = false;
    memset(_mac_addr, 0, sizeof(_mac_addr));
}

bool RA8P1_EMAC::get_hwaddr(uint8_t *addr) const
{
    memcpy(addr, s_mac_address, 6);
    return true;
}

void RA8P1_EMAC::set_hwaddr(const uint8_t *addr)
{
    memcpy(s_mac_address, addr, 6);
    memcpy(s_mac_address_port1, addr, 6);
}

bool RA8P1_EMAC::configure_pins()
{
    /* PHY reset (P7_7, net ET1_RESET, active low). The PHY also has an
     * RC power-on reset on board, this pulse just makes it deterministic. */
    R_IOPORT_PinCfg(NULL, BSP_IO_PORT_07_PIN_07,
                    IOPORT_CFG_PORT_DIRECTION_OUTPUT | IOPORT_CFG_DRIVE_HIGH);
    R_IOPORT_PinWrite(NULL, BSP_IO_PORT_07_PIN_07, BSP_IO_LEVEL_LOW);
    rtos::ThisThread::sleep_for(50ms);
    R_IOPORT_PinWrite(NULL, BSP_IO_PORT_07_PIN_07, BSP_IO_LEVEL_HIGH);
    rtos::ThisThread::sleep_for(100ms);

    /* RGMII signal pins (drive settings from the official Titan Mini FSP
     * pin configuration; P3_9 TXC is the only middle-drive pin). */
    static const struct
    {
        bsp_io_port_pin_t pin;
        uint32_t drive;
    } rgmii_pins[] =
    {
        { BSP_IO_PORT_02_PIN_06, IOPORT_CFG_DRIVE_HIGH }, /* RX_CTL   */
        { BSP_IO_PORT_03_PIN_04, IOPORT_CFG_DRIVE_HIGH }, /* TXD3     */
        { BSP_IO_PORT_03_PIN_05, IOPORT_CFG_DRIVE_HIGH }, /* TXD2     */
        { BSP_IO_PORT_03_PIN_06, IOPORT_CFG_DRIVE_HIGH }, /* TXD1     */
        { BSP_IO_PORT_03_PIN_07, IOPORT_CFG_DRIVE_HIGH }, /* TXD0     */
        { BSP_IO_PORT_03_PIN_09, IOPORT_CFG_DRIVE_MID  }, /* TXC      */
        { BSP_IO_PORT_03_PIN_10, IOPORT_CFG_DRIVE_HIGH }, /* TX_CTL   */
        { BSP_IO_PORT_09_PIN_05, IOPORT_CFG_DRIVE_HIGH }, /* RXC      */
        { BSP_IO_PORT_09_PIN_06, IOPORT_CFG_DRIVE_HIGH }, /* RXD0     */
        { BSP_IO_PORT_09_PIN_07, IOPORT_CFG_DRIVE_HIGH }, /* RXD1     */
        { BSP_IO_PORT_09_PIN_08, IOPORT_CFG_DRIVE_HIGH }, /* RXD2     */
        { BSP_IO_PORT_09_PIN_09, IOPORT_CFG_DRIVE_HIGH }, /* RXD3     */
    };
    for (size_t i = 0; i < sizeof(rgmii_pins) / sizeof(rgmii_pins[0]); i++)
    {
        R_IOPORT_PinCfg(NULL, rgmii_pins[i].pin,
                        IOPORT_CFG_PERIPHERAL_PIN | rgmii_pins[i].drive |
                        IOPORT_PERIPHERAL_ETHER_RGMII);
    }
    return true;
}

/* MDIO/MDC sit on P12_12/P12_11 (schematic nets ET0_MDIO/ET0_MDC) and are
 * muxed with the ETHER_RGMII peripheral select - exactly the two pins the
 * official Titan Mini FSP pin configuration sets up. The MDIO master only
 * becomes usable once the layer-3 switch driver has started the ESW module
 * clocks, so the mux is verified with a full R_RMAC_Open per candidate PSEL
 * (opening the PHY driver standalone fails with FSP_ERR_INVALID_MODE
 * because the ETHA IP is unclocked). The first candidate that opens
 * successfully wins and stays open. */
bool RA8P1_EMAC::mdio_psel_probe()
{
    fsp_err_t err = FSP_ERR_NOT_INITIALIZED;

    /* The official Titan Mini FSP pin configuration routes P12_11/P12_12
     * to the ETHER_RGMII peripheral (PSEL 0x18) with high drive. Try that
     * known-good configuration first. */
    static const uint32_t order[] = {0x18};
    const uint32_t *p_order = order;
    uint32_t order_count = sizeof(order) / sizeof(order[0]);

    for (uint32_t i = 0; i < order_count; i++)
    {
        uint32_t psel = p_order[i];
        uint32_t psel_cfg = psel << IOPORT_PRV_PFS_PSEL_OFFSET;
        R_IOPORT_PinCfg(NULL, BSP_IO_PORT_12_PIN_12,
                        IOPORT_CFG_PERIPHERAL_PIN | IOPORT_CFG_DRIVE_HIGH | psel_cfg);
        R_IOPORT_PinCfg(NULL, BSP_IO_PORT_12_PIN_11,
                        IOPORT_CFG_PERIPHERAL_PIN | IOPORT_CFG_DRIVE_HIGH | psel_cfg);

        memset(&g_ra8p1_ether0_ctrl, 0, sizeof(g_ra8p1_ether0_ctrl));
        memset(&g_ra8p1_rmac_phy0_ctrl, 0, sizeof(g_ra8p1_rmac_phy0_ctrl));
        memset(&g_ra8p1_layer3_switch0_ctrl, 0, sizeof(g_ra8p1_layer3_switch0_ctrl));
        ra8p1_emac_queues_init();

        err = R_RMAC_Open(&g_ra8p1_ether0_ctrl, &s_ether_cfg);
        if (FSP_SUCCESS == err)
        {
            uint32_t id1 = 0, id2 = 0;
            R_RMAC_PHY_Read(&g_ra8p1_rmac_phy0_ctrl, 2, &id1);
            R_RMAC_PHY_Read(&g_ra8p1_rmac_phy0_ctrl, 3, &id2);
            /* RTL8211F: PHYID1 = 0x001C (Realtek OUI). An all-zero or
             * all-ones ID means the MDIO bus is dead (timeouts read back
             * garbage), keep searching. */
            if (id1 == 0x001C)
            {
                return true;
            }
        }
        else
        {
            printf("ETH: MDIO PSEL 0x%02lx open failed: %d\r\n", (unsigned long)psel, (int)err);
        }
        R_RMAC_Close(&g_ra8p1_ether0_ctrl);
    }
    return false;
}

bool RA8P1_EMAC::power_up()
{
    if (_powered_up)
    {
        return true;
    }

    configure_pins();
    if (!mdio_psel_probe())
    {
        printf("ETH: R_RMAC_Open failed for every MDIO PSEL candidate\r\n");
        return false;
    }

    _rx_thread = new rtos::Thread(osPriorityAboveNormal, 2048, nullptr, "eth_rx");
    _rx_thread->start(mbed::callback(this, &RA8P1_EMAC::rx_thread));

    _powered_up = true;
    return true;
}

void RA8P1_EMAC::power_down()
{
    if (!_powered_up)
    {
        return;
    }
    R_RMAC_Close(&g_ra8p1_ether0_ctrl);
    if (_rx_thread)
    {
        _rx_thread->terminate();
        delete _rx_thread;
        _rx_thread = nullptr;
    }
    _powered_up = false;
}

void RA8P1_EMAC::ether_callback(ether_callback_args_t *p_args)
{
    RA8P1_EMAC &self = get_instance();
    switch (p_args->event)
    {
        case ETHER_EVENT_RX_COMPLETE:
        case ETHER_EVENT_RX_MESSAGE_LOST:
            self._rx_sem.release();
            break;
        case ETHER_EVENT_TX_COMPLETE:
            tx_reclaim_completed();
            break;
        default:
            break;
    }
}

void RA8P1_EMAC::rx_thread()
{
    auto next_poll = rtos::Kernel::Clock::now() + 200ms;
    while (true)
    {
        bool got_rx = _rx_sem.try_acquire_for(50ms);

        if (rtos::Kernel::Clock::now() >= next_poll)
        {
            phy_poll();
            next_poll = rtos::Kernel::Clock::now() + 200ms;
        }

        if (!got_rx || !_link_state)
        {
            continue;
        }

        uint32_t len = RA8P1_ETH_BUFFER_SIZE;
        uint8_t *rx_buffer = NULL;
        NVIC_DisableIRQ((IRQn_Type)VECTOR_NUMBER_ETHER_GWDI0);
        fsp_err_t err = R_RMAC_Read(&g_ra8p1_ether0_ctrl, &rx_buffer, &len);
        NVIC_EnableIRQ((IRQn_Type)VECTOR_NUMBER_ETHER_GWDI0);
        while (err == FSP_SUCCESS && rx_buffer != NULL && len > 0)
        {
            if (_memory_manager != nullptr && _emac_input_data_cb)
            {
                emac_mem_buf_t *p = _memory_manager->alloc_pool(len, 0);
                if (p != nullptr)
                {
                    _memory_manager->copy_to_buf(p, rx_buffer, len);
                    _emac_input_data_cb(p);
                }
            }
            NVIC_DisableIRQ((IRQn_Type)VECTOR_NUMBER_ETHER_GWDI0);
            R_RMAC_BufferRelease(&g_ra8p1_ether0_ctrl);

            len = RA8P1_ETH_BUFFER_SIZE;
            rx_buffer = NULL;
            err = R_RMAC_Read(&g_ra8p1_ether0_ctrl, &rx_buffer, &len);
            NVIC_EnableIRQ((IRQn_Type)VECTOR_NUMBER_ETHER_GWDI0);
        }
    }
}

void RA8P1_EMAC::phy_poll()
{
    fsp_err_t err = R_RMAC_LinkProcess(&g_ra8p1_ether0_ctrl);
    if (err != FSP_SUCCESS)
    {
        return;
    }

    bool new_state =
        (g_ra8p1_ether0_ctrl.link_establish_status == ETHER_LINK_ESTABLISH_STATUS_UP);
    if (new_state != _link_state)
    {
        _link_state = new_state;
        if (new_state)
        {
            uint32_t speed = 0, local_pause = 0, partner_pause = 0;
            if (FSP_SUCCESS == R_RMAC_PHY_LinkPartnerAbilityGet(
                    &g_ra8p1_rmac_phy0_ctrl, &speed, &local_pause, &partner_pause))
            {
                printf("ETH: link up (speed/duplex code %lu)\r\n", (unsigned long)speed);
            }
            else
            {
                printf("ETH: link up\r\n");
            }
        }
        else
        {
            printf("ETH: link down\r\n");
        }
        if (_emac_link_state_cb)
        {
            _emac_link_state_cb(new_state);
        }
    }
}

bool RA8P1_EMAC::link_out(emac_mem_buf_t *buf)
{
    if (buf == nullptr || _memory_manager == nullptr || !_powered_up)
    {
        return false;
    }

    uint8_t *frame = tx_acquire();
    if (frame == NULL)
    {
        _memory_manager->free(buf);
        return false;
    }

    uint32_t offset = 0;
    emac_mem_buf_t *chain = buf;
    while (chain != nullptr && offset < RA8P1_ETH_BUFFER_SIZE)
    {
        uint8_t *ptr = (uint8_t *)_memory_manager->get_ptr(chain);
        uint32_t len = _memory_manager->get_len(chain);
        if (offset + len > RA8P1_ETH_BUFFER_SIZE)
        {
            len = RA8P1_ETH_BUFFER_SIZE - offset;
        }
        if (ptr != nullptr && len > 0)
        {
            memcpy(frame + offset, ptr, len);
        }
        offset += len;
        chain = _memory_manager->get_next(chain);
    }
    _memory_manager->free(buf);

    if (offset < 60)
    {
        memset(frame + offset, 0, 60 - offset);
        offset = 60;
    }

    fsp_err_t err = R_RMAC_Write(&g_ra8p1_ether0_ctrl, frame, offset);
    return err == FSP_SUCCESS;
}

/* Weak override: register as the default EMAC */
MBED_WEAK EMAC &EMAC::get_default_instance()
{
    return RA8P1_EMAC::get_instance();
}

/* Queue list initialization, run before R_RMAC_Open. Mirrors the FSP
 * generated configuration: one TS queue and two TX/RX queues each, all
 * bound to switch port 1 (RGMII1). */
extern "C" void ra8p1_emac_queues_init(void)
{
    for (uint32_t i = 0; i < RA8P1_ETH_RX_BUFFERS; i++)
    {
        s_pp_ether_buffers[i] = s_ether_buffers[i];
    }

    s_ts_queue[0].queue_cfg.array_length = 8;
    s_ts_queue[0].queue_cfg.p_descriptor_array = NULL;
    s_ts_queue[0].queue_cfg.p_ts_descriptor_array = s_ts_descriptors;
    s_ts_queue[0].queue_cfg.ports = (1 << 1);
    s_ts_queue[0].queue_cfg.type = LAYER3_SWITCH_QUEUE_TYPE_TX;
    s_ts_queue[0].queue_cfg.write_back_mode = LAYER3_SWITCH_WRITE_BACK_MODE_FULL;
    s_ts_queue[0].queue_cfg.descriptor_format = LAYER3_SWITCH_DISCRIPTOR_FORMTAT_TX_TIMESTAMP;
    s_ts_queue[0].queue_cfg.rx_timestamp_storage = LAYER3_SWITCH_RX_TIMESTAMP_STORAGE_DISABLE;

    for (uint32_t q = 0; q < 2; q++)
    {
        s_tx_queue_list[q].queue_cfg.array_length = RA8P1_ETH_TX_DESC_PER_QUEUE;
        s_tx_queue_list[q].queue_cfg.p_descriptor_array = s_tx_descriptors[q];
        s_tx_queue_list[q].queue_cfg.p_ts_descriptor_array = NULL;
        s_tx_queue_list[q].queue_cfg.ports = (1 << 1);
        s_tx_queue_list[q].queue_cfg.type = LAYER3_SWITCH_QUEUE_TYPE_TX;
        s_tx_queue_list[q].queue_cfg.write_back_mode = LAYER3_SWITCH_WRITE_BACK_MODE_FULL;
        s_tx_queue_list[q].queue_cfg.descriptor_format = LAYER3_SWITCH_DISCRIPTOR_FORMTAT_EXTENDED;
        s_tx_queue_list[q].queue_cfg.rx_timestamp_storage = LAYER3_SWITCH_RX_TIMESTAMP_STORAGE_DISABLE;

        s_rx_queue_list[q].queue_cfg.array_length = RA8P1_ETH_RX_DESC_PER_QUEUE;
        s_rx_queue_list[q].queue_cfg.p_descriptor_array = s_rx_descriptors[q];
        s_rx_queue_list[q].queue_cfg.p_ts_descriptor_array = NULL;
        s_rx_queue_list[q].queue_cfg.ports = (1 << 1);
        s_rx_queue_list[q].queue_cfg.type = LAYER3_SWITCH_QUEUE_TYPE_RX;
        s_rx_queue_list[q].queue_cfg.write_back_mode = LAYER3_SWITCH_WRITE_BACK_MODE_FULL;
        s_rx_queue_list[q].queue_cfg.descriptor_format = LAYER3_SWITCH_DISCRIPTOR_FORMTAT_EXTENDED;
        s_rx_queue_list[q].queue_cfg.rx_timestamp_storage = LAYER3_SWITCH_RX_TIMESTAMP_STORAGE_DISABLE;
    }
}
