#pragma once

/**
 * @brief Function that is called when reading data from the disk
 * 
 * @param data_out pointer to the array where the read data will be stored
 * @param offset_in the offset in bytes from the start of the disk where the read will start
 * @param size_in the number of bytes to read
 * @param user_args user arguments that are passed to the function
 * 
 * @return None
*/
typedef void(*fat_disk_read_func_t)(unsigned char* data_out, unsigned int offset_in, unsigned int size_in, void* user_args);

// for compiling outside of the kernel
// #define kprintf printf

#define fat_NOT_FOUND 0
#define fat_FOUND_DIR 1
#define fat_FOUND_FILE 2

#define fat_SEEK_SET 0
#define fat_SEEK_CUR 1
#define fat_SEEK_END 2

#pragma pack(1)
typedef struct fat_bpb_s {
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
} fat_bpb;
#pragma pack()

#pragma pack(1)
typedef struct fat_dir_entry_s {
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
} fat_dir_entry;
#pragma pack()

// forward declaration
typedef struct fat_DISK_s fat_DISK;

typedef struct fat_FILE_s {
	fat_dir_entry fat_dir;
	fat_DISK* disk;
	unsigned int cursor;
	unsigned char long_filename[260 * 2];
} fat_FILE;

typedef struct fat_DIR_s {
	fat_dir_entry fat_dir;
	fat_DISK* disk;
	unsigned int cursor;
	unsigned char long_filename[260 * 2];
} fat_DIR;

typedef struct fat_DISK_s {
	fat_disk_read_func_t read_func;
	void* user_args;
	fat_bpb bpb;
	fat_DIR root_directory;
} fat_DISK;

/**
 * @brief Read the data from the file
 * 
 * @param data_out pointer to the array where the read objects will be stored
 * @param element_size size of each object in bytes
 * @param element_count number of objects to read
 * @param file file to read from
 *  
 * @return None
*/
void fat_fread(void* data_out, unsigned int element_size, unsigned int element_count, fat_FILE* file);

/**
 * @brief Move the cursor in the file
 * 
 * @param file file to move the cursor in
 * @param offset the number of bytes to move the cursor
 * @param origin the position from which the offset is added (fat_SEEK_SET, fat_SEEK_CUR, fat_SEEK_END)
 * 
 * @return None
*/
void fat_fseek(fat_FILE* file, int offset, int origin);

/**
 * @brief Get the current position of the cursor in the file
 * 
 * @param file file to get the cursor position from
 * 
 * @return The current position of the cursor in the file
*/
int fat_ftell(fat_FILE* file);

/**
 * @brief Reset the cursor in the directory
 * 
 * @param dir directory to reset the cursor in
 * 
 * @return None
*/
void fat_rewinddir(fat_DIR* dir);

/**
 * @brief Read the directory entry at the cursor position, entry can be a file or a directory. Useful for iterating over the directory.
 * 
 * @param dir_out valid directory if found and entry is a directory
 * @param file_out valid file if found and entry is a file
 * @param parent_dir parent directory from which to read the entry
 * 
 * @return fat_NOT_FOUND if not found, fat_FOUND_DIR if found and is a directory, fat_FOUND_FILE if found and is a file
*/
int fat_readdir(fat_DIR* dir_out, fat_FILE* file_out, fat_DIR* parent_dir);

/**
 * @brief Open the directory at the given path
 * 
 * @param subdir_out valid directory if found
 * @param root_dir root directory from which to open the directory
 * @param path path to the directory
 * 
 * @return 1 if the directory was opened, 0 if the directory was not found
*/
int fat_opendir(fat_DIR* subdir_out, fat_DIR* root_dir, const char* path);

/**
 * @brief Open the file at the given path
 * 
 * @param file_out valid file if found
 * @param root_dir root directory from which to open the file
 * @param path path to the file
 * 
 * @return 1 if the file was opened, 0 if the file was not found
*/
int fat_fopen(fat_FILE* file_out, fat_DIR* root_dir, const char* path);

/**
 * @brief Initialize the FAT disk
 * 
 * @param disk FAT disk to initialize
 * @param read_func function to read the data from the disk
 * @param user_args arguments to pass to the read
 * 
 * @return 1 if the disk was initialized, 0 if the disk was not initialized
*/
int fat_init(fat_DISK* disk, fat_disk_read_func_t read_func, void* user_args);

/* helper functions */

/**
 * @brief Convert long filename stored in unicode to ascii
 * 
 * @param buffer_long input long filename in unicode
 * @param buffer_string output long filename in ascii
 * 
 * @return None
*/
void fat_longname_to_string(const char* buffer_long, char* buffer_string);

/**
 * @brief Create copy of the directory
 * 
 * @param to Destination directory
 * @param from Source directory
 * 
 * @return None
*/
void fat_copy_DIR(fat_DIR* to, fat_DIR* from);

/**
 * @brief Print buffer in hex and ascii
 * 
 * @param buffer Buffer to print
 * @param size Size of the buffer
 * 
 * @return None
*/
void fat_print_buffer(const unsigned char* buffer, int size);

/**
 * @brief Print BPB structure (useful for debugging)
 * 
 * @param bpb BPB to print
 * 
 * @return None
*/
void fat_print_bpb(fat_bpb* bpb);

/**
 * @brief Print date stored in fat_dir_entry
 * 
 * @param date Date to print
 * 
 * @return None
*/
void fat_print_date(unsigned short date);

/**
 * @brief Print time stored in fat_dir_entry
 * 
 * @param time Time to print
 * 
 * @return None
*/
void fat_print_time(unsigned short time);

/**
 * @brief Print directory entry properties (useful for debugging)
 * 
 * @param dir Directory entry to print
 * 
 * @return None
*/
void fat_print_dir(fat_dir_entry* dir);

/**
 * @brief Print long filename stored in unicode
 * 
 * @param longname Long filename to print
 * 
 * @return None
*/
void fat_print_longname(const char* longname);

