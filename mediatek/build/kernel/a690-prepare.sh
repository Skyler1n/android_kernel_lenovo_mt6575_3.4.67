#!/bin/bash
set -euo pipefail
SOURCE=$(cd "$(dirname "$0")/../../.." && pwd)
export TARGET_PRODUCT=lcsh75_cu_gb2 PROJECT=lcsh75_cu_gb2 FULL_PROJECT=lcsh75_cu_gb2
export TARGET_BUILD_VARIANT=${TARGET_BUILD_VARIANT:-eng} ARCH=arm
export CROSS_COMPILE=${CROSS_COMPILE:-arm-eabi-}
export MTK_PLATFORM=MT6575 ARCH_MTK_PLATFORM=mt6575
export MTK_GPL_PACKAGE=no MTK_CUSTGEN_ERROR=no LEGACY_DFO_GEN=yes
export MTK_ROOT_CUSTOM="$SOURCE/mediatek/custom"
export MTK_ROOT_CONFIG=mediatek/config OUT_DIR=out
export MTK_NAND_PAGE_SIZE=4K MTK_EMMC_SUPPORT=no MTK_EMMC_SUPPORT_OTP=no
export MTK_NAND_UBIFS_SUPPORT=no MTK_SHARED_SDCARD=no MTK_CIP_SUPPORT=no
export MTK_ROOT_OUT=out/target/product/lcsh75_cu_gb2/obj
export MTK_ROOT_CONFIG_OUT=$MTK_ROOT_OUT/CUSTGEN/config
cd "$SOURCE"
# Cleaning/help targets must not recreate configuration or generated headers.
prepare=yes
if [ "$#" -gt 0 ]; then
  prepare=no
  for goal in "$@"; do
    case "$goal" in clean|mrproper|distclean|help|kernelversion|kernelrelease) ;;
      *) prepare=yes ;;
    esac
  done
fi
copy_changed() {
  if ! cmp -s "$1" "$2"; then cp "$1" "$2"; fi
}
if [ "$prepare" = yes ]; then
  # The upstream PTGEN workbook has no A690 NAND sheet. These checked-in
  # headers describe the confirmed stock NAND geometry, not an eMMC table.
  board=mediatek/custom/lcsh75_cu_gb2
  mkdir -p "$MTK_ROOT_OUT/PTGEN/common" "$MTK_ROOT_OUT/PTGEN/kernel" "$MTK_ROOT_OUT/NANDGEN/common"
  for file in partition_define.h pmt.h; do
    copy_changed "$board/common/$file" "$MTK_ROOT_OUT/PTGEN/common/$file"
  done
  for file in partition.h partition_define_private.h; do
    copy_changed "$board/kernel/core/src/$file" "$MTK_ROOT_OUT/PTGEN/kernel/$file"
  done
  perl mediatek/build/tools/emigen/MT6575/nandgen.pl
  "${MAKE:-make}" MTK_ROOT_CUSTOM="$SOURCE/mediatek/custom" -f mediatek/build/custgen.mk MTK_CUSTGEN_CUSTOM_FILES='kernel common' all
  # gendfoboot.pl reads its table from the environment, not ProjectConfig.mk.
  dfo_config=mediatek/config/lcsh75_cu_gb2/ProjectConfig.mk
  export DFO_MISC="$(sed -n 's/^DFO_MISC[[:space:]]*=[[:space:]]*//p' "$dfo_config" | tail -n 1)"
  test -n "$DFO_MISC"
  for key in $DFO_MISC; do
    value=$(sed -n "s/^${key}[[:space:]]*=[[:space:]]*//p" "$dfo_config" | tail -n 1)
    test -n "$value"
    export "$key=$value"
  done
  mkdir -p kernel/include/mach
  for mode in boot bootdft; do
    if [ "$mode" = boot ]; then header=kernel/include/mach/dfo_boot.h; else header=kernel/include/mach/dfo_boot_default.h; fi
    perl mediatek/build/tools/gendfoboot.pl "$mode" "$header.new"
    copy_changed "$header.new" "$header"
    rm "$header.new"
  done
  if [ ! -f kernel/.config ]; then
    cp kernel/arch/arm/configs/lcsh75_cu_gb2_defconfig kernel/.config
  fi
fi
cd kernel
# The original MTK shell initializer exports the complete ProjectConfig table
# and relative MTK paths used throughout recursive Kbuild makefiles.
set +u
source ../mediatek/build/shell.sh ../ kernel
set -u
exec "${MAKE:-make}" A690_MAKE_READY=1 "$@"
