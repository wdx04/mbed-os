# Copyright (c) 2020-2021 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

# Target for MIMXRT101x common code.
if(${MBED_TOOLCHAIN} STREQUAL "GCC_ARM")
    set(RT101X_STARTUP_FILE device/TOOLCHAIN_GCC_ARM/startup_MIMXRT1011.S)
    set(RT101X_LINKER_FILE device/TOOLCHAIN_GCC_ARM/MIMXRT1011xxxxx.ld)
endif()

add_library(mbed-mimxrt101x INTERFACE)

target_include_directories(mbed-mimxrt101x
    INTERFACE
        .
        device
        drivers
)

target_sources(mbed-mimxrt101x
    INTERFACE
        usb_device_ch9.c
        usb_device_class.c
        analogin_api.c
        serial_api.c
        sleep.c
        trng_api.c
        us_ticker.c
        clock_config.c
        pinmap.c
        mimxrt_clock_adjustment.c
        lpm.c
        mbed_overrides.c

        device/system_MIMXRT1011.c

        drivers/fsl_adc.c
        drivers/fsl_adc_etc.c
        drivers/fsl_aipstz.c
        drivers/fsl_aoi.c
        drivers/fsl_cache.c
        drivers/fsl_clock.c
        drivers/fsl_common.c
        drivers/fsl_dcdc.c
        drivers/fsl_dcp.c
        drivers/fsl_dmamux.c
        drivers/fsl_edma.c
        drivers/fsl_ewm.c
        drivers/fsl_flexio.c
        drivers/fsl_flexio_camera.c
        drivers/fsl_flexio_camera_edma.c
        drivers/fsl_flexio_i2c_master.c
        drivers/fsl_flexio_i2s.c
        drivers/fsl_flexio_i2s_edma.c
        drivers/fsl_flexio_mculcd.c
        drivers/fsl_flexio_mculcd_edma.c
        drivers/fsl_flexio_spi.c
        drivers/fsl_flexio_spi_edma.c
        drivers/fsl_flexio_uart.c
        drivers/fsl_flexio_uart_edma.c
        drivers/fsl_flexram.c
        drivers/fsl_flexspi.c
        drivers/fsl_gpc.c
        drivers/fsl_gpio.c
        drivers/fsl_gpt.c
        drivers/fsl_kpp.c
        drivers/fsl_lpi2c.c
        drivers/fsl_lpi2c_edma.c
        drivers/fsl_lpspi.c
        drivers/fsl_lpspi_edma.c
        drivers/fsl_lpuart.c
        drivers/fsl_lpuart_edma.c
        drivers/fsl_ocotp.c
        drivers/fsl_pit.c
        drivers/fsl_pmu.c
        drivers/fsl_pwm.c
        drivers/fsl_rtwdog.c
        drivers/fsl_sai.c
        drivers/fsl_sai_edma.c
        drivers/fsl_snvs_hp.c
        drivers/fsl_snvs_lp.c
        drivers/fsl_spdif.c
        drivers/fsl_spdif_edma.c
        drivers/fsl_src.c
        drivers/fsl_tempmon.c
        drivers/fsl_trng.c
        drivers/fsl_wdog.c
        drivers/fsl_xbara.c

        ${RT101X_STARTUP_FILE}
)

target_link_libraries(mbed-mimxrt101x
    INTERFACE
        mbed-mcuxpresso-mcus
        mbed-imx
        mbed-nxp-usb
)

mbed_set_linker_script(mbed-mimxrt101x ${CMAKE_CURRENT_SOURCE_DIR}/${RT101X_LINKER_FILE})

add_subdirectory(TARGET_EVK EXCLUDE_FROM_ALL)
