# UEFI-Bootloader
## Usage:
Place it on some bootable device in EFI/BOOT/BOOTX64.EFI and boot your computer with that device.
UEFI-Bootloader recognizes all EFI-enabled, 64-bit:
* Windows
* Ubuntu
* Debian
* Fedora
* Mac OSX


systems installed on hard disk and places them in a selection list.
User may choose desired system to boot by simply pressing a proper digit (ASCII order) and then the system will be loaded.
UEF-Bootloader doesn't recognize any other systems than listed before.
## Building:
You are ought to use UDK2014 (EDK2) project to build UEFI-Bootloader. It may be compiled in 64 bits.
#### Authors:
andi9310,
bartekmp

