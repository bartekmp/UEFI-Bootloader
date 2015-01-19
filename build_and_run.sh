#!/bin/bash
. edksetup.sh BaseTools
pattern=$( grep -c -F 'BootLoaderEfi/BootLoaderEfi.inf' DuetPkg/DuetPkgX64.dsc )
if [ $pattern -eq 0 ]; then
 sed -i '#[Components]#a BootLoaderEfi/BootLoaderEfi.inf#' DuetPkg/DuetPkgX64.dsc
fi &&
build -a X64 -p DuetPkg/DuetPkgX64.dsc && 
cp Build/DuetPkgX64/DEBUG_GCC48/X64/BootLoaderEfi.efi OVMF/machine/hda-contents/ &&
qemu-system-x86_64 -L OVMF/machine -bios OVMF/machine/bios.bin -hda fat:OVMF/machine/hda-contents
