#ifndef SEPI_STRING_H
#define SEPI_STRING_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include "base.h"
#include "arena.h"

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
  CStr cstr;
  Sz length;
};

typedef struct {
  CBuf cbuf;
  Sz size;
} Buf8;

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

// Str8
Str8 str8(CStr cstr);
Str8 str8_raw(RawPtr rptr, Sz length);
Str8 str8_clone(Arena* a, Str8 str);
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

SLAVE_PROFILING_CONTEXT;

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

// TODO:
// add profiling to these functions!

Str8
str8(CStr cstr) {
  START_PROFILING(1);

  Assert(cstr != 0);
  Assert(strlen(cstr) > 0);

  Str8 result = {cstr, (Sz)strlen(cstr)};

  END_PROFILING();
  return result;
}

Str8
str8_raw(RawPtr rptr, Sz length) {
  START_PROFILING(1);

  Assert(rptr != 0);
  Assert(length > 0);

  Str8 result = {(CStr)rptr, length};

  END_PROFILING();
  return result;
}

Str8
str8_clone(Arena* arena, Str8 str) {
  START_PROFILING(1);

  Assert(arena != 0);
  Assert(str.cstr != 0);
  Assert(str.length > 0);

  Str copy = arena_push(arena, sizeof(I8) * (str.length + 1), AlignOf(I8), TRUE);
  MemCopy(copy, str.cstr, str.length);
  Str8 result = {copy, str.length};

  END_PROFILING();
  return result;
}

Str8
str8_zero(void) {
  START_PROFILING(1);

  Str8 result = {0};

  END_PROFILING();
  return result;
}

Str8
str8_join(Arena* arena, Str8 s1, Str8 s2, char separator) {
  START_PROFILING(1);

  Assert(arena != 0);
  Assert(s1.cstr != 0);
  Assert(s1.length > 0);
  Assert(s2.cstr != 0);
  Assert(s2.length > 0);
  Assert(separator != 0);

  Sz length = s1.length + s2.length + 1; /*/*/
  Str str = arena_push(arena, sizeof(I8) * (length + 1 /*0*/), AlignOf(I8), TRUE);

  memcpy(str, s1.cstr, s1.length);
  str[s1.length] = separator;
  memcpy(str + s1.length + 1, s2.cstr, s2.length);


  END_PROFILING();
  return (Str8){str, length};
}

Nothing
str8_reset(Str8* ptr) {
  START_PROFILING(1);

  ptr->cstr = 0;
  ptr->length = 0;

  END_PROFILING();
}

Bool
str8_is_equal(Str8 str_a, Str8 str_b) {
  START_PROFILING(1);

  Assert(str_a.cstr != 0);
  Assert(str_a.length > 0);
  Assert(str_b.cstr != 0);
  Assert(str_b.length > 0);

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
  END_PROFILING();
  return result;
}

I32
str8_find_first(Str8 str, I8 chr) {
  START_PROFILING(1);

  Assert(str.cstr != 0);
  Assert(str.length > 0);

  I32 index = 0;
  Bool found = FALSE;

  for (; index < (I32)str.length; index++) {
    if (str.cstr[index] == chr) {
      found = TRUE;
      break;
    }
  }

  END_PROFILING();

  if (found)
    return index;
  return -1;
}

I32
str8_find_last(Str8 str, I8 chr) {
  START_PROFILING(1);

  Assert(str.cstr != 0);
  Assert(str.length > 0);

  I32 index = str.length;
  Bool found = FALSE;

  for (; index > 0; index--) {
    if (str.cstr[index] == chr) {
      found = TRUE;
      break;
    }
  }

  END_PROFILING();

  if (found)
    return index;
  return -1;
}

Bool
str8_equal(Str8 str_a, Str8 str_b, StringCompareFlags flags) {
  START_PROFILING(1);

  Assert(str_a.cstr != 0);
  Assert(str_a.length > 0);
  Assert(str_b.cstr != 0);
  Assert(str_b.length > 0);

  Bool result = FALSE;

  if (str_a.length == str_b.length && flags == 0) {
    result = MemoryEq(str_a.cstr, str_b.cstr, str_b.length);
  } else if (str_a.length == str_b.length ||
             (flags & StringCompareFlag_RightSideSloppy)) {
    Bool case_insensitive = (flags & StringCompareFlag_CaseInsensitive);
    Bool slash_insensitive = (flags & StringCompareFlag_SlashInsensitive);
    U64 length = Min(str_a.length, str_b.length);

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

  END_PROFILING();
  return result;
}

Buf8
buf8(CBuf cbuf, Sz size) {
  START_PROFILING(1);

  Assert(cbuf != 0);
  Assert(size > 0);

  Buf8 result = {cbuf, size};

  END_PROFILING();
  return result;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_STRING_IMPLEMENTATION */
#endif /* SEPI_STRING_H */
