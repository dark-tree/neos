#pragma once
#include "vfs.h"

typedef enum {
	RUNNABLE,
	SLEEPING,
	STOPPED,
	ZOMBIE
} ProcessState;

typedef struct {
	bool exists;
	void* stack;
	void* process_memory;
	ProcessState state;
	int parent_index;
	vRef* files;
	bool* fileExists;
	vRef cwd;
	vRef exe;
    uint32_t mount;
    int processSegmentsIndex;
} ProcessDescriptor;

extern int scheduler_get_current_pid();

extern int get_index(int i);

extern void scheduler_init();

int scheduler_move_process(int pid, void* new_address);

int scheduler_create_process(int parent_pid, vRef* processFile);

int scheduler_context_switch(void* old_stack);

int scheduler_kill_process(int pid);

int scheduler_load_process_info(ProcessDescriptor* processInfo, int pid);

int scheduler_process_list(int* pid);

int scheduler_chdir(int pid, vRef* cwd);

int scheduler_fput(int pid, vRef vref);

vRef* scheduler_fget(int pid, int fd);

int scheduler_fremove(int pid, int fd);
