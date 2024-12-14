#include "vfs.h"
#include "memory.h"
#include "kmalloc.h"
#include "print.h"
#include "util.h"
#include "errno.h"

/**
 * Example directory structure:
 *
 * Root
 * |-> etc
 * |   |-> ex1
 * |   \-> ex2
 * |-> tmp
 * \-> bar
 *    \->ex3
 *
 * The corespoding VFS tree:
 *
 * + ----- +            + ----- +            + ----- +
 * | vNode | -sibling-> | vNode |            | vNode |
 * | "ex1" |            | "ex2" |            | "ex3" |
 * + ----- +            + ----- +            + ----- +
 *  /'\  |                    |               /'\  |
 *   | parent <-parent--------"                | parent
 * child |                                   child |
 *   |  \'/                                    |  \'/
 * + ----- +            + ----- +            + ----- +
 * | vNode | -sibling-> | vNode | -sibling-> | vNode |
 * | "etc" |            | "tmp" |            | "bar" |
 * + ----- +            + ----- +            + ----- +
 *  /'\  |                    |                    |
 *   | parent <-parent--------" <-parent-----------"
 * child |
 *   |  \'/
 * + ----- +
 * | vNode | -parent-\ (loop on root element)
 * |  "/"  | <-------/
 * + ----- +
 */

/* private */

static vNode vfs_root_node;
static vRef vfs_root_ref;

static int vfs_update(vRef* vref) {

	if ((vref->driver != NULL) && (vref->driver != vref->node->driver)) {
		int res = vref->driver->close(vref);

		if (res) {
			return res;
		}

		goto enable;
	}

	if (vref->driver == NULL) {
		enable:
		vref->driver = vref->node->driver;
		return vref->driver->root(vref);
	}

	return 0;

}

static vNode* vfs_mknode(vNode* parent, const char* name) {
	vNode* node = kmalloc(sizeof(vNode));

	node->sibling = NULL;
	node->child = NULL;
	node->parent = parent;
	node->driver = NULL;
	memcpy(node->name, name, FILE_MAX_NAME);

	return node;
}

static void vfs_refcpy(vRef* vref, vRef* src) {
	vref->node = src->node;
	vref->offset = src->offset;
	vref->driver = NULL;
	vref->state = NULL;

	if (src->driver != NULL) {
		vref->driver = src->driver;
		//src->driver->clone(vref, src);

		kprintf(" * VFS %s: clone\n", vref->driver->identifier);
	}
}

static int vfs_findchld(vNode** node, const char* name) {
	vNode* child = (*node)->child;

	while (true) {

		if (streq(child->name, name)) {
			*node = child;
			return 1;
		}

		if (child->sibling != NULL) {
			child = child->sibling;
			continue;
		}

		break;

	}

	*node = child;
	return 0;
}

static int vfs_enter(vRef* vref, const char* part, int flags) {

	vNode* node = vref->node;

	if (streq(part, ".")) {
		return 0;
	}

	if (streq(part, "..")) {

		if (vref->offset > 0) {
			vref->offset --;
			int res = vfs_update(vref);

			if (res) {
				return res;
			}

			return vref->driver->open(vref, part, flags);
		}

		// change mount point
		// this is safe, root loopbacks back onto itself
		vref->node = vref->node->parent;
		return vfs_update(vref);
	}

	if ((vref->offset == 0) && vfs_findchld(&node, part)) {
		vref->node = node;
		return vfs_update(vref);
	}

	// step into the unknown
	vref->offset ++;

	int res = vfs_update(vref);

	if (res) {
		return res;
	}

	return vref->driver->open(vref, part, flags);
}

/* public */

vRef vfs_root() {
	return vfs_root_ref;
}

bool vfs_iswriteable(int open_flags) {
	return (open_flags & OPEN_RDWR) || (open_flags & OPEN_WRONLY);
}

bool vfs_isreadable(int open_flags) {
	return (open_flags & OPEN_RDWR) || !(open_flags & OPEN_WRONLY);
}

int vfs_open(vRef* vref, vRef* relation, const char* path, uint32_t flags) {

	kprintf("vfs_open: '%s'\n", path);

	bool enter = false;
	char front[FILE_MAX_NAME];
	char back[FILE_MAX_NAME];

	vPath vpth;
	vpth.string = path;
	vpth.offset = 0;
	vpth.resolves = 0;
	vpth.errno = 0;

	// absolute path
	if (path[0] == '/') {
		vfs_refcpy(vref, &vfs_root_ref);
	} else {
		vfs_refcpy(vref, relation);
	}

	while (vfs_resolve(&vpth, front)) {

		// check if error occured in resolve
		if (vpth.errno) {
			return vpth.errno;
		}

		if (enter) {
			int res = vfs_enter(vref, back, OPEN_DIRECTORY | (flags & OPEN_NOFOLLOW));

			if (res) {
				return res;
			}
		}

		// took too many resolves to reach the end
		if (vpth.resolves > PATH_MAX_RESOLVES) {
			return LINUX_ELOOP;
		}

		enter = true;
		memcpy(back, front, FILE_MAX_NAME);
	}

	if (enter) {
		return vfs_enter(vref, back, flags);
	}

	return 0;

}

int vfs_close(vRef* vref) {
	if (vref->driver) {
		return vref->driver->close(vref);
	}

	// TODO No driver at leaf node, return error?
	return 0;
}

int vfs_read(vRef* vref, void* buffer, uint32_t size) {
	if (vref->driver) {
		return vref->driver->read(vref, buffer, size);
	}

	// TODO No driver at leaf node, return error?
	return 0;
}

int vfs_write(vRef* vref, void* buffer, uint32_t size) {
	if (vref->driver) {
		return vref->driver->write(vref, buffer, size);
	}

	// TODO No driver at leaf node, return error?
	return 0;
}

int vfs_seek(vRef* vref, int offset, int whence) {
	if (whence != SEEK_CUR && whence != SEEK_END && whence != SEEK_SET) {
		return -LINUX_EINVAL;
	}

	if (vref->driver) {
		return vref->driver->seek(vref, offset, whence);
	}

	// TODO No driver at leaf node, return error?
	return 0;
}

int vfs_list(vRef* vref, vEntry* entries, int max) {
	if (vref->driver) {
		return vref->driver->list(vref, entries, max);
	}

	// TODO No driver at leaf node, return error?
	return 0;
}

int vfs_mkdir(vRef* vref, const char* name) {
	if (vref->driver) {
		return vref->driver->mkdir(vref, name);
	}

	// TODO No driver at leaf node, return error?
	return 0;
}

int vfs_remove(vRef* vref, bool rmdir) {
	if (vref->driver) {
		return vref->driver->remove(vref, rmdir);
	}

	// TODO No driver at leaf node, return error?
	return 0;
}

int vfs_stat(vRef* vref, vStat* stat) {
	if (vref->driver) {
		return vref->driver->stat(vref, stat);
	}

	// TODO No driver at leaf node, return error?
	return 0;
}

int vfs_resolve(vPath* path, char* buffer) {

	// index into the filename buffer
	int i = 0;
	int last = FILE_MAX_NAME - 1;

	// the path is parsed in sections that start with / and end
	// before the start of next section:
	//  [/some][/path][/] => ['some', 'path'] (notice the discarded section!)
	//  [foo][/bar]       => ['foo', 'bar']   (the initial slash is optional from our perspective!)
	//  [/proc]           => ['proc']
	//  [.][/..]          => ['.', '..']

	// this 'consumes' the initial '/'
	// this is fine even if offset point to the \0, we will not increment in that case
	if (path->string[path->offset] == '/') {
		path->offset ++;
	}

	// stop resolving once we reach the end
	// handles "/\0" end as this is done after consuming the '/'
	if (path->string[path->offset] == '\0') {
		return 0;
	}

	// match the name until we reach the end of section (start of next section or \0)
	while (true) {

		// filename limit exceded, we will handle it as if the section ended here
		if (i >= last) {
			path->errno = LINUX_ENAMETOOLONG;
			break;
		}

		const char chr = path->string[path->offset ++];

		// handle interpath separator and string end
		if ((chr == '/') || (chr == '\0')) {

			// 'unget' last char
			path->offset --;
			break;
		}

		// copy into filename buffer
		buffer[i ++] = chr;

	}

	// insert null-byte
	buffer[i] = '\0';
	path->resolves ++;

	return 1;
}

void vfs_init() {
	vfs_root_node.child = NULL;
	vfs_root_node.sibling = NULL;
	vfs_root_node.parent = &vfs_root_node;
	memcpy(vfs_root_node.name, "root", 5);

	vfs_root_ref.node = &vfs_root_node;
	vfs_root_ref.offset = 0;
	vfs_root_ref.driver = NULL;
	vfs_root_ref.state = NULL;
}

int vfs_mount(const char* path, FilesystemDriver* driver) {

	vPath vpth;
	vpth.string = path;
	vpth.offset = 0;
	vpth.resolves = 0;

	if (path[0] != '/') {
		panic("Can't mount to relative directory!");
	}

	char buffer[FILE_MAX_NAME];
	vNode* node = &vfs_root_node;

	while (vfs_resolve(&vpth, buffer)) {
		if (node->child == NULL) {
			vNode* vn = vfs_mknode(node, buffer);

			node->child = vn;
			node = vn;
			continue;
		}

		if (vfs_findchld(&node, buffer) == 0) {
			vNode* vn = vfs_mknode(node->parent, buffer);

			node->sibling = vn;
			node = vn;
			continue;
		}
	}

	kprintf("Mounted %s at %s\n", driver->identifier, path);
	node->driver = driver;

	return 0;
}

void vfs_print(vNode* node, int depth) {

	if (node == NULL) {
		node = &vfs_root_node;
	}

	for (int i = 0; i < depth; i ++) {
		kprintf("%c ", 0xB3);
	}

	if (depth > 0) {
		kprintf("\x8\x8%c ", 0xC3);
	}

	kprintf("%s at '%s' (child of: %s)\n", node->name, node->driver->identifier, node->parent->name);
	node = node->child;

	while (node != NULL) {
		vfs_print(node, depth + 1);
		node = node->sibling;
	}

}
