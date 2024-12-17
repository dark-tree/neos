#include "types.h"
#include "config.h"
#include "kmalloc.h"
#include "routine.h"
#include "print.h"
#include "scheduler.h"
#include "rivendell.h"
#include "tables.h"
#include "gdt.h"


ProcessDescriptor* general_process_table;
int process_count;
int process_table_size = INITIAL_PROCESS_TABLE_SIZE;

int process_running;

int processes_existing;

bool scheduler_pid_invalid(int pid)
{
	if (pid<=0)
	{
		return true;
	}

	pid--;
	if (pid>=process_count)
	{
		return true;
	}

	return !general_process_table[pid].exists;
}

int scheduler_get_current_pid()
{
	return process_running+1;
}

int scheduler_load_process_info(ProcessDescriptor* processInfo, int pid)
{
	if(scheduler_pid_invalid(pid))
	{
		return 1;
	}

	pid--;

	ProcessDescriptor* process = general_process_table + pid;

	processInfo->process_memory = process->process_memory;
	processInfo->state = process->state;
	processInfo->parent_index = process->parent_index;
	processInfo->files = process->files;
	processInfo->fileExists = process->fileExists;
	processInfo->cwd = process->cwd;
	processInfo->exe = process->exe;
    processInfo->mount = process->mount;
    processInfo->processSegmentsIndex = process->processSegmentsIndex;
	return 0;
}

int scheduler_process_list(int* pid)
{
	int i = *pid;

	// should never happen
	if (i < 0) {
		return 0;
	}

	while (i <= process_count) {
		i ++;

		if (general_process_table[i - 1].exists) {
			*pid = i;
			return 1;
		}
	}

	return 0;
}

//For testing only
int get_index(int i)
{
	return general_process_table[i].parent_index;
}

void scheduler_init()
{
    process_running = (-1);
    general_process_table = (ProcessDescriptor*)kmalloc(sizeof(ProcessDescriptor)*process_table_size);
}


void scheduler_new_entry(int parent_index, void* stack, void* process_memory, vRef* exe, uint32_t mount, int processSegmentIndex)
{
	int index = 0;
	while(index!=process_count && general_process_table[index].exists!=false)
	{
			index++;
	}
	if(index==process_count)
	{
		if(process_table_size==process_count)
		{
			process_table_size *= 2;
			general_process_table = (ProcessDescriptor*)krealloc((void*)general_process_table, sizeof(ProcessDescriptor)*process_table_size);
		}
		process_count++;
	}
	ProcessDescriptor* new_entry = general_process_table+index;
	new_entry->exists = true;
	new_entry->stack = stack;
	new_entry->parent_index = parent_index;
	new_entry->process_memory = process_memory;
	new_entry->files = kmalloc(sizeof(vRef)*MAX_FILES_PER_PROCESS);
	new_entry->fileExists = kmalloc(sizeof(bool)*MAX_FILES_PER_PROCESS);
	new_entry->exe = *exe;
    new_entry->mount = mount;
    new_entry->processSegmentsIndex = processSegmentIndex;

	// TODO set this to some better value
	new_entry->cwd = vfs_root();

	for(int i=0; i<MAX_FILES_PER_PROCESS;i++)
	{
		new_entry->fileExists[i] = false;
	}
	new_entry->state = RUNNABLE;
    process_count++;
}

void scheduler_remove_entry(int index)
{
	ProcessDescriptor* process = general_process_table+index;
	process->exists=false;
	kfree(process->files);
	kfree(process->fileExists);
}

int scheduler_kill_process(int pid)
{
	if(scheduler_pid_invalid(pid))
	{
		return 1;
	}
	pid--;
	ProcessDescriptor* process = general_process_table+pid;
	kfree(general_process_table[pid].process_memory);
	vfs_close(&process->cwd);
	scheduler_remove_entry(pid);
	return 0;
}

int scheduler_create_process(int parent_pid, vRef* processFile)
{
	if(parent_pid!=(-1) && scheduler_pid_invalid(parent_pid))
	{
		return 1;
	}
	ProgramImage image;
    image.prefix = 134512640;
	image.sufix = INITIAL_STACK_SIZE;
	int errorCode = elf_load(processFile, &image, true);
	if(errorCode)
	{
		return 1;
	}
	uint32_t size = kmsz(image.image);
	void* stack = image.image + size;
    int virtualStack = ((stack - image.image) - image.prefix)+image.mount;
    int processSegmentIndex = gput(image.image, size);
    stack = isr_stub_stack(stack, image.entry, processSegmentIndex+1, processSegmentIndex);
    scheduler_new_entry(parent_pid, virtualStack, image.image, processFile, image.mount, processSegmentIndex);
	return 0;
}

int scheduler_context_switch(void* old_stack)
{
	if(process_count==0)
	{
		return (int) old_stack;
	}

	general_process_table[process_running].stack = old_stack;
	process_running++;
	while(general_process_table[process_running].exists==false)
	{
		process_running++;
	}
	if(process_running>=process_count)
	{
		process_running=0;
	}
	return (int) general_process_table[process_running].stack;
}

int scheduler_chdir(int pid, vRef* cwd) {
	if(scheduler_pid_invalid(pid))
	{
		return 1;
	}
	general_process_table[pid-1].cwd = *cwd;
	return 0;
}

int scheduler_fput(int pid, vRef vref)
{
	if(scheduler_pid_invalid(pid))
	{
		return 0;
	}
	int index = pid-1;
	for(int i = 0; i<MAX_FILES_PER_PROCESS; i++)
	{
		if(!general_process_table[index].fileExists[i])
		{
			general_process_table[index].files[i]=vref;
			general_process_table[index].fileExists[i] = true;
			return i+1;
		}
	}

	return 0;
}

vRef* scheduler_fget(int pid, int fd)
{
	if(scheduler_pid_invalid(pid))
	{
		return NULL;
	}
	int index = pid-1;

	fd = fd-1;
	if(fd<0 || fd >=MAX_FILES_PER_PROCESS)
	{
		return NULL;
	}
	if(!general_process_table[index].fileExists[fd])
	{
		return NULL;
	}
	return general_process_table[index].files + fd;

	return NULL;
}

int scheduler_fremove(int pid, int fd)
{
	if(scheduler_pid_invalid(pid))
	{
		return 1;
	}
	int index = pid-1;
	fd = fd-1;
	if(fd<0 || fd >=MAX_FILES_PER_PROCESS)
	{
		return 1;
	}
	if(!general_process_table[index].fileExists[fd])
	{
		return 1;
	}
	general_process_table[index].fileExists = false;
	return 0;

	return 1;
}

int scheduler_move_process(int pid, void* new_address)
{
    if(scheduler_pid_invalid(pid))
    {
        return 1;
    }
    general_process_table[pid-1].process_memory = new_address;
    return 0;
}
