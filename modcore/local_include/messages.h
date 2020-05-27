#ifndef MODCORE_MESSAGES_H
#define MODCORE_MESSAGES_H
#include "defs.h"

void handle_recvmsg(stackframe *frame);
void handle_sendmsg(stackframe *frame);
void handle_listen_interface(stackframe *frame);

void handle_transfer_message(stackframe *frame);
void handle_transfer_interface(stackframe *frame);

void setup_interface_data();

#endif