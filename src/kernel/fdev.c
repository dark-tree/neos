//
//#include "fdev.h"
//
///* private */
//
//
//
///* public */
//
//void fdev_path(FilePath* path, const char* absolute, const char* r) {
//	path->absolute
//}
//
//int fdev_resolve(FilePath* path, char buffer[FILE_MAX_NAME]) {
//
//	int last = FILE_MAX_NAME - 1;
//	int i = 0;
//
//	if (path->offset == 0 && path->relative[0] == '/') {
//		path->current = path->relative;
//	}
//
//	while (true) {
//
//		const char chr = path->current[path->offset ++];
//
//		if (chr == '/') {
//			break;
//		}
//
//		buffer[i ++] = chr;
//
//		if (i >= last) {
//			break;
//		}
//
//	}
//
//	buffer[i] = '\0';
//	path->resolves ++;
//
//}
//
//void fdev_init() {
//
//}
//
//bool dev_open_can_write(int open_flags) {
//	return (open_flags & OPEN_RDWR) || (open_flags & OPEN_WRONLY);
//}
//
//bool dev_open_can_read(int open_flags) {
//	return (open_flags & OPEN_RDWR) || !(open_flags & OPEN_WRONLY);
//}
//
//bool fdev_mount(const char* path, DeviceDriver* driver) {
//
//}
