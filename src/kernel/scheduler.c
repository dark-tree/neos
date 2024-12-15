#include "types.h"
#include "config.h"
#include "kmalloc.h"
#include "routine.h"
#include "print.h"
#include "scheduler.h"

ProcessDescriptor* general_process_table;
int process_count;
int process_table_size = INITIAL_PROCESS_TABLE_SIZE;

int process_running;

int scheduler_get_current_pid()
{
	return process_running+1;
}

int scheduler_load_process_info(ProcessDescriptor* processInfo, int pid)
{
	if (pid<=0)
	{
		return 1;
	}
	pid--;

	if (pid>=process_count)
	{
		return 1;
	}

	ProcessDescriptor* process = general_process_table + pid;

	if (!process->exists)
	{
		return 1;
	}

	processInfo->process_memory = process->process_memory;
	processInfo->state = process->state;
	processInfo->parent_index = process->parent_index;
	processInfo->files = process->files;
	processInfo->fileExists = process->fileExists;
	processInfo->cwd = process->cwd;
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


void scheduler_new_entry(int parent_index, void* stack, void* process_memory)
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

	// TODO set this to some better value
	new_entry->cwd = vfs_root();

	for(int i=0; i<MAX_FILES_PER_PROCESS;i++)
	{
		new_entry->fileExists[i] = false;
	}
	new_entry->state = RUNNABLE;
}

void scheduler_remove_entry(int index)
{
	general_process_table[index].exists=false;
}

int scheduler_kill_process(int pid)
{
	if(pid<=0)
	{
		return 1;
	}
	pid--;

	ProcessDescriptor* process = general_process_table+pid;
	scheduler_remove_entry(pid);
	//kfree(general_process_table[pid].process_memory);

	vfs_close(&process->cwd);
	kfree(process->files);
	kfree(process->fileExists);
	return 0;
}

int scheduler_create_process(int parent_pid, void* process_memory)
{
	if(parent_pid<=0 && parent_pid!=(-1))
	{
		return 1;
	}
	if(parent_pid!=(-1))
	{
		parent_pid--;
	}
	if(parent_pid>=process_count)
	{
		return 1;
	}
	int process_size = 4096; // TODO For testing only - will be replaced with an allocator call, to check size. Followed by a krealloc call to create space for heap and stack.
	void* stack = process_memory+process_size;
	stack = isr_stub_stack(stack, process_memory, 2, 1);
	scheduler_new_entry(parent_pid, stack, process_memory);
	return 0;
}

int scheduler_context_switch(void* old_stack)
{
	if(0==0)
	{
		return old_stack;
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
	return (int)general_process_table[process_running].stack;
}

void scheduler_chdir(int pid, vRef* cwd) {
	general_process_table[pid].cwd = *cwd;
}

int scheduler_fput(int pid, vRef vref)
{
	if(pid<=0)
	{
		return 0;
	}
	int index = pid-1;
	if(index>=process_count)
	{
		return 0;
	}
	if(general_process_table[index].exists)
	{
		for(int i = 0; i<MAX_FILES_PER_PROCESS; i++)
		{
			if(!general_process_table[index].fileExists[i])
			{
				general_process_table[index].files[i]=vref;
				general_process_table[index].fileExists[i] = true;
				return i+1;
			}
		}
	}
	return 0;
}

vRef* scheduler_fget(int pid, int fd)
{
	if(pid<=0)
	{
		return NULL;
	}
	int index = pid-1;
	if(index>=process_count)
	{
		return NULL;
	}
	if(general_process_table[index].exists)
	{
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
	}
	return NULL;
}

int scheduler_fremove(int pid, int fd)
{
	if(pid<=0)
	{
		return 1;
	}
	int index = pid-1;
	if(index>=process_count)
	{
		return 1;
	}
	if(general_process_table[index].exists)
	{
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
	}
	return 1;
}
