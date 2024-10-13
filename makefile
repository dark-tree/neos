
# https://stackoverflow.com/questions/5618615/check-if-a-program-exists-from-a-makefile
REQUIRED_BINS := qemu-system-i386
$(foreach bin,$(REQUIRED_BINS), $(if $(shell command -v $(bin) 2> /dev/null),$(true),$(error Please install `$(bin)`)))

KERNEL_FLAGS = -fno-pie -nostdinc -std=gnu99 -fomit-frame-pointer -fno-builtin -nodefaultlibs -nostdlib -ffreestanding

.PHONY : clean all run debug

# Create the build directory
build:
	mkdir build

# Compile the boot loader
build/bootloader.bin: build src/boot.asm
	nasm -f bin src/boot.asm -o build/bootloader.bin

build/kernel.bin: build src/linker.ld src/kernel.c
	gcc -m32 -c src/kernel.c $(KERNEL_FLAGS) -O2 -Wall -Wextra -o build/kernel.o
	objcopy --remove-section .eh_frame --remove-section .rel.eh_frame --remove-section .rela.eh_frame build/kernel.o build/kernel.o
	ld -nostdlib -m elf_i386 -T src/linker.ld -o build/kernel.bin build/kernel.o

# Create the floppy disc image 940
build/floppy.img: build build/bootloader.bin src/tailcap build/kernel.bin
	dd if=/dev/zero of=build/floppy.img bs=1024 count=1440
	dd if=build/bootloader.bin of=build/floppy.img seek=0 count=1 conv=notrunc
	dd if=/dev/zero of=build/floppy.img seek=1 count=10 conv=notrunc
	bash -c 'printf "\x10"' | dd of=build/floppy.img seek=10 count=1 conv=notrunc
	bash -c 'printf "\x11"' | dd of=build/floppy.img seek=11 count=1 conv=notrunc
	bash -c 'printf "\x12"' | dd of=build/floppy.img seek=12 count=1 conv=notrunc
	bash -c 'printf "\x13"' | dd of=build/floppy.img seek=13 count=1 conv=notrunc
	bash -c 'printf "\x14"' | dd of=build/floppy.img seek=14 count=1 conv=notrunc
	bash -c 'printf "\x15"' | dd of=build/floppy.img seek=15 count=1 conv=notrunc
	bash -c 'printf "\x16"' | dd of=build/floppy.img seek=16 count=1 conv=notrunc
	bash -c 'printf "\x17"' | dd of=build/floppy.img seek=17 count=1 conv=notrunc
	bash -c 'printf "\x18"' | dd of=build/floppy.img seek=18 count=1 conv=notrunc
	bash -c 'printf "\x19"' | dd of=build/floppy.img seek=19 count=1 conv=notrunc
	bash -c 'printf "\x20"' | dd of=build/floppy.img seek=20 count=1 conv=notrunc
	bash -c 'printf "\x21"' | dd of=build/floppy.img seek=21 count=1 conv=notrunc
	bash -c 'printf "\x22"' | dd of=build/floppy.img seek=22 count=1 conv=notrunc
	bash -c 'printf "\x23"' | dd of=build/floppy.img seek=23 count=1 conv=notrunc
	bash -c 'printf "\x24"' | dd of=build/floppy.img seek=24 count=1 conv=notrunc
	bash -c 'printf "\x25"' | dd of=build/floppy.img seek=25 count=1 conv=notrunc
	bash -c 'printf "\x26"' | dd of=build/floppy.img seek=26 count=1 conv=notrunc
	bash -c 'printf "\x27"' | dd of=build/floppy.img seek=27 count=1 conv=notrunc
	bash -c 'printf "\x28"' | dd of=build/floppy.img seek=28 count=1 conv=notrunc
	bash -c 'printf "\x29"' | dd of=build/floppy.img seek=29 count=1 conv=notrunc
	bash -c 'printf "\x30"' | dd of=build/floppy.img seek=30 count=1 conv=notrunc
	bash -c 'printf "\x31"' | dd of=build/floppy.img seek=31 count=1 conv=notrunc
	bash -c 'printf "\x32"' | dd of=build/floppy.img seek=32 count=1 conv=notrunc
	bash -c 'printf "\x33"' | dd of=build/floppy.img seek=33 count=1 conv=notrunc
	bash -c 'printf "\x34"' | dd of=build/floppy.img seek=34 count=1 conv=notrunc
	bash -c 'printf "\x35"' | dd of=build/floppy.img seek=35 count=1 conv=notrunc
	bash -c 'printf "\x36"' | dd of=build/floppy.img seek=36 count=1 conv=notrunc
	bash -c 'printf "\x37"' | dd of=build/floppy.img seek=37 count=1 conv=notrunc
	bash -c 'printf "\x38"' | dd of=build/floppy.img seek=38 count=1 conv=notrunc
	bash -c 'printf "\x39"' | dd of=build/floppy.img seek=39 count=1 conv=notrunc
	bash -c 'printf "\x40"' | dd of=build/floppy.img seek=40 count=1 conv=notrunc
	bash -c 'printf "\x41"' | dd of=build/floppy.img seek=41 count=1 conv=notrunc
	bash -c 'printf "\x42"' | dd of=build/floppy.img seek=42 count=1 conv=notrunc
	bash -c 'printf "\x43"' | dd of=build/floppy.img seek=43 count=1 conv=notrunc
	bash -c 'printf "\x44"' | dd of=build/floppy.img seek=44 count=1 conv=notrunc
	bash -c 'printf "\x45"' | dd of=build/floppy.img seek=45 count=1 conv=notrunc
	bash -c 'printf "\x46"' | dd of=build/floppy.img seek=46 count=1 conv=notrunc
	bash -c 'printf "\x47"' | dd of=build/floppy.img seek=47 count=1 conv=notrunc
	bash -c 'printf "\x48"' | dd of=build/floppy.img seek=48 count=1 conv=notrunc
	bash -c 'printf "\x49"' | dd of=build/floppy.img seek=49 count=1 conv=notrunc


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
	rm -f build/*.o build/bootloader.bin build/floppy.img build/final.iso

# Invoke QEMU wihtout waiting for GDB
run: build/final.iso
	qemu-system-i386 -monitor stdio -cdrom ./build/final.iso -boot a

# Invoke QEMU and wait for GDB
debug: build/final.iso
	qemu-system-i386 -cdrom ./build/final.iso -boot a -s -S &
	gdb -ex 'target remote localhost:1234' -ex 'break *0x7c00' -ex 'c'

disasm: build/bootloader.bin
	ndisasm -b 16 -o 7c00h ./build/bootloader.bin
