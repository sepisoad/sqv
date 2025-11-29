#ifndef SEPI_STRING_H
#define SEPI_STRING_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include "base.h"

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
  Sz size;
} Str8;

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

Str8 str8(CStr cstr);
Str8 str8_raw(RawPtr rptr, Sz size);
Str8 str8_zero(void);
Bool is_space_char(U8 c);
Bool is_upper_char(U8 c);
Bool is_lower_char(U8 c);
Bool is_alpha_char(U8 c);
Bool is_slash_char(U8 c);
Bool is_digit_char(U8 c, U32 base);
U8 to_lower_char(U8 c);
U8 to_upper_char(U8 c);
U8 correct_slash_from_char(U8 c);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_STRING_IMPLEMENTATION

internal U8 integer_symbol_reverse[128] = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};


Str8
str8(CStr cstr) {
  Str8 result = {cstr, strlen(cstr)};
  return result;
}

Str8
str8_raw(RawPtr rptr, Sz size) {
  Str8 result = {(CStr)rptr, size};
  return result;
}

Str8
str8_zero(void) {
  Str8 result = {0};
  return result;
}

Bool
is_space_char(U8 c) {
  return (c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\f'
          || c == '\v');
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
  if(0 < base && base <= 16) {
    U8 val = integer_symbol_reverse[c];
    if(val < base) {
      result = 1;
    }
  }
  return result;
}

U8
to_lower_char(U8 c) {
  if(is_upper_char(c)) {
    c += ('a' - 'A');
  }
  return c;
}

U8
to_upper_char(U8 c) {
  if(is_lower_char(c)) {
    c += ('A' - 'a');
  }
  return c;
}

U8
correct_slash_from_char(U8 c) {
  if(is_slash_char(c)) {
    c = '/';
  }
  return c;
}

Bool
str8_cmp(Str8 a, Str8 b, StringCompareFlags flags) {
  Bool result = FALSE;
  if(a.size == b.size && flags == 0) {
    result = IsMemoryEq(a.cstr, b.cstr, b.size);
  } else if(a.size == b.size || (flags & StringCompareFlag_RightSideSloppy)) {
    Bool case_insensitive = (flags & StringCompareFlag_CaseInsensitive);
    Bool slash_insensitive = (flags & StringCompareFlag_SlashInsensitive);
    U64 size = Min(a.size, b.size);
    result = 1;
    for(U64 i = 0; i < size; i += 1) {
      U8 at = a.cstr[i];
      U8 bt = b.cstr[i];
      if(case_insensitive) {
        at = to_upper_char(at);
        bt = to_upper_char(bt);
      }
      if(slash_insensitive) {
        at = correct_slash_from_char(at);
        bt = correct_slash_from_char(bt);
      }
      if(at != bt) {
        result = 0;
        break;
      }
    }
  }
  return result;
}


/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_STRING_IMPLEMENTATION */
#endif /* SEPI_STRING_H */
