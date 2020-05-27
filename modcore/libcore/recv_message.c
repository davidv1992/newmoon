#include <core.h>
#include <_core_ops.h>

int recv_message(void *message, int flags) {
    int result;
    asm("int $0x20": "=a"(result) : "a"(CORE_CALL_RECV), "b" (message), "d"(flags) );
    return result;
}