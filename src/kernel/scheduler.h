#ifndef SCHEDULER_H
#define SCHEDULER_H


extern int scheduler_get_current_pid();

extern int get_index(int i);

extern void scheduler_init();

void scheduler_new_entry(int parent_index, void* stack, void* process_memory);

void scheduler_create_process(int parent_index, void* process_memory);

int scheduler_context_switch(void* old_stack);

#endif // SCHEDULER_H
