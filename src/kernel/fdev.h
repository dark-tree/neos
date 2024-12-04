//#pragma once
//
//#include "types.h"
//#include "config.h"
//
///**
// * @brief values for the driver_seek's whence
// * @note See lseek(2) man page for more info
// */
//#define SEEK_SET 0 /* Seek in relation to the start of the file or directory */
//#define SEEK_CUR 1 /* Seek in relation to the current file or directory cursor position */
//#define SEEK_END 2 /* Seek in relation to the end of the file or directory */
//
///**
// * @brief driver open flags
// * @note taken from glibc
// */
//#define OPEN_WRONLY    0x000001 /* Open in write-only mode */
//#define OPEN_RDWR      0x000002 /* Open in read-write mode */
//#define OPEN_APPEND    0x000008 /* Open in append mode */
//#define OPEN_CREAT     0x000200 /* Create if missing */
//#define OPEN_EXCL      0x000800 /* Require creation, fail if present */
//#define OPEN_DIRECTORY 0x200000 /* Require target file to be a directory */
//#define OPEN_NOFOLLOW  0x000100 /* Don't follow links in path */
//
///**
// * @brief This is analoguse to the `struct old_linux_dirent`
// * @note See readdir(2) man page for more info
// */
//typedef struct {
//
//	// offset in the directory to this entry
//	uint32_t seek_offset;
//
//	// the length of the string in name
//	uint32_t name_length;
//
//	// name of the file or directory, null terminated
//	char name[FILE_MAX_NAME];
//
//} FileEntry;
//
///**
// * @brief This is analoguse to the `struct stat`
// * @note See stat(3type) man page for more info
// */
//typedef struct {
//
//	// size of file or directory in bytes
//	uint32_t size;
//
//	// last access time, as linux timestamp
//	uint32_t atime;
//
//	// last modefication time, as linux timestamp
//	uint32_t mtime;
//
//	// number of blocks used to stroe the file
//	uint32_t blocks;
//
//	// can this file be written to
//	bool writtable;
//
//} FileStat;
//
///**
// * @brief open and possibly create a file
// *
// *
// */
//typedef int (*driver_open) (void* context, const char* path, uint32_t flags);
//
///**
// *
// */
//typedef int (*driver_read) (void* context, void* buffer, uint32_t size, uint32_t count);
//
///**
// *
// */
//typedef int (*driver_write) (void* context, void* buffer, uint32_t size, uint32_t count);
//
///**
// *
// */
//typedef int (*driver_seek) (void* context, int offset, int whence);
//
///**
// *
// */
//typedef int (*driver_tell) (void* context);
//
///**
// *
// */
//typedef int (*driver_list) (void* context, FileEntry* entries, int max);
//
///**
// *
// */
//typedef int (*driver_mkdir) (void* context, const char* name);
//
///**
// *
// */
//typedef int (*driver_rmdir) (void* context, const char* name);
//
///**
// *
// */
//typedef int (*driver_stat) (void* context, FileStat* stat);
//
//typedef struct {
//	driver_open  open;
//	driver_read  read;
//	driver_write write;
//	driver_seek  seek;
//	driver_tell  tell;
//	driver_list  list;
//	driver_mkdir mkdir;
//	driver_rmdir rmdir;
//} DeviceDriver;
//
//typedef struct {
//	DeviceDriver* driver;
//	void* file;
//} FileDescriptor;
//
//typedef struct {
//	const char* current;
//	const char* relative;
//	const char* absolute;
//	int offset;
//	int resolves;
//} FilePath;
//
///**
// * @brief Get next path element
// */
//int fdev_resolve(FilePath* path, char buffer[FILE_MAX_NAME]);
//
///**
// * @brief Starts the File Devices Subsystem, this must be the first call into fdev.
// * @return None.
// */
//void fdev_init();
//
///**
// * @brief Check if the flags allow writing.
// * @return 1 if the file can be writen to, 0 otherwise.
// */
//bool fdev_open_can_write(int open_flags);
//
///**
// * @brief Check if the flags allow reading.
// * @return 1 if the file can be read from, 0 otherwise.
// */
//bool fdev_open_can_read(int open_flags);
//
///**
// * @brief Mount the given driver into the filesystem.
// * @return 1 if the driver was mounted, 0 otherwise.
// */
//bool fdev_mount(const char* path, DeviceDriver* driver);
