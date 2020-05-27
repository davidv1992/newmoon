#ifndef _MESSAGE_H
#define _MESSAGE_H

#include <stdint.h>
#include <_core_defs.h>

typedef struct {
    uint32_t type;
    uint32_t payload[MESSAGE_SIZE/sizeof(uint32_t)-1];
} __attribute__((packed)) message_base;

#endif