#ifndef SEPI_STRING_H
#define SEPI_STRING_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <sepi/base.h>
#include <sepi/arena.h>

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef U32 StringCompareFlags;
enum {
  StringCompareFlag_CaseInsensitive = (1 << 0),
  StringCompareFlag_RightSideSloppy = (1 << 1),
  StringCompareFlag_SlashInsensitive = (1 << 2),
};

typedef struct Str Str;
struct Str {
  Sz length;
  const char* zstr;
};

typedef struct WStr WStr;
struct WStr {
  Sz length;
  const U16* zstr;
};

typedef struct UStr UStr;
struct UStr {
  Sz length;
  const U32* zstr;
};

#define _DefineFStr_Internal(length)                                         \
  typedef struct Str##length {                                               \
    char zstr[(length) + 1];                                                 \
  } Str##length;                                                             \
                                                                             \
  static inline U32 str##length##_length() {                                 \
    return (length);                                                         \
  }                                                                          \
                                                                             \
  static inline Str##length str##length(const char* str) {                   \
    assert(str != NULL);                                                     \
    assert(strlen(str) < (length) + 1);                                      \
    struct {                                                                 \
      char c[(length) + 1];                                                  \
    } mutable_tmp = {0};                                                     \
    strncpy(mutable_tmp.c, str, (length));                                   \
    return *(Str##length*)&mutable_tmp;                                      \
  }                                                                          \
                                                                             \
  static inline Nothing str##length##_set(Str##length* f, const char* str) { \
    assert(f != NULL);                                                       \
    assert(str != NULL);                                                     \
    assert(strlen(str) < (length) + 1);                                      \
    zero_memory(f->zstr, (length) + 1);                                      \
    copy_memory(f->zstr, str, strlen(str));                                  \
  }                                                                          \
                                                                             \
  static inline Nothing str##length##_reset(Str##length* f) {                \
    assert(f != NULL);                                                       \
    zero_memory(f->zstr, (length) + 1);                                      \
  }                                                                          \
                                                                             \
  static inline Str##length* str##length##_clone(Arena* arena,               \
                                                 Str##length f) {            \
    assert(arena != NULL);                                                   \
    Str##length* res =                                                       \
        arena_push(arena, sizeof(Str##length), alignof(Str##length), TRUE);  \
    strncpy((char*)res->zstr, f.zstr, (length));                             \
    return res;                                                              \
  }                                                                          \
                                                                             \
  static inline Str str##length##_view(Str##length f) {                      \
    return str(f.zstr);                                                      \
  }

#define DefineFStr(length) _DefineFStr_Internal(length)

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

// Str
Str str(const char* zstr);
Str str_raw(RawPtr rptr, Sz length);
Str str_clone(Arena* arena, Str str);
Str str_slice(Arena* arena, Str str, Sz from, Sz to);
Str str_zero(void);
Str str_join(Arena* arena, Str str_a, Str str_b, char separator);
Nothing str_reset(Str* ptr);
Bool str_is_equal(Str a, Str b);
I64 str_find_first(Str str, I8 chr);
I64 str_find_last(Str str, I8 chr);
Bool str_equal(Str str_a, Str str_b, StringCompareFlags flags);

// Buf
Buf buf(const U8* cbuf, Sz size);

// Macros
#define ZS(str) (str).zstr
#define S(zstr)                 \
  _Generic((zstr),              \
      Str: (zstr),              \
      ZStr: str((CZStr)(zstr)), \
      CZStr: str((zstr)),       \
      default: str((CZStr)(zstr)))
#define SL(str) (str).length

/////

static inline Bool
is_white_space_char(U8 c) {
  return (c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\f' ||
          c == '\v');
}

static inline Bool
is_upper_case_char(U8 c) {
  return ('A' <= c && c <= 'Z');
}

static inline Bool
is_lower_case_char(U8 c) {
  return ('a' <= c && c <= 'z');
}

static inline Bool
is_alpha_char(U8 c) {
  return is_upper_case_char(c) || is_lower_case_char(c);
}

static inline Bool
is_slash_char(U8 c) {
  return (c == '/' || c == '\\');
}

static inline Bool
is_digit_char(U8 c) {
  return ('0' <= (c) && (c) <= '9');
}

static inline U8
to_lower_char(U8 c) {
  return is_upper_case_char(c) ? c + 32 : c;
}

static inline U8
to_upper_char(U8 c) {
  return is_lower_case_char(c) ? c - 32 : c;
}

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_STRING_IMPLEMENTATION

mount_slave_profiling_context();

static U8 integer_symbol_reverse[128] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

// TODO: turn all these functions into inline function

Str
str(const char* zstr) {
  start_profiling(1);

  assert(zstr != 0);
  // assert(strlen(zstr) > 0);

  Str result = {.zstr = zstr, .length = (Sz)strlen(zstr)};

  end_profiling();
  return result;
}

Str
str_raw(RawPtr rptr, Sz length) {
  start_profiling(1);

  assert(rptr != 0);
  // assert(length > 0);

  Str result = {.zstr = (const char*)rptr, .length = length};

  end_profiling();
  return result;
}

Str
str_clone(Arena* arena, Str str) {
  start_profiling(1);

  assert(arena != 0);
  assert(str.zstr != 0);
  // assert(str.length > 0);

  char* copy =
      arena_push(arena, sizeof(I8) * (str.length + 1), alignof(I8), TRUE);
  copy_memory(copy, str.zstr, str.length);
  Str result = {.zstr = copy, .length = str.length};

  end_profiling();
  return result;
}

Str
str_slice(Arena* arena, Str str, Sz from, Sz to) {
  start_profiling(1);

  assert(arena != 0);
  assert(str.zstr != 0);
  assert(str.length > 0);
  assert(from <= to);
  assert(from <= str.length);
  assert(to <= str.length);

  Sz new_length = to - from;
  char* copy =
      arena_push(arena, sizeof(I8) * (new_length + 1), alignof(I8), TRUE);

  copy_memory(copy, str.zstr + from, new_length);
  Str result = {.zstr = copy, .length = new_length};

  end_profiling();
  return result;
}

Str
str_zero(void) {
  start_profiling(1);

  Str result = {0};

  end_profiling();
  return result;
}

Str
str_join(Arena* arena, Str s1, Str s2, char separator) {
  start_profiling(1);

  assert(arena != 0);
  assert(s1.zstr != 0);
  assert(s1.length > 0);
  assert(s2.zstr != 0);
  assert(s2.length > 0);
  assert(separator != 0);

  Sz length = s1.length + s2.length + 1; /*/*/
  char* str =
      arena_push(arena, sizeof(I8) * (length + 1 /*0*/), alignof(I8), TRUE);

  memcpy(str, s1.zstr, s1.length);
  str[s1.length] = separator;
  memcpy(str + s1.length + 1, s2.zstr, s2.length);

  end_profiling();
  return (Str){.zstr = str, .length = length};
}

Nothing
str_reset(Str* ptr) {
  start_profiling(1);

  zero_memory((RawPtr)ptr->zstr, ptr->length);
  ptr->length = 0;

  end_profiling();
}

Bool
str_is_equal(Str str_a, Str str_b) {
  start_profiling(1);

  assert(str_a.zstr != 0);
  assert(str_a.length > 0);
  assert(str_b.zstr != 0);
  assert(str_b.length > 0);

  Bool result = TRUE;

  if (str_a.length != str_b.length) {
    result = FALSE;
    goto cleanup;
  }

  for (U32 index = 0; index < str_a.length; index++) {
    if (str_a.zstr[index] != str_b.zstr[index]) {
      result = FALSE;
      goto cleanup;
    }
  }

cleanup:
  end_profiling();
  return result;
}

I64
str_find_first(Str str, I8 chr) {
  start_profiling(1);

  assert(str.zstr != 0);
  assert(str.length > 0);

  I64 index = 0;
  Bool found = FALSE;

  for (; index < (I32)str.length; index++) {
    if (str.zstr[index] == chr) {
      found = TRUE;
      break;
    }
  }

  end_profiling();

  if (found)
    return index;
  return -1;
}

I64
str_find_last(Str str, I8 chr) {
  start_profiling(1);

  assert(str.zstr != 0);
  assert(str.length > 0);

  I64 index = str.length;
  Bool found = FALSE;

  for (; index > 0; index--) {
    if (str.zstr[index] == chr) {
      found = TRUE;
      break;
    }
  }

  end_profiling();

  if (found)
    return index;
  return -1;
}

Bool
str_equal(Str str_a, Str str_b, StringCompareFlags flags) {
  start_profiling(1);

  assert(str_a.zstr != 0);
  assert(str_a.length > 0);
  assert(str_b.zstr != 0);
  assert(str_b.length > 0);

  Bool result = FALSE;

  if (str_a.length == str_b.length && flags == 0) {
    result = is_memory_equal(str_a.zstr, str_b.zstr, str_b.length);
  } else if (str_a.length == str_b.length ||
             (flags & StringCompareFlag_RightSideSloppy)) {
    Bool case_insensitive = (flags & StringCompareFlag_CaseInsensitive);
    Bool slash_insensitive = (flags & StringCompareFlag_SlashInsensitive);
    U64 length = min(str_a.length, str_b.length);

    result = 1;
    for (U64 i = 0; i < length; i += 1) {
      U8 char_a = str_a.zstr[i];
      U8 char_b = str_b.zstr[i];
      if (case_insensitive) {
        char_a = to_upper_char(char_a);
        char_b = to_upper_char(char_b);
      }
      if (slash_insensitive) {
        char_a = is_slash_char(char_a) ? '/' : char_a;
        char_b = is_slash_char(char_b) ? '/' : char_b;
      }
      if (char_a != char_b) {
        result = 0;
        break;
      }
    }
  }

  end_profiling();
  return result;
}

Buf
buf(const U8* cbuf, Sz size) {
  start_profiling(1);

  assert(cbuf != 0);
  assert(size > 0);

  Buf result = {.cbuf = cbuf, .size = size};

  end_profiling();
  return result;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_STRING_IMPLEMENTATION */
#endif /* SEPI_STRING_H */
