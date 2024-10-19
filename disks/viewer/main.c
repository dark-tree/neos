#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FAT32_DEBUG
#define FAT32_PRINT
#include "fat32.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

void interactive_file_explorer(fat32_DISK* disk) {
	printf("---===### FAT Explorer 2000 ###===---\n");
	printf("Commands:\n");
	printf("  ls               list files in current directory\n");
	printf("  cd <directory>   change directory\n");
	printf("  cat <file>       print file contents\n");
	printf("  save <file>      save file to disk\n");
	printf("  exit             exit the program\n");

	fat32_DIR current_dir;

	fat32_DIR tmp_dir;
	fat32_FILE tmp_file;

	int type;
	char command[256];

	fat32_copy_DIR(&current_dir, &disk->root_directory);

	while (1) {
		
		print_longname(current_dir.long_filename);
		printf("> ");
		fgets(command, 256, stdin);
		command[strlen(command) - 1] = '\0';

		if (strcmp(command, "exit") == 0) {
			break;
		} 
		else {
			if (strncmp(command, "ls", 2) == 0) {
				fat32_rewinddir(&current_dir);
				while (type = fat32_readdir(&tmp_dir, &tmp_file, &current_dir)) {
					if (type == fat32_FOUND_FILE) {
						print_longname(tmp_file.long_filename);
						printf("\n");
					}
					else if (type == fat32_FOUND_DIR) {
						print_longname(tmp_dir.long_filename);
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
				if (fat32_opendir(&tmp_dir, &current_dir, path)) {
					fat32_copy_DIR(&current_dir, &tmp_dir);
				}
				else {
					printf("Error: Could not open directory\n");
				}
				continue;
			}
			else if (strncmp(command, "cat ", 4) == 0) {
				char* path = command + 4;
				if (fat32_fopen(&tmp_file, &current_dir, path)) {
					fat32_fseek(&tmp_file, 0, fat32_SEEK_END);
					unsigned int size = fat32_ftell(&tmp_file);
					fat32_fseek(&tmp_file, 0, fat32_SEEK_SET);

					char* buffer = (char*)malloc(size + 1);
					fat32_fread(buffer, 1, size, &tmp_file);
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
				if (fat32_fopen(&tmp_file, &current_dir, path)) {
					char buffer[256];
					longname_to_string(tmp_file.long_filename, buffer);

					char save_path[256];
					realpath(buffer, save_path);
					printf("Saving file %s to %s\n", buffer, save_path);

					FILE* file = fopen(buffer, "wb");
					if (file == NULL) {
						printf("Error: Could not open save file\n");
					}
					else {
						fat32_fseek(&tmp_file, 0, fat32_SEEK_END);
						unsigned int size = fat32_ftell(&tmp_file);
						fat32_fseek(&tmp_file, 0, fat32_SEEK_SET);

						char* buffer = (char*)malloc(size);
						fat32_fread(buffer, 1, size, &tmp_file);

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

	FILE* file = fopen(image_file, "rb");
	if (file == NULL) {
		printf("Error: Could not open file %s\n", image_file);
		return 1;
	}

	fat32_DISK disk;
	if(!fat32_init(&disk, disk_read_func, file)) {
		return 1;
	}

	interactive_file_explorer(&disk);

	fclose(file);

	return 0;
}
