#include <core.h>
#include <_core_ops.h>

void send_message(const void *message, int destination, int flags) {
    asm("int $0x20": : "a"(CORE_CALL_SEND), "b"(message), "c"(destination), "d"(flags));
}