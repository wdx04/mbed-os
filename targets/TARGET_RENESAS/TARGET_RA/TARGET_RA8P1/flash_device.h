/* mbed Microcontroller Library
 * Copyright (c) 2024 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef FLASH_DEVICE_H
#define FLASH_DEVICE_H

/* RA8P1: 1MB Code MRAM, no Data Flash.
 * The Code MRAM is programmed in 1-32 byte chunks and is "erased"
 * (written to 0xFF) in 32-byte blocks, so both the sector size and the
 * minimum write unit are 32 bytes. */
#ifndef RA_CF_START
#define RA_CF_START      (0x02000000u)
#endif
#ifndef RA_CF_SIZE
#define RA_CF_SIZE       (1024u * 1024u)
#endif
#ifndef RA_CF_SECTOR_SIZE
#define RA_CF_SECTOR_SIZE (32u)
#endif
#ifndef RA_CF_WRITE_SIZE
#define RA_CF_WRITE_SIZE  (32u)
#endif

#ifndef RA_DF_START
#define RA_DF_START      (0x27000000u)
#endif
#ifndef RA_DF_SIZE
#define RA_DF_SIZE       (0u)
#endif
#ifndef RA_DF_SECTOR_SIZE
#define RA_DF_SECTOR_SIZE (64u)
#endif
#ifndef RA_DF_WRITE_SIZE
#define RA_DF_WRITE_SIZE  (4u)
#endif

#endif
