
#include "types.h"
#include "console.h"
#include "floppy.h"
#include "print.h"
#include "fat.h"

void start() __attribute__((section(".text.start")));

void read_func(unsigned char* data_out, unsigned int offset_in, unsigned int size_in, void* user_args) {
	floppy_read(data_out, offset_in, size_in);
}

void write_func(unsigned char* data_in, unsigned int offset_in, unsigned int size_in, void* user_args) {
	floppy_write(data_in, offset_in, size_in, true);
}

void start() {
	con_init(80, 25);

	kprintf("\033[2J");

	if(floppy_init()){

		fat_DISK disk;
		if (fat_init(&disk, read_func, write_func, 0)) {
			if(fat_remove(&disk.root_directory, "files/mainlongfilename.c", 1)){
				kprintf("File removed\n");
			}
			else{
				kprintf("Error: Failed to remove file\n");
			}

			fat_DIR dir;
			if (fat_create_dir(&dir, &disk.root_directory, "files/test", 0)) {
				kprintf("Directory created\n");
			}
			else{
				kprintf("Error: Failed to create directory\n");
			}

			fat_FILE file;
			if(fat_fopen(&file, &disk.root_directory, "files/test/mainlongfilename8.c", "a")){
				
				// Write to the file
				fat_fwrite("Hello, World!\n", 1, 14, &file);

				// Read the file
				fat_fseek(&file, 0, fat_SEEK_END);
				unsigned int size = fat_ftell(&file);
				fat_fseek(&file, 0, fat_SEEK_SET);

				char buffer[size + 1];
				fat_fread(buffer, 1, size, &file);
				buffer[size] = '\0';

				fat_print_buffer(buffer, size);

			}
			else{
				kprintf("Error: Failed to open file\n");
			}
		}
		else{
			kprintf("Error: Failed to initialize FAT32 disk\n");
		}
	}
	else{
		kprintf("Error: Floppy not initialized\n");
	}

	done:
	kprintf("Done\n");

	while (true) {
		__asm("hlt");
	}
}
