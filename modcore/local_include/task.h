#ifndef MODCORE_TASK_H
#define MODCORE_TASK_H

void make_runnable(int task);
void free_task(int task);
void stop_current();

#endif