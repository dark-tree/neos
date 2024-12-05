#pragma once

#include "types.h"
#include "config.h"

/**
 * @brief values for the driver_seek's whence
 * @note See lseek(2) man page for more info
 */
#define SEEK_SET 0 /* Seek in relation to the start of the file or directory */
#define SEEK_CUR 1 /* Seek in relation to the current file or directory cursor position */
#define SEEK_END 2 /* Seek in relation to the end of the file or directory */

/**
 * @brief driver open flags
 * @note taken from glibc
 */
#define OPEN_WRONLY    0x000001 /* Open in write-only mode */
#define OPEN_RDWR      0x000002 /* Open in read-write mode */
#define OPEN_APPEND    0x000008 /* Open in append mode */
#define OPEN_CREAT     0x000200 /* Create if missing */
#define OPEN_EXCL      0x000800 /* Require creation, fail if present */
#define OPEN_DIRECTORY 0x200000 /* Require target file to be a directory */
#define OPEN_NOFOLLOW  0x000100 /* Don't follow links in path */
#define OPEN_TRUNC     0x000200 /* Clear (truncate) the file */

/**
 * @brief This is analoguse to the `struct old_linux_dirent`
 * @note See readdir(2) man page for more info
 */
typedef struct {

	// offset in the directory to this entry
	uint32_t seek_offset;

	// the length of the string in name
	uint32_t name_length;

	// name of the file or directory, null terminated
	char name[FILE_MAX_NAME];

} vEntry;

/**
 * @brief Used mostly internally to iterate paths using `vfs_resolve()`
 */
typedef struct {

	// the whole path to be resolved
	const char* string;

	// index into the path where the next segment starts
	int offset;

	// number of path resolves done
	int resolves;

} vPath;

/**
 * @brief This is analoguse to the `struct stat`
 * @note See stat(3type) man page for more info
 */
typedef struct {

	// size of file or directory in bytes
	uint32_t size;

	// last access time, as linux timestamp
	uint32_t atime;

	// last modefication time, as linux timestamp
	uint32_t mtime;

	// number of blocks used to stroe the file
	uint32_t blocks;

	// can this file be written to
	bool writtable;

} vStat;

typedef struct vNode_tag {
	struct vNode_tag* sibling;
	struct vNode_tag* child;
	struct vNode_tag* parent;
	struct FilesystemDriver_tag* driver;
	char name[FILE_MAX_NAME];
} vNode;

typedef struct {
	vNode* node;
	int offset;
	struct FilesystemDriver_tag* driver;
	void* state;
} vRef;

/**
 * @brief Load root state into vRef
 *
 * @param[in] dst The object to which the root should be written.
 *
 * @return Returns 0 on success and a negated ERRNO code (LINUX_EIO, LINUX_ENOTDIR) on error
 *         LINUX_EIO     - Internal IO error occured in the filesystem itself
 */
typedef int (*driver_root) (vRef* dst);

/**
 * @brief Duplicate the file reference
 *
 * @param[out] dst The object to which the copy should be written.
 * @param[in] src The object from which the copy should be made.
 *
 * @return Returns 0 on success
 */
typedef int (*driver_clone) (vRef* dst, vRef* src);

/**
 * @brief Open (and possibly create) a file
 *
 * @param[in] vref  The directory to open the file/directory from
 * @param[in] part  The name of the file/directory to open
 * @param[in] flags combination of the following flags:
 *                  OPEN_WRONLY    Open in write-only mode (See vfs_iswriteable())
 *                  OPEN_RDWR      Open in read-only mode (See vfs_isreadable())
 *                  OPEN_APPEND    Set the file cursor initially at the end of the file
 *                  OPEN_CREAT     Create if file is missing
 *                  OPEN_EXCL      Require creation, fail if present
 *                  OPEN_DIRECTORY Require target file to be a directory
 *                  OPEN_NOFOLLOW  Don't follow the link (if part is a name of a link)
 *                  OPEN_TRUNC     Clear (truncate) the file (this is not required for files opened in read-only mode)
 *
 * @note If flags contains a combination of OPEN_DIRECTORY and OPEN_CREAT directory should not be
 *       created, instead OPEN_DIRECTORY should be ignored and a file created - this is to match linux behaviour.
 *
 * @return Returns 0 on success, and a negated ERRNO code on error
 *         LINUX_EIO     - Internal IO error occured in the filesystem itself
 *         LINUX_EEXIST  - File exists while creation was mandated (OPEN_EXCL)
 *         LINUX_ENOTDIR - Target is not a directory while OPEN_DIRECTORY was set
 */
typedef int (*driver_open) (vRef* vref, const char* part, uint32_t flags);

/**
 * @brief Close and dealocate vRef
 *
 * @param[in,out] vref The object to close.
 *
 * @return Returns 0 on success.
 */
typedef int (*driver_close) (vRef* vref);

/**
 * @brief Read count bytes from vRef into buffer
 *
 * @param[out] buffer the buffer to write to
 * @param[in] count number of bytes to read
 *
 * @return the number of bytes read on success and a negated ERRNO code on error
 *         LINUX_EIO     - Internal IO error occured in the filesystem itself
 *         LINUX_EINVAL  - Can't read from the given vRef
 *         LINUX_EISDIR  - vRef is a directory
 */
typedef int (*driver_read) (vRef* vref, void* buffer, uint32_t count);

/**
 * @brief Write count bytes to vRef from buffer
 *
 * @param[in] buffer the buffer to read from
 * @param[in] count number of bytes to write
 *
 * @return the number of bytes written on success and a negated ERRNO code on error
 *         LINUX_EIO     - Internal IO error occured in the filesystem itself
 *         LINUX_EINVAL  - Can't write to the give vRef
 *         LINUX_EISDIR  - vRef is a directory
 */
typedef int (*driver_write) (vRef* vref, void* buffer, uint32_t count);

/**
 * @brief Manipulate the file cursor
 *
 * @param[in] vref the vRef of node to seek in
 * @param[in] offset offset in bytes from the whence specificed point
 * @param[in] whence specificed from where to count the offset
 *
 * @note whence is alredy verified to be a valid value by the VFS subsystem
 *
 * @return Returns number of bytes from file start on success and a negated ERRNO code on error
 *         LINUX_EIO     - Internal IO error occured in the filesystem itself
 *         LINUX_EINVAL  - Offset falls outside seekable range
 */
typedef int (*driver_seek) (vRef* vref, int offset, int whence);

/**
 * @brief Fills at most max entries in the given vEntry buffer
 *
 * @param[in] vref the vRef of node to list
 * @param[out] entries the buffer to write to
 * @param[in] max buffer size (in vEntry multiples)
 *
 * @return Returns number of entries written on success and a negated ERRNO code on error
 *         LINUX_EIO     - Internal IO error occured in the filesystem itself
 *         LINUX_ENOTDIR - Vref isn't a directory
 */
typedef int (*driver_list) (vRef* vref, vEntry* entries, int max);

/**
 * @brief Create a new directory of the given name
 *
 * @param[in] vref the vRef of the parent node
 * @param[in] name the name of the directory to create
 *
 * @return Returns 0 on success and a negated ERRNO code on error
 *         LINUX_EIO     - Internal IO error occured in the filesystem itself
 *         LINUX_ENOTDIR - Vref isn't a directory
 *         LINUX_EEXIST  - Name `name` is alredy taken
 *         LINUX_EINVAL  - Name is invalid for the filesystem
 */
typedef int (*driver_mkdir) (vRef* vref, const char* name);

/**
 * @brief Remove the file under the given handle
 *
 * @param[in] vref the vRef of the node that is to be removed from the filesystem
 * @param[in] rmdir should the removal be recursive, can only be used for directory vRefs
 *
 * @return Returns 0 on success and a negated ERRNO code on error
 *         LINUX_EIO     - Internal IO error occured in the filesystem itself
 *         LINUX_ENOTDIR - Vref ISN'T a directory and rmdir WAS specificed
 *         LINUX_EISDIR  - Vref IS a directory and rmdir WAS NOT specificed
 */
typedef int (*driver_remove) (vRef* vref, bool rmdir);

/**
 * @brief Get file information
 *
 * @param[in] vref the vRef of the node that is to be queried
 * @param[out] stat structure to write to
 *
 * @return Returns 0 on success and a negated ERRNO code (LINUX_EIO, LINUX_ENOTDIR) on error
 *         LINUX_EIO     - Internal IO error occured in the filesystem itself
 */
typedef int (*driver_stat) (vRef* vref, vStat* stat);

/**
 * @brief Read link path into the given buffer
 *
 * @param[in] vref    The vRef of the node that is to be queried
 * @param[in] name    Name of the link to read
 * @param[out] buffer The buffer into which the link will be written
 * @param[in] size    The length (in bytes) of the given buffer.
 *
 * @note If size is smaller than the number of bytes to be written the excess data should
 *       be truncated.
 *
 * @return Returns number of bytes from file start on success and a negated ERRNO code on error
 *         LINUX_EIO     - Internal IO error occured in the filesystem itself
 *         LINUX_EINVAL  - Given file exists but is not a link
 *         LINUX_ENOENT  - No such link exists
 */
typedef int (*driver_readlink) (vRef* vref, const char* name, const char* buffer, int size);

typedef struct FilesystemDriver_tag {
	const char identifier[16];

	driver_root     root;
	driver_clone    clone;
	driver_open     open;
	driver_close    close;
	driver_read     read;
	driver_write    write;
	driver_seek     seek;
	driver_list     list;
	driver_mkdir    mkdir;
	driver_remove   remove;
	driver_stat     stat;
	driver_readlink readlink;
} FilesystemDriver;

/**
 * @brief Get next path element
 *
 * @param[in] path    Path object to get the next element of.
 * @param[out] buffer Buffer to write the next name into.
 *
 * @return 1 on success, 0 if nothing was written to buffer.
 */
int vfs_resolve(vPath* path, char* buffer);

/**
 * @brief Starts the File Devices Subsystem, this must be the first call into fdev.
 * @return None.
 */
void vfs_init();

/**
 *
 */
int vfs_open(vRef* vref, vRef* relation, const char* path, uint32_t flags);

/**
 *
 */
int vfs_close(vRef* vref);

/**
 *
 */
int vfs_read(vRef* vref, void* buffer, uint32_t size, uint32_t count);

/**
 *
 */
int vfs_write(vRef* vref, void* buffer, uint32_t size, uint32_t count);

/**
 *
 */
int vfs_seek(vRef* vref, int offset, int whence);

/**
 *
 */
int vfs_list(vRef* vref, vEntry* entries, int max);

/**
 *
 */
int vfs_mkdir(vRef* vref, const char* name);

/**
 *
 */
int vfs_remove(vRef* vref, bool rmdir);

/**
 *
 */
int vfs_stat(vRef* vref, vStat* stat);

/**
 * @brief Get a reference to the root of the VFS, this can be then used to oepn paths
 * @return Root VFS
 */
vRef vfs_root();

/**
 * @brief Check if the flags allow writing.
 *
 * @param[in] open_flags open() flags
 *
 * @return 1 if the file can be writen to, 0 otherwise.
 */
bool vfs_iswriteable(int open_flags);

/**
 * @brief Check if the flags allow reading.
 *
 * @param[in] open_flags open() flags
 *
 * @return 1 if the file can be read from, 0 otherwise.
 */
bool vfs_isreadable(int open_flags);

/**
 * @brief Mount the given driver into the filesystem.
 *
 * @param[in] path Path to mount to.
 * @param[in] value The driver to mount.
 *
 * @return 1 if the driver was mounted, 0 otherwise.
 */
int vfs_mount(const char* path, FilesystemDriver* value);

/**
 * @brief Print the VFS tree
 *
 * @param[in] node Node to print, set to NULL to print from root.
 * @param[in] depth The print depth, set to 0.
 *
 * @return None.
 */
void vfs_print(vNode* node, int depth);
