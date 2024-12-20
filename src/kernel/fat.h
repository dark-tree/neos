#pragma once

/**
 * @brief Signature of the function that is called when reading/writing data from/to the disk
 * 
 * @param data pointer to the array where the read data will be stored or read from
 * @param offset_in the offset in bytes from the start of the disk where the read/write will start
 * @param size_in the number of bytes to read/write
 * @param user_args user arguments that are passed to the function
 * 
 * @return None
*/
typedef void(*fat_disk_access_func_t)(unsigned char* data, unsigned int offset_in, unsigned int size_in, void* user_args);

#define fat_NOT_FOUND 0
#define fat_FOUND_DIR 1
#define fat_FOUND_FILE 2

#define fat_SEEK_SET 0
#define fat_SEEK_CUR 1
#define fat_SEEK_END 2

#define fat_ATTR_READ_ONLY 0x01
#define fat_ATTR_HIDDEN 0x02
#define fat_ATTR_SYSTEM 0x04
#define fat_ATTR_VOLUME_ID 0x08
#define fat_ATTR_DIRECTORY 0x10
#define fat_ATTR_ARCHIVE 0x20

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
	unsigned int BPB_FATSz32;			// table_size_32, count of sectors occupied by one FAT
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

#pragma pack(1)
typedef struct fat_lfn_entry_s {
	unsigned char LDIR_Ord;
	unsigned short LDIR_Name1[5];
	unsigned char LDIR_Attr;		// Always 0x0F
	unsigned char LDIR_Type;		// Always 0x00
	unsigned char LDIR_Chksum;
	unsigned short LDIR_Name2[6];
	unsigned short LDIR_FstClusLO;	// Always 0x00
	unsigned short LDIR_Name3[2];
} fat_lfn_entry;
#pragma pack()

// forward declaration
typedef struct fat_DISK_s fat_DISK;

typedef struct fat_FILE_s {
	union {
		fat_dir_entry fat_dir;
		fat_lfn_entry fat_lfn;
	};
	unsigned int entry_position;
	unsigned int first_parent_cluster;
	unsigned int lfn_present;
	fat_DISK* disk;
	unsigned int cursor;
	unsigned short long_filename[260];
} fat_FILE;

typedef struct fat_DIR_s {
	fat_FILE dir_file;
} fat_DIR;

typedef struct fat_DISK_s {
	fat_disk_access_func_t read_func;
	fat_disk_access_func_t write_func;
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
 * @return 1 if the read was successful, 0 if the read was not successful
*/
unsigned char fat_fread(void* data_out, unsigned int element_size, unsigned int element_count, fat_FILE* file);

/**
 * @brief Write the data to the file
 * 
 * @param data_in pointer to the array where the objects are stored
 * @param element_size size of each object in bytes
 * @param element_count number of objects to write
 * @param file file to write to
 * 
 * @return 1 if the write was successful, 0 if the write was not successful
*/
unsigned char fat_fwrite(void* data_in, unsigned int element_size, unsigned int element_count, fat_FILE* file);

/**
 * @brief Move the cursor in the file
 * 
 * @param file file to move the cursor in
 * @param offset the number of bytes to move the cursor
 * @param origin the position from which the offset is added (fat_SEEK_SET, fat_SEEK_CUR, fat_SEEK_END)
 * 
 * @return 1 if the seek was successful, 0 if the seek was not successful
*/
int fat_fseek(fat_FILE* file, int offset, int origin);

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
 * @brief Seek to the given offset in the directory
 * 
 * @param dir directory to seek in
 * @param offset the number of bytes to seek
 * @param origin the position from which the offset is added (fat_SEEK_SET, fat_SEEK_CUR, fat_SEEK_END)
 * 
 * @return 1 if the seek was successful, 0 if the seek was not successful
*/
int fat_dirseek(fat_DIR* dir, int offset, int origin);

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
 * @brief Remove the file at the given path
 * 
 * @param file file to remove
 * 
 * @return 1 if the file was removed, 0 if the file was not removed
*/
unsigned char fat_fremove(fat_FILE* file);

/**
 * @brief Remove the directory at the given path
 * 
 * @param dir directory to remove
 * 
 * @return 1 if the directory was removed, 0 if the directory was not removed
*/
unsigned char fat_dirremove(fat_DIR* dir);

/**
 * @brief Remove the file at the given path
 * 
 * @param root_dir root directory from which the path starts
 * @param path path to the file relative to the root_dir
 * @param is_file 1 if the path is a file, 0 if the path is a directory
 * 
 * @return 1 if the file was removed, 0 if the file was not removed
*/
unsigned char fat_remove(fat_DIR* root_dir, const char* path, unsigned char is_file);

/**
 * @brief Create the file at the given path
 * 
 * @param new_file_out valid file if created
 * @param root_dir root directory from which the path starts
 * @param path path to the file relative to the root_dir
 * @param attributes file attributes (fat_ATTR_HIDDEN, fat_ATTR_READ_ONLY)
 * 
 * @return 1 if the file was created, 0 if the file was not created
*/
int fat_create_file(fat_FILE* new_file_out, fat_DIR* root_dir, const char* path, unsigned char attributes);

/**
 * @brief Create the directory at the given path
 * 
 * @param new_dir_out valid directory if created
 * @param root_dir root directory from which the path starts
 * @param path path to the directory relative to the root_dir
 * @param attributes directory attributes (fat_ATTR_HIDDEN, fat_ATTR_READ_ONLY)
 * 
 * @return 1 if the directory was created, 0 if the directory was not created
*/
int fat_create_dir(fat_DIR* new_dir_out, fat_DIR* root_dir, const char* path, unsigned char attributes);

/**
 * @brief Open the file at the given path
 * 
 * @param file_out valid file if found
 * @param root_dir root directory from which to open the file
 * @param path path to the file
 * @param mode mode to open the file in. Possible values are:
 * 				"r" - read and write to existing file,
 * 				"w" - read and write to file, truncate if file exists, create if file does not exist,
 * 				"a" - read and write to file, seek to the end of the file, create if file does not exist
 * 
 * @return 1 if the file was opened, 0 if the file was not found
*/
int fat_fopen(fat_FILE* file_out, fat_DIR* root_dir, const char* path, const char* mode);

/**
 * @brief Initialize the FAT disk
 * 
 * @param disk FAT disk to initialize
 * @param read_func function to read the data from the disk
 * @param write_func function to write the data to the disk
 * @param user_args arguments to pass to the read
 * 
 * @return 1 if the disk was initialized, 0 if the disk was not initialized
*/
int fat_init(fat_DISK* disk, fat_disk_access_func_t read_func, fat_disk_access_func_t write_func, void* user_args);

/* helper functions */

/**
 * @brief Convert long filename stored in unicode to ascii
 * 
 * @param buffer_long input long filename in unicode
 * @param buffer_string output long filename in ascii
 * 
 * @return Number of characters in the long filename
*/
int fat_longname_to_string(const unsigned short* buffer_long, char* buffer_string);

/**
 * @brief Convert long filename stored in ascii to unicode
 * 
 * @param buffer_string input long filename in ascii
 * @param buffer_long output long filename in unicode
 * 
 * @return Number of characters in the long filename
*/
int fat_string_to_longname(const char* buffer_string, unsigned short* buffer_long);

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
void fat_print_longname(const unsigned short* longname);

