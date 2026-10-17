#!/bin/sh
# Builds a DS test ROM that starts directly in a battle (for emulator perf checks).
set -e
B=/opt/wonderful/thirdparty/blocksds/core; P=/opt/wonderful/toolchain/gcc-arm-none-eabi/bin/arm-none-eabi-
ARCHF="-mthumb -mcpu=arm946e-s+nofp"; export BLOCKSDS=$B
mkdir -p build/t
${P}gcc -O2 $ARCHF -D__NDS__ -DARM9 -DDS_TEST_BATTLE=${1:-210} -DDS_TEST_MAP=${2:-10} -I$B/libs/libnds/include -specs=$B/sys/crts/ds_arm9.specs -c source/main.c -o build/t/main.o
${P}gcc -o build/t/test.elf build/t/main.o $(ls build/nds/*.o | grep -v main.o) $ARCHF -L$B/libs/libnds/lib -Wl,--gc-sections -Wl,--start-group -lnds9 -lc -Wl,--end-group -specs=$B/sys/crts/ds_arm9.specs
$B/tools/ndstool/ndstool -c build/t/test.nds -7 $B/sys/arm7/main_core/arm7_maxmod.elf -9 build/t/test.elf -g VXLD 01 VEXEL 0 >/dev/null
