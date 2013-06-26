IMPORTANT NOTE
--------------

If you find bugs or errors in this code, please let us know.  If you fix
any bugs or errors in this code, or add new enhancements, please send us the
code changes, so we can be sure we implement the changes correctly.



WHAT'S IN THIS TARBALL
----------------------

The following files are provided in this tarball.

drivers/input/touchscreen/synaptics_dsx_i2c.[ch]
   The source code of the Synaptics DSX main driver containing support for
   2D touch and 0D buttons.

drivers/input/touchscreen/synaptics_dsx_rmi_dev.c
   The source code of the RMI Dev module used for direct register access from
   user space programs via a character device node or sysfs entries.

drivers/input/touchscreen/synaptics_dsx_fw_update.c
   The source code of the firmware update module used for carrying out both
   in-system and command-line reflash.

drivers/input/touchscreen/Kconfig_panda
   Example Kconfig for the OMAP Panda Board.
drivers/input/touchscreen/Kconfig_beagle
   Example Kconfig for the OMAP Beagle Board.

drivers/input/touchscreen/Makefile_panda
   Example Makefile for the OMAP Panda Board.
drivers/input/touchscreen/Makefile_beagle
   Example Makefile for the OMAP Beagle Board.

firmware/Makefile
   Example Makefile for the OMAP Panda Board for the inclusion of the firmware
   image in the kernel build for doing reflash during device initialization.

include/linux/input/synaptics_dsx.h
   The Synaptics DSX header file shared between the device and the driver.

arch/arm/configs/panda_defconfig
   Example defconfig for the OMAP Panda Board.
arch/arm/configs/omap3_beagle_android_defconfig
   Example defconfig for the OMAP Beagle Board.

arch/arm/mach-omap2/board-omap4panda.c
   Example board file for the OMAP Panda Board.
arch/arm/mach-omap2/board-omap3beagle.c
   Example board file for the OMAP Beagle Board.



HOW TO INSTALL THE DRIVER
-------------------------

** Copy the .c and .h souce code files in drivers/input/touchscreen to the
   equivalent directory in your kernel tree.

** Copy synaptics_dsx.h in include/linux/input to the equivalent directory
   in your kernel tree.

** Update Makefile and Kconfig in your kernel tree's drivers/input/touchscreen
   directory to include support for building the Synaptics DSX driver. Use the
   equivalent files in the tarball as examples.

** Update your defconfig by referring to the defconfigs in the tarball as
   examples.

** Update your board file by referring to the board files in the tarball as
   examples.

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
