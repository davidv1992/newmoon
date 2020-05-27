#include <core.h>
#include <_core_ops.h>

int listen_interface(int interface) {
    int result;
    asm("int $0x20": "=a"(result): "a"(CORE_CALL_LISTEN), "b"(interface));
    return result;
}