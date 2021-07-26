
ARMGNU ?= arm-none-eabi

COPS = -g3 -O0 -Wall -O2 -nostdlib -nostartfiles -ffreestanding -mcpu=cortex-a53

gcc : multi00.bin

all : gcc

clean :
	rm -f *.o
	rm -f *.bin
	rm -f *.hex
	rm -f *.elf
	rm -f *.list
	rm -f *.img
	rm -f *.bc
	rm -f *.clang.s

start.o : start.s
	$(ARMGNU)-gcc $(COPS) -c start.s -o start.o

multi00.o : multi00.c
	$(ARMGNU)-gcc $(COPS) -c multi00.c -o multi00.o

periph.o : periph.c
	$(ARMGNU)-gcc $(COPS) -c periph.c -o periph.o

handler.o : handler.c
	$(ARMGNU)-gcc $(COPS) -c handler.c -o handler.o

mmu.o : mmu.c
	$(ARMGNU)-gcc $(COPS) -c mmu.c -o mmu.o

multi00.bin : memmap start.o periph.o multi00.o handler.o mmu.o
	$(ARMGNU)-ld start.o mmu.o periph.o multi00.o handler.o -T memmap -o multi00.elf
	$(ARMGNU)-objdump -D multi00.elf > multi00.list
	$(ARMGNU)-objcopy multi00.elf -O ihex multi00.hex
	$(ARMGNU)-objcopy multi00.elf -O binary multi00.bin


run: multi00.bin
	qemu-system-aarch64 -M raspi2 -cpu cortex-a53  -serial null -serial mon:stdio -kernel multi00.elf

debug: multi00.bin
	konsole -e arm-none-eabi-gdb -ex "target remote:1235" -ex "set confirm off" -ex "add-symbol-file multi00.elf" &
	qemu-system-aarch64 -M raspi2 -cpu cortex-a53  -gdb tcp::1235 -S -serial null -serial mon:stdio -kernel multi00.elf

debug-gdb: multi00.bin
	konsole -e gdb-multiarch -ex "target remote:1235" -ex "set confirm off" -ex "add-symbol-file multi00.elf" &
	qemu-system-aarch64 -M raspi2 -cpu cortex-a53  -gdb tcp::1235 -S -serial null -serial mon:stdio -kernel multi00.elf


# LOPS = -Wall -m32 -emit-llvm
# LLCOPS0 = -march=arm
# LLCOPS1 = -march=arm -mcpu=arm1176jzf-s
# LLCOPS = $(LLCOPS1)
# COPS = -Wall  -O2 -nostdlib -nostartfiles -ffreestanding
# OOPS = -std-compile-opts

# clang : multi00.clang.bin

# multi00.bc : multi00.c
# 	clang $(LOPS) -c multi00.c -o multi00.bc

# periph.bc : periph.c
# 	clang $(LOPS) -c periph.c -o periph.bc

# multi00.clang.elf : memmap start.o multi00.bc periph.bc
# 	llvm-link periph.bc multi00.bc -o multi00.nopt.bc
# 	opt $(OOPS) multi00.nopt.bc -o multi00.opt.bc
# 	llc $(LLCOPS) multi00.opt.bc -o multi00.clang.s
# 	$(ARMGNU)-as multi00.clang.s -o multi00.clang.o
# 	$(ARMGNU)-ld -o multi00.clang.elf -T memmap start.o multi00.clang.o
# 	$(ARMGNU)-objdump -D multi00.clang.elf > multi00.clang.list

# multi00.clang.bin : multi00.clang.elf
# 	$(ARMGNU)-objcopy multi00.clang.elf multi00.clang.hex -O ihex
# 	$(ARMGNU)-objcopy multi00.clang.elf multi00.clang.bin -O binary


