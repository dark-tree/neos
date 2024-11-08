#include "fat.h"
#include "print.h"

#ifndef NULL
    #define NULL 0
#endif

/* private helper functions */

static char to_lowercase(char c) {
	if (c >= 'A' && c <= 'Z') {
		return c + ('a' - 'A');
	}
	return c;
}

static void to_lowercase_str(char* str) {
	while (*str) {
		*str = to_lowercase(*str);
		str++;
	}
}

static void shortname_to_longname(const char* shortname, char* longname) {
	/*
	* Convert a short name to a long name.
	* The short name is in the format "FILE.TXT" and the long name is in the format "FILE    TXT".
	* The short name is 11 characters long. First 8 characters are the name and the last 3 characters are the extension.
	* If the name is less than 8 characters, it is padded with spaces.
	*/

	// Cast to short* so that we can write 2 bytes at a time
	short* longname_short = (short*)longname;

	for (int i = 0; i < 8; i++) {
		if (shortname[i] == ' ') {
			break;
		}
		*(longname_short++) = shortname[i];
	}
	*(longname_short++) = '.';
	for (int i = 8; i < 11; i++) {
		if (shortname[i] == ' ') {
			if (i == 8) {
				// No extension - remove the dot
				*(longname_short - 1) = 0;
			}
			break;
		}
		*(longname_short++) = shortname[i];
	}
	*(longname_short++) = 0;
}

static int path_compare(const char* path, const char* dir_name, unsigned char char_size, unsigned char case_sensitive) {
	/*
	* Check if the directory name is a part of the path.
	* If it is, return the length of the directory name.
	* Otherwise, return 0.
	*/
	int i = 0;
	while (path[i] != '\0' && dir_name[i * char_size] != '\0' && path[i] != '/') {
		if (case_sensitive) {
			if (path[i] != dir_name[i * char_size]) {
				return 0;
			}
		}
		else {
			if (to_lowercase(path[i]) != to_lowercase(dir_name[i * char_size])) {
				return 0;
			}
		}
		i++;
	}

	if (dir_name[i * char_size] == '\0' && (path[i] == '/' || path[i] == '\0')) {
		return i;
	}

	return 0;
}

static void memory_copy(unsigned char* dest, unsigned char* src, unsigned int size) {
	for (unsigned int i = 0; i < size; i++) {
		dest[i] = src[i];
	}
}

/* FAT32 functions */

static unsigned int read_fat_entry(fat_DISK* disk, unsigned int cluster) {
	unsigned int fat_offset = disk->bpb.BPB_RsvdSecCnt * disk->bpb.BPB_BytsPerSec;

	// Each entry is 4 bytes long
	unsigned int fat_entry = fat_offset + cluster * 4;
	disk->read_func((unsigned char*)&fat_entry, fat_entry, sizeof(fat_entry), disk->user_args);

	return fat_entry;
}

static void write_fat_entry(fat_DISK* disk, unsigned int cluster, unsigned int value) {
	unsigned int fat_offset = disk->bpb.BPB_RsvdSecCnt * disk->bpb.BPB_BytsPerSec;

	// Each entry is 4 bytes long
	unsigned int fat_entry = fat_offset + cluster * 4;
	// Update the FAT table
	disk->write_func((unsigned char*)&value, fat_entry, sizeof(value), disk->user_args);
	// Update the backup FAT table
	unsigned int fat_size = disk->bpb.BPB_FATSz32 * disk->bpb.BPB_BytsPerSec;
	disk->write_func((unsigned char*)&value, fat_entry + fat_size, sizeof(value), disk->user_args);
}

void fat_fread(void* data_out, unsigned int element_size, unsigned int element_count, fat_FILE* file) {
	/*
	* In order to read the contents of a file, first we need to read the DIR_FstClusLO value from the directory entry.
	* This value is the first cluster of the file where the data is stored.
	* In order to access the rest of the file, we need to read the FAT table.
	* The FAT table contains the cluster number of the next cluster in the file.
	*/

	fat_dir_entry* file_dir = &file->fat_dir;

	unsigned int first_fat_sector = file->disk->bpb.BPB_RsvdSecCnt;
	unsigned int first_data_sector = first_fat_sector + (file->disk->bpb.BPB_NumFATs * file->disk->bpb.BPB_FATSz32);

	unsigned int current_file_cluster = file_dir->DIR_FstClusLO;
	unsigned int cluster_read_data_offset = 0;
	unsigned int read_total_bytes = 0;

	unsigned int cluster_size = file->disk->bpb.BPB_SecPerClus * file->disk->bpb.BPB_BytsPerSec;

	unsigned int data_size = element_size * element_count;

	while (1) {
		if (current_file_cluster < 2) {
			break;
		}

		unsigned int next_cluster = read_fat_entry(file->disk, current_file_cluster);

		if (next_cluster == 0x0) {
			// Cluster is free
			break;
		}

		if (next_cluster == 0x0FFFFFF7) {
			// Cluster is bad
			break;
		}

		unsigned int cluster_offset = first_data_sector + (current_file_cluster - 2) * file->disk->bpb.BPB_SecPerClus;

		// Don't read more than the file size
		cluster_read_data_offset += cluster_size;
		unsigned int part_size = (cluster_read_data_offset > file_dir->DIR_FileSize) ? cluster_size - (cluster_read_data_offset - file_dir->DIR_FileSize) : cluster_size;

		if (cluster_read_data_offset > file->cursor) {
			unsigned int bytes_to_read = (data_size > (part_size + read_total_bytes)) ? part_size : (data_size - read_total_bytes);
			unsigned int src_read_offset = cluster_offset * file->disk->bpb.BPB_BytsPerSec;
			
			if (cluster_read_data_offset - file->cursor <= cluster_size) {
				// if reading from the first cluster, we might want to skip the first bytes
				// next clusters will always be read from the beginning
				unsigned int skip_bytes = cluster_size - (cluster_read_data_offset - file->cursor);
				src_read_offset += skip_bytes;

				if (bytes_to_read + skip_bytes > part_size) {
					// If the read goes over the cluster boundary, trim the read size
					bytes_to_read -= (bytes_to_read + skip_bytes) - part_size;
				}
			}

			// Read the part of the cluster that is needed
			// subtract the size of the cluster because it has already been added to the total size
			file->disk->read_func((unsigned char*)data_out + read_total_bytes, src_read_offset, bytes_to_read, file->disk->user_args);

			read_total_bytes += bytes_to_read;
		}

		if (next_cluster >= 0x0FFFFFF8 && next_cluster <= 0x0FFFFFFF) {
			// Last cluster in the file.
			// May be interpreted as an allocated cluster and the final	cluster in the file(indicating end-of-file condition).
			break;
		}

		if (next_cluster == 0xFFFFFFFF) {
			// Cluster is allocated and is the final cluster for the file (indicates end-of-file).
			break;
		}

		current_file_cluster = next_cluster & 0x0FFFFFFF;
	}

	file->cursor += read_total_bytes;
}

static void fat_update_entry(fat_FILE* dir) {
	dir->disk->write_func((unsigned char*)(&dir->fat_dir), dir->entry_position, sizeof(fat_dir_entry), dir->disk->user_args);
}

unsigned char fat_fwrite(void* data_in, unsigned int element_size, unsigned int element_count, fat_FILE* file) {
	fat_dir_entry* file_dir = &file->fat_dir;

	unsigned int first_fat_sector = file->disk->bpb.BPB_RsvdSecCnt;
	unsigned int first_data_sector = first_fat_sector + (file->disk->bpb.BPB_NumFATs * file->disk->bpb.BPB_FATSz32);

	unsigned int current_file_cluster = file_dir->DIR_FstClusLO;
	unsigned int cluster_write_data_offset = 0;
	unsigned int write_total_bytes = 0;

	unsigned int cluster_size = file->disk->bpb.BPB_SecPerClus * file->disk->bpb.BPB_BytsPerSec;

	unsigned int data_size = element_size * element_count;

	while (1) {
		if (current_file_cluster < 2) {
			break;
		}

		unsigned int next_cluster = read_fat_entry(file->disk, current_file_cluster);

		if (next_cluster == 0x0FFFFFF7) {
			// Cluster is bad
			break;
		}

		if (next_cluster == 0x0 || (next_cluster >= 0x0FFFFFF8 && next_cluster <= 0x0FFFFFFF)) {
			// Cluster is free, allocate a new cluster or if it is the last cluster, check if the data fits
			if (file->cursor + data_size <= cluster_write_data_offset + cluster_size) {
				// Set the current cluster as the last cluster
				next_cluster = 0x0FFFFFFF;
			}
			else{
				// The remaining data does not fit in the current cluster
				// Find next free cluster
				unsigned int disk_size = (file->disk->bpb.BPB_TotSec32 == 0) ? file->disk->bpb.BPB_TotSec16 : file->disk->bpb.BPB_TotSec32;
				unsigned int free_cluster = current_file_cluster + 1;
				while (read_fat_entry(file->disk, free_cluster) != 0x0) {
					free_cluster++;
					if (free_cluster * file->disk->bpb.BPB_SecPerClus >= disk_size) {
						// No free clusters
						return 0;
					}
				}
				// Set the current cluster to point to the next cluster
				next_cluster = free_cluster;
			}

			write_fat_entry(file->disk, current_file_cluster, next_cluster);
		}

		unsigned int cluster_offset = first_data_sector + (current_file_cluster - 2) * file->disk->bpb.BPB_SecPerClus;
		
		cluster_write_data_offset += cluster_size;
		unsigned int part_size = cluster_size;

		if (cluster_write_data_offset > file->cursor) {
			unsigned int bytes_to_write = (data_size > (part_size + write_total_bytes)) ? part_size : (data_size - write_total_bytes);
			unsigned int dst_write_offset = cluster_offset * file->disk->bpb.BPB_BytsPerSec;
			
			if (cluster_write_data_offset - file->cursor <= cluster_size) {
				// if writing to the first cluster, we might want to skip the first bytes
				// next clusters will always be written from the beginning
				unsigned int skip_bytes = cluster_size - (cluster_write_data_offset - file->cursor);
				dst_write_offset += skip_bytes;

				if (bytes_to_write + skip_bytes > part_size) {
					// If the read goes over the cluster boundary, trim the read size
					bytes_to_write -= (bytes_to_write + skip_bytes) - part_size;
				}
			}

			// Write the part of the cluster that is needed
			// subtract the size of the cluster because it has already been added to the total size
			file->disk->write_func((unsigned char*)data_in + write_total_bytes, dst_write_offset, bytes_to_write, file->disk->user_args);

			write_total_bytes += bytes_to_write;
		}


		if (next_cluster >= 0x0FFFFFF8 && next_cluster <= 0x0FFFFFFF) {
			// Last cluster in the file.
			// May be interpreted as an allocated cluster and the final	cluster in the file(indicating end-of-file condition).
			break;
		}

		if (next_cluster == 0xFFFFFFFF) {
			// Cluster is allocated and is the final cluster for the file (indicates end-of-file).
			break;
		}

		current_file_cluster = next_cluster & 0x0FFFFFFF;
	}

	file->cursor += write_total_bytes;

	if (file->cursor > file_dir->DIR_FileSize) {
		file_dir->DIR_FileSize = file->cursor;
		fat_update_entry(file);
	}

	return 1;
}

void fat_fseek(fat_FILE* file, int offset, int origin) {
	if (origin == fat_SEEK_SET) {
		file->cursor = offset;
	}
	else if (origin == fat_SEEK_CUR) {
		file->cursor += offset;
	}
	else if (origin == fat_SEEK_END) {
		file->cursor = file->fat_dir.DIR_FileSize + offset;
	}
}

int fat_ftell(fat_FILE* file) {
	return file->cursor;
}

// Returns fat_NOT_FOUND if not found, fat_FOUND_DIR if found and is a directory, fat_FOUND_FILE if found and is a file
// If read_at_cursor is set to 1, the function will read the entry at the cursor position and return the result. The enrtry can be a file or a directory.
// If is_file is set to 1, the function will return fat_FOUND_FILE only if the entry is a file. fat_FOUND_DIR will not be returned, even if the directory with the same name exists.
static int fat_find(fat_DIR* subdir_out, fat_FILE* file_out, fat_DIR* root_dir, const char* path, unsigned char is_file, unsigned char read_at_cursor) {
	subdir_out->disk = root_dir->disk;
	if (file_out != NULL) {
		file_out->disk = root_dir->disk;
	}

	// Special case for the root directory
	if (root_dir->fat_dir.DIR_FstClusLO < 2) {
		root_dir->fat_dir.DIR_FstClusLO = 2;
	}

	// The directory is a file that contains the directory entries
	fat_FILE directory_file = {
		.disk = subdir_out->disk,
		.fat_dir = root_dir->fat_dir,
		.cursor = 0
	};
	// Directories do not hold information about their size so
	// set the file size to -1 which is maximum unsigned int value
	// the arguments of the read function will control the size of the read
	directory_file.fat_dir.DIR_FileSize = -1;

	// For iterating through the directory
	unsigned int dir_offset = 0;
	unsigned int entries = 0;

	// Long file name:
	// "Up to 20 of these 13-character entries may be chained, supporting a maximum length of 255 UCS-2 characters"
	// 20 * 13 = 260 just to be safe
	// 260 * 2 because each character is 2 bytes
	unsigned char lfn_present = 0;

	int found = fat_NOT_FOUND;

	while (1) {
		// Read the directory entry
		// TODO: fat_fread function performs at least 2 reads each time it's called, and it's called for each directory entry which is a lot of reads,
		// in real world application it would probably be better to read for example one cluster at a time and then iterate through the entries in memory.
		// The way it's implemented now is more readable though, so it is what it is for now.
		fat_fseek(&directory_file, dir_offset, fat_SEEK_SET);
		fat_fread((unsigned char*)&subdir_out->fat_dir, sizeof(fat_dir_entry), 1, &directory_file);
		dir_offset += sizeof(fat_dir_entry);

		if (subdir_out->fat_dir.DIR_Name[0] == 0x00) {
			// "0x00 is an additional indicator that all directory entries following the current free entry are also free"
			break;
		}

		if (subdir_out->fat_dir.DIR_Name[0] == 0xE5) {
			// "indicates the directory entry is free (available)"
			continue;
		}

		if (subdir_out->fat_dir.DIR_Attr == 0x0F) {
			// Long file name
			unsigned char long_filename_index = subdir_out->fat_dir.DIR_Name[0] & 0x0F;
			unsigned int long_filename_offset = (long_filename_index - 1) * (13 * 2);

			// the size of the directory entry is 32 bytes and the DIR_Name field is in the beginning of the struct
			// so by accessing indices higher than 11 (size of DIR_Name), we can access the rest of the fields 
			// in struct without worrying about their names
			for (int i = 0; i < 5 * 2; i++) {
				subdir_out->long_filename[long_filename_offset + i] = subdir_out->fat_dir.DIR_Name[1 + i];
			}
			for (int i = 0; i < 6 * 2; i++) {
				subdir_out->long_filename[long_filename_offset + 10 + i] = subdir_out->fat_dir.DIR_Name[14 + i];
			}
			for (int i = 0; i < 2 * 2; i++) {
				subdir_out->long_filename[long_filename_offset + ((5 + 6) * 2) + i] = subdir_out->fat_dir.DIR_Name[28 + i];
			}

			if (subdir_out->fat_dir.DIR_Name[0] & 0x40) {
				// Last long file name entry
				// but actually, it's the first one in the memory
				lfn_present = 1;

				// add the null terminator
				subdir_out->long_filename[long_filename_offset + 13 * 2] = '\0';
				subdir_out->long_filename[long_filename_offset + 13 * 2 + 1] = '\0';
			}

			continue;
		}

		int dir_name_length = 0;
		if (lfn_present) {
			lfn_present = 0;
			dir_name_length = path_compare(path, subdir_out->long_filename, 2, 1);
		}
		else {
			shortname_to_longname(subdir_out->fat_dir.DIR_Name, subdir_out->long_filename);
			dir_name_length = path_compare(path, subdir_out->long_filename, 2, 0);
		}

		if (read_at_cursor){
			if (entries == root_dir->cursor) {
				found = (subdir_out->fat_dir.DIR_Attr & 0x10) ? fat_FOUND_DIR : fat_FOUND_FILE;
				if (file_out != NULL && found == fat_FOUND_FILE) {
					file_out->fat_dir = subdir_out->fat_dir;
					memory_copy(file_out->long_filename, subdir_out->long_filename, sizeof(subdir_out->long_filename));
				}
				root_dir->cursor++;
				break;
			}
		}
		else {
			if (dir_name_length > 0) {
				// +1 for the '/'
				const char* next_path = path + dir_name_length + 1;

				// Check if the entry is a directory
				if (subdir_out->fat_dir.DIR_Attr & 0x10) {
					// Entry is a directory
					if (path[dir_name_length] == '\0') {
						// check if the entry we are looking for is a directory
						if (!is_file) {
							// Found the directory, end the search
							found = fat_FOUND_DIR;
							break;
						}
					}
					else {
						// Recursively search the directory
						found = fat_find(subdir_out, file_out, subdir_out, next_path, is_file, read_at_cursor);
						break;
					}
				}
				else {
					// check if the entry we are looking for is a file
					if (is_file) {
						if (path[dir_name_length] == '\0') {
							// Found the file, end the search
							found = fat_FOUND_FILE;
							if (file_out != NULL) {
								file_out->fat_dir = subdir_out->fat_dir;
								
								unsigned int first_fat_sector = directory_file.disk->bpb.BPB_RsvdSecCnt;
								unsigned int first_data_sector = first_fat_sector + (directory_file.disk->bpb.BPB_NumFATs * directory_file.disk->bpb.BPB_FATSz32);
								unsigned int current_file_cluster = directory_file.fat_dir.DIR_FstClusLO;
								unsigned int cluster_offset = first_data_sector + (current_file_cluster - 2) * directory_file.disk->bpb.BPB_SecPerClus;

								file_out->entry_position = dir_offset - sizeof(fat_dir_entry) + cluster_offset * directory_file.disk->bpb.BPB_BytsPerSec;
								
								memory_copy(file_out->long_filename, subdir_out->long_filename, sizeof(subdir_out->long_filename));
							}
							break;
						}
					}
				}
			}
		}

		entries++;
	}

	return found;
}

void fat_rewinddir(fat_DIR* dir) {
	dir->cursor = 0;
}

// Returns fat_NOT_FOUND if not found, fat_FOUND_DIR if found and is a directory, fat_FOUND_FILE if found and is a file
int fat_readdir(fat_DIR* dir_out, fat_FILE* file_out, fat_DIR* parent_dir) {
	return fat_find(dir_out, file_out, parent_dir, "", 0, 1);
}

int fat_opendir(fat_DIR* subdir_out, fat_DIR* root_dir, const char* path) {
	if (path[0] == '\0') {
		fat_copy_DIR(subdir_out, root_dir);
		return 1;
	}
	int success = fat_find(subdir_out, NULL, root_dir, path, 0, 0);
	subdir_out->cursor = 0;
	return success;
}

int fat_fopen(fat_FILE* file_out, fat_DIR* root_dir, const char* path) {
	fat_DIR helper_dir;
	int success = fat_find(&helper_dir, file_out, root_dir, path, 1, 0);
	return success;
}

int fat_init(fat_DISK* disk, fat_disk_access_func_t read_func, fat_disk_access_func_t write_func, void* user_args) {
	disk->read_func = read_func;
	disk->write_func = write_func;
	disk->user_args = user_args;

	// Read the BPB
	disk->read_func((unsigned char*)&disk->bpb, 0x00, sizeof(fat_bpb), disk->user_args);

	// Check if the BPB is valid
	if (disk->bpb.BS_jmpBoot[0] != 0xEB && disk->bpb.BS_jmpBoot[0] != 0xE9) {
	#ifdef fat_DEBUG
		kprintf("Error: Invalid BPB\n");
	#endif
		return 0;
	}

	// Check if the file system is FAT32, path_compare is used as a string compare function
	if (path_compare(disk->bpb.BS_FilSysType, "FAT32   ", 1, 1) == 9) {
	#ifdef fat_DEBUG
		kprintf("Error: File system is not FAT32\n");
	#endif
		return 0;
	}

	// Read the root directory
	disk->root_directory.disk = disk;
	disk->root_directory.fat_dir.DIR_FstClusLO = (disk->bpb.BPB_RootClus == 0) ? 2 : disk->bpb.BPB_RootClus;
	disk->root_directory.fat_dir.DIR_FileSize = -1;
	disk->root_directory.cursor = 0;
	disk->root_directory.long_filename[0] = '\0';

	return 1;
}

/* public helper functions */

void fat_longname_to_string(const char* buffer_long, char* buffer_string) {
	// long filename is in unicode, convert to ascii
	int i = 0;
	while (buffer_long[i * 2] != 0) {
		buffer_string[i] = buffer_long[i * 2];
		i++;
	}
	buffer_string[i] = 0;
}


void fat_copy_DIR(fat_DIR* to, fat_DIR* from) {
	to->cursor = from->cursor;
	to->disk = from->disk;
	to->fat_dir = from->fat_dir;
	for (int i = 0; i < 260 * 2; i++) {
		to->long_filename[i] = from->long_filename[i];
	}
}

void fat_print_buffer(const unsigned char* buffer, int size) {
	for (int i = 0; i < size; i++) {
		kprintf("%x ", buffer[i]);
		if ((i + 1) % 16 == 0 || i == size - 1) {
			kprintf("    ");
			for (int j = i - i % 16; j <= i; j++) {
				if (buffer[j] >= 32 && buffer[j] <= 126) {
					kprintf("%c", buffer[j]);
				}
				else {
					kprintf(".");
				}
			}
			kprintf("\n");
		}
	}
}

void fat_print_bpb(fat_bpb* bpb) {
	kprintf("BS_jmpBoot: ");
	fat_print_buffer(bpb->BS_jmpBoot, sizeof(bpb->BS_jmpBoot));
	kprintf("BS_OEMName: ");
	fat_print_buffer(bpb->BS_OEMName, sizeof(bpb->BS_OEMName));
	kprintf("BPB_BytsPerSec: %d\n", (unsigned int)bpb->BPB_BytsPerSec);
	kprintf("BPB_SecPerClus: %d\n", (unsigned int)bpb->BPB_SecPerClus);
	kprintf("BPB_RsvdSecCnt: %d\n", (unsigned int)bpb->BPB_RsvdSecCnt);
	kprintf("BPB_NumFATs: %d\n", (unsigned int)bpb->BPB_NumFATs);
	kprintf("BPB_RootEntCnt: %d\n", (unsigned int)bpb->BPB_RootEntCnt);
	kprintf("BPB_TotSec16: %d\n", (unsigned int)bpb->BPB_TotSec16);
	kprintf("BPB_Media: %x\n", (unsigned int)bpb->BPB_Media);
	kprintf("BPB_FATSz16: %d\n", (unsigned int)bpb->BPB_FATSz16);
	kprintf("BPB_SecPerTrk: %d\n", (unsigned int)bpb->BPB_SecPerTrk);
	kprintf("BPB_NumHeads: %d\n", (unsigned int)bpb->BPB_NumHeads);
	kprintf("BPB_HiddSec: %d\n", (unsigned int)bpb->BPB_HiddSec);
	kprintf("BPB_TotSec32: %d\n", (unsigned int)bpb->BPB_TotSec32);
	kprintf("BPB_FATSz32: %d\n", (unsigned int)bpb->BPB_FATSz32);
	kprintf("BPB_ExtFlags: %d\n", (unsigned int)bpb->BPB_ExtFlags);
	kprintf("BPB_FSVer: %d\n", (unsigned int)bpb->BPB_FSVer);
	kprintf("BPB_RootClus: %d\n", (unsigned int)bpb->BPB_RootClus);
	kprintf("BPB_FSInfo: %d\n", (unsigned int)bpb->BPB_FSInfo);
	kprintf("BPB_BkBootSec: %d\n", (unsigned int)bpb->BPB_BkBootSec);
	kprintf("BPB_Reserved: ");
	fat_print_buffer(bpb->BPB_Reserved, sizeof(bpb->BPB_Reserved));
	kprintf("BS_DrvNum: %x\n", (unsigned int)bpb->BS_DrvNum);
	kprintf("BS_Reserved1: %x\n", (unsigned int)bpb->BS_Reserved1);
	kprintf("BS_BootSig: %x\n", (unsigned int)bpb->BS_BootSig);
	kprintf("BS_VolID: %d\n", (unsigned int)bpb->BS_VolID);
	kprintf("BS_VolLab: ");
	fat_print_buffer(bpb->BS_VolLab, sizeof(bpb->BS_VolLab));
	kprintf("BS_FilSysType: ");
	fat_print_buffer(bpb->BS_FilSysType, sizeof(bpb->BS_FilSysType));
}

void fat_print_date(unsigned short date) {
	unsigned int day = date & 0x1F;
	unsigned int month = (date >> 5) & 0x0F;
	unsigned int year = ((date >> 9) & 0x7F) + 1980;
	kprintf("%d/%d/%d", day, month, year);
}

void fat_print_time(unsigned short time) {
	unsigned int second = (time & 0x1F) * 2;
	unsigned int minute = (time >> 5) & 0x3F;
	unsigned int hour = (time >> 11) & 0x1F;
	kprintf("%d:%d:%d", hour, minute, second);
}

void fat_print_dir(fat_dir_entry* dir) {
	kprintf("DIR_Name: ");
	fat_print_buffer(dir->DIR_Name, sizeof(dir->DIR_Name));
	kprintf("DIR_Attr: %x\n", (unsigned int)dir->DIR_Attr);
	kprintf("DIR_NTRes: %x\n", (unsigned int)dir->DIR_NTRes);
	kprintf("DIR_CrtTimeTenth: %x\n", (unsigned int)dir->DIR_CrtTimeTenth);
	kprintf("DIR_CrtTime: ");
	fat_print_time(dir->DIR_CrtTime);
	kprintf("\n");
	kprintf("DIR_CrtDate: ");
	fat_print_date(dir->DIR_CrtDate);
	kprintf("\n");
	kprintf("DIR_LstAccDate: ");
	fat_print_date(dir->DIR_LstAccDate);
	kprintf("\n");
	kprintf("DIR_FstClusHI: %d\n", (unsigned int)dir->DIR_FstClusHI);
	kprintf("DIR_WrtTime: ");
	fat_print_time(dir->DIR_WrtTime);
	kprintf("\n");
	kprintf("DIR_WrtDate: ");
	fat_print_date(dir->DIR_WrtDate);
	kprintf("\n");
	kprintf("DIR_FstClusLO: %d\n", (unsigned int)dir->DIR_FstClusLO);
	kprintf("DIR_FileSize: %d\n", (unsigned int)dir->DIR_FileSize);
}

void fat_print_longname(const char* longname) {
	for (int i = 0; i < 260 * 2; i += 2) {
		if (longname[i] == 0x00) {
			break;
		}
		kprintf("%c", longname[i]);
	}
}
