# Mbed OS upload method configuration file for target RZ_A1H (GR-PEACH)
# To change any of these parameters from their default values, set them in your build script between where you
# include app.cmake and where you add mbed os as a subdirectory.

# General config parameters
# -------------------------------------------------------------
# GR-PEACH has no on-board debug probe; it is flashed through the SWD/JTAG
# header with an external probe.  Both the SEGGER J-Link software (which
# handles the on-board serial flash download automatically) and OpenOCD
# (using the renesas_r7s72100 SPIBSC flash driver) are supported.
# The MBED method (drag-and-drop onto the DAPLink MSD drive) is also
# available.
#
# NOTE: the OPENOCD method can only flash the serial flash if OpenOCD was
# built with the out-of-tree renesas_r7s72100 SPIBSC flash driver.  Stock
# OpenOCD 0.12 builds (xpack, sysprogs, ...) do not include it, and fail
# with "flash driver 'renesas_r7s72100' not found".  With such builds the
# OpenOCD method is still useful for debugging; use MBED or JLINK to flash.
set(UPLOAD_METHOD_DEFAULT MBED)

# Config options for MBED
# -------------------------------------------------------------

set(MBED_UPLOAD_ENABLED TRUE)

# Config options for PYOCD
# -------------------------------------------------------------

set(PYOCD_UPLOAD_ENABLED FALSE)

# Config options for JLINK
# -------------------------------------------------------------

set(JLINK_UPLOAD_ENABLED TRUE)
set(JLINK_CPU_NAME R7S721000)
set(JLINK_CLOCK_SPEED 4000)
set(JLINK_UPLOAD_INTERFACE SWD)
