#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  while (*fmt != '\0') {
    if (*fmt == '%') {
      fmt++;
      switch (*fmt) {
        case 'd':
          int num = va_arg(ap, int);
          char tmp[20];
          char *p = tmp;
          do {
            *p++ = '0' + num % 10;
            num /= 10;
          } while (num > 0);
          while (p > tmp) {
            *out++ = *--p;
          }
          fmt++;
          break;
        case 's':
          char *s = va_arg(ap, char *);
          while (*s != '\0') {
            *out++ = *s++;
          }
          fmt++;
          break;
        default:
          panic("Not implemented");
          break;
      }
    } else {
      *out++ = *fmt++;
    }
  }
  *out = '\0';
  return strlen(out);
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsprintf(out, fmt, ap);
  va_end(ap);
  return strlen(out);
}


int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
