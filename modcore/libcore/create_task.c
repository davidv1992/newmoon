#include <core.h>
#include <_core_ops.h>

int create_task(void* entry, void *stack) {
    int res;
    asm("int $0x20": "=a"(res) : "a"(CORE_CALL_CREATE), "b"(stack), "c"(entry));
    return res;
}