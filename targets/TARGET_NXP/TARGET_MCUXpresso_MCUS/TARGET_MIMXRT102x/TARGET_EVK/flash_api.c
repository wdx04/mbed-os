/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "flash_api.h"
#include "mbed_toolchain.h"
#include "mbed_critical.h"

#if DEVICE_FLASH

#include "fsl_flexspi.h"
#include "fsl_cache.h"
#include "flash_defines.h"
#include "mimxrt_flash_api.h"

#include <inttypes.h>
#include <stdio.h>

AT_QUICKACCESS_SECTION_CODE(void flexspi_update_lut_ram(void));
AT_QUICKACCESS_SECTION_CODE(status_t flexspi_nor_write_enable_ram(uint32_t baseAddr));
AT_QUICKACCESS_SECTION_CODE(status_t flexspi_nor_wait_bus_busy_ram(void));
AT_QUICKACCESS_SECTION_CODE(status_t flexspi_nor_flash_erase_sector_ram(uint32_t address));
AT_QUICKACCESS_SECTION_CODE(status_t flexspi_nor_flash_page_program_ram(uint32_t address,
                                                                        const uint32_t *src,
                                                                        uint32_t size));
AT_QUICKACCESS_SECTION_CODE(void flexspi_nor_flash_read_data_ram(uint32_t addr,
                                                                 uint32_t *buffer,
                                                                 uint32_t size));
AT_QUICKACCESS_SECTION_CODE(void *flexspi_memset(void *buf, int c, size_t n));
/**
 * @brief Set bytes in memory. If put this code in SRAM, Make sure this code
 * does not call functions in Flash.
 *
 * @return pointer to start of buffer
 */
void *flexspi_memset(void *buf, int c, size_t n)
{
    /* do byte-sized initialization until word-aligned or finished */
    unsigned char *d_byte = (unsigned char *)buf;
    unsigned char c_byte = (unsigned char)c;

    while (((unsigned int)d_byte) & 0x3) {
        if (n == 0) {
            return buf;
        }
        *(d_byte++) = c_byte;
        n--;
    };

    /* do word-sized initialization as long as possible */

    unsigned int *d_word = (unsigned int *)d_byte;
    unsigned int c_word = (unsigned int)(unsigned char)c;

    c_word |= c_word << 8;
    c_word |= c_word << 16;

    while (n >= sizeof(unsigned int)) {
        *(d_word++) = c_word;
        n -= sizeof(unsigned int);
    }

    /* do byte-sized initialization until finished */

    d_byte = (unsigned char *)d_word;

    while (n > 0) {
        *(d_byte++) = c_byte;
        n--;
    }

    return buf;
}

AT_QUICKACCESS_SECTION_CODE(status_t flexspi_nor_enable_quad_mode_ram(void));
AT_QUICKACCESS_SECTION_CODE(status_t flexspi_nor_read_status_register_ram(uint32_t * result));

/*
 * Check if quad SPI mode is enabled and, if not, enable it.
 *
 * Note that I'm not totally sure if this function is needed because I don't think
 * that the application could boot without quad mode enabled, but this might be
 * useful for programming non-boot-device flashes at a later date.
 * Or, if you must run the application on a flash which does not have quad mode enabled,
 * you could temporarily change the boot header read command to use 1-pad read,
 * then rely on this function to update the setting.
 */
status_t flexspi_nor_enable_quad_mode_ram(void)
{
    uint32_t readResult = 0;
    status_t status = flexspi_nor_read_status_register_ram(&readResult);
    if (status != kStatus_Success) {
        return status;
    }

    if(readResult & (1 << FLASH_QE_STATUS_OFFSET)) {
        // QSPI mode already enabled, don't need to do anything
        return kStatus_Success;
    }

    /* Write enable */
    status = flexspi_nor_write_enable_ram(0);

    if (status != kStatus_Success) {
        return status;
    }

    /* Enable quad mode. */
    flexspi_transfer_t flashXfer = {};
    uint32_t writeValue = (1 << FLASH_QE_STATUS_OFFSET);
    flashXfer.deviceAddress = 0;
    flashXfer.port          = kFLEXSPI_PortA1;
    flashXfer.cmdType       = kFLEXSPI_Write;
    flashXfer.SeqNumber     = 1;
    flashXfer.seqIndex      = NOR_CMD_LUT_SEQ_IDX_WRITESTATUSREG;
    flashXfer.data          = &writeValue;
    flashXfer.dataSize      = 1;

    status = FLEXSPI_TransferBlocking(FLEXSPI, &flashXfer);
    if (status != kStatus_Success) {
        return status;
    }

    status = flexspi_nor_wait_bus_busy_ram();

    return status;
}

void flexspi_update_lut_ram(void)
{
    flexspi_config_t config = {};

    /* To store custom's LUT table in local. */
    uint32_t tempLUT[CUSTOM_LUT_LENGTH] = {0x00U};

#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    bool DCacheEnableFlag = false;
    /* Disable D cache. */
    if (SCB_CCR_DC_Msk == (SCB_CCR_DC_Msk & SCB->CCR))
    {
        SCB_DisableDCache();
        DCacheEnableFlag = true;
    }
#endif /* __DCACHE_PRESENT */

    /* Copy LUT information from flash region into RAM region, because LUT update maybe corrupt read sequence(LUT[0])
     * and load wrong LUT table from FLASH region. */
    memcpy(tempLUT, customLUT, sizeof(tempLUT));

    /*Get FLEXSPI default settings and configure the flexspi. */
    flexspi_memset(&config, 0, sizeof(config));
    FLEXSPI_GetDefaultConfig(&config);

    /*Set AHB buffer size for reading data through AHB bus. */
    config.ahbConfig.enableAHBPrefetch    = true;
    config.ahbConfig.enableAHBBufferable  = true;
    config.ahbConfig.enableReadAddressOpt = true;
    config.ahbConfig.enableAHBCachable    = true;
    config.rxSampleClock                  = kFLEXSPI_ReadSampleClkLoopbackFromDqsPad;

    /* Configure flash settings according to serial flash feature. */
    FLEXSPI_SetFlashConfig(FLEXSPI, &deviceconfig, kFLEXSPI_PortA1);

    /* Update LUT table. */
    FLEXSPI_UpdateLUT(FLEXSPI, 0, tempLUT, CUSTOM_LUT_LENGTH);

    /* Do software reset. */
    FLEXSPI_SoftwareReset(FLEXSPI);
    /* Wait for bus idle. */
    while (!FLEXSPI_GetBusIdleStatus(FLEXSPI)) {
    }

    flexspi_nor_enable_quad_mode_ram();

    // Just in case any bad data got into the I-cache while we were reconfiguring
    // the flash, wipe it.
    SCB_InvalidateICache();

#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    if (DCacheEnableFlag)
    {
        /* Enable D cache. */
        SCB_EnableDCache();
    }
#endif /* __DCACHE_PRESENT */
}

status_t flexspi_nor_write_enable_ram(uint32_t baseAddr)
{
    flexspi_transfer_t flashXfer;
    status_t status = kStatus_Success;

    flexspi_memset(&flashXfer, 0, sizeof(flashXfer));
    /* Write enable */
    flashXfer.deviceAddress = baseAddr;
    flashXfer.port          = kFLEXSPI_PortA1;
    flashXfer.cmdType       = kFLEXSPI_Command;
    flashXfer.SeqNumber     = 1;
    flashXfer.seqIndex      = NOR_CMD_LUT_SEQ_IDX_WRITEENABLE;

    status = FLEXSPI_TransferBlocking(FLEXSPI, &flashXfer);

    return status;
}

// Read the status register and save the result into the given pointer
status_t flexspi_nor_read_status_register_ram(uint32_t * result)
{
    flexspi_transfer_t flashXfer = {};

    flashXfer.deviceAddress = 0;
    flashXfer.port          = kFLEXSPI_PortA1;
    flashXfer.cmdType       = kFLEXSPI_Read;
    flashXfer.SeqNumber     = 1;
    flashXfer.seqIndex      = NOR_CMD_LUT_SEQ_IDX_READSTATUSREG;
    flashXfer.data          = result;
    flashXfer.dataSize      = 1;

    return FLEXSPI_TransferBlocking(FLEXSPI, &flashXfer);
}

status_t flexspi_nor_wait_bus_busy_ram(void)
{
    /* Wait status ready. */
    bool isBusy;

    do {
        uint32_t readResult;
        status_t status = flexspi_nor_read_status_register_ram(&readResult);

        if (status != kStatus_Success) {
            return status;
        }

        if(readResult & (1U << FLASH_BUSY_STATUS_OFFSET)) {
            isBusy = FLASH_BUSY_STATUS_POL;
        }
        else
        {
            isBusy = !FLASH_BUSY_STATUS_POL;
        }

    } while (isBusy);

    return kStatus_Success;
}


status_t flexspi_nor_flash_erase_sector_ram(uint32_t address)
{
    flexspi_transfer_t flashXfer;
    status_t status = kStatus_Success;

    flexspi_memset(&flashXfer, 0, sizeof(flashXfer));

    /* Write enable */
    flashXfer.deviceAddress = address;
    flashXfer.port          = kFLEXSPI_PortA1;
    flashXfer.cmdType       = kFLEXSPI_Command;
    flashXfer.SeqNumber     = 1;
    flashXfer.seqIndex      = NOR_CMD_LUT_SEQ_IDX_WRITEENABLE;

    status = FLEXSPI_TransferBlocking(FLEXSPI, &flashXfer);

    if (status != kStatus_Success) {
        return status;
    }

    flashXfer.deviceAddress = address;
    flashXfer.port          = kFLEXSPI_PortA1;
    flashXfer.cmdType       = kFLEXSPI_Command;
    flashXfer.SeqNumber     = 1;
    flashXfer.seqIndex      = NOR_CMD_LUT_SEQ_IDX_ERASESECTOR;
    status                  = FLEXSPI_TransferBlocking(FLEXSPI, &flashXfer);

    if (status != kStatus_Success) {
        return status;
    }

    status = flexspi_nor_wait_bus_busy_ram();

    /* Do software reset. */
    FLEXSPI_SoftwareReset(FLEXSPI);

    return status;
}

status_t flexspi_nor_flash_page_program_ram(uint32_t address, const uint32_t *src, uint32_t size)
{
    flexspi_transfer_t flashXfer;
    status_t status = kStatus_Success;
    uint32_t offset = 0;

    flexspi_memset(&flashXfer, 0, sizeof(flashXfer));

    while (size > 0) {
        /* Write enable */
        status = flexspi_nor_write_enable_ram(address + offset);

        if (status != kStatus_Success) {
            return status;
        }

        /* Prepare page program command */
        flashXfer.deviceAddress = address + offset;
        flashXfer.port          = kFLEXSPI_PortA1;
        flashXfer.cmdType       = kFLEXSPI_Write;
        flashXfer.SeqNumber     = 1;
        flashXfer.seqIndex      = NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM_QUAD;
        flashXfer.data          = (uint32_t *)((uint32_t)src + offset);
        flashXfer.dataSize      = BOARD_FLASH_PAGE_SIZE;

        status = FLEXSPI_TransferBlocking(FLEXSPI, &flashXfer);

        if (status != kStatus_Success) {
            return status;
        }

        status = flexspi_nor_wait_bus_busy_ram();

        if (status != kStatus_Success) {
            return status;
        }

        size -= BOARD_FLASH_PAGE_SIZE;
        offset += BOARD_FLASH_PAGE_SIZE;
    }

    /* Do software reset. */
    FLEXSPI_SoftwareReset(FLEXSPI);

    return status;
}

void flexspi_nor_flash_read_data_ram(uint32_t addr, uint32_t *buffer, uint32_t size)
{
    memcpy(buffer, (void *)addr, size);
}

void mimxrt_flash_setup(void)
{
    core_util_critical_section_enter();
    flexspi_update_lut_ram();
    core_util_critical_section_exit();
}

int32_t flash_init(flash_t *obj)
{
    // Setup is already done when the application boots by flash_setup().
    // Nothing left to do.
    (void)obj;
    return 0;
}

int32_t flash_erase_sector(flash_t *obj, uint32_t address)
{
    status_t status = kStatus_Success;
    int32_t ret = 0;

    core_util_critical_section_enter();

    status = flexspi_nor_flash_erase_sector_ram(address - FlexSPI_AMBA_BASE);

    if (status != kStatus_Success) {
        ret = -1;
    } else {
        DCACHE_InvalidateByRange(address, BOARD_FLASH_SECTOR_SIZE);
    }

    core_util_critical_section_exit();

    return ret;
}

int32_t flash_program_page(flash_t *obj, uint32_t address, const uint8_t *data, uint32_t size)
{
    status_t status = kStatus_Success;
    int32_t ret = 0;

    core_util_critical_section_enter();

    status = flexspi_nor_flash_page_program_ram(address - FlexSPI_AMBA_BASE, (uint32_t *)data, size);

    if (status != kStatus_Success) {
        ret = -1;
    } else {
        SCB_InvalidateICache_by_Addr((void*)address, (int32_t)size);
        SCB_InvalidateDCache_by_Addr((void*)address, (int32_t)size);
        DCACHE_InvalidateByRange(address, size);
    }

    core_util_critical_section_exit();

    return ret;
}

int32_t flash_read(flash_t *obj, uint32_t address, uint8_t *data, uint32_t size)
{
    flexspi_nor_flash_read_data_ram(address, (uint32_t *)data, size);

    return 0;
}

int32_t flash_free(flash_t *obj)
{
    return 0;
}

uint32_t flash_get_sector_size(const flash_t *obj, uint32_t address)
{
    uint32_t sectorsize = MBED_FLASH_INVALID_SIZE;
    uint32_t devicesize = BOARD_FLASH_SIZE;
    uint32_t startaddr = BOARD_FLASH_START_ADDR;

    if ((address >= startaddr) && (address < (startaddr + devicesize))) {
        sectorsize = BOARD_FLASH_SECTOR_SIZE;
    }

    return sectorsize;
}

uint32_t flash_get_page_size(const flash_t *obj)
{
    return BOARD_FLASH_PAGE_SIZE;
}

uint32_t flash_get_start_address(const flash_t *obj)
{
    return BOARD_FLASH_START_ADDR;
}

uint32_t flash_get_size(const flash_t *obj)
{
    return BOARD_FLASH_SIZE;
}

uint8_t flash_get_erase_value(const flash_t *obj)
{
    (void)obj;

    return 0xFF;
}

#endif //DEVICE_FLASH

