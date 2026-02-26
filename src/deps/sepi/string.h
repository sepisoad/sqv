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

typedef struct Str8 Str8;
struct Str8 {
  Sz length;
  CStr cstr;
};

typedef struct {
  Sz size;
  CBuf cbuf;
} Buf8;

#define _DefineFStr8_Internal(length)                                      \
  typedef struct FL##length##_Str8 {                                       \
    char cstr[(length) + 1];                                               \
  } FL##length##_Str8;                                                     \
                                                                           \
  static inline U32 fl##length##_str8_length() {                           \
    return (length);                                                       \
  }                                                                        \
                                                                           \
  static inline FL##length##_Str8 fl##length##_str8(const char* str) {     \
    assert(str != NULL);                                                   \
    assert(strlen(str) < (length) + 1);                                    \
    struct {                                                               \
      char c[(length) + 1];                                                \
    } mutable_tmp = {0};                                                   \
    strncpy(mutable_tmp.c, str, (length));                                 \
    return *(FL##length##_Str8*)&mutable_tmp;                              \
  }                                                                        \
                                                                           \
  static inline Nothing fl##length##_str8_set(FL##length##_Str8* f,        \
                                              const char* str) {           \
    assert(f != NULL);                                                     \
    assert(str != NULL);                                                   \
    assert(strlen(str) < (length) + 1);                                    \
    zero_memory(f->cstr, (length) + 1);                                    \
    copy_memory(f->cstr, str, strlen(str));                                \
  }                                                                        \
                                                                           \
  static inline Nothing fl##length##_str8_reset(FL##length##_Str8* f) {    \
    assert(f != NULL);                                                     \
    zero_memory(f->cstr, (length) + 1);                                    \
  }                                                                        \
                                                                           \
  static inline FL##length##_Str8* fl##length##_str8_clone(                \
      Arena* arena, FL##length##_Str8 f) {                                 \
    assert(arena != NULL);                                                 \
    FL##length##_Str8* res = arena_push(arena, sizeof(FL##length##_Str8),  \
                                        alignof(FL##length##_Str8), TRUE); \
    strncpy((char*)res->cstr, f.cstr, (length));                           \
    return res;                                                            \
  }                                                                        \
                                                                           \
  static inline Str8 fl##length##_str8_view(FL##length##_Str8 f) {         \
    return str8(f.cstr);                                                   \
  }

#define DefineFStr8(length) _DefineFStr8_Internal(length)

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

// Str8
Str8 str8(CStr cstr);
Str8 str8_raw(RawPtr rptr, Sz length);
Str8 str8_clone(Arena* arena, Str8 str);
Str8 str8_slice(Arena* arena, Str8 str, U32 from, U32 to);
Str8 str8_zero(void);
Str8 str8_join(Arena* arena, Str8 str_a, Str8 str_b, char separator);
Nothing str8_reset(Str8* ptr);
Bool str8_is_equal(Str8 a, Str8 b);
I32 str8_find_first(Str8 str, I8 chr);
I32 str8_find_last(Str8 str, I8 chr);
Bool str8_equal(Str8 str_a, Str8 str_b, StringCompareFlags flags);

// Buf8
Buf8 buf8(CBuf cbuf, Sz size);

// Macros
#define CS(str) (str).cstr
#define S(cstr)                \
  _Generic((cstr),             \
      Str8: (cstr),            \
      Str: str8((CStr)(cstr)), \
      CStr: str8((cstr)),      \
      default: str8((CStr)(cstr)))
#define SL(str) (str).length

#define IsWhiteSpaceChar(c)                                                  \
  ((c) == ' ' || (c) == '\n' || (c) == '\t' || (c) == '\r' || (c) == '\f' || \
   (c) == '\v')

#define IsUpperCaseChar(c) ('A' <= (c) && (c) <= 'Z')

#define IsLowerCaseChar(c) ('a' <= (c) && (c) <= 'z')

#define is_alpha_char(c) (IsUpperCaseChar((c)) || IsLowerCaseChar((c)))

#define IsSlashChar(c) ((c) == '/' || (c) == '\\')

#define IsDigitChar(c) ('0' <= (c) && (c) <= '9')

#define ToLowerChar(c) (IsUpperCaseChar(c) ? ((c) + ('a' - 'A')) : (c))

#define ToUpperChar(c) (IsLowerCaseChar(c) ? ((c) + ('A' - 'a')) : (c))

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

Str8
str8(CStr cstr) {
  start_profiling(1);

  assert(cstr != 0);
  // assert(strlen(cstr) > 0);

  Str8 result = {.cstr = cstr, .length = (Sz)strlen(cstr)};

  end_profiling();
  return result;
}

Str8
str8_raw(RawPtr rptr, Sz length) {
  start_profiling(1);

  assert(rptr != 0);
  // assert(length > 0);

  Str8 result = {.cstr = (CStr)rptr, .length = length};

  end_profiling();
  return result;
}

Str8
str8_clone(Arena* arena, Str8 str) {
  start_profiling(1);

  assert(arena != 0);
  assert(str.cstr != 0);
  // assert(str.length > 0);

  Str copy =
      arena_push(arena, sizeof(I8) * (str.length + 1), alignof(I8), TRUE);
  copy_memory(copy, str.cstr, str.length);
  Str8 result = {.cstr = copy, .length = str.length};

  end_profiling();
  return result;
}

Str8
str8_slice(Arena* arena, Str8 str, U32 from, U32 to) {
  start_profiling(1);

  assert(arena != 0);
  assert(str.cstr != 0);
  assert(str.length > 0);
  assert(from <= to);
  assert(from <= str.length);
  assert(to <= str.length);

  Sz new_length = to - from;
  Str copy =
      arena_push(arena, sizeof(I8) * (new_length + 1), alignof(I8), TRUE);

  copy_memory(copy, str.cstr + from, new_length);
  Str8 result = {.cstr = copy, .length = new_length};

  end_profiling();
  return result;
}

Str8
str8_zero(void) {
  start_profiling(1);

  Str8 result = {0};

  end_profiling();
  return result;
}

Str8
str8_join(Arena* arena, Str8 s1, Str8 s2, char separator) {
  start_profiling(1);

  assert(arena != 0);
  assert(s1.cstr != 0);
  assert(s1.length > 0);
  assert(s2.cstr != 0);
  assert(s2.length > 0);
  assert(separator != 0);

  Sz length = s1.length + s2.length + 1; /*/*/
  Str str =
      arena_push(arena, sizeof(I8) * (length + 1 /*0*/), alignof(I8), TRUE);

  memcpy(str, s1.cstr, s1.length);
  str[s1.length] = separator;
  memcpy(str + s1.length + 1, s2.cstr, s2.length);

  end_profiling();
  return (Str8){.cstr = str, .length = length};
}

Nothing
str8_reset(Str8* ptr) {
  start_profiling(1);

  zero_memory((RawPtr)ptr->cstr, ptr->length);
  ptr->length = 0;

  end_profiling();
}

Bool
str8_is_equal(Str8 str_a, Str8 str_b) {
  start_profiling(1);

  assert(str_a.cstr != 0);
  assert(str_a.length > 0);
  assert(str_b.cstr != 0);
  assert(str_b.length > 0);

  Bool result = TRUE;

  if (str_a.length != str_b.length) {
    result = FALSE;
    goto cleanup;
  }

  for (U32 index = 0; index < str_a.length; index++) {
    if (str_a.cstr[index] != str_b.cstr[index]) {
      result = FALSE;
      goto cleanup;
    }
  }

cleanup:
  end_profiling();
  return result;
}

I32
str8_find_first(Str8 str, I8 chr) {
  start_profiling(1);

  assert(str.cstr != 0);
  assert(str.length > 0);

  I32 index = 0;
  Bool found = FALSE;

  for (; index < (I32)str.length; index++) {
    if (str.cstr[index] == chr) {
      found = TRUE;
      break;
    }
  }

  end_profiling();

  if (found)
    return index;
  return -1;
}

I32
str8_find_last(Str8 str, I8 chr) {
  start_profiling(1);

  assert(str.cstr != 0);
  assert(str.length > 0);

  I32 index = str.length;
  Bool found = FALSE;

  for (; index > 0; index--) {
    if (str.cstr[index] == chr) {
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
str8_equal(Str8 str_a, Str8 str_b, StringCompareFlags flags) {
  start_profiling(1);

  assert(str_a.cstr != 0);
  assert(str_a.length > 0);
  assert(str_b.cstr != 0);
  assert(str_b.length > 0);

  Bool result = FALSE;

  if (str_a.length == str_b.length && flags == 0) {
    result = is_memory_equal(str_a.cstr, str_b.cstr, str_b.length);
  } else if (str_a.length == str_b.length ||
             (flags & StringCompareFlag_RightSideSloppy)) {
    Bool case_insensitive = (flags & StringCompareFlag_CaseInsensitive);
    Bool slash_insensitive = (flags & StringCompareFlag_SlashInsensitive);
    U64 length = min(str_a.length, str_b.length);

    result = 1;
    for (U64 i = 0; i < length; i += 1) {
      U8 char_a = str_a.cstr[i];
      U8 char_b = str_b.cstr[i];
      if (case_insensitive) {
        char_a = ToUpperChar(char_a);
        char_b = ToUpperChar(char_b);
      }
      if (slash_insensitive) {
        char_a = IsSlashChar(char_a) ? '/' : char_a;
        char_b = IsSlashChar(char_b) ? '/' : char_b;
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

Buf8
buf8(CBuf cbuf, Sz size) {
  start_profiling(1);

  assert(cbuf != 0);
  assert(size > 0);

  Buf8 result = {.cbuf = cbuf, .size = size};

  end_profiling();
  return result;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_STRING_IMPLEMENTATION */
#endif /* SEPI_STRING_H */
