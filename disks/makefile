.PHONY : all clean image build run

all: image run

image:
	@echo "Creating floppy image..."
	dd if=/dev/zero of=floppy.img count=1440 bs=1k
	mkfs.msdos -F 32 floppy.img
	mcopy -s -i floppy.img files/ ::/

build:
	@echo "Building..."
	if [ ! -d "viewer/build" ]; then mkdir viewer/build; fi
	gcc viewer/main.c -I../src/kernel -o viewer/build/main

clean:
	@echo "Cleaning up..."
	rm -rf viewer/build
	rm -f floppy.img

run: build
	@echo "Running..."
	viewer/build/main

