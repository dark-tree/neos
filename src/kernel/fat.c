#include "fat.h"
#include "print.h"

#ifndef NULL
    #define NULL 0
#endif

#define fat_DEBUG

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

static int fat_itoa(unsigned int value, char* buffer, unsigned int base) {
	if (value == 0) {
		buffer[0] = '0';
		buffer[1] = '\0';
		return 1;
	}

	int i = 0;
	while (value > 0) {
		int digit = value % base;
		buffer[i++] = (digit < 10) ? '0' + digit : 'A' + digit - 10;
		value /= base;
	}
	buffer[i] = '\0';

	int length = i;

	// Reverse the string
	int j = 0;
	i--;
	while (j < i) {
		char tmp = buffer[j];
		buffer[j] = buffer[i];
		buffer[i] = tmp;
		j++;
		i--;
	}

	return length;
}

static void shortname_to_longname(const char* shortname, unsigned short* longname) {
	/*
	* Convert a short name to a long name.
	* The short name is in the format "FILE.TXT" and the long name is in the format "FILE    TXT".
	* The short name is 11 characters long. First 8 characters are the name and the last 3 characters are the extension.
	* If the name is less than 8 characters, it is padded with spaces.
	*/

	unsigned short* longname_short = (unsigned short*)longname;

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

static void memory_copy(void* dest, void* src, unsigned int size) {
	unsigned char* dest_b = (unsigned char*)dest;
	unsigned char* src_b = (unsigned char*)src;

	for (unsigned int i = 0; i < size; i++) {
		dest_b[i] = src_b[i];
	}
}

/* FAT32 functions */

static void fat_file_default(fat_FILE* file, fat_DISK* disk) {
	file->cursor = 0;
	file->entry_position = 0;
	file->first_parent_cluster = 0;
	file->lfn_present = 0;
	file->disk = disk;
	file->long_filename[0] = '\0';
	for (int i = 1; i < sizeof(file->long_filename) / sizeof(file->long_filename[0]); i++) {
		file->long_filename[i] = 0xffff;
	}
	for (int i = 0; i < sizeof(file->fat_dir); i++) {
		((unsigned char*)&(file->fat_dir))[i] = '\0';
	}
}

static unsigned char fat_cache[512];
static unsigned int fat_cache_sector = 0xFFFFFFFF;

static void fat_invalidate_cache() {
	fat_cache_sector = 0xFFFFFFFF;
}

static unsigned int fat_read_fat_entry(fat_DISK* disk, unsigned int cluster, unsigned char cache) {
	unsigned int fat_offset = disk->bpb.BPB_RsvdSecCnt * disk->bpb.BPB_BytsPerSec;

	// Each entry is 4 bytes long
	unsigned int fat_entry = fat_offset + cluster * 4;

	if (cache) {
		unsigned int sector = fat_entry / disk->bpb.BPB_BytsPerSec;
		if (sector != fat_cache_sector) {
			disk->read_func(fat_cache, sector * disk->bpb.BPB_BytsPerSec, disk->bpb.BPB_BytsPerSec, disk->user_args);
			fat_cache_sector = sector;
		}
		return *((unsigned int*)(fat_cache + (fat_entry % disk->bpb.BPB_BytsPerSec)));
	}
	else {
		unsigned int value;
		disk->read_func((unsigned char*)&value, fat_entry, sizeof(value), disk->user_args);
		return value;
	}

	return fat_entry;
}

static void fat_write_fat_entry(fat_DISK* disk, unsigned int cluster, unsigned int value) {
	unsigned int fat_offset = disk->bpb.BPB_RsvdSecCnt * disk->bpb.BPB_BytsPerSec;

	// Each entry is 4 bytes long
	unsigned int fat_entry = fat_offset + cluster * 4;
	// Update the FAT table
	disk->write_func((unsigned char*)&value, fat_entry, sizeof(value), disk->user_args);
	// Update the backup FAT table
	unsigned int fat_size = disk->bpb.BPB_FATSz32 * disk->bpb.BPB_BytsPerSec;
	disk->write_func((unsigned char*)&value, fat_entry + fat_size, sizeof(value), disk->user_args);
}

unsigned char fat_fread(void* data_out, unsigned int element_size, unsigned int element_count, fat_FILE* file) {
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

	fat_invalidate_cache();

	while (1) {
		if (current_file_cluster < 2) {
			break;
		}

		unsigned int next_cluster = fat_read_fat_entry(file->disk, current_file_cluster, 1);

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
			if (bytes_to_read > 0) {
				file->disk->read_func((unsigned char*)data_out + read_total_bytes, src_read_offset, bytes_to_read, file->disk->user_args);
			}
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

	return 1;
}

unsigned int fat_file_cluster_count(fat_FILE* file) {
	unsigned int current_file_cluster = file->fat_dir.DIR_FstClusLO;
	unsigned int next_cluster = 0;
	unsigned int cluster_count = 0;

	fat_invalidate_cache();

	while (1) {
		if (current_file_cluster < 2) {
			break;
		}

		next_cluster = fat_read_fat_entry(file->disk, current_file_cluster, 1);

		if (next_cluster == 0x0FFFFFF7) {
			// Cluster is bad
			break;
		}

		if (next_cluster == 0x0) {
			// Cluster is free
			break;
		}

		cluster_count++;

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

	return cluster_count;
}

static void fat_update_entry(fat_FILE* dir) {
	if (dir->first_parent_cluster < 2) {
		return;
	}

	fat_DIR parent_dir;
	fat_file_default(&parent_dir.dir_file, dir->disk);
	parent_dir.dir_file.fat_dir.DIR_FstClusLO = dir->first_parent_cluster;
	parent_dir.dir_file.fat_dir.DIR_FileSize = fat_file_cluster_count(&parent_dir.dir_file) * parent_dir.dir_file.disk->bpb.BPB_SecPerClus * parent_dir.dir_file.disk->bpb.BPB_BytsPerSec;

	fat_fseek(&parent_dir.dir_file, dir->entry_position, fat_SEEK_SET);
	fat_fwrite((unsigned char*)&dir->fat_dir, sizeof(fat_dir_entry), 1, &parent_dir.dir_file);
}

static void fat_read_entry(fat_FILE* dir) {
	fat_DIR parent_dir;
	fat_file_default(&parent_dir.dir_file, dir->disk);
	parent_dir.dir_file.fat_dir.DIR_FstClusLO = dir->first_parent_cluster;
	parent_dir.dir_file.fat_dir.DIR_FileSize = fat_file_cluster_count(&parent_dir.dir_file) * parent_dir.dir_file.disk->bpb.BPB_SecPerClus * parent_dir.dir_file.disk->bpb.BPB_BytsPerSec;

	fat_fseek(&parent_dir.dir_file, dir->entry_position, fat_SEEK_SET);
	fat_fread((unsigned char*)&dir->fat_dir, sizeof(fat_dir_entry), 1, &parent_dir.dir_file);
}

static void fat_clear_cluster(fat_DISK* disk, unsigned int cluster) {
	unsigned int first_data_sector = disk->bpb.BPB_RsvdSecCnt + (disk->bpb.BPB_NumFATs * disk->bpb.BPB_FATSz32);
	unsigned int cluster_offset = first_data_sector + (cluster - 2) * disk->bpb.BPB_SecPerClus;
	unsigned int cluster_size_bytes = disk->bpb.BPB_SecPerClus * disk->bpb.BPB_BytsPerSec;
	unsigned char clear_buffer[cluster_size_bytes];

	for (unsigned int i = 0; i < cluster_size_bytes; i++) {
		clear_buffer[i] = 0;
	}

	disk->write_func(clear_buffer, cluster_offset * disk->bpb.BPB_BytsPerSec, cluster_size_bytes, disk->user_args);
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

		unsigned int next_cluster = fat_read_fat_entry(file->disk, current_file_cluster, 0);

		if (next_cluster == 0x0FFFFFF7) {
			// Cluster is bad
			break;
		}

		if (next_cluster == 0x0 || (next_cluster >= 0x0FFFFFF8 && next_cluster <= 0x0FFFFFFF)) {
			unsigned char new_end = 1;

			// Cluster is free, allocate a new cluster or if it is the last cluster, check if the data fits
			if (file->cursor + data_size <= cluster_write_data_offset + cluster_size) {
				if (next_cluster == 0x0FFFFFFF) {
					new_end = 0;
				}
				
				// Set the current cluster as the last cluster
				next_cluster = 0x0FFFFFFF;
			}
			else{
				// The remaining data does not fit in the current cluster
				// Find next free cluster
				unsigned int disk_size = (file->disk->bpb.BPB_TotSec32 == 0) ? file->disk->bpb.BPB_TotSec16 : file->disk->bpb.BPB_TotSec32;
				unsigned int free_cluster = current_file_cluster + 1;
				fat_invalidate_cache();
				while (fat_read_fat_entry(file->disk, free_cluster, 1) != 0x0) {
					free_cluster++;
					if (free_cluster * file->disk->bpb.BPB_SecPerClus >= disk_size) {
						// No free clusters
						return 0;
					}
				}
				// Set the current cluster to point to the next cluster
				next_cluster = free_cluster;
			}

			if (new_end){
				fat_write_fat_entry(file->disk, current_file_cluster, next_cluster);

				// Clear the new cluster
				fat_clear_cluster(file->disk, next_cluster);
			}
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

int fat_fseek(fat_FILE* file, int offset, int origin) {
	if (origin == fat_SEEK_SET) {
		file->cursor = offset;
	}
	else if (origin == fat_SEEK_CUR) {
		file->cursor += offset;
	}
	else if (origin == fat_SEEK_END) {
		file->cursor = file->fat_dir.DIR_FileSize + offset;
	}

	return 1;
}

int fat_dirseek(fat_DIR* dir, int offset, int origin) {
	if (origin == fat_SEEK_SET) {
		dir->dir_file.cursor = offset;
	}
	else if (origin == fat_SEEK_CUR) {
		dir->dir_file.cursor += offset;
	}
	else if (origin == fat_SEEK_END) {
		return 0;
	}

	return 1;
}

int fat_ftell(fat_FILE* file) {
	return file->cursor;
}

// Returns fat_NOT_FOUND if not found, fat_FOUND_DIR if found and is a directory, fat_FOUND_FILE if found and is a file
// If read_at_cursor is set to 1, the function will read the entry at the cursor position and return the result. The entry can be a file or a directory. The is_file parameter will be ignored.
// If is_file is set to 1, the function will return fat_FOUND_FILE only if the entry is a file. fat_FOUND_DIR will not be returned, even if the directory with the same name exists.
// If is_file is set to 0, the function will return fat_FOUND_DIR only if the entry is a directory. fat_FOUND_FILE will not be returned, even if the file with the same name exists.
static int fat_find_full(fat_DIR* subdir_out, fat_FILE* file_out, fat_DIR* root_dir, const char* path, unsigned char is_file, unsigned char read_at_cursor, unsigned char use_shortname) {
	fat_file_default(&subdir_out->dir_file, root_dir->dir_file.disk);
	if (file_out != NULL) {
		fat_file_default(file_out, root_dir->dir_file.disk);
	}

	// Special case for the root directory
	if (root_dir->dir_file.fat_dir.DIR_FstClusLO < 2) {
		root_dir->dir_file.fat_dir.DIR_FstClusLO = 2;
	}

	// The directory is a file that contains the directory entries
	fat_FILE directory_file = {
		.disk = root_dir->dir_file.disk,
		.fat_dir = root_dir->dir_file.fat_dir,
		.cursor = 0
	};
	// Directories do not hold information about their size so
	// set the file size to the number of sectors of the directory
	directory_file.fat_dir.DIR_FileSize = fat_file_cluster_count(&directory_file) * directory_file.disk->bpb.BPB_SecPerClus * directory_file.disk->bpb.BPB_BytsPerSec;

	// For iterating through the directory
	unsigned int dir_offset = 0;
	unsigned int entries = 0;

	// Long file name:
	// "Up to 20 of these 13-character entries may be chained, supporting a maximum length of 255 UCS-2 characters"
	// 20 * 13 = 260 just to be safe
	unsigned char lfn_present = 0;

	int found = fat_NOT_FOUND;

	while (1) {
		// Check if reached the end of the directory
		if (dir_offset >= directory_file.fat_dir.DIR_FileSize) {
			break;
		}

		// Read the directory entry
		// TODO: fat_fread function performs at least 2 reads each time it's called, and it's called for each directory entry which is a lot of reads,
		// in real world application it would probably be better to read for example one cluster at a time and then iterate through the entries in memory.
		// The way it's implemented now is more readable though, so it is what it is for now.
		fat_fseek(&directory_file, dir_offset, fat_SEEK_SET);
		fat_fread((unsigned char*)&subdir_out->dir_file.fat_dir, sizeof(fat_dir_entry), 1, &directory_file);
		dir_offset += sizeof(fat_dir_entry);

		if (subdir_out->dir_file.fat_dir.DIR_Name[0] == 0x00) {
			// "0x00 is an additional indicator that all directory entries following the current free entry are also free"
			break;
		}

		if (subdir_out->dir_file.fat_dir.DIR_Name[0] == 0xE5) {
			// "indicates the directory entry is free (available)"
			continue;
		}

		if (subdir_out->dir_file.fat_dir.DIR_Attr == 0x0F) {
			// Long file name
			unsigned char long_filename_index = subdir_out->dir_file.fat_dir.DIR_Name[0] & 0x0F;
			unsigned int long_filename_offset = (long_filename_index - 1) * 13;

			// the long file name is split into 3 parts in the single entry
			for (int i = 0; i < 5; i++) {
				subdir_out->dir_file.long_filename[long_filename_offset + i] = subdir_out->dir_file.fat_lfn.LDIR_Name1[i];
			}
			for (int i = 0; i < 6; i++) {
				subdir_out->dir_file.long_filename[long_filename_offset + 5 + i] = subdir_out->dir_file.fat_lfn.LDIR_Name2[i];
			}
			for (int i = 0; i < 2; i++) {
				subdir_out->dir_file.long_filename[long_filename_offset + (5 + 6) + i] = subdir_out->dir_file.fat_lfn.LDIR_Name3[i];
			}

			if (subdir_out->dir_file.fat_dir.DIR_Name[0] & 0x40) {
				// Last long file name entry
				// but actually, it's the first one in the memory
				lfn_present = 1;

				// add the null terminator
				subdir_out->dir_file.long_filename[long_filename_offset + 13] = '\0';
			}

			continue;
		}

		int dir_name_length = 0;
		if (lfn_present && !use_shortname) {
			dir_name_length = path_compare(path, (char*)subdir_out->dir_file.long_filename, 2, 1);
		}
		else {
			shortname_to_longname(subdir_out->dir_file.fat_dir.DIR_Name, subdir_out->dir_file.long_filename);
			dir_name_length = path_compare(path, (char*)subdir_out->dir_file.long_filename, 2, 0);
		}

		if (read_at_cursor){
			if (entries == root_dir->dir_file.cursor) {
				found = (subdir_out->dir_file.fat_dir.DIR_Attr & fat_ATTR_DIRECTORY) ? fat_FOUND_DIR : fat_FOUND_FILE;
				if (file_out != NULL && found == fat_FOUND_FILE) {
					file_out->fat_dir = subdir_out->dir_file.fat_dir;
					memory_copy(file_out->long_filename, subdir_out->dir_file.long_filename, sizeof(subdir_out->dir_file.long_filename));
				}
				fat_FILE* file_out_ptr = (found == fat_FOUND_FILE) ? file_out : &subdir_out->dir_file;
				file_out_ptr->entry_position = dir_offset - sizeof(fat_dir_entry);
				file_out_ptr->first_parent_cluster = directory_file.fat_dir.DIR_FstClusLO;
				file_out_ptr->lfn_present = lfn_present;

				root_dir->dir_file.cursor++;
				break;
			}
		}
		else {
			if (dir_name_length > 0) {
				// +1 for the '/'
				const char* next_path = path + dir_name_length + 1;

				// Check if the entry is a directory
				if (subdir_out->dir_file.fat_dir.DIR_Attr & 0x10) {
					// Entry is a directory
					if (path[dir_name_length] == '\0') {
						// check if the entry we are looking for is a directory
						if (!is_file) {
							// Found the directory, end the search
							found = fat_FOUND_DIR;

							subdir_out->dir_file.entry_position = dir_offset - sizeof(fat_dir_entry);
							subdir_out->dir_file.first_parent_cluster = directory_file.fat_dir.DIR_FstClusLO;
							subdir_out->dir_file.lfn_present = lfn_present;

							break;
						}
					}
					else {
						// Recursively search the directory
						fat_DIR new_root_dir = *subdir_out;
						found = fat_find_full(subdir_out, file_out, &new_root_dir, next_path, is_file, read_at_cursor, use_shortname);
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
								file_out->fat_dir = subdir_out->dir_file.fat_dir;

								file_out->entry_position = dir_offset - sizeof(fat_dir_entry);
								file_out->first_parent_cluster = directory_file.fat_dir.DIR_FstClusLO;
								file_out->lfn_present = lfn_present;

								memory_copy(file_out->long_filename, subdir_out->dir_file.long_filename, sizeof(subdir_out->dir_file.long_filename));
							}
							break;
						}
					}
				}
			}
		}

		if (lfn_present) {
			lfn_present = 0;
		}

		entries++;
	}

	return found;
}

static int fat_find(fat_DIR* subdir_out, fat_FILE* file_out, fat_DIR* root_dir, const char* path, unsigned char is_file, unsigned char read_at_cursor) {
	return fat_find_full(subdir_out, file_out, root_dir, path, is_file, read_at_cursor, 0);
}

void fat_rewinddir(fat_DIR* dir) {
	dir->dir_file.cursor = 0;
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
	subdir_out->dir_file.cursor = 0;
	if (subdir_out->dir_file.fat_dir.DIR_FstClusLO < 2) {
		subdir_out->dir_file.fat_dir.DIR_FstClusLO = 2;
	}
	return success;
}

void fat_remove_fat_chain(fat_FILE* file){
	unsigned int first_fat_sector = file->disk->bpb.BPB_RsvdSecCnt;
	unsigned int first_data_sector = first_fat_sector + (file->disk->bpb.BPB_NumFATs * file->disk->bpb.BPB_FATSz32);

	unsigned int current_file_cluster = file->fat_dir.DIR_FstClusLO;
	unsigned int next_cluster = 0;
	unsigned int cluster_size = file->disk->bpb.BPB_SecPerClus * file->disk->bpb.BPB_BytsPerSec;

	while (1) {
		if (current_file_cluster < 2) {
			break;
		}

		next_cluster = fat_read_fat_entry(file->disk, current_file_cluster, 0);

		if (next_cluster == 0x0FFFFFF7) {
			// Cluster is bad
			break;
		}

		if (next_cluster == 0x0) {
			// Cluster is free
			break;
		}

		unsigned int cluster_offset = first_data_sector + (current_file_cluster - 2) * file->disk->bpb.BPB_SecPerClus;

		// Set the current cluster as free
		fat_write_fat_entry(file->disk, current_file_cluster, 0x0);

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
}

unsigned char fat_lfn_checksum(const unsigned char* shortname) {
	unsigned char checksum = 0;

	for (int i = 0; i < 11; i++) {
		checksum = ((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + shortname[i];
	}

	return checksum;
}

// If remove is set to 1, the function will remove the long filename entries from the directory
// If remove is set to 0, the function will recreate the long filename entries in the directory
void fat_update_lfn(fat_FILE* file, unsigned char remove) {
	fat_DIR parent_dir;
	fat_file_default(&parent_dir.dir_file, file->disk);
	parent_dir.dir_file.fat_dir.DIR_FstClusLO = file->first_parent_cluster;
	parent_dir.dir_file.fat_dir.DIR_FstClusHI = file->first_parent_cluster >> 16;
	parent_dir.dir_file.fat_dir.DIR_FileSize = fat_file_cluster_count(&parent_dir.dir_file) * parent_dir.dir_file.disk->bpb.BPB_SecPerClus * parent_dir.dir_file.disk->bpb.BPB_BytsPerSec;

	if (file->lfn_present){
		unsigned char lfn_entries = 0;
		unsigned char lfn_checksum = 0;

		if (!remove){
			unsigned int lfn_length = 1;
			while (file->long_filename[lfn_length] != '\0') {
				lfn_length++;
			}
			// Round up
			lfn_entries = (lfn_length - 1) / 13 + 1;

			lfn_checksum = fat_lfn_checksum(file->fat_dir.DIR_Name);
		}

		unsigned char lfn_entry = 1;

		unsigned int entry_position = file->entry_position;
		while (1) {
			entry_position -= sizeof(fat_dir_entry);
			
			if (remove){
				fat_dir_entry fat_dir;
				fat_fseek(&parent_dir.dir_file, entry_position, fat_SEEK_SET);
				fat_fread((unsigned char*)&fat_dir, sizeof(fat_dir_entry), 1, &parent_dir.dir_file);

				if (fat_dir.DIR_Attr != 0x0F) {
					break;
				}

				// Mark the long filename entry as free
				fat_dir.DIR_Name[0] = 0xE5;

				fat_fseek(&parent_dir.dir_file, entry_position, fat_SEEK_SET);
				fat_fwrite((unsigned char*)&fat_dir, sizeof(fat_dir_entry), 1, &parent_dir.dir_file);
			}
			else {
				fat_lfn_entry fat_lfn = {
					.LDIR_Ord = ((lfn_entry == lfn_entries) ? 0x40 : 0x00) | lfn_entry,
					.LDIR_Attr = 0x0F,
					.LDIR_Chksum = lfn_checksum
				};
				
				unsigned int long_filename_offset = (lfn_entry - 1) * 13;
				for (int i = 0; i < 5; i++) {
					fat_lfn.LDIR_Name1[i] = file->long_filename[long_filename_offset + i];
				}
				for (int i = 0; i < 6; i++) {
					fat_lfn.LDIR_Name2[i] = file->long_filename[long_filename_offset + 5 + i];
				}
				for (int i = 0; i < 2; i++) {
					fat_lfn.LDIR_Name3[i] = file->long_filename[long_filename_offset + 5 + 6 + i];
				}

				fat_fseek(&parent_dir.dir_file, entry_position, fat_SEEK_SET);
				fat_fwrite((unsigned char*)&fat_lfn, sizeof(fat_lfn_entry), 1, &parent_dir.dir_file);

				lfn_entry++;
				if (lfn_entry > lfn_entries) {
					break;
				}
			}
		}
	}
}

unsigned char fat_fremove(fat_FILE* file) {
	fat_update_lfn(file, 1);

	// Set the entry in the directory as free
	file->fat_dir.DIR_Name[0] = 0xE5;
	fat_update_entry(file);

	fat_remove_fat_chain(file);

	return 1;
}

unsigned char fat_dirremove(fat_DIR* dir) {
	fat_DIR helper_dir;
	fat_FILE helper_file;
	
	fat_rewinddir(dir);

	while (1) {
		int dir_entry = fat_readdir(&helper_dir, &helper_file, dir);

		if (dir_entry == fat_NOT_FOUND) {
			break;
		}

		fat_FILE* file_ptr = dir_entry == fat_FOUND_FILE ? &helper_file : &helper_dir.dir_file;

		if (file_ptr->fat_dir.DIR_Name[0] == '.' || (file_ptr->fat_dir.DIR_Name[0] == '.' && file_ptr->fat_dir.DIR_Name[1] == '.')) {
			continue;
		}

		char new_path[sizeof(file_ptr->long_filename) / 2];
		fat_longname_to_string(file_ptr->long_filename, new_path);
		fat_remove(dir, new_path, (file_ptr->fat_dir.DIR_Attr & fat_ATTR_DIRECTORY) == 0);
	}

	// Set the entry in the directory as free
	dir->dir_file.fat_dir.DIR_Name[0] = 0xE5;
	fat_update_entry(&dir->dir_file);

	fat_remove_fat_chain(&dir->dir_file);

	return 1;
}

unsigned char fat_remove(fat_DIR* root_dir, const char* path, unsigned char is_file) {
	fat_DIR found_dir;
	fat_FILE found_file;
	int find_success = fat_find(&found_dir, &found_file, root_dir, path, is_file, 0);

	if (find_success == fat_FOUND_FILE) {
		return fat_fremove(&found_file);
	}

	if (find_success == fat_FOUND_DIR) {
		return fat_dirremove(&found_dir);
	}

	return 0;
}

int fat_create(fat_FILE* new_file_out, fat_DIR* root_dir, const char* path, unsigned char attributes) {
	// Find the parent directory
	unsigned int path_length = 0;
	while (path[path_length] != '\0') {
		path_length++;
	}
	int last_slash = path_length;
	while (last_slash >= 0 && path[last_slash] != '/') {
		last_slash--;
	}
	fat_DIR parent_dir;
	if (last_slash > 0) {
		// The path contains a directory
		char parent_path[last_slash + 1];
		for (int i = 0; i < last_slash; i++) {
			parent_path[i] = path[i];
		}
		parent_path[last_slash] = '\0';

		if (!fat_opendir(&parent_dir, root_dir, parent_path)) {
			// The parent directory does not exist
			return 0;
		}
	}
	else {
		// The root directory is the parent directory
		fat_copy_DIR(&parent_dir, root_dir);
	}

	// Initialize the new file
	fat_file_default(new_file_out, parent_dir.dir_file.disk);
	new_file_out->fat_dir.DIR_Attr = attributes;

	/* File name */

	int name_length = path_length - last_slash - 1;
	if (name_length > 255) {
		// The file name is too long
		return 0;
	}

	for (int i = 0; i < 11; i++) {
		new_file_out->fat_dir.DIR_Name[i] = ' ';
	}

	// Find last dot in the file name
	unsigned int dot_position = path_length;
	for (int i = last_slash + 1; i < path_length; i++) {
		if (path[i] == '.') {
			dot_position = i;
		}
	}

	// Copy the file name
	int short_name_index = 0;
	for (int i = last_slash + 1; i < dot_position; i++) {
		if (short_name_index >= 8) {
			// The file name is too long
			new_file_out->lfn_present = 1;
			break;
		}
		// Copy the character and convert it to uppercase
		new_file_out->fat_dir.DIR_Name[short_name_index++] = (path[i] >= 'a' && path[i] <= 'z') ? path[i] - ('a' - 'A') : path[i];
	}

	// Copy the file extension
	short_name_index = 8;
	for (int i = dot_position + 1; i < path_length; i++) {
		if (short_name_index >= 11) {
			// The file extension is too long
			new_file_out->lfn_present = 1;
			break;
		}
		// Copy the character and convert it to uppercase
		new_file_out->fat_dir.DIR_Name[short_name_index++] = (path[i] >= 'a' && path[i] <= 'z') ? path[i] - ('a' - 'A') : path[i];
	}

	fat_string_to_longname(path + last_slash + 1, new_file_out->long_filename);

	unsigned int lfn_entries = 0;

	if (new_file_out->lfn_present) {
		// The file name is too long, long file name will be used

		// Create short version of the file name
		// The short name is in the format: "FILE~1.TXT"
		// The short name should be unique, so we increment the number at the end of the name if the name is already taken
		unsigned int repeat = 1;
		while (repeat < 1000) {
			char repeat_str[4];
			int repeat_len = fat_itoa(repeat, repeat_str, 10);

			// Copy to the short name
			new_file_out->fat_dir.DIR_Name[8 - repeat_len - 1] = '~';
			for (int i = 0; i < repeat_len; i++) {
				new_file_out->fat_dir.DIR_Name[8 - repeat_len + i] = repeat_str[i];
			}

			// Check if the short name is unique
			fat_DIR helper_dir;
			char helper_path[13];
			for (int i = 0; i < 8; i++) {
				helper_path[i] = new_file_out->fat_dir.DIR_Name[i];
			}
			helper_path[8] = '.';
			for (int i = 0; i < 3; i++) {
				helper_path[9 + i] = new_file_out->fat_dir.DIR_Name[8 + i];
				if (helper_path[9 + i] == ' ') {
					helper_path[9 + i] = '\0';
					break;
				}
			}
			helper_path[12] = '\0';

			if (fat_find_full(&helper_dir, NULL, &parent_dir, helper_path, 1, 0, 1) == fat_NOT_FOUND) {
				break;
			}

			repeat++;
		}

		// "name_length + 1" for the null terminator,
		// "name_length + 1 - 1" and "/ 13 + 1" to round up
		lfn_entries = (name_length + 1 - 1) / 13 + 1;
	}

	/* Create file */

	fat_dir_entry helper_dir_entry;
	unsigned int dir_offset = 0;

	unsigned int required_entries = 1 + lfn_entries;
	unsigned int found_contiguous_entries = 0;

	// In FAT32, the size of the directory file is not saved in the directory entry (DIR_FileSize is 0), so we need to calculate it
	// The size is required for the fat_fread and fat_fwrite functions to work correctly
	parent_dir.dir_file.fat_dir.DIR_FileSize = fat_file_cluster_count(&parent_dir.dir_file) * parent_dir.dir_file.disk->bpb.BPB_BytsPerSec * parent_dir.dir_file.disk->bpb.BPB_SecPerClus;

	// Find the first free entry in the directory
	while (1){
		if (dir_offset + sizeof(fat_dir_entry) > parent_dir.dir_file.fat_dir.DIR_FileSize) {
			// The directory is full, increase the size of the directory with new zeroed entries
			fat_fseek(&parent_dir.dir_file, dir_offset + sizeof(fat_dir_entry), fat_SEEK_SET);
			for (int i = 0; i < sizeof(fat_dir_entry); i++) {
				((unsigned char*)&helper_dir_entry)[i] = 0;
			}

			if (!fat_fwrite((unsigned char*)&helper_dir_entry, sizeof(fat_dir_entry), 1, &parent_dir.dir_file)) {
				// Could not increase the size of the directory
				return 0;
			}
		}

		fat_fseek(&parent_dir.dir_file, dir_offset, fat_SEEK_SET);
		fat_fread((unsigned char*)&helper_dir_entry, sizeof(fat_dir_entry), 1, &parent_dir.dir_file);			

		if (helper_dir_entry.DIR_Name[0] == 0x00 || helper_dir_entry.DIR_Name[0] == 0xE5) {
			found_contiguous_entries++;
			if (found_contiguous_entries >= required_entries) {
				break;
			}
		}
		else {
			found_contiguous_entries = 0;
		}

		dir_offset += sizeof(fat_dir_entry);
	}

	// Save the entry position
	new_file_out->entry_position = dir_offset;
	new_file_out->first_parent_cluster = parent_dir.dir_file.fat_dir.DIR_FstClusLO + (parent_dir.dir_file.fat_dir.DIR_FstClusHI << 16);

	// Allocate a new cluster for the file
	unsigned int disk_size = (parent_dir.dir_file.disk->bpb.BPB_TotSec32 == 0) ? parent_dir.dir_file.disk->bpb.BPB_TotSec16 : parent_dir.dir_file.disk->bpb.BPB_TotSec32;
	unsigned int free_cluster = 2;
	fat_invalidate_cache();
	while (fat_read_fat_entry(parent_dir.dir_file.disk, free_cluster, 1) != 0x0) {
		free_cluster++;
		if (free_cluster * parent_dir.dir_file.disk->bpb.BPB_SecPerClus >= disk_size) {
			// No free clusters
			return 0;
		}
	}

	new_file_out->fat_dir.DIR_FstClusLO = free_cluster;
	new_file_out->fat_dir.DIR_FstClusHI = free_cluster >> 16;
	
	fat_write_fat_entry(parent_dir.dir_file.disk, free_cluster, 0x0FFFFFFF);
	fat_clear_cluster(parent_dir.dir_file.disk, free_cluster);

	// Update the parent directory with the new entry
	fat_update_entry(new_file_out);
	fat_update_lfn(new_file_out, 0);

	return 1;
}

int fat_create_file(fat_FILE* new_file_out, fat_DIR* root_dir, const char* path, unsigned char attributes){
	fat_DIR dummy_dir;
	if (fat_find(&dummy_dir, new_file_out, root_dir, path, 1, 0) != fat_NOT_FOUND) {
		return 0;
	}
	return fat_create(new_file_out, root_dir, path, fat_ATTR_ARCHIVE | attributes);
}

int fat_create_dir(fat_DIR* new_dir_out, fat_DIR* root_dir, const char* path, unsigned char attributes) {
	// Check if the directory already exists
	if (fat_find(new_dir_out, NULL, root_dir, path, 0, 0) != fat_NOT_FOUND) {
		return 0;
	}

	// Create the directory
	if(!fat_create(&new_dir_out->dir_file, root_dir, path, fat_ATTR_DIRECTORY | attributes)){
		return 0;
	}

	// Create . and .. entries
	fat_DIR parent_dir;
	fat_file_default(&parent_dir.dir_file, new_dir_out->dir_file.disk);
	parent_dir.dir_file.fat_dir.DIR_FstClusLO = new_dir_out->dir_file.first_parent_cluster;
	parent_dir.dir_file.fat_dir.DIR_FstClusHI = new_dir_out->dir_file.first_parent_cluster >> 16;
	parent_dir.dir_file.fat_dir.DIR_FileSize = fat_file_cluster_count(&parent_dir.dir_file) * parent_dir.dir_file.disk->bpb.BPB_SecPerClus * parent_dir.dir_file.disk->bpb.BPB_BytsPerSec;

	fat_dir_entry entry;
	for (int i = 0; i < sizeof(fat_dir_entry); i++) {
		((unsigned char*)&entry)[i] = 0;
	}
	for (int i = 0; i < 11; i++) {
		entry.DIR_Name[i] = ' ';
	}

	// .
	entry.DIR_Name[0] = '.';
	entry.DIR_Attr = fat_ATTR_DIRECTORY;
	entry.DIR_FstClusLO = new_dir_out->dir_file.fat_dir.DIR_FstClusLO;
	entry.DIR_FstClusHI = new_dir_out->dir_file.fat_dir.DIR_FstClusHI;
	entry.DIR_FileSize = 0;

	fat_fseek(&new_dir_out->dir_file, 0, fat_SEEK_SET);
	if (!fat_fwrite((unsigned char*)&entry, sizeof(fat_dir_entry), 1, &new_dir_out->dir_file)) {
		return 0;
	}

	// ..
	entry.DIR_Name[1] = '.';
	entry.DIR_Attr = fat_ATTR_DIRECTORY;
	entry.DIR_FstClusLO = parent_dir.dir_file.fat_dir.DIR_FstClusLO;
	entry.DIR_FstClusHI = parent_dir.dir_file.fat_dir.DIR_FstClusHI;
	entry.DIR_FileSize = 0;

	fat_fseek(&new_dir_out->dir_file, sizeof(fat_dir_entry), fat_SEEK_SET);
	if (!fat_fwrite((unsigned char*)&entry, sizeof(fat_dir_entry), 1, &new_dir_out->dir_file)) {
		return 0;
	}

	return 1;
}

int fat_fopen(fat_FILE* file_out, fat_DIR* root_dir, const char* path, const char* mode) {
	fat_DIR helper_dir;
	int find_success = fat_find(&helper_dir, file_out, root_dir, path, 1, 0);

	int success = 0;
	if (mode[0] == 'r') {
		success = (find_success == fat_FOUND_FILE) ? 1 : 0;
	}
	else if (mode[0] == 'w' || mode[0] == 'a') {
		if (find_success == fat_FOUND_FILE) {
			if (mode[0] == 'w') {
				// File exists, truncate it
				file_out->fat_dir.DIR_FileSize = 0;
				fat_update_entry(file_out);
				fat_remove_fat_chain(file_out);
				fat_write_fat_entry(file_out->disk, file_out->fat_dir.DIR_FstClusLO, 0x0FFFFFFF);
				success = 1;
			}
			else if (mode[0] == 'a') {
				// Seek to the end of the file
				fat_fseek(file_out, 0, fat_SEEK_END);
				success = 1;
			}
		}
		else if (find_success == fat_NOT_FOUND) {
			// File does not exist, create it
			success = fat_create_file(file_out, root_dir, path, 0);
		}
	}
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
	fat_file_default(&disk->root_directory.dir_file, disk);
	disk->root_directory.dir_file.fat_dir.DIR_FstClusLO = (disk->bpb.BPB_RootClus == 0) ? 2 : disk->bpb.BPB_RootClus;
	disk->root_directory.dir_file.fat_dir.DIR_FileSize = -1;

	unsigned int first_fat_sector = disk->bpb.BPB_RsvdSecCnt;
	unsigned int first_data_sector = first_fat_sector + (disk->bpb.BPB_NumFATs * disk->bpb.BPB_FATSz32);
	disk->root_directory.dir_file.entry_position = first_data_sector * disk->bpb.BPB_BytsPerSec;

	return 1;
}

/* public helper functions */

int fat_longname_to_string(const unsigned short* buffer_long, char* buffer_string) {
	// long filename is in unicode, convert to ascii
	int i = 0;
	while (buffer_long[i] != 0) {
		buffer_string[i] = (char)buffer_long[i];
		i++;
	}
	buffer_string[i] = 0;

	return i;
}

int fat_string_to_longname(const char* buffer_string, unsigned short* buffer_long) {
	int i = 0;
	while (buffer_string[i] != 0) {
		buffer_long[i] = buffer_string[i];
		i++;
	}
	buffer_long[i] = 0;

	return i;
}

void fat_copy_DIR(fat_DIR* to, fat_DIR* from) {
	*to = *from;
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

void fat_print_longname(const unsigned short* longname) {
	for (int i = 0; i < 260; i++) {
		if (longname[i] == 0x00) {
			break;
		}
		kprintf("%c", longname[i]);
	}
}
