
ARMGNU ?= arm-none-eabi

COPS = -g3 -O2 -Wall -nostdlib -nostartfiles -ffreestanding -mcpu=cortex-a53

gcc : kernel.bin

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

kernel.o : kernel.c
	$(ARMGNU)-gcc $(COPS) -c kernel.c -o kernel.o

periph.o : periph.c
	$(ARMGNU)-gcc $(COPS) -c periph.c -o periph.o

timer.o : timer.c
	$(ARMGNU)-gcc $(COPS) -c timer.c -o timer.o

mmu.o : mmu.c
	$(ARMGNU)-gcc $(COPS) -c mmu.c -o mmu.o

kernel.bin : memmap start.o periph.o kernel.o timer.o mmu.o
	$(ARMGNU)-ld start.o mmu.o periph.o kernel.o timer.o -T memmap -o kernel.elf
	$(ARMGNU)-objdump -D kernel.elf > kernel.list
	$(ARMGNU)-objcopy kernel.elf -O ihex kernel.hex
	$(ARMGNU)-objcopy kernel.elf -O binary kernel.bin

run: kernel.bin
	qemu-system-aarch64 -M raspi2 -cpu cortex-a53 -smp 4 -m 1G -serial null -serial mon:stdio -kernel kernel.elf

debug: kernel.bin
	konsole -e arm-none-eabi-gdb -ex "target remote:1235" -ex "set confirm off" -ex "add-symbol-file kernel.elf" &
	qemu-system-aarch64 -M raspi2 -cpu cortex-a53  -gdb tcp::1235 -S -serial null -serial mon:stdio -kernel kernel.elf

debug-gdb: kernel.bin
	konsole -e gdb-multiarch -ex "target remote:1235" -ex "set confirm off" -ex "add-symbol-file kernel.elf" &
	qemu-system-aarch64 -M raspi2 -cpu cortex-a53 -smp 4 -m 1G -gdb tcp::1235 -S -serial null -serial mon:stdio -kernel kernel.elf
