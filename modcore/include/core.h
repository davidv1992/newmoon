#ifndef _CORE_H
#define _CORE_H

#include <_core_defs.h>

void send_message(const void *message, int destination, int flags);
int recv_message(void *message, int flags);

int listen_interface(int interface);
int create_task(void* entry, void* stack);

#endif /*_CORE_H*/