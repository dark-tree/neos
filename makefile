
# https://stackoverflow.com/questions/5618615/check-if-a-program-exists-from-a-makefile
REQUIRED_BINS := qemu-system-i386
$(foreach bin,$(REQUIRED_BINS), $(if $(shell command -v $(bin) 2> /dev/null),$(true),$(error Please install `$(bin)`)))

# Kernel assembly object files
KERNEL_AS = \
	build/kernel/kmalloc.o \
	build/kernel/tables.o \
	build/kernel/routine.o \
	build/kernel/pic.o \
	build/kernel/util.o \
	build/kernel/cursor.o \
	build/kernel/switch.o

# Kernel C object files
KERNEL_CC = \
	build/kernel/entry.o \
	build/kernel/console.o \
	build/kernel/print.o \
	build/kernel/memory.o \
	build/kernel/math.o \
	build/kernel/io.o \
	build/kernel/floppy.o \
	build/kernel/fat.o \
	build/kernel/interrupt.o \
	build/kernel/syscall.o \
	build/kernel/scheduler.o \
	build/kernel/mem.o \
	build/kernel/rivendell.o \
	build/kernel/vfs.o \
	build/kernel/procfs.o \
	build/kernel/fatfs.o \
	build/kernel/gdt.o

# Configuration
CC_FLAGS = -nostdinc -fomit-frame-pointer -fno-builtin -nodefaultlibs -nostdlib -ffreestanding -g
LD = ld -melf_i386 -nostdlib -g
CC = gcc -m32 -fno-pie -std=gnu99 -Wall -Wextra $(CC_FLAGS) -O0 -c
AS = nasm -f elf32
OC = objcopy -O binary

.PHONY : clean image all run debug

# Create the build directory
build: src/kernel/systable.h
	mkdir -p build/boot
	mkdir -p build/kernel

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

# Describes the recipes for all kernel .c files
$(KERNEL_CC): build/kernel/%.o: src/kernel/%.c build
	$(CC) $< -o $@

# Describes the recipes for all kernel .asm files
$(KERNEL_AS): build/kernel/%.o: src/kernel/%.asm build
	$(AS) $< -o $@

# Construct the symbol file
build/kernel.dwarf: build src/link/kernel.ld $(KERNEL_AS) $(KERNEL_CC)
	$(LD) -T src/link/debug.ld -o build/kernel.dwarf $(KERNEL_CC) $(KERNEL_AS)

# Compile the kernel to a flat binary
build/kernel/kernel.bin: build src/link/kernel.ld $(KERNEL_AS) $(KERNEL_CC)
	$(LD) -T src/link/kernel.ld -o build/kernel/kernel.o $(KERNEL_CC) $(KERNEL_AS)
	$(OC) --only-section=.text --only-section=.data build/kernel/kernel.o build/kernel/kernel.bin

# Create the floppy disc system image
build/floppy.img: build build/boot/load.bin build/boot/start.bin build/kernel/kernel.bin
	dd if=/dev/zero of=build/floppy.img bs=1024 count=1440
	dd if=build/boot/load.bin of=build/floppy.img bs=512 seek=0 count=1 conv=notrunc
	dd if=build/boot/start.bin of=build/floppy.img bs=512 seek=1 count=1 conv=notrunc
	dd if=build/kernel/kernel.bin of=build/floppy.img bs=512 seek=2 count=200 conv=notrunc

# Wrap into a ISO image file
build/final.iso: build build/floppy.img
	mkdir -p build/iso
	cp build/floppy.img build/iso/
	genisoimage -quiet -V 'neos' -input-charset iso8859-1 -o build/final.iso -b floppy.img -hide floppy.img build/iso/
	rm -rf ./build/iso

# Rebuild syscall table
src/kernel/systable.h: util/sysgen.py
	rm -f src/kernel/systable.h
	python3 util/sysgen.py > src/kernel/systable.h

# Generate the floppy disk image with FAT filesystem
disks/floppy.img:
	$(MAKE) makefile -C ./disks image

# Build all
all: build/final.iso image

# Remove all build elements
clean:
	rm -rf ./build
	$(MAKE) makefile -C ./disks clean

# Invoke QEMU wihtout waiting for GDB
run: build/final.iso disks/floppy.img
	rm -f ./output
	qemu-system-i386 -m 2G -monitor stdio -cdrom ./build/final.iso -boot a -drive file=./disks/floppy.img,if=floppy,index=1,format=raw -d cpu_reset -D ./output

# Invoke QEMU and wait for GDB
debug: build/final.iso build/kernel.dwarf disks/floppy.img
	qemu-system-i386 -m 2G -cdrom ./build/final.iso -boot a -s -S -drive file=./disks/floppy.img,if=floppy,index=1,format=raw &
	gdb -ex 'target remote localhost:1234' -ex 'symbol-file build/kernel.dwarf' -ex 'break *0x8000' -ex 'c'

disasm: build/bootloader.bin
	ndisasm -b 16 -o 7c00h ./build/bootloader.bin
