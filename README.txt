IMPORTANT NOTE
--------------

If you find bugs or errors in this code, please let us know.  If you fix
any bugs or errors in this code, or add new enhancements, please send us the
code changes, so we can be sure we implement the changes correctly.



WHAT'S IN THIS TARBALL
----------------------

The following files are provided in this tarball.

drivers/input/touchscreen/synaptics_dsx/synaptics_dsx_core.[ch]
   The source code of the Synaptics DSX core driver containing support for
   2D touch and 0D buttons.

drivers/input/touchscreen/synaptics_dsx/synaptics_dsx_i2c.c
   The source code of the Synaptics DSX I2C module used for providing support
   for touch devices with an I2C communication interface.

drivers/input/touchscreen/synaptics_dsx/synaptics_dsx_spi.c
   The source code of the Synaptics DSX SPI module used for providing support
   for touch devices with an SPI communication interface.

drivers/input/touchscreen/synaptics_dsx/synaptics_dsx_rmi_dev.c
   The source code of the RMI Dev module used for direct RMI register access
   from user space programs via a character device node or sysfs attributes.

drivers/input/touchscreen/synaptics_dsx/synaptics_dsx_fw_update.c
   The source code of the firmware update module used for carrying out both
   in-system and command-line reflash.

[optional] drivers/input/touchscreen/synaptics_dsx_test_reporting.c
   The source code of the test reporting module used for retrieving self-test
   reports and accessing analog diagnostic and control functions.

[optional] drivers/input/touchscreen/synaptics_dsx_proximity.c
   The source code of the proximity module used for providing proximity
   functionalities.

[optional] drivers/input/touchscreen/synaptics_dsx_active_pen.c
   The source code of the active pen module used for providing active pen
   functionalities.

drivers/input/touchscreen/synaptics_dsx/Kconfig
   Kconfig for the Synaptics DSX driver.

drivers/input/touchscreen/synaptics_dsx/Makefile
   Makefile for the Synaptics DSX driver.

drivers/input/touchscreen/Kconfig
   Example Kconfig for the OMAP Panda board to include the Synaptics DSX
   driver.

drivers/input/touchscreen/Makefile
   Example Makefile for the OMAP Panda board to include the Synaptics DSX
   driver.

firmware/Makefile
   Example Makefile for the OMAP Panda board for the inclusion of the firmware
   image in the kernel build for doing reflash during device initialization.

include/linux/input/synaptics_dsx.h
   The Synaptics DSX header file shared between the device and the driver.

arch/arm/configs/panda_defconfig
   Example defconfig for the OMAP Panda board to include the Synaptics DSX
   driver.

arch/arm/mach-omap2/board-omap4panda.c
   Example board file for the OMAP Panda board to include support for Synaptics
   touch devices.



HOW TO INSTALL THE DRIVER
-------------------------

** Copy the synaptics_dsx folder in drivers/input/touchscreen to the
   equivalent directory in your kernel tree.

** Copy synaptics_dsx.h in include/linux/input to the equivalent directory
   in your kernel tree.

** Update Makefile and Kconfig in your kernel tree's drivers/input/touchscreen
   directory to include support for building the Synaptics DSX driver. Use the
   equivalent files in the tarball as examples.

** Update your defconfig by referring to the defconfig in the tarball as
   an example.

** Update your board file by referring to the board file in the tarball as
   an example.

** "make clean" your kernel.

** "make" your defconfig.

** Rebuild your kernel.

** Install the new kernel on your Android system.

** Reboot your Android system.



HOW TO INCLUDE FIRMWARE IMAGE IN KERNEL BUILD FOR DOING REFLASH
---------------------------------------------------------------

** Convert the .img image file to the .ihex format using the command below.
   objcopy -I binary -O ihex <firmware_name>.img startup_fw_update.img.ihex

** Place startup_fw_update.img.ihex in the firmware/synaptics directory in your
   kernel tree.

** Include the line below in firmware/Makefile (this line is commented out by
   default in the example Makefile in the driver).
   fw-shipped-$(CONFIG_TOUCHSCREEN_SYNAPTICS_DSX_FW_UPDATE) += synaptics/startup_fw_update.img

** Rebuild your kernel.
