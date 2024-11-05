#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define fat_DEBUG
#include "fat.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

void interactive_file_explorer(fat_DISK* disk) {
	printf("---===### FAT Explorer 2000 ###===---\n");
	printf("Commands:\n");
	printf("  ls               list files in current directory\n");
	printf("  cd <directory>   change directory\n");
	printf("  cat <file>       print file contents\n");
	printf("  save <file>      save file to disk\n");
	printf("  exit             exit the program\n");

	fat_DIR current_dir;

	fat_DIR tmp_dir;
	fat_FILE tmp_file;

	int type;
	char command[256];

	fat_copy_DIR(&current_dir, &disk->root_directory);

	while (1) {
		
		fat_print_longname(current_dir.long_filename);
		printf("> ");
		fgets(command, 256, stdin);
		command[strlen(command) - 1] = '\0';

		if (strcmp(command, "exit") == 0) {
			break;
		} 
		else {
			if (strncmp(command, "ls", 2) == 0) {
				fat_rewinddir(&current_dir);
				while (type = fat_readdir(&tmp_dir, &tmp_file, &current_dir)) {
					if (type == fat_FOUND_FILE) {
						fat_print_longname(tmp_file.long_filename);
						printf("\n");
					}
					else if (type == fat_FOUND_DIR) {
						fat_print_longname(tmp_dir.long_filename);
						printf("/\n");
					}
				}
				continue;
			}
			else if (strncmp(command, "cd ", 3) == 0) {
				char* path = command + 3;
				if (path[strlen(path) - 1] == '/') {
					path[strlen(path) - 1] = '\0';
				}
				if (fat_opendir(&tmp_dir, &current_dir, path)) {
					fat_copy_DIR(&current_dir, &tmp_dir);
				}
				else {
					printf("Error: Could not open directory\n");
				}
				continue;
			}
			else if (strncmp(command, "cat ", 4) == 0) {
				char* path = command + 4;
				if (fat_fopen(&tmp_file, &current_dir, path)) {
					fat_fseek(&tmp_file, 0, fat_SEEK_END);
					unsigned int size = fat_ftell(&tmp_file);
					fat_fseek(&tmp_file, 0, fat_SEEK_SET);

					char* buffer = (char*)malloc(size + 1);
					fat_fread(buffer, 1, size, &tmp_file);
					buffer[size] = '\0';

					printf("%s", buffer);
					free(buffer);
				}
				else {
					printf("Error: Could not open file\n");
				}
				continue;
			}
			else if (strncmp(command, "save ", 5) == 0) {
				char* path = command + 5;
				if (fat_fopen(&tmp_file, &current_dir, path)) {
					char buffer[256];
					fat_longname_to_string(tmp_file.long_filename, buffer);

					char save_path[256];
					realpath(buffer, save_path);
					printf("Saving file %s to %s\n", buffer, save_path);

					FILE* file = fopen(buffer, "wb");
					if (file == NULL) {
						printf("Error: Could not open save file\n");
					}
					else {
						fat_fseek(&tmp_file, 0, fat_SEEK_END);
						unsigned int size = fat_ftell(&tmp_file);
						fat_fseek(&tmp_file, 0, fat_SEEK_SET);

						char* buffer = (char*)malloc(size);
						fat_fread(buffer, 1, size, &tmp_file);

						// limit max file size to 256MB
						size = min(size, 0xfffffff);
						fwrite(buffer, 1, size, file);

						free(buffer);
						fclose(file);
					}
				}
				else {
					printf("Error: Could not open file\n");
				}
				continue;
			}
			else {
				printf("Error: Unknown command\n");
			}
		}
	}
}

void disk_read_func(unsigned char* data_out, unsigned int offset_in, unsigned int size_in, void* user_args) {
	FILE* file = (FILE*)user_args;
	fseek(file, offset_in, SEEK_SET);
	fread(data_out, 1, size_in, file);
}

int main() {
	const char* image_file = "floppy.img";

	// open the image file
	FILE* file = fopen(image_file, "rb");
	if (file == NULL) {
		printf("Error: Could not open file %s\n", image_file);
		return 1;
	}

	// create FAT disk with attached disk image access method
	fat_DISK disk;
	if(!fat_init(&disk, disk_read_func, file)) {
		return 1;
	}

	interactive_file_explorer(&disk);

	fclose(file);

	return 0;
}
