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

typedef struct {
  CStr cstr;
  Sz length;
} Str8;

typedef struct {
  CBuf cbuf;
  Sz size;
} Buf8;

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

Str8 str8(CStr cstr);
Str8 str8_raw(RawPtr rptr, Sz length);
Str8 str8_clone(Arena* a, Str8 str);
Str8 str8_zero(void);
Str8 str8_join(Arena* a, Str8 str_a, Str8 str_b, char separator);
Bool str8_is_equal(Str8 a, Str8 b);
Bool is_space_char(U8 c);
Bool is_upper_char(U8 c);
Bool is_lower_char(U8 c);
Bool is_alpha_char(U8 c);
Bool is_slash_char(U8 c);
Bool is_digit_char(U8 c, U32 base);
U8 to_lower_char(U8 c);
U8 to_upper_char(U8 c);
U8 correct_slash_from_char(U8 c);

Buf8 buf8(CBuf cbuf, Sz size);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_STRING_IMPLEMENTATION

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

Str8
str8(CStr cstr) {
  Assert(cstr != 0);
  Assert(strlen(cstr) > 0);

  Str8 result = {cstr, strlen(cstr)};
  return result;
}

Str8
str8_raw(RawPtr rptr, Sz length) {
  Assert(rptr != 0);
  Assert(length > 0);

  Str8 result = {(CStr)rptr, length};
  return result;
}

Str8
str8_clone(Arena* a, Str8 str) {
  Assert(a != 0);
  Assert(str.cstr != 0);
  Assert(strlen(str.cstr) > 0);

  Str copy = arena_push(a, sizeof(I8) * (str.length + 1), AlignOf(I8), TRUE);
  MemCopy(copy, str.cstr, str.length);
  Str8 result = {copy, str.length};
  return result;
}

Str8
str8_zero(void) {
  Str8 result = {0};
  return result;
}

Str8
str8_join(Arena* a, Str8 s1, Str8 s2, char separator) {
  Assert(a != 0);
  Assert(s1.cstr != 0);
  Assert(s1.length > 0);
  Assert(s2.cstr != 0);
  Assert(s2.length > 0);
  Assert(separator != 0);

  Sz length = s1.length + s2.length + 1; /*/*/
  Str str = arena_push(a, sizeof(I8) * (length + 1 /*0*/), AlignOf(I8), TRUE);

  memcpy(str, s1.cstr, s1.length);
  str[s1.length] = separator;
  memcpy(str + s1.length + 1, s2.cstr, s2.length);

  return (Str8){str, length};
}

Bool
str8_is_equal(Str8 str_a, Str8 str_b) {
  Assert(str_a.cstr != 0);
  Assert(str_a.length > 0);
  Assert(str_b.cstr != 0);
  Assert(str_b.length > 0);

  if (str_a.length != str_b.length) {
    return FALSE;
  }

  for (U32 index = 0; index < str_a.length; index++) {
    if (str_a.cstr[index] != str_b.cstr[index]) {
      return FALSE;
    }
  }

  return TRUE;
}

Bool
is_space_char(U8 c) {
  return (c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\f' ||
          c == '\v');
}

Bool
is_upper_char(U8 c) {
  return ('A' <= c && c <= 'Z');
}

Bool
is_lower_char(U8 c) {
  return ('a' <= c && c <= 'z');
}

Bool
is_alpha_char(U8 c) {
  return (is_upper_char(c) || is_lower_char(c));
}

Bool
is_slash_char(U8 c) {
  return (c == '/' || c == '\\');
}

Bool
is_digit_char(U8 c, U32 base) {
  Bool result = FALSE;
  if (0 < base && base <= 16) {
    U8 val = integer_symbol_reverse[c];
    if (val < base) {
      result = 1;
    }
  }
  return result;
}

U8
to_lower_char(U8 c) {
  if (is_upper_char(c)) {
    c += ('a' - 'A');
  }
  return c;
}

U8
to_upper_char(U8 c) {
  if (is_lower_char(c)) {
    c += ('A' - 'a');
  }
  return c;
}

U8
correct_slash_from_char(U8 c) {
  if (is_slash_char(c)) {
    c = '/';
  }
  return c;
}

Bool
str8_cmp(Str8 str_a, Str8 str_b, StringCompareFlags flags) {
  Assert(str_a.cstr != 0);
  Assert(str_a.length > 0);
  Assert(str_b.cstr != 0);
  Assert(str_b.length > 0);

  Bool result = FALSE;

  if (str_a.length == str_b.length && flags == 0) {
    result = IsMemoryEq(str_a.cstr, str_b.cstr, str_b.length);
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
        char_a = to_upper_char(char_a);
        char_b = to_upper_char(char_b);
      }
      if (slash_insensitive) {
        char_a = correct_slash_from_char(char_a);
        char_b = correct_slash_from_char(char_b);
      }
      if (char_a != char_b) {
        result = 0;
        break;
      }
    }
  }
  return result;
}

Buf8
buf8(CBuf cbuf, Sz size) {
  Assert(cbuf != 0);
  Assert(size > 0);

  Buf8 result = {cbuf, size};
  return result;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_STRING_IMPLEMENTATION */
#endif /* SEPI_STRING_H */
