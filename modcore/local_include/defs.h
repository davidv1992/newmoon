#ifndef MODCORE_DEFS_H
#define MODCORE_DEFS_H

#include <stdint.h>
#include <_core_defs.h>

// Structures for ldt
typedef struct {
    uint32_t data[2];
} __attribute__((packed)) gdt_entry;

typedef struct {
    uint16_t size;
    uint32_t offset;
} __attribute__((packed)) xdt_ptr;

//Interrupt stack layout
typedef struct {
    uint32_t ds;
    uint32_t es;
    uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp_ign;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
    uint32_t intno;
    uint32_t error;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;
} __attribute__((packed)) stackframe;

// Task datastructures
#define MESSAGE_QUEUE_SIZE 16
#define MAX_TASKS 1024
#define MAX_IF 256

#define TASK_STATE_UNUSED          0
#define TASK_STATE_RUNNABLE        1
#define TASK_STATE_WAIT_RECV       2
#define TASK_STATE_WAIT_SEND       3
#define TASK_STATE_SELF            4
#define TASK_STATE_TRANSFERRED     5
#define TASK_STATE_TRANSFER_TARGET 6

#define MESSAGE_TYPE_FRESH 0
#define MESSAGE_TYPE_RESP  1
#define MESSAGE_TYPE_SKIP  2

typedef struct {
    int type;
    uint32_t data[MESSAGE_SIZE/sizeof(uint32_t)];
    int sender;
} message;

typedef struct {
    stackframe *stack;
    message msgqueue[MESSAGE_QUEUE_SIZE];
    int mq_head;
    int mq_used;
    int task_state;

    int send_wait_start;
    int send_wait_end;

    int list_next;
} task;

#endif