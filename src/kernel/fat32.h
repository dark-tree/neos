#pragma once

#ifndef NULL
	#define NULL 0
#endif

typedef void(*disk_read_func_t)(unsigned char* data_out, unsigned int offset_in, unsigned int size_in, void* user_args);

#pragma pack(1)
typedef struct fat32_bpb_s {
	/*
	*  BPB - BIOS Parameter Block
	*  BS - Boot Sector
	*/

	// BPB
	unsigned char BS_jmpBoot[3];		// bootjmp
	unsigned char BS_OEMName[8];		// oem_name
	unsigned short BPB_BytsPerSec;		// bytes_per_sector
	unsigned char BPB_SecPerClus;		// sectors_per_cluster
	unsigned short BPB_RsvdSecCnt;		// reserved_sector_count
	unsigned char BPB_NumFATs;			// table_count
	unsigned short BPB_RootEntCnt;		// root_entry_count
	unsigned short BPB_TotSec16;		// total_sectors_16
	unsigned char BPB_Media;			// media_type
	unsigned short BPB_FATSz16;			// table_size_16
	unsigned short BPB_SecPerTrk;		// sectors_per_track
	unsigned short BPB_NumHeads;		// head_side_count
	unsigned int BPB_HiddSec;			// hidden_sector_count
	unsigned int BPB_TotSec32;			// total_sectors_32

	// FAT32 Extended BPB
	unsigned int BPB_FATSz32;			// table_size_32
	unsigned short BPB_ExtFlags;		// extended_flags
	unsigned short BPB_FSVer;			// fat_version
	unsigned int BPB_RootClus;			// root_cluster
	unsigned short BPB_FSInfo;			// fat_info
	unsigned short BPB_BkBootSec;		// backup_BS_sector
	unsigned char BPB_Reserved[12];		// reserved_0
	unsigned char BS_DrvNum;			// drive_number
	unsigned char BS_Reserved1;			// reserved_1
	unsigned char BS_BootSig;			// boot_signature
	unsigned int BS_VolID;				// volume_id
	unsigned char BS_VolLab[11];		// volume_label
	unsigned char BS_FilSysType[8];		// fat_type_label
} fat32_bpb;
#pragma pack()

#pragma pack(1)
typedef struct fat32_dir_entry_s {
	unsigned char DIR_Name[11];
	unsigned char DIR_Attr;
	unsigned char DIR_NTRes;
	unsigned char DIR_CrtTimeTenth;
	unsigned short DIR_CrtTime;
	unsigned short DIR_CrtDate;
	unsigned short DIR_LstAccDate;
	unsigned short DIR_FstClusHI;
	unsigned short DIR_WrtTime;
	unsigned short DIR_WrtDate;
	unsigned short DIR_FstClusLO;
	unsigned int DIR_FileSize;
} fat32_dir_entry;
#pragma pack()

// forward declaration
typedef struct fat32_DISK_s fat32_DISK;

typedef struct fat32_FILE_s {
	fat32_dir_entry fat_dir;
	fat32_DISK* disk;
	unsigned int cursor;
	unsigned char long_filename[260 * 2];
} fat32_FILE;

typedef struct fat32_DIR_s {
	fat32_dir_entry fat_dir;
	fat32_DISK* disk;
	unsigned int cursor;
	unsigned char long_filename[260 * 2];
} fat32_DIR;

typedef struct fat32_DISK_s {
	disk_read_func_t read_func;
	void* user_args;
	fat32_bpb bpb;
	fat32_DIR root_directory;
} fat32_DISK;

void fat32_copy_DIR(fat32_DIR* to, fat32_DIR* from) {
	to->cursor = from->cursor;
	to->disk = from->disk;
	to->fat_dir = from->fat_dir;
	for (int i = 0; i < 260 * 2; i++) {
		to->long_filename[i] = from->long_filename[i];
	}
}

#ifdef FAT32_PRINT

void print_buffer(const unsigned char* buffer, int size) {
	for (int i = 0; i < size; i++) {
		printf("%02X ", buffer[i]);
		if ((i + 1) % 16 == 0 || i == size - 1) {
			printf("    ");
			for (int j = i - i % 16; j <= i; j++) {
				if (buffer[j] >= 32 && buffer[j] <= 126) {
					printf("%c", buffer[j]);
				}
				else {
					printf(".");
				}
			}
			printf("\n");
		}
	}
}

void print_bpb(fat32_bpb* bpb) {
	printf("BS_jmpBoot: ");
	print_buffer(bpb->BS_jmpBoot, sizeof(bpb->BS_jmpBoot));
	printf("BS_OEMName: ");
	print_buffer(bpb->BS_OEMName, sizeof(bpb->BS_OEMName));
	printf("BPB_BytsPerSec: %u\n", (unsigned int)bpb->BPB_BytsPerSec);
	printf("BPB_SecPerClus: %u\n", (unsigned int)bpb->BPB_SecPerClus);
	printf("BPB_RsvdSecCnt: %u\n", (unsigned int)bpb->BPB_RsvdSecCnt);
	printf("BPB_NumFATs: %u\n", (unsigned int)bpb->BPB_NumFATs);
	printf("BPB_RootEntCnt: %u\n", (unsigned int)bpb->BPB_RootEntCnt);
	printf("BPB_TotSec16: %u\n", (unsigned int)bpb->BPB_TotSec16);
	printf("BPB_Media: %02X\n", (unsigned int)bpb->BPB_Media);
	printf("BPB_FATSz16: %u\n", (unsigned int)bpb->BPB_FATSz16);
	printf("BPB_SecPerTrk: %u\n", (unsigned int)bpb->BPB_SecPerTrk);
	printf("BPB_NumHeads: %u\n", (unsigned int)bpb->BPB_NumHeads);
	printf("BPB_HiddSec: %u\n", (unsigned int)bpb->BPB_HiddSec);
	printf("BPB_TotSec32: %u\n", (unsigned int)bpb->BPB_TotSec32);
	printf("BPB_FATSz32: %u\n", (unsigned int)bpb->BPB_FATSz32);
	printf("BPB_ExtFlags: %u\n", (unsigned int)bpb->BPB_ExtFlags);
	printf("BPB_FSVer: %u\n", (unsigned int)bpb->BPB_FSVer);
	printf("BPB_RootClus: %u\n", (unsigned int)bpb->BPB_RootClus);
	printf("BPB_FSInfo: %u\n", (unsigned int)bpb->BPB_FSInfo);
	printf("BPB_BkBootSec: %u\n", (unsigned int)bpb->BPB_BkBootSec);
	printf("BPB_Reserved: ");
	print_buffer(bpb->BPB_Reserved, sizeof(bpb->BPB_Reserved));
	printf("BS_DrvNum: %02X\n", (unsigned int)bpb->BS_DrvNum);
	printf("BS_Reserved1: %02X\n", (unsigned int)bpb->BS_Reserved1);
	printf("BS_BootSig: %02X\n", (unsigned int)bpb->BS_BootSig);
	printf("BS_VolID: %u\n", (unsigned int)bpb->BS_VolID);
	printf("BS_VolLab: ");
	print_buffer(bpb->BS_VolLab, sizeof(bpb->BS_VolLab));
	printf("BS_FilSysType: ");
	print_buffer(bpb->BS_FilSysType, sizeof(bpb->BS_FilSysType));
}

void print_date(unsigned short date) {
	unsigned int day = date & 0x1F;
	unsigned int month = (date >> 5) & 0x0F;
	unsigned int year = ((date >> 9) & 0x7F) + 1980;
	printf("%02u/%02u/%04u", day, month, year);
}

void print_time(unsigned short time) {
	unsigned int second = (time & 0x1F) * 2;
	unsigned int minute = (time >> 5) & 0x3F;
	unsigned int hour = (time >> 11) & 0x1F;
	printf("%02u:%02u:%02u", hour, minute, second);
}

void print_dir(fat32_dir_entry* dir) {
	printf("DIR_Name: ");
	print_buffer(dir->DIR_Name, sizeof(dir->DIR_Name));
	printf("DIR_Attr: %02X\n", (unsigned int)dir->DIR_Attr);
	printf("DIR_NTRes: %02X\n", (unsigned int)dir->DIR_NTRes);
	printf("DIR_CrtTimeTenth: %02X\n", (unsigned int)dir->DIR_CrtTimeTenth);
	printf("DIR_CrtTime: ");
	print_time(dir->DIR_CrtTime);
	printf("\n");
	printf("DIR_CrtDate: ");
	print_date(dir->DIR_CrtDate);
	printf("\n");
	printf("DIR_LstAccDate: ");
	print_date(dir->DIR_LstAccDate);
	printf("\n");
	printf("DIR_FstClusHI: %u\n", (unsigned int)dir->DIR_FstClusHI);
	printf("DIR_WrtTime: ");
	print_time(dir->DIR_WrtTime);
	printf("\n");
	printf("DIR_WrtDate: ");
	print_date(dir->DIR_WrtDate);
	printf("\n");
	printf("DIR_FstClusLO: %u\n", (unsigned int)dir->DIR_FstClusLO);
	printf("DIR_FileSize: %u\n", (unsigned int)dir->DIR_FileSize);
}

void print_longname(const char* longname) {
	for (int i = 0; i < 260 * 2; i += 2) {
		if (longname[i] == 0x00) {
			break;
		}
		printf("%c", longname[i]);
	}
}

#endif

char to_lowercase(char c) {
	if (c >= 'A' && c <= 'Z') {
		return c + ('a' - 'A');
	}
	return c;
}

void to_lowercase_str(char* str) {
	while (*str) {
		*str = to_lowercase(*str);
		str++;
	}
}

void longname_to_string(const char* buffer_long, char* buffer_string) {
	// long filename is in unicode, convert to ascii
	int i = 0;
	while (buffer_long[i * 2] != 0) {
		buffer_string[i] = buffer_long[i * 2];
		i++;
	}
	buffer_string[i] = 0;
}

void shortname_to_longname(const char* shortname, char* longname) {
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

unsigned int read_fat_entry(fat32_DISK* disk, unsigned int cluster) {
	unsigned int fat_offset = disk->bpb.BPB_RsvdSecCnt * disk->bpb.BPB_BytsPerSec;

	// Each entry is 4 bytes long
	unsigned int fat_entry = fat_offset + cluster * 4;
	disk->read_func((unsigned char*)&fat_entry, fat_entry, sizeof(fat_entry), disk->user_args);

	return fat_entry;
}

void fat32_fread(void* data_out, unsigned int element_size, unsigned int element_count, fat32_FILE* file) {
	/*
	* In order to read the contents of a file, first we need to read the DIR_FstClusLO value from the directory entry.
	* This value is the first cluster of the file where the data is stored.
	* In order to access the rest of the file, we need to read the FAT table.
	* The FAT table contains the cluster number of the next cluster in the file.
	*/

	fat32_dir_entry* file_dir = &file->fat_dir;

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
			// if reading from the first cluster, we might want to skip the first bytes
			// next clusters will always be read from the beginning
			if (cluster_read_data_offset - file->cursor <= cluster_size) {
				src_read_offset += cluster_size - (cluster_read_data_offset - file->cursor);
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

#define fat32_SEEK_SET 0
#define fat32_SEEK_CUR 1
#define fat32_SEEK_END 2

void fat32_fseek(fat32_FILE* file, int offset, int origin) {
	if (origin == fat32_SEEK_SET) {
		file->cursor = offset;
	}
	else if (origin == fat32_SEEK_CUR) {
		file->cursor += offset;
	}
	else if (origin == fat32_SEEK_END) {
		file->cursor = file->fat_dir.DIR_FileSize + offset;
	}
}

int fat32_ftell(fat32_FILE* file) {
	return file->cursor;
}

int path_compare(const char* path, const char* dir_name, unsigned char char_size, unsigned char case_sensitive) {
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

void memory_copy(unsigned char* dest, unsigned char* src, unsigned int size) {
	for (unsigned int i = 0; i < size; i++) {
		dest[i] = src[i];
	}
}

#define fat32_NOT_FOUND 0
#define fat32_FOUND_DIR 1
#define fat32_FOUND_FILE 2

// Returns fat32_NOT_FOUND if not found, fat32_FOUND_DIR if found and is a directory, fat32_FOUND_FILE if found and is a file
// If read_at_cursor is set to 1, the function will read the entry at the cursor position and return the result. The enrtry can be a file or a directory.
int fat32_find(fat32_DIR* subdir_out, fat32_FILE* file_out, fat32_DIR* root_dir, const char* path, unsigned char is_file, unsigned char read_at_cursor) {
	subdir_out->disk = root_dir->disk;
	if (file_out != NULL) {
		file_out->disk = root_dir->disk;
	}

	// Special case for the root directory
	if (root_dir->fat_dir.DIR_FstClusLO < 2) {
		root_dir->fat_dir.DIR_FstClusLO = 2;
	}

	// The directory is a file that contains the directory entries
	fat32_FILE directory_file = {
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

	int found = fat32_NOT_FOUND;

	while (1) {
		// Read the directory entry
		// TODO: fat32_fread function performs at least 2 reads each time it's called, and it's called for each directory entry which is a lot of reads,
		// in real world application it would probably be better to read for example one cluster at a time and then iterate through the entries in memory.
		// The way it's implemented now is more readable though, so it is what it is for now.
		fat32_fseek(&directory_file, dir_offset, fat32_SEEK_SET);
		fat32_fread((unsigned char*)&subdir_out->fat_dir, sizeof(fat32_dir_entry), 1, &directory_file);
		dir_offset += sizeof(fat32_dir_entry);

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
				found = (subdir_out->fat_dir.DIR_Attr & 0x10) ? fat32_FOUND_DIR : fat32_FOUND_FILE;
				if (file_out != NULL && found == fat32_FOUND_FILE) {
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
							found = fat32_FOUND_DIR;
							break;
						}
					}
					else {
						// Recursively search the directory
						found = fat32_find(subdir_out, file_out, subdir_out, next_path, is_file, read_at_cursor);
						break;
					}
				}
				else {
					// check if the entry we are looking for is a file
					if (is_file) {
						if (path[dir_name_length] == '\0') {
							// Found the file, end the search
							found = fat32_FOUND_FILE;
							if (file_out != NULL) {
								file_out->fat_dir = subdir_out->fat_dir;
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

void fat32_rewinddir(fat32_DIR* dir) {
	dir->cursor = 0;
}

// Returns fat32_NOT_FOUND if not found, fat32_FOUND_DIR if found and is a directory, fat32_FOUND_FILE if found and is a file
int fat32_readdir(fat32_DIR* dir_out, fat32_FILE* file_out, fat32_DIR* parent_dir) {
	return fat32_find(dir_out, file_out, parent_dir, "", 0, 1);
}

int fat32_opendir(fat32_DIR* subdir_out, fat32_DIR* root_dir, const char* path) {
	if (path[0] == '\0') {
		fat32_copy_DIR(subdir_out, root_dir);
		return 1;
	}
	int success = fat32_find(subdir_out, NULL, root_dir, path, 0, 0);
	subdir_out->cursor = 0;
	return success;
}

int fat32_fopen(fat32_FILE* file_out, fat32_DIR* root_dir, const char* path) {
	fat32_DIR helper_dir;
	int success = fat32_find(&helper_dir, file_out, root_dir, path, 1, 0);
	return success;
}

int fat32_init(fat32_DISK* disk, disk_read_func_t read_func, void* user_args) {
	disk->read_func = read_func;
	disk->user_args = user_args;

	// Read the BPB
	disk->read_func((unsigned char*)&disk->bpb, 0x00, sizeof(fat32_bpb), disk->user_args);

	// Check if the BPB is valid
	if (disk->bpb.BS_jmpBoot[0] != 0xEB && disk->bpb.BS_jmpBoot[0] != 0xE9) {
	#ifdef FAT32_DEBUG
		printf("Error: Invalid BPB\n");
	#endif
		return 0;
	}

	// Check if the file system is FAT32, path_compare is used as a string compare function
	if (path_compare(disk->bpb.BS_FilSysType, "FAT32   ", 1, 1) == 9) {
	#ifdef FAT32_DEBUG
		printf("Error: File system is not FAT32\n");
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
