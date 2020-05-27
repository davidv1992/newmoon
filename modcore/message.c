#include <stdint.h>
#include <stdbool.h>
#include "local_include/defs.h"
#include "local_include/task.h"
#include "local_include/transfer_calls.h"

extern task task_table[MAX_TASKS];
int interface_table[MAX_IF];
extern int current_task;

static inline void copy_message(uint32_t* restrict src, uint32_t* restrict dst) {
    for (int i=0; i<(int)(MESSAGE_SIZE/sizeof(uint32_t)); i++) dst[i] = src[i];
}

static inline int message_index(int head, int offset) {
    return (head + offset) % MESSAGE_QUEUE_SIZE;
}

static void complete_send(int task) {
    if (task_table[task].stack->edx & SEND_FLAG_EXIT) {
        if (task == current_task) stop_current();
        free_task(task);
    } else {
        make_runnable(task);
    }
}

static void place_message_in_queue(uint32_t* msg, int flags, int dest, int slot, int sender) {
    copy_message(msg, task_table[dest].msgqueue[slot].data);
    task_table[dest].msgqueue[slot].sender = sender;
    if (flags & SEND_FLAG_RESPONSE)
        task_table[dest].msgqueue[slot].type = MESSAGE_TYPE_RESP;
    else
        task_table[dest].msgqueue[slot].type = MESSAGE_TYPE_FRESH;
}

void handle_recvmsg(stackframe *frame) {
    int flags = frame->edx;

    bool need_sleep = true;
    bool progress_mq = false;
    
    // Handle the two main cases
    if (flags & RECV_FLAG_RESPONSE) {
        // Harder, might need scan
        if (task_table[current_task].mq_used != 0) {
            if (task_table[current_task].msgqueue[task_table[current_task].mq_head].type == MESSAGE_TYPE_RESP) {
                need_sleep = false;
                progress_mq = true;
                copy_message(task_table[current_task].msgqueue[task_table[current_task].mq_head].data, (uint32_t*)frame->ebx);
                frame->eax = task_table[current_task].msgqueue[task_table[current_task].mq_head].sender;
            } else {
                // Scan mq
                int mq_head = task_table[current_task].mq_head;
                for (int i=1; i<task_table[current_task].mq_used && need_sleep; i++) {
                    int cur_msg = message_index(mq_head, i);
                    if (task_table[current_task].msgqueue[cur_msg].type == MESSAGE_TYPE_RESP) {
                        // Found one
                        need_sleep = false;
                        copy_message(task_table[current_task].msgqueue[cur_msg].data, (uint32_t*)frame->ebx);
                        task_table[current_task].msgqueue[cur_msg].type = MESSAGE_TYPE_SKIP;
                        frame->eax = task_table[current_task].msgqueue[cur_msg].sender;
                    }
                }
                
                // Scan waiting senders list
                int cur = task_table[current_task].send_wait_start;
                int prev = -1;
                while (cur != -1 && need_sleep) {
                    if (task_table[cur].stack->edx & SEND_FLAG_RESPONSE) {
                        // Remove task from list
                        task_table[prev].list_next = task_table[cur].list_next;
                        if (cur == task_table[current_task].send_wait_end)
                            task_table[current_task].send_wait_end = prev;
                        // Copy over message
                        copy_message((uint32_t*)task_table[cur].stack->ebx, (uint32_t*)frame->ebx);
                        frame->eax = cur;
                        complete_send(cur);
                        need_sleep = false;
                    }

                    prev = cur;
                    cur = task_table[current_task].list_next;
                }
            }
        }
    } else {
        // Easy, just take first message (msg_head)
        if (task_table[current_task].mq_used != 0) {
            need_sleep = false;
            progress_mq = true;
            copy_message(task_table[current_task].msgqueue[task_table[current_task].mq_head].data, (uint32_t*)frame->ebx);
            frame->eax = task_table[current_task].msgqueue[task_table[current_task].mq_head].sender;
        }
    }

    // Go to sleep if needed
    if (need_sleep) {
        task_table[current_task].task_state = TASK_STATE_WAIT_RECV;
        stop_current();
    }

    // Progress message queue
    if (progress_mq) {
        task_table[current_task].msgqueue[task_table[current_task].mq_head].type = MESSAGE_TYPE_SKIP;

        while (task_table[current_task].mq_used > 0 && task_table[current_task].msgqueue[task_table[current_task].mq_head].type == MESSAGE_TYPE_SKIP) {
            if (task_table[current_task].send_wait_start != 0) {
                // Copy message over
                int ref_task = task_table[current_task].send_wait_start;
                place_message_in_queue(
                    (uint32_t*)task_table[ref_task].stack->ebx,
                    task_table[ref_task].stack->edx, 
                    current_task,
                    task_table[current_task].mq_head,
                    ref_task
                );

                // Remove task from wait list
                task_table[current_task].send_wait_start = task_table[ref_task].list_next;
                if (task_table[current_task].send_wait_start == -1)
                    task_table[current_task].send_wait_end = -1;
                
                // And add it to runnables
                complete_send(ref_task);
            } else {
                // Just remove the message
                task_table[current_task].mq_used--;
            }

            // progress mq_head
            task_table[current_task].mq_head++;
            if (task_table[current_task].mq_head == MESSAGE_QUEUE_SIZE) task_table[current_task].mq_head = 0;
        }
    }
}

void handle_sendmsg(stackframe *frame) {
    int dest = frame->ecx;
    int flags = frame->edx;

    // Handle interfaces
    if (flags & SEND_FLAG_INTERFACE) {
        if (dest < 0 || dest >= MAX_IF) return;
        dest = interface_table[dest];
    }

    // Check destination
    if (dest < 0 || dest >= MAX_TASKS || task_table[dest].task_state == TASK_STATE_UNUSED) return;

    // Figure out whether we need to queue the message
    bool needQueue = true;
    if (task_table[dest].task_state == TASK_STATE_WAIT_RECV && (task_table[dest].stack->edx & RECV_FLAG_RESPONSE) <= (flags & SEND_FLAG_RESPONSE)) {
        needQueue = false;
    }

    // Process message
    if (!needQueue) {
        // Copy message
        copy_message((uint32_t*)frame->ebx, (uint32_t*)task_table[dest].stack->ebx);
        // flag handling not needed here, as all the needed checks on them are already done

        // Make destination runnable
        make_runnable(dest);
    } else if (task_table[dest].mq_used < MESSAGE_QUEUE_SIZE) {
        // Add message to queue
        int mqidx = message_index(task_table[dest].mq_head, task_table[dest].mq_used);
        place_message_in_queue((uint32_t*)frame->ebx, flags, dest, mqidx, current_task);
        task_table[dest].mq_used++;
    } else {
        // Add current task to send wait list of destination
        task_table[current_task].task_state = TASK_STATE_WAIT_SEND;
        task_table[current_task].list_next = -1;
        if (task_table[dest].send_wait_end == -1) {
            task_table[dest].send_wait_start = task_table[dest].send_wait_end = current_task;
        } else {
            task_table[task_table[dest].send_wait_end].list_next = current_task;
            task_table[dest].send_wait_end = current_task;
        }
    }

    // Handle exit_after if needed
    if ((flags & SEND_FLAG_EXIT) && task_table[current_task].task_state == TASK_STATE_RUNNABLE) {
        stop_current();
        free_task(current_task);
    }
}

void handle_listen_interface(stackframe *frame) {
    unsigned int interface = frame->ebx;
    if (interface > MAX_IF) return;
    frame->eax = interface_table[interface];
    interface_table[interface] = current_task;
}

void handle_transfer_message(stackframe *frame) {
    uint32_t *dest_buffer = (uint32_t*)frame->edx;
    int task_id = frame->ebx;
    int msg_idx = frame->ecx;

    if (task_id < 0 || task_id >= MAX_TASKS) {
        frame->eax = -1;
        return;
    }

    if (msg_idx < 0 || task_table[task_id].mq_used <= msg_idx) {
        frame->eax = -1;
        return;
    }

    int ref_idx = message_index(task_table[task_id].mq_head, msg_idx);

    copy_message(task_table[task_id].msgqueue[ref_idx].data, dest_buffer);
    frame->eax = task_table[task_id].msgqueue[ref_idx].type;
}

void handle_transfer_interface(stackframe *frame) {
    int if_id = frame->ebx;
    if (if_id < 0 || if_id >= MAX_IF) {
        frame->eax = -1;
        return;
    }
    frame->eax = interface_table[if_id];
}

void setup_interface_data() {
    for (int i=0; i<MAX_IF; i++) {
        asm("int $0x21" : "=a"(interface_table[i]) : "a" (TRANSFER_INTERFACE), "b"(i));
    }
}