/* generated HAL header file - do not edit */
#ifndef HAL_DATA_H_
#define HAL_DATA_H_
#include <stdint.h>
#include "bsp_api.h"
#include "common_data.h"
#include "r_crc.h"
#include "r_crc_api.h"
#include "r_sci_b_uart.h"
            #include "r_uart_api.h"
#include "r_dmac.h"
#include "r_transfer_api.h"
#include "r_ospi_b.h"
#include "r_spi_flash_api.h"
#include "r_dac_b.h"
#include "r_dac_api.h"
#include "r_adc_b.h"
                      #include "r_adc_api.h"
#include "r_canfd.h"
#include "r_can_api.h"
#include "r_iic_master.h"
#include "r_i2c_master_api.h"
#include "r_spi_b.h"
#include "r_gpt.h"
#include "r_timer_api.h"
#include "r_agt.h"
#include "r_timer_api.h"
#include "r_wdt.h"
#include "r_wdt_api.h"
#include "r_lpm.h"
#include "r_lpm_api.h"
#include "r_rtc.h"
#include "r_rtc_api.h"
FSP_HEADER
extern const crc_instance_t g_crc0;
extern crc_instance_ctrl_t g_crc0_ctrl;
extern const crc_cfg_t g_crc0_cfg;
/** UART on SCI Instance. */
            extern const uart_instance_t      g_uart8;

            /** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
            extern sci_b_uart_instance_ctrl_t     g_uart8_ctrl;
            extern const uart_cfg_t g_uart8_cfg;
            extern const sci_b_uart_extended_cfg_t g_uart8_cfg_extend;

            #ifndef uart_callback
            void uart_callback(uart_callback_args_t * p_args);
            #endif
/** UART on SCI Instance. */
            extern const uart_instance_t      g_uart7;

            /** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
            extern sci_b_uart_instance_ctrl_t     g_uart7_ctrl;
            extern const uart_cfg_t g_uart7_cfg;
            extern const sci_b_uart_extended_cfg_t g_uart7_cfg_extend;

            #ifndef uart_callback
            void uart_callback(uart_callback_args_t * p_args);
            #endif
/* Transfer on DMAC Instance. */
extern const transfer_instance_t g_transfer2;

/** Access the DMAC instance using these structures when calling API functions directly (::p_api is not used). */
extern dmac_instance_ctrl_t g_transfer2_ctrl;
extern const transfer_cfg_t g_transfer2_cfg;

#ifndef NULL
void NULL(transfer_callback_args_t * p_args);
#endif
#if OSPI_B_CFG_DMAC_SUPPORT_ENABLE
    #include "r_dmac.h"
#endif
#if OSPI_CFG_DOTF_SUPPORT_ENABLE
    #include "r_sce_if.h"
#endif

extern const spi_flash_instance_t g_ospi0;
extern ospi_b_instance_ctrl_t g_ospi0_ctrl;
extern const spi_flash_cfg_t g_ospi0_cfg;
/** UART on SCI Instance. */
            extern const uart_instance_t      g_uart6;

            /** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
            extern sci_b_uart_instance_ctrl_t     g_uart6_ctrl;
            extern const uart_cfg_t g_uart6_cfg;
            extern const sci_b_uart_extended_cfg_t g_uart6_cfg_extend;

            #ifndef uart_callback
            void uart_callback(uart_callback_args_t * p_args);
            #endif
/** UART on SCI Instance. */
            extern const uart_instance_t      g_uart5;

            /** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
            extern sci_b_uart_instance_ctrl_t     g_uart5_ctrl;
            extern const uart_cfg_t g_uart5_cfg;
            extern const sci_b_uart_extended_cfg_t g_uart5_cfg_extend;

            #ifndef uart_callback
            void uart_callback(uart_callback_args_t * p_args);
            #endif
/** DAC on DAC_B Instance. */
extern const dac_instance_t g_dac_b1;

/** Access the DAC_B instance using these structures when calling API functions directly (::p_api is not used). */
extern dac_b_instance_ctrl_t g_dac_b1_ctrl;
extern const dac_cfg_t g_dac_b1_cfg;
/** DAC on DAC_B Instance. */
extern const dac_instance_t g_dac_b0;

/** Access the DAC_B instance using these structures when calling API functions directly (::p_api is not used). */
extern dac_b_instance_ctrl_t g_dac_b0_ctrl;
extern const dac_cfg_t g_dac_b0_cfg;
/** ADC on ADC_B instance. */
                    extern const adc_instance_t g_adc0;

                    /** Access the ADC_B instance using these structures when calling API functions directly (::p_api is not used). */
                    extern adc_b_instance_ctrl_t g_adc0_ctrl;
                    extern const adc_cfg_t g_adc0_cfg;
                    extern const adc_b_scan_cfg_t g_adc0_scan_cfg;

                    #ifndef NULL
                    void NULL(adc_callback_args_t * p_args);
                    #endif
/** ADC on ADC_B instance. */
                    extern const adc_instance_t g_adc1;

                    /** Access the ADC_B instance using these structures when calling API functions directly (::p_api is not used). */
                    extern adc_b_instance_ctrl_t g_adc1_ctrl;
                    extern const adc_cfg_t g_adc1_cfg;
                    extern const adc_b_scan_cfg_t g_adc1_scan_cfg;

                    #ifndef NULL
                    void NULL(adc_callback_args_t * p_args);
                    #endif
/** CANFD on CANFD Instance. */
extern const can_instance_t g_canfd1;
/** Access the CANFD instance using these structures when calling API functions directly (::p_api is not used). */
extern canfd_instance_ctrl_t g_canfd1_ctrl;
extern const can_cfg_t g_canfd1_cfg;
extern const canfd_extended_cfg_t g_canfd1_cfg_extend;

#ifndef can_callback
void can_callback(can_callback_args_t * p_args);
#endif

/* Global configuration (referenced by all instances) */
extern canfd_global_cfg_t g_canfd_global_cfg;
/** CANFD on CANFD Instance. */
extern const can_instance_t g_canfd0;
/** Access the CANFD instance using these structures when calling API functions directly (::p_api is not used). */
extern canfd_instance_ctrl_t g_canfd0_ctrl;
extern const can_cfg_t g_canfd0_cfg;
extern const canfd_extended_cfg_t g_canfd0_cfg_extend;

#ifndef can_callback
void can_callback(can_callback_args_t * p_args);
#endif

/* Global configuration (referenced by all instances) */
extern canfd_global_cfg_t g_canfd_global_cfg;
/* I2C Master on IIC Instance. */
extern const i2c_master_instance_t g_i2c_master1;

/** Access the I2C Master instance using these structures when calling API functions directly (::p_api is not used). */
extern iic_master_instance_ctrl_t g_i2c_master1_ctrl;
extern const i2c_master_cfg_t g_i2c_master1_cfg;

#ifndef i2c_callback
void i2c_callback(i2c_master_callback_args_t * p_args);
#endif
/* I2C Master on IIC2 Instance. */
extern const i2c_master_instance_t g_i2c_master2;

/** Access the I2C Master instance using these structures when calling API functions directly (::p_api is not used). */
extern iic_master_instance_ctrl_t g_i2c_master2_ctrl;
extern const i2c_master_cfg_t g_i2c_master2_cfg;

#ifndef i2c_callback
void i2c_callback(i2c_master_callback_args_t * p_args);
#endif
/* Transfer on DMAC Instance. */
extern const transfer_instance_t g_transfer1;

/** Access the DMAC instance using these structures when calling API functions directly (::p_api is not used). */
extern dmac_instance_ctrl_t g_transfer1_ctrl;
extern const transfer_cfg_t g_transfer1_cfg;

#ifndef g_spi1_tx_transfer_callback
void g_spi1_tx_transfer_callback(transfer_callback_args_t * p_args);
#endif
/** SPI on SPI Instance. */
extern const spi_instance_t g_spi1;

/** Access the SPI instance using these structures when calling API functions directly (::p_api is not used). */
extern spi_b_instance_ctrl_t g_spi1_ctrl;
extern const spi_cfg_t g_spi1_cfg;

/** Callback used by SPI Instance. */
#ifndef spi_callback
void spi_callback(spi_callback_args_t * p_args);
#endif


#define RA_NOT_DEFINED (1)
#if (RA_NOT_DEFINED == g_transfer1)
    #define g_spi1_P_TRANSFER_TX (NULL)
#else
    #define g_spi1_P_TRANSFER_TX (&g_transfer1)
#endif
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
    #define g_spi1_P_TRANSFER_RX (NULL)
#else
    #define g_spi1_P_TRANSFER_RX (&RA_NOT_DEFINED)
#endif
#undef RA_NOT_DEFINED
/** UART on SCI Instance. */
            extern const uart_instance_t      g_uart9;

            /** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
            extern sci_b_uart_instance_ctrl_t     g_uart9_ctrl;
            extern const uart_cfg_t g_uart9_cfg;
            extern const sci_b_uart_extended_cfg_t g_uart9_cfg_extend;

            #ifndef uart_callback
            void uart_callback(uart_callback_args_t * p_args);
            #endif
/** UART on SCI Instance. */
            extern const uart_instance_t      g_uart4;

            /** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
            extern sci_b_uart_instance_ctrl_t     g_uart4_ctrl;
            extern const uart_cfg_t g_uart4_cfg;
            extern const sci_b_uart_extended_cfg_t g_uart4_cfg_extend;

            #ifndef uart_callback
            void uart_callback(uart_callback_args_t * p_args);
            #endif
/** UART on SCI Instance. */
            extern const uart_instance_t      g_uart3;

            /** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
            extern sci_b_uart_instance_ctrl_t     g_uart3_ctrl;
            extern const uart_cfg_t g_uart3_cfg;
            extern const sci_b_uart_extended_cfg_t g_uart3_cfg_extend;

            #ifndef uart_callback
            void uart_callback(uart_callback_args_t * p_args);
            #endif
/** UART on SCI Instance. */
            extern const uart_instance_t      g_uart2;

            /** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
            extern sci_b_uart_instance_ctrl_t     g_uart2_ctrl;
            extern const uart_cfg_t g_uart2_cfg;
            extern const sci_b_uart_extended_cfg_t g_uart2_cfg_extend;

            #ifndef uart_callback
            void uart_callback(uart_callback_args_t * p_args);
            #endif
/** UART on SCI Instance. */
            extern const uart_instance_t      g_uart1;

            /** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
            extern sci_b_uart_instance_ctrl_t     g_uart1_ctrl;
            extern const uart_cfg_t g_uart1_cfg;
            extern const sci_b_uart_extended_cfg_t g_uart1_cfg_extend;

            #ifndef uart_callback
            void uart_callback(uart_callback_args_t * p_args);
            #endif
/** Timer on GPT Instance. */
extern const timer_instance_t g_timer_gpt0;

/** Access the GPT instance using these structures when calling API functions directly (::p_api is not used). */
extern gpt_instance_ctrl_t g_timer_gpt0_ctrl;
extern const timer_cfg_t g_timer_gpt0_cfg;

/* GPT6 (PWM_GPT6) */
extern const timer_instance_t g_timer_gpt6;
extern gpt_instance_ctrl_t g_timer_gpt6_ctrl;
extern const timer_cfg_t g_timer_gpt6_cfg;

/* GPT7 (PWM_GPT7) */
extern const timer_instance_t g_timer_gpt7;
extern gpt_instance_ctrl_t g_timer_gpt7_ctrl;
extern const timer_cfg_t g_timer_gpt7_cfg;

/* GPT8 (PWM_GPT8) */
extern const timer_instance_t g_timer_gpt8;
extern gpt_instance_ctrl_t g_timer_gpt8_ctrl;
extern const timer_cfg_t g_timer_gpt8_cfg;

/* GPT9 (PWM_GPT9) */
extern const timer_instance_t g_timer_gpt9;
extern gpt_instance_ctrl_t g_timer_gpt9_ctrl;
extern const timer_cfg_t g_timer_gpt9_cfg;

#ifndef NULL
void NULL(timer_callback_args_t * p_args);
#endif
/** Timer on GPT Instance. */
extern const timer_instance_t g_timer_gpt13;

/** Access the GPT instance using these structures when calling API functions directly (::p_api is not used). */
extern gpt_instance_ctrl_t g_timer_gpt13_ctrl;
extern const timer_cfg_t g_timer_gpt13_cfg;

#ifndef NULL
void NULL(timer_callback_args_t * p_args);
#endif
/** Timer on GPT Instance. */
extern const timer_instance_t g_timer_gpt12;

/** Access the GPT instance using these structures when calling API functions directly (::p_api is not used). */
extern gpt_instance_ctrl_t g_timer_gpt12_ctrl;
extern const timer_cfg_t g_timer_gpt12_cfg;

#ifndef NULL
void NULL(timer_callback_args_t * p_args);
#endif
/** Timer on GPT Instance. */
extern const timer_instance_t g_timer_gpt11;

/** Access the GPT instance using these structures when calling API functions directly (::p_api is not used). */
extern gpt_instance_ctrl_t g_timer_gpt11_ctrl;
extern const timer_cfg_t g_timer_gpt11_cfg;

#ifndef NULL
void NULL(timer_callback_args_t * p_args);
#endif
/** Timer on GPT Instance. */
extern const timer_instance_t g_timer_gpt10;

/** Access the GPT instance using these structures when calling API functions directly (::p_api is not used). */
extern gpt_instance_ctrl_t g_timer_gpt10_ctrl;
extern const timer_cfg_t g_timer_gpt10_cfg;

#ifndef NULL
void NULL(timer_callback_args_t * p_args);
#endif
/** Timer on GPT Instance. */
extern const timer_instance_t g_timer_gpt3;

/** Access the GPT instance using these structures when calling API functions directly (::p_api is not used). */
extern gpt_instance_ctrl_t g_timer_gpt3_ctrl;
extern const timer_cfg_t g_timer_gpt3_cfg;

#ifndef NULL
void NULL(timer_callback_args_t * p_args);
#endif
/** UART on SCI Instance. */
            extern const uart_instance_t      g_uart0;

            /** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
            extern sci_b_uart_instance_ctrl_t     g_uart0_ctrl;
            extern const uart_cfg_t g_uart0_cfg;
            extern const sci_b_uart_extended_cfg_t g_uart0_cfg_extend;

            #ifndef uart_callback
            void uart_callback(uart_callback_args_t * p_args);
            #endif
/* Transfer on DMAC Instance. */
extern const transfer_instance_t g_transfer0;

/** Access the DMAC instance using these structures when calling API functions directly (::p_api is not used). */
extern dmac_instance_ctrl_t g_transfer0_ctrl;
extern const transfer_cfg_t g_transfer0_cfg;

#ifndef g_spi0_tx_transfer_callback
void g_spi0_tx_transfer_callback(transfer_callback_args_t * p_args);
#endif
/** SPI on SPI Instance. */
extern const spi_instance_t g_spi0;

/** Access the SPI instance using these structures when calling API functions directly (::p_api is not used). */
extern spi_b_instance_ctrl_t g_spi0_ctrl;
extern const spi_cfg_t g_spi0_cfg;

/** Callback used by SPI Instance. */
#ifndef spi_callback
void spi_callback(spi_callback_args_t * p_args);
#endif


#define RA_NOT_DEFINED (1)
#if (RA_NOT_DEFINED == g_transfer0)
    #define g_spi0_P_TRANSFER_TX (NULL)
#else
    #define g_spi0_P_TRANSFER_TX (&g_transfer0)
#endif
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
    #define g_spi0_P_TRANSFER_RX (NULL)
#else
    #define g_spi0_P_TRANSFER_RX (&RA_NOT_DEFINED)
#endif
#undef RA_NOT_DEFINED
/** AGT Timer Instance */
extern const timer_instance_t g_timer_agt1;

/** Access the AGT instance using these structures when calling API functions directly (::p_api is not used). */
extern agt_instance_ctrl_t g_timer_agt1_ctrl;
extern const timer_cfg_t g_timer_agt1_cfg;

#ifndef NULL
void NULL(timer_callback_args_t * p_args);
#endif
/** AGT Timer Instance */
extern const timer_instance_t g_timer_agt0;

/** Access the AGT instance using these structures when calling API functions directly (::p_api is not used). */
extern agt_instance_ctrl_t g_timer_agt0_ctrl;
extern const timer_cfg_t g_timer_agt0_cfg;

#ifndef NULL
void NULL(timer_callback_args_t * p_args);
#endif
/** Timer on GPT Instance. */
extern const timer_instance_t g_timer_gpt5;

/** Access the GPT instance using these structures when calling API functions directly (::p_api is not used). */
extern gpt_instance_ctrl_t g_timer_gpt5_ctrl;
extern const timer_cfg_t g_timer_gpt5_cfg;

#ifndef NULL
void NULL(timer_callback_args_t * p_args);
#endif
/** Timer on GPT Instance. */
extern const timer_instance_t g_timer_gpt4;

/** Access the GPT instance using these structures when calling API functions directly (::p_api is not used). */
extern gpt_instance_ctrl_t g_timer_gpt4_ctrl;
extern const timer_cfg_t g_timer_gpt4_cfg;

#ifndef NULL
void NULL(timer_callback_args_t * p_args);
#endif
/** Timer on GPT Instance. */
extern const timer_instance_t g_timer_gpt2;

/** Access the GPT instance using these structures when calling API functions directly (::p_api is not used). */
extern gpt_instance_ctrl_t g_timer_gpt2_ctrl;
extern const timer_cfg_t g_timer_gpt2_cfg;

#ifndef NULL
void NULL(timer_callback_args_t * p_args);
#endif
/* I2C Master on IIC Instance. */
extern const i2c_master_instance_t g_i2c_master0;

/** Access the I2C Master instance using these structures when calling API functions directly (::p_api is not used). */
extern iic_master_instance_ctrl_t g_i2c_master0_ctrl;
extern const i2c_master_cfg_t g_i2c_master0_cfg;

#ifndef i2c_callback
void i2c_callback(i2c_master_callback_args_t * p_args);
#endif
/** WDT on WDT Instance. */
extern const wdt_instance_t g_wdt0;

/** Access the WDT instance using these structures when calling API functions directly (::p_api is not used). */
extern wdt_instance_ctrl_t g_wdt0_ctrl;
extern const wdt_cfg_t g_wdt0_cfg;

#ifndef NULL
void NULL(wdt_callback_args_t * p_args);
#endif
/** lpm Instance */
extern const lpm_instance_t g_lpm0;

/** Access the LPM instance using these structures when calling API functions directly (::p_api is not used). */
extern lpm_instance_ctrl_t g_lpm0_ctrl;
extern const lpm_cfg_t g_lpm0_cfg;
/** Timer on GPT Instance. */
extern const timer_instance_t g_timer_gpt1;

/** Access the GPT instance using these structures when calling API functions directly (::p_api is not used). */
extern gpt_instance_ctrl_t g_timer_gpt1_ctrl;
extern const timer_cfg_t g_timer_gpt1_cfg;

#ifndef usticker_compare_match_callback
void usticker_compare_match_callback(timer_callback_args_t * p_args);
#endif
/* RTC Instance. */
extern const rtc_instance_t g_rtc0;

/** Access the RTC instance using these structures when calling API functions directly (::p_api is not used). */
extern rtc_instance_ctrl_t g_rtc0_ctrl;
extern const rtc_cfg_t g_rtc0_cfg;

#ifndef NULL
void NULL(rtc_callback_args_t * p_args);
#endif
void hal_entry(void);
void g_hal_init(void);
FSP_FOOTER
#endif /* HAL_DATA_H_ */
