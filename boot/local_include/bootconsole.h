#ifndef BOOTCONSOLE_H
#define BOOTCONSOLE_H

#include <stdarg.h>

void bc_putchar(char c);
void bc_puts(const char *c);
void bc_print_va(const char *format, va_list params);
void bc_print(const char *format, ...);
void panic(const char *format, ...);

#endif