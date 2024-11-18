#include "types.h"
#include "config.h"
#include "kmalloc.h"

//Only for testing. Will be replace with an actual fire descriptor.
typedef struct
{

} FileDescriptor;

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
    FileDescriptor* files;
} ProcessDescriptor;



ProcessDescriptor* general_process_table;
int process_count;
int process_table_size = INITIAL_PROCESS_TABLE_SIZE;

//For testing only
int get_index(int i)
{
    return general_process_table[i].parent_index;
}

void scheduler_init()
{
    general_process_table = (ProcessDescriptor*)kmalloc(sizeof(ProcessDescriptor)*process_table_size);
}


void scheduler_new_entry(int parent_index, void* stack, void* process_memory)
{
    if(process_table_size==process_count)
    {
        general_process_table = (ProcessDescriptor*)krealloc((void*)general_process_table, sizeof(ProcessDescriptor)*process_table_size*2);
    }
    ProcessDescriptor* new_entry = general_process_table+process_count;
    new_entry->exists = true;
    new_entry->stack = stack;
    new_entry->parent_index = parent_index;
    new_entry->process_memory = process_memory;
    new_entry->files = NULL;
    new_entry->state = RUNNABLE;
    process_count++;
}
