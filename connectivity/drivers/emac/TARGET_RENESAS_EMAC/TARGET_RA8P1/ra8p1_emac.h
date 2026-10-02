#pragma once

#include "netsocket/EMAC.h"
#include "rtos/Thread.h"
#include "rtos/Semaphore.h"
#include "hal_data.h"
#include "r_rmac.h"
#include "r_layer3_switch.h"
#include "r_rmac_phy.h"

/* Driver resource sizing */
#define RA8P1_ETH_RX_BUFFERS        24  /* ether buffers / buffer nodes */
#define RA8P1_ETH_TX_DESC_PER_QUEUE 12
#define RA8P1_ETH_RX_DESC_PER_QUEUE 12
#define RA8P1_ETH_TX_POOL           8   /* driver-side TX DMA buffers */
#define RA8P1_ETH_BUFFER_SIZE       1536

class RA8P1_EMAC : public EMAC
{
public:
    static RA8P1_EMAC &get_instance()
    {
        static RA8P1_EMAC emac;
        return emac;
    }

    RA8P1_EMAC();

    uint32_t get_mtu_size() const override { return 1500; }
    uint32_t get_align_preference() const override { return 0; }
    void get_ifname(char *name, uint8_t size) const override
    {
        if(size > 0) name[0] = 'e';
        if(size > 1) name[1] = 'n';
        if(size > 2) name[2] = '\0';
    }
    uint8_t get_hwaddr_size() const override { return 6; }
    bool get_hwaddr(uint8_t *addr) const override;
    void set_hwaddr(const uint8_t *addr) override;

    bool link_out(emac_mem_buf_t *buf) override;
    bool power_up() override;
    void power_down() override;

    void set_link_input_cb(emac_link_input_cb_t input_cb) override
    {
        _emac_input_data_cb = input_cb;
    }
    void set_link_state_cb(emac_link_state_change_cb_t state_cb) override
    {
        _emac_link_state_cb = state_cb;
        if(_link_state && _emac_link_state_cb)
        {
            _emac_link_state_cb(true);
        }
    }
    void add_multicast_group(const uint8_t *address) override { (void)address; }
    void remove_multicast_group(const uint8_t *address) override { (void)address; }
    void set_all_multicast(bool all) override { (void)all; }
    void set_memory_manager(EMACMemoryManager &mem_mngr) override
    {
        _memory_manager = &mem_mngr;
    }

private:
    static void ether_callback(ether_callback_args_t *p_args);
    void rx_thread();
    void phy_poll();
    bool configure_pins();
    bool mdio_psel_probe();

public:
    static void ether_isr_cb(ether_callback_args_t *p_args) { ether_callback(p_args); }

    EMACMemoryManager *_memory_manager;
    emac_link_input_cb_t _emac_input_data_cb;
    emac_link_state_change_cb_t _emac_link_state_cb;
    bool _link_state;
    bool _powered_up;
    uint8_t _mac_addr[6];
    rtos::Thread *_rx_thread;
    rtos::Semaphore _rx_sem;
};

/* FSP Ethernet instances, defined in ra8p1_emac.cpp */
extern rmac_instance_ctrl_t g_ra8p1_ether0_ctrl;
extern rmac_phy_instance_ctrl_t g_ra8p1_rmac_phy0_ctrl;
extern layer3_switch_instance_ctrl_t g_ra8p1_layer3_switch0_ctrl;
extern const ether_instance_t g_ra8p1_ether0;
extern const ether_phy_instance_t g_ra8p1_rmac_phy0;
extern const ether_switch_instance_t g_ra8p1_layer3_switch0;
