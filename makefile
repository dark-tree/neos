
# https://stackoverflow.com/questions/5618615/check-if-a-program-exists-from-a-makefile
REQUIRED_BINS := qemu-system-i386
$(foreach bin,$(REQUIRED_BINS), $(if $(shell command -v $(bin) 2> /dev/null),$(true),$(error Please install `$(bin)`)))

# Configuration
CC_FLAGS = -nostdinc -fomit-frame-pointer -fno-builtin -nodefaultlibs -nostdlib -ffreestanding
LD = ld -melf_i386 -nostdlib
CC = gcc -m32 -fno-pie -std=gnu99 -Wall -Wextra $(CC_FLAGS) -O2
AS = nasm -f elf32
OC = objcopy -O binary

.PHONY : clean all run debug

# Create the build directory
build:
	mkdir build
	mkdir build/boot
	mkdir build/kernel

# Compile the bootloader, first stage
build/boot/load.bin: build src/boot/load.asm src/link/load.ld
	$(AS) src/boot/load.asm -o build/boot/load.o
	$(LD) -T src/link/load.ld -o build/boot/load.elf build/boot/load.o
	$(OC) build/boot/load.elf build/boot/load.bin

# Compile the bootloader, second stage
build/boot/start.bin: build src/boot/start.asm src/link/start.ld build/boot/load.bin
	$(AS) src/boot/start.asm -o build/boot/start.o
	$(LD) -T src/link/start.ld -o build/boot/start.elf build/boot/start.o
	$(OC) build/boot/start.elf build/boot/start.bin

build/kernel/kmalloc.o: build src/kernel/kmalloc.asm
	$(AS) src/kernel/kmalloc.asm -o build/kernel/kmalloc.o

build/kernel/console.o: build src/kernel/console.c
	$(CC) -c src/kernel/console.c -o build/kernel/console.o

build/kernel/memory.o: build src/kernel/memory.c
	$(CC) -c src/kernel/memory.c -o build/kernel/memory.o

build/kernel/entry.o: build src/kernel/entry.c
	$(CC) -c src/kernel/entry.c -o build/kernel/entry.o

# Compile the kernel to a flat binary
build/kernel/kernel.bin: build src/link/kernel.ld build/kernel/entry.o build/kernel/kmalloc.o build/kernel/console.o build/kernel/memory.o
	$(LD) -T src/link/kernel.ld -o build/kernel/kernel.o
	$(OC) --only-section=.text --only-section=.data build/kernel/kernel.o build/kernel/kernel.bin

# Create the floppy disc system image
build/floppy.img: build build/boot/load.bin build/boot/start.bin build/kernel/kernel.bin
	dd if=/dev/zero of=build/floppy.img bs=1024 count=1440
	dd if=build/boot/load.bin of=build/floppy.img bs=512 seek=0 count=1 conv=notrunc
	dd if=build/boot/start.bin of=build/floppy.img bs=512 seek=1 count=1 conv=notrunc
	dd if=build/kernel/kernel.bin of=build/floppy.img bs=512 seek=2 count=100 conv=notrunc

# Wrap into a ISO image file
build/final.iso: build build/floppy.img
	mkdir iso
	cp build/floppy.img iso/
	genisoimage -quiet -V 'neos' -input-charset iso8859-1 -o build/final.iso -b floppy.img -hide floppy.img iso/
	rm -rf ./iso

# Build all
all: build/final.iso

# Remove all build elements
clean:
	rm -rf ./build

# Invoke QEMU wihtout waiting for GDB
run: build/final.iso
	qemu-system-i386 -monitor stdio -cdrom ./build/final.iso -boot a

# Invoke QEMU and wait for GDB
debug: build/final.iso
	qemu-system-i386 -cdrom ./build/final.iso -boot a -s -S &
	gdb -ex 'target remote localhost:1234' -ex 'break *0x7c00' -ex 'c'

disasm: build/bootloader.bin
	ndisasm -b 16 -o 7c00h ./build/bootloader.bin
