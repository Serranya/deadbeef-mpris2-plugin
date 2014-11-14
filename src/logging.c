#include <stdio.h>
#include <stdarg.h>

void logDebug (const char *fmt, ...) {
	va_list arg_ptr;
	va_start(arg_ptr, fmt);
	printf("\e[32m\e[1mMPRIS Debug Info: \e[0m\e[34m");
	vprintf(fmt, arg_ptr);
	printf("\e[0m\n");
	va_end(arg_ptr);
}

void logError (const char *fmt, ...) {
	va_list arg_ptr;
	va_start(arg_ptr, fmt);
	fprintf(stderr, "\e[32m\e[1mMPRIS Error Info: \e[0m\e[34m");
	vfprintf(stderr, fmt, arg_ptr);
	fprintf(stderr, "\e[0m\n");
	va_end(arg_ptr);
}
