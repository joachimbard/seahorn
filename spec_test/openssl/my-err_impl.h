/* My own very simple error implementation
 */
#include <stdio.h>
#include <stdarg.h>

void ERR_new() {
  return;
}

void ERR_set_debug(const char *file, int line, const char *func) {
  fprintf(stderr, "Debug call in %s line %d\n", func, line);
}

void ERR_set_error(int lib, int reason, const char *fmt, ...) {
  fprintf(stderr, "Error call\n");
//  va_list args;
//  va_start(args, fmt);
//  vfprintf(stderr, fmt, args);
//  va_end(args);
}
