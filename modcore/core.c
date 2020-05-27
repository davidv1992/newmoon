#include <stdint.h>
#include <stdbool.h>
#include <core.h>
#include <_core_defs.h>
#include <_core_ops.h>
#include "local_include/proc.h"
#include "local_include/defs.h"
#include "local_include/messages.h"
#include "local_include/transfer_calls.h"

// Idle task logic
void idle_task();

stackframe idle_frame = {
    ds: 0x10,
    es: 0x10,
    edi: 0,
    esi: 0,
    ebp: 0,
    esp_ign: 0,
    edx: 0,
    ecx: 0,
    ebx: 0,
    eax: 0,
    intno: 0,
    error: 0,
    eip: (uint32_t)&idle_task,
    cs: 0x08,
    eflags: 0, //filled in on startup
    esp: ((uint32_t)&idle_frame) + sizeof(stackframe),
};

// Task logic
task task_table[MAX_TASKS];
int current_task;
int runnable_head;
int runnable_tail;
int unused_head;
int unused_tail;
int transfer_idx;
int transfer_queue_head;
int transfer_queue_tail;
int task_self;

void stop_current() {
    runnable_head = task_table[current_task].list_next;
    if (runnable_head == -1) runnable_tail = -1;
}

void make_runnable(int task) {
    task_table[task].task_state = TASK_STATE_RUNNABLE;
    task_table[task].list_next = -1;
    if (runnable_tail == -1) {
        runnable_tail = runnable_head = task;
    } else {
        task_table[runnable_tail].list_next = task;
        runnable_tail = task;
    }
}

void free_task(int task) {
    task_table[task].task_state = TASK_STATE_UNUSED;
    task_table[task].list_next = -1;
    if (unused_tail == -1) {
        unused_head = unused_tail = task;
    } else {
        task_table[unused_tail].list_next = task;
        unused_tail = task;
    }
}

static void handle_create_task(stackframe *frame) {
    // Take first free task
    if (unused_head == -1) {
        frame->eax = -1;
        return;
    }
    int result = unused_head;
    unused_head = task_table[unused_head].list_next;
    if (unused_head == -1) unused_tail = -1;

    // Setup task structure
    task_table[result].send_wait_start = -1;
    task_table[result].send_wait_end = -1;
    task_table[result].mq_head = 0;
    task_table[result].mq_used = 0;

    // Setup stackframe
    task_table[result].stack = (void*)(frame->ebx-sizeof(stackframe));
    task_table[result].stack->eax = result;
    task_table[result].stack->ebx = 0;
    task_table[result].stack->ecx = 0;
    task_table[result].stack->edx = 0;
    task_table[result].stack->esi = 0;
    task_table[result].stack->edi = 0;
    task_table[result].stack->ebp = 0;
    task_table[result].stack->eflags = idle_frame.eflags;
    task_table[result].stack->ds = 0x10;
    task_table[result].stack->es = 0x10;
    task_table[result].stack->cs = 0x08;
    task_table[result].stack->eip = frame->ecx;
    task_table[result].stack->esp = frame->ebx;

    // Add to runnable queue
    make_runnable(result);

    // Return result
    frame->eax = result;
}

static void handle_corecall(stackframe *frame) {
    if (frame->eax == CORE_CALL_SEND) {
        handle_sendmsg(frame);
    } else if (frame->eax == CORE_CALL_RECV) {
        handle_recvmsg(frame);
    } else if (frame->eax == CORE_CALL_LISTEN) {
        handle_listen_interface(frame);
    } else if (frame->eax == CORE_CALL_CREATE) {
        handle_create_task(frame);
    } else {
        // Problem, hlt for now
        asm("hlt");
    }
}

extern int interface_table[MAX_IF];

static void handle_transfer_task(stackframe *frame) {
    // Select task to transfer
    int task = -1;
    if (transfer_queue_head != -1) {
        task = transfer_queue_head;
        transfer_queue_head = task_table[task].list_next;
        if (transfer_queue_head == -1) transfer_queue_tail = -1;
    } else {
        while (transfer_idx < MAX_TASKS && 
            (task_table[transfer_idx].task_state == TASK_STATE_UNUSED 
            || task_table[transfer_idx].task_state == TASK_STATE_SELF
            || task_table[transfer_idx].task_state == TASK_STATE_TRANSFER_TARGET)) {
            // SELF is always transferred last, and the transfer target
            // is never transferred
            transfer_idx++;
        }
        if (transfer_idx < MAX_TASKS) {
            task = transfer_idx;
            transfer_idx++;
        }
    }

    // Check if this task is waiting to send to a non-transferred task
    // we loop because this might be cascading in extreme cases.
    // this can technically infinite-loop, but only in the case
    // of a deadlock, in which case we are screwed anyway
    while (task_table[task].task_state == TASK_STATE_WAIT_SEND) {
        int dest = task_table[task].stack->ecx;
        int flags = task_table[task].stack->edx;

        // Handle interfaces
        if (flags & SEND_FLAG_INTERFACE) {
            if (dest < 0 || dest >= MAX_IF) return;
            dest = interface_table[dest];
        }

        if (task_table[dest].task_state != TASK_STATE_TRANSFERRED) {
            // Transfer destination over first;
            transfer_idx--;
            task = dest;
        } else {
            break;
        }
    }

    // Handle case of final task
    if (task == -1) {
        frame->eax = task_self;
        frame->ebx = TASK_STATE_SELF;
        frame->ecx = (uint32_t)task_table[task_self].stack;
        frame->edx = 0;
        return;
    }

    // Transfer task data
    frame->eax = task;
    frame->ebx = task_table[task].task_state;
    frame->ecx = (uint32_t)task_table[task].stack;
    frame->edx = task_table[task].mq_used;

    // Mark task transferred
    task_table[task].task_state = TASK_STATE_TRANSFERRED;

    // And add to transfer queue if needed
    if (task_table[task].send_wait_start != -1) {
        if (transfer_queue_tail == -1) {
            transfer_queue_head = task_table[task].send_wait_start;
            transfer_queue_tail = task_table[task].send_wait_end;
        } else {
            task_table[transfer_queue_tail].list_next = task_table[task].send_wait_start;
            transfer_queue_tail = task_table[task].send_wait_end;
        }
    }
}

static void handle_transfer(stackframe *frame) {
    task_table[current_task].task_state = TASK_STATE_TRANSFER_TARGET;
    if (frame->eax == TRANSFER_INTERFACE) {
        handle_transfer_interface(frame);
    } else if (frame->eax == TRANSFER_TASK) {
        handle_transfer_task(frame);
    } else if (frame->eax == TRANSFER_MESSAGE) {
        handle_transfer_message(frame);
    } else {
        // Problem, hlt for now
        asm("hlt");
    }
}


stackframe *isr_base_handler(stackframe *frame) {
    task_table[current_task].stack = frame;
    if (frame->intno == 0x20) {
        handle_corecall(frame);
    } else if (frame->intno == 0x21) {
        handle_transfer(frame);
    } else if (frame->intno == 0x22) {
        // Handover, ignore
    } else {
        // Problem, hlt for now (will stop everything as interrupts are disabled here)
        asm("hlt");
    }
    // Check which task to run next
    current_task = runnable_head;
    if (current_task == -1) return &idle_frame;
    else return task_table[current_task].stack;
}

void idtbuildtable();

void module_main(int self) {
    // Setup information on self
    task_self = self;

    // setup all the lists except unused
    runnable_head = runnable_tail = -1;
    transfer_idx = 0;
    transfer_queue_head = -1;
    transfer_queue_tail = -1;
    current_task = self;

    // Setup interupt table
    idtbuildtable();

    // Setup basics of task table
    for (int i=0; i<MAX_TASKS; i++) task_table[i].task_state = TASK_STATE_UNUSED;

    // Transfer interfaces, from this point we will get marked as core transfer
    //  target, and can no longer use any of the message interfaces.
    setup_interface_data();

    // Transfer tasks
    int last_task_state = TASK_STATE_UNUSED;
    while (last_task_state != TASK_STATE_SELF) {
        // Transfer task
        int task_id;
        int task_state;
        void *stack;
        int messages;
        asm("int $0x21": "=a"(task_id), "=b"(task_state), "=c"(stack), "=d"(messages) : "a"(TRANSFER_TASK));
        task_table[task_id].task_state = task_state;
        task_table[task_id].stack = stack;
        task_table[task_id].send_wait_start = -1;
        task_table[task_id].send_wait_end = -1;
        last_task_state = task_state;
        
        // Transfer messages
        for (int i=0; i<messages; i++) {
            asm("int $0x21": "=a"(task_table[task_id].msgqueue[i].type) : "a"(TRANSFER_MESSAGE), "b"(task_id), "c"(i), "d"(task_table[task_id].msgqueue[i].data));
        }
        task_table[task_id].mq_head = 0;
        task_table[task_id].mq_used = messages;

        // Figure out the rest of task state
        if (task_state == TASK_STATE_RUNNABLE) {
            make_runnable(task_id);
        } else if (task_state == TASK_STATE_WAIT_SEND) {
            current_task = task_id; // fake this task as currently running
            handle_sendmsg(stack);
        } else if (task_state == TASK_STATE_SELF) {
            make_runnable(task_id);
        }
    }

    // Place ourselves in the expected state
    task_table[self].task_state = TASK_STATE_SELF;

    // Setup unused task list
    unused_head = -1;
    unused_tail = -1;
    for (int i=0; i<MAX_TASKS; i++) {
        if (task_table[i].task_state == TASK_STATE_UNUSED) {
            task_table[i].list_next = -1;
            if (unused_tail == -1) {
                unused_head = unused_tail = i;
            } else {
                task_table[unused_tail].list_next = i;
                unused_tail = i;
            }
        }
    }

    // AND GO
    switchover();
    // Messages are ok again, and in fact, we need them to finalize properly
    uint32_t msgbuf[MESSAGE_SIZE/sizeof(uint32_t)];
    msgbuf[0] = self;
    msgbuf[1] = 0;
    send_message(msgbuf, 0, SEND_FLAG_INTERFACE | SEND_FLAG_EXIT);

    __builtin_unreachable();
}