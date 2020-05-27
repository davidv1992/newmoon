#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#define BC_COLS 80
#define BC_ROWS 25

static uint8_t *console_buffer = (uint8_t*)0xB8000;

static int cur_row = 0, cur_col = 0;
static bool has_initialized = false;

static void bc_init() {
	for (int i=0; i<BC_ROWS; i++) {
		for (int j=0; j<BC_COLS; j++) {
			console_buffer[2*(i*BC_COLS+j)] = ' ';
			console_buffer[2*(i*BC_COLS+j)+1] = 15;
		}
	}
	has_initialized = true;
}

static void bc_scroll() {
	for (int i=0; i<BC_ROWS-1; i++) {
		for (int j=0; j<BC_COLS; j++) {
			console_buffer[2*(i*BC_COLS+j)] =
				console_buffer[2*((i+1)*BC_COLS+j)];
		}
	}
	
	for (int j=0; j<BC_COLS; j++) {
		console_buffer[2*((BC_ROWS-1)*BC_COLS+j)] = ' ';
	}
}

void bc_putchar(char c) {
	if (!has_initialized) bc_init();
		
	// special characters
	if (c == '\n') {
		cur_row++;
		cur_col = 0;
		if (cur_row == BC_ROWS) {
			bc_scroll();
			cur_row--;
		}
		return;
	}
	if (c == '\r') {
		cur_col = 0;
		return;
	}
	
	// normal case
	if (cur_col == BC_COLS) {
		cur_col = 0;
		cur_row++;
	}
	if (cur_row == BC_ROWS) {
		bc_scroll();
		cur_row--;
	}
	console_buffer[2*(cur_row*BC_COLS+cur_col)] = c;
	cur_col++;
}

void bc_puts(const char *c) {
	while (*c != 0) bc_putchar(*(c++));
}

void bc_print_va(const char *format, va_list params) {
	bool inSpecial = false;
	
	while (*format != '\0') {
		if (inSpecial) {
			if (*format == 'x' || *format == 'X') {
				uint32_t cur = (uint32_t)va_arg(params,uint32_t);
				for (int i=7; i>=0; i--) {
					uint32_t curdig = (cur & (0xF << (i*4))) >> (i*4);
					if (curdig < 10)
						bc_putchar('0'+(char)curdig);
					else if (*format == 'x')
						bc_putchar('a'+(char)(curdig-10));
					else
						bc_putchar('A'+(char)(curdig-10));
				}
				inSpecial = false;
				format++;
				continue;
			} else if (*format == 'd') {
				uint32_t cur = (uint32_t)va_arg(params, uint32_t);
				bool hasDig = false;
				uint32_t pow = 1000000000;
				while (pow) {
					uint32_t curdig = cur/pow;
					cur %= pow;
					if (curdig != 0 || pow == 1) hasDig = true;
					if (hasDig) bc_putchar('0'+(char)curdig);
					pow /= 10;
				}
				inSpecial = false;
				format++;
				continue;
			} else if (*format == 's') {
				char *str = (char*)va_arg(params, char*);
				while (*str != '\0') {
					bc_putchar(*str);
					str++;
				}
				format++;
				continue;
			} else if (*format == 'c') {
				char c = (char)va_arg(params,int);
				bc_putchar(c);
				format++;
				continue;
			} else {
				inSpecial = false;
				//falltrough
			}
		}
		
		if (*format == '%') {
			inSpecial = true;
		} else {
			bc_putchar(*format);
		}
		format++;
	}
}

void bc_print(const char *format, ...) {
	va_list params;
	va_start(params, format);
	bc_print_va(format, params);
	va_end(params);
}

void panic(const char *format, ...) {
	va_list params;
	va_start(params, format);
	bc_print_va(format, params);
	va_end(params);
	asm("cli\nhlt\n");
	while(1);
}