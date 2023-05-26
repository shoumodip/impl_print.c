#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef void (*ShowFn)(FILE *, void *);

typedef struct {
  size_t size;
  const char *data;
} Str;

Str str_new(const char *data) {
  return (Str){.data = data, .size = strlen(data)};
}

bool str_eq(Str a, Str b) {
  return a.size == b.size && !memcmp(a.data, b.data, b.size);
}

Str str_part(Str str, size_t from, size_t size) {
  return (Str){.data = str.data + from, .size = size};
}

long str_find(Str str, char ch, size_t from) {
  for (size_t i = from; i < str.size; ++i) {
    if (str.data[i] == ch) {
      return i - from;
    }
  }
  return -1;
}

typedef struct {
  Str name;
  ShowFn fn;
  size_t size;
} ShowImpl;

#define IMPLS_CAP 1024
ShowImpl impls[IMPLS_CAP];
size_t impls_count;

void impls_push(Str name, ShowFn fn, size_t size) {
  assert(size == 0 || size == 4 || size == 8);
  assert(impls_count < IMPLS_CAP);
  impls[impls_count++] = (ShowImpl){.name = name, .fn = fn, .size = size};
}

ShowImpl *impls_find(Str name) {
  for (size_t i = 0; i < impls_count; ++i) {
    if (str_eq(impls[i].name, name)) {
      return &impls[i];
    }
  }
  return NULL;
}

void print_shows(FILE *file, Str format, ...) {
  va_list args;
  va_start(args, format);

  for (size_t i = 0; i < format.size; ++i) {
    if (format.data[i] == '{') {
      long end = str_find(format, '}', i + 1);
      if (end > 0) {
        Str name = str_part(format, i + 1, end);
        ShowImpl *impl = impls_find(name);
        if (impl) {
          switch (impl->size) {
          case 0:
            fprintf(file, "%g", va_arg(args, double));
            break;

          case 4:
            impl->fn(file, (void *)(long)va_arg(args, int));
            break;

          case 8:
            impl->fn(file, va_arg(args, void *));
            break;

          default:
            assert(false && "unreachable");
          }
          i += end + 1;
          continue;
        }
      }
    }

    fputc(format.data[i], file);
  }

  va_end(args);
}

void show_impl_int(FILE *file, void *data) {
  fprintf(file, "%d", (int)(long)data);
}

void show_impl_char(FILE *file, void *data) {
  fprintf(file, "%c", (char)(long)data);
}

void show_impl_cstr(FILE *file, void *data) {
  fprintf(file, "%s", (char *)data);
}

void show_impl_bool(FILE *file, void *data) {
  if (data) {
    fprintf(file, "true");
  } else {
    fprintf(file, "false");
  }
}

void show_impl_str(FILE *file, void *data) {
  Str *str = data;
  fwrite(str->data, str->size, 1, file);
}

int main(void) {
  impls_push(str_new("int"), show_impl_int, 4);
  impls_push(str_new("str"), show_impl_str, 8);

  impls_push(str_new("bool"), show_impl_bool, 4);
  impls_push(str_new("char"), show_impl_char, 4);
  impls_push(str_new("cstr"), show_impl_cstr, 8);

  impls_push(str_new("float"), NULL, 0);
  impls_push(str_new("double"), NULL, 0);

  print_shows(stdout, str_new("Hello, {cstr}! {float}\n"), "world", 420.69);
}
