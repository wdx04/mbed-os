/*
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include "hal_data.h"

FSP_CPP_HEADER
void R_BSP_WarmStart(bsp_warm_start_event_t event);

FSP_CPP_FOOTER

/* SDRAM pin configuration diagnostics, printed by the application. */
#if defined(TARGET_RA8P1_TITAN_MINI) && BSP_CFG_SDRAM_ENABLED
volatile int  g_sdram_pin_fails    = -1;   /* -1: check not run yet */
volatile int  g_sdram_pin_first_bad_index = -1;
volatile uint32_t g_sdram_pin_bad_pfs     = 0;
volatile uint32_t g_sdram_pin_bad_pin     = 0;
volatile int  g_sdram_pin_retry_ok = -1;   /* -1: no retry needed/attempted */
volatile int  g_sdram_pin_fails_after_cache_flush = -2; /* -2: not attempted */

static uint32_t sdram_pin_pfs_read(bsp_io_port_pin_t pin)
{
    return R_PFS->PORT[pin >> 8U].PIN[pin & 0xFFU].PmnPFS;
}

static int sdram_pins_verify(bool repair)
{
    int fails = 0;

    for (uint32_t i = 0; i < g_bsp_pin_cfg.number_of_pins; i++)
    {
        bsp_io_port_pin_t pin  = g_bsp_pin_cfg.p_pin_cfg_data[i].pin;
        uint32_t          want = g_bsp_pin_cfg.p_pin_cfg_data[i].pin_cfg;
        /* Compare the writable function fields: PSEL, PMR, DSCR. */
        const uint32_t mask = (R_PFS_PORT_PIN_PmnPFS_PSEL_Msk |
                               R_PFS_PORT_PIN_PmnPFS_PMR_Msk  |
                               R_PFS_PORT_PIN_PmnPFS_DSCR_Msk);

        if ((sdram_pin_pfs_read(pin) & mask) != (want & mask))
        {
            if (0 == fails)
            {
                g_sdram_pin_first_bad_index = (int) i;
                g_sdram_pin_bad_pin         = (uint32_t) pin;
                g_sdram_pin_bad_pfs         = sdram_pin_pfs_read(pin);
            }
            fails++;

            if (repair)
            {
                R_IOPORT_PinCfg(NULL, pin, want);
                __DSB();
                __ISB();
            }
        }
    }

    return fails;
}

static void sdram_pins_verify_and_repair(void)
{
    g_sdram_pin_fails = sdram_pins_verify(true);

    /* If the first verification pass saw everything fine but a cache line
     * hiding a lost register write is suspected, flush the D-cache and
     * check again against the real hardware state. */
    SCB_CleanInvalidateDCache();
    __DSB();
    __ISB();
    g_sdram_pin_fails_after_cache_flush = sdram_pins_verify(true);
    g_sdram_pin_retry_ok = (0 == g_sdram_pin_fails_after_cache_flush) ? 1 : 0;
}
#endif

/*******************************************************************************************************************//**
 * This function is called at various points during the startup process.  This implementation uses the event that is
 * called right before main() to set up the pins.
 *
 * @param[in]  event    Where at in the start up process the code is currently at
 **********************************************************************************************************************/
void R_BSP_WarmStart (bsp_warm_start_event_t event)
{
    if (BSP_WARM_START_RESET == event)
    {
#if BSP_FEATURE_FLASH_LP_VERSION != 0

        /* Enable reading from data flash. */
        R_FACI_LP->DFLCTL = 1U;

        /* Would normally have to wait tDSTOP(6us) for data flash recovery. Placing the enable here, before clock and
         * C runtime initialization, should negate the need for a delay since the initialization will typically take more than 6us. */
#endif
    }

#if BSP_CFG_OSPI_B_STARTUP_ENABLED && defined(BSP_CFG_OSPI_B_STARTUP_FN)
    if (BSP_WARM_START_POST_CLOCK == event)
    {
        /* Setup OSPI_B SiP flash and initialize it. */
        R_BSP_OspiBInit(BSP_CFG_OSPI_B_STARTUP_FN, true);
    }
#endif

    if (BSP_WARM_START_POST_C == event)
    {
        /* C runtime environment and system clocks are setup. */

        /* Configure pins. */
        R_IOPORT_Open(&IOPORT_CFG_CTRL, &IOPORT_CFG_NAME);

#if defined(TARGET_RA8P1_TITAN_MINI) && BSP_CFG_SDRAM_ENABLED
        /* Verify that every SDRAM pin actually took effect and repair the
         * ones that did not before initializing the SDRAM (the mode register
         * write is only captured when CKE is properly muxed). */
        sdram_pins_verify_and_repair();

        /* Setup SDRAM and initialize it. Must configure pins first. */
        R_BSP_SdramInit(true);
#endif
    }
}
