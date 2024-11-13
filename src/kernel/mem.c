
#include "mem.h"
#include "config.h"
#include "types.h"
#include "memory.h"
#include "print.h"
#include "kmalloc.h"

/* private */

#define ENTRY_FREE 1

typedef struct {
	uint64_t base;
	uint64_t length;
	uint32_t type;
} ABI_PACKED MemoryEntry;

typedef struct {
	uint32_t length;
	uint32_t bytes;
	MemoryEntry* array;
} ABI_PACKED MemoryMap;

void mem_loadmap(MemoryMap* map) {
	void* region = ((void*) MEMORY_MAP_RAM);

	map->length = *((uint32_t*) region);
	map->bytes = map->length * sizeof(MemoryEntry) + 4;
	map->array = region + 4;
}

/* public */

void mem_init(uint32_t offset) {

	MemoryMap map;
	mem_loadmap(&map);

	kprintf("Found %d memory map entries\n", map.length);

	uint64_t low = 0xFFFFFFFF;
	uint64_t high = 0;

	for (uint64_t i = 0; i < map.length; i ++) {
		MemoryEntry entry = map.array[i];

		if (entry.type == ENTRY_FREE) {
			uint64_t begin = entry.base;
			uint64_t end = entry.base + entry.length;

			if (begin < low) low = begin;
			if (end > high) high = end;
		}
	}

	// always reserve the kernel area
	if (low < offset) {
		low = offset;
	}

	// don't allow more than 4GB in total
	if (high > 0xFFFFFFFF) {
		high = 0xFFFFFFFF;
	}

	uint64_t size = high - low;
	kset(size, low);
	kprintf("Allocator arena set to %#0.8x:%#0.8x, excluding read-only blocks...\n", (int) low, (int) high);

	// exclude reserved regions
	for (uint64_t i = 0; i < map.length; i ++) {
		MemoryEntry entry = map.array[i];

		if (entry.type != ENTRY_FREE) {
			size -= entry.length;

			kprintf(" * Reserved at %#0.8x (%ud bytes)\n", (int) entry.base, (int) entry.length);
			kmres(entry.base, entry.length);
		}
	}

	kprintf("Found %d bytes of usable memory\n", (int) size);

	// we don't need the map anymore, trash it on purpose so
	// it won't mistakenly be used again later
	memset((void*) MEMORY_MAP_RAM, 0, map.bytes);

}
