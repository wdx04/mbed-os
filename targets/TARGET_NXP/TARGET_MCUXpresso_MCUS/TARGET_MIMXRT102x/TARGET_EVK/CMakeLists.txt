# Copyright (c) 2021 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

# MIMXRT1020_EVK
add_library(mbed-mimxrt1020-evk INTERFACE)

target_include_directories(mbed-mimxrt1020-evk
    INTERFACE
        .
)

target_sources(mbed-mimxrt1020-evk
    INTERFACE
		fsl_flexspi_nor_boot.c
		flash_api.c

        TARGET_1020_EVK/xip/evkmimxrt1020_flexspi_nor_config.c
        TARGET_1020_EVK/xip/dcd.c
)

target_link_libraries(mbed-mimxrt1020-evk
	INTERFACE
		mbed-mimxrt102x
)