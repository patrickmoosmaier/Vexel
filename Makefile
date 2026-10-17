# VEXEL - DS build (BlocksDS)
export BLOCKSDS ?= /opt/wonderful/thirdparty/blocksds/core
WONDERFUL_TOOLCHAIN ?= /opt/wonderful
PREFIX   := $(WONDERFUL_TOOLCHAIN)/toolchain/gcc-arm-none-eabi/bin/arm-none-eabi-
CC       := $(PREFIX)gcc
NAME     := VEXEL
ROM      := $(NAME).nds
ARM7ELF  := $(BLOCKSDS)/sys/arm7/main_core/arm7_maxmod.elf
SPECS    := $(BLOCKSDS)/sys/crts/ds_arm9.specs
ARCH     := -mthumb -mcpu=arm946e-s+nofp
DEFINES  := -D__NDS__ -D__BLOCKSDS__ -DARM9
CFLAGS   := -Wall -Wno-unused-function -O2 $(ARCH) -ffunction-sections -fdata-sections $(DEFINES) \
            -I$(BLOCKSDS)/libs/libnds/include -specs=$(SPECS)
LDFLAGS  := $(ARCH) -L$(BLOCKSDS)/libs/libnds/lib -Wl,--gc-sections -Wl,-Map,build/$(NAME).map \
            -Wl,--start-group -lnds9 -lc -Wl,--end-group -specs=$(SPECS)
SRC      := $(wildcard source/*.c)
OBJ      := $(patsubst source/%.c,build/nds/%.o,$(SRC))

all: $(ROM)

build/nds/%.o: source/%.c $(wildcard source/*.h)
	@mkdir -p build/nds
	$(CC) $(CFLAGS) -c -o $@ $<

build/nds/gen_%.o: source/gen_%.c
	@mkdir -p build/nds
	$(CC) $(CFLAGS) -O1 -c -o $@ $<

build/$(NAME).elf: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

$(ROM): build/$(NAME).elf icon.bmp
	$(BLOCKSDS)/tools/ndstool/ndstool -c $@ -7 $(ARM7ELF) -9 build/$(NAME).elf \
		-b icon.bmp "VEXEL;Protokoll Null;Neo-Kyra 2187" -g VXLD 01 "VEXEL" 0

assets:
	python3 tools/gen_gfx.py source/gen_gfx.c
	python3 tools/gen_mon.py source/gen_mon.c
	python3 tools/gen_maps.py source/gen_maps.c
	python3 tools/gen_icon.py icon.bmp

clean:
	rm -rf build $(ROM)
