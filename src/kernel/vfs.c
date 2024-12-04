#include "vfs.h"
#include "memory.h"
#include "kmalloc.h"
#include "print.h"
#include "util.h"

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

static void vfs_update(vRef* vref) {

	if ((vref->driver != NULL) && (vref->driver != vref->node->driver)) {
		kprintf(" * driver %s: close\n", vref->driver->identifier);
		goto enable;
	}

	if (vref->driver == NULL) {
		enable:
		vref->driver = vref->node->driver;

		kprintf(" * driver %s: root\n", vref->driver->identifier);
		return;
	}

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

		kprintf(" * driver %s: clone\n", vref->driver->identifier);
	}
}

static void vfs_enter(vRef* vref, const char* part) {

	vNode* node = vref->node;

	if (streq(part, ".")) {
		return;
	}

	if (streq(part, "..")) {

		if (vref->offset > 0) {
			vref->offset --;
			vfs_update(vref);
			kprintf(" * driver %s: open %s\n", vref->driver->identifier, part);
			return;
		}

		// change mount point
		// this is safe, root loopbacks back onto itself
		vref->node = vref->node->parent;
		vfs_update(vref);
		return;
	}

	if ((vref->offset == 0) && vfs_findchld(&node, part)) {
		vref->node = node;
		vfs_update(vref);
		return;
	}

	// step into the unknown
	vref->offset ++;

	vfs_update(vref);
	kprintf(" * driver %s: open %s\n", vref->driver->identifier, part);

}

/* public */

vRef vfs_root() {
	return vfs_root_ref;
}

vRef vfs_open(vRef* relation, const char* path) {

	kprintf("open '%s'\n", path);

	char buffer[FILE_MAX_NAME];

	vPath vpth;
	vpth.string = path;
	vpth.offset = 0;
	vpth.resolves = 0;

	vRef vref;

	// absolute path
	if (path[0] == '/') {
		vfs_refcpy(&vref, &vfs_root_ref);
	} else {
		vfs_refcpy(&vref, relation);
	}

	while (vfs_resolve(&vpth, buffer)) {
		vfs_enter(&vref, buffer);
	}

	return vref;

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

		const char chr = path->string[path->offset ++];

		// handle interpath separator and string end
		if ((chr == '/') || (chr == '\0')) {

			// 'unget' last char
			path->offset --;
			break;
		}

		// copy into filename buffer
		buffer[i ++] = chr;

		// filename limit exceded, we will handle it as if the section ended here
		if (i >= last) {
			break;
		}

	}

	// insert null-byte
	buffer[i] = '\0';
	path->resolves ++;

	return 1;
}

int vfs_findchld(vNode** node, const char* name) {
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
