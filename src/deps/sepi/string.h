#ifndef SEPI_STRING_H
#define SEPI_STRING_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <wchar.h>

#include <tracy/tracy.h>

#include <sepi/base.h>
#include <sepi/arena.h>

/* ===================================================== */
/*                  FORWARD DECLERATION                  */
/* ===================================================== */

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

static U8 str_utf8_class[32] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 3, 3, 4, 5,
};

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef U32 StrCmpFlags;
enum {
  STR_CMP_CASE_INSENSITIVE = (1 << 0),
  STR_CMP_CASE_RIGHT_SIDE_SLOPPY = (1 << 1),
  STR_CMP_CASE_SLASH_INSENSITIVE = (1 << 2),
};

typedef struct StrUnicodeDecode StrUnicodeDecode;
struct StrUnicodeDecode {
  U32 inc;
  U32 codepoint;
};

typedef struct Str Str;
struct Str {
  U8* zstr;
  Sz length;
};

typedef struct WStr WStr;
struct WStr {
  U16* zstr;
  Sz length;
};

typedef struct UStr UStr;
struct UStr {
  U32* zstr;
  Sz length;
};

#define _DefineFStr_Internal(length)                                        \
  typedef struct Str##length {                                              \
    U8 zstr[(length) + 1];                                                  \
  } Str##length;                                                            \
                                                                            \
  embed fn U32 str##length##_length() {                                \
    return (length);                                                        \
  }                                                                         \
                                                                            \
  embed fn Str##length str##length(U8* str) {                          \
    assert(str != NULL);                                                    \
    assert(str_length(str) < (length) + 1);                                 \
    struct {                                                                \
      U8 c[(length) + 1];                                                   \
    } mutable_tmp = {0};                                                    \
    copy_memory(mutable_tmp.c, str, (length));                              \
    return *(Str##length*)&mutable_tmp;                                     \
  }                                                                         \
                                                                            \
  embed fn Nothing str##length##_set(Str##length* f, const U8* str) {  \
    assert(f != NULL);                                                      \
    assert(str != NULL);                                                    \
    assert(str_length(str) < (length) + 1);                                 \
    zero_memory(f->zstr, (length) + 1);                                     \
    copy_memory(f->zstr, str, str_length(str));                             \
  }                                                                         \
                                                                            \
  embed fn Nothing str##length##_reset(Str##length* f) {               \
    assert(f != NULL);                                                      \
    zero_memory(f->zstr, (length) + 1);                                     \
  }                                                                         \
                                                                            \
  embed fn Str##length* str##length##_clone(Arena* arena,              \
                                                 Str##length f) {           \
    assert(arena != NULL);                                                  \
    Str##length* res =                                                      \
        arena_push(arena, sizeof(Str##length), alignof(Str##length), TRUE); \
    copy_memory((U8*)res->zstr, f.zstr, (length));                          \
    return res;                                                             \
  }                                                                         \
                                                                            \
  embed fn Str str##length##_view(Str##length f) {                     \
    return str(f.zstr);                                                     \
  }

#define DefineFStr(length) _DefineFStr_Internal(length)

#define ZS(str) ((char*)(str).zstr)

#define S(zstr) str((U8*)(zstr))

#define SL(str) (str).length

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

fn Str str(U8* zstr);
fn WStr wstr(U16* zstr);
fn UStr ustr(U32* zstr);

fn Str str_from_wstr(Arena* arena, WStr in);
fn Str str_from_ustr(Arena* arena, UStr in);
fn WStr wstr_from_str(Arena* arena, Str in);
fn UStr ustr_from_str(Arena* arena, Str in);

fn U32 str_encode(U8* str, U32 codepoint);
fn U32 wstr_encode(U16* str, U32 codepoint);

fn StrUnicodeDecode str_decode(U8* str, U64 max);
fn StrUnicodeDecode wstr_decode(U16* str, U64 max);

fn Sz str_length(const U8* zstr);
fn Sz wstr_length(const U16* zstr);
fn Sz ustr_length(const U32* zstr);

fn Str str_raw(RawPtr rptr, Sz length);
fn WStr wstr_raw(RawPtr rptr, Sz length);
fn UStr ustr_raw(RawPtr rptr, Sz length);

fn Str str_clone(Arena* arena, Str str);
fn Str str_slice(Arena* arena, Str str, Sz from, Sz to);
fn Str str_join(Arena* arena, Str str_a, Str str_b, U8 separator);
fn Nothing str_reset(Str* ptr);
fn Bool str_is_equal(Str a, Str b);
fn I64 str_find_first(Str str, I8 chr);
fn I64 str_find_last(Str str, I8 chr);
fn Bool str_equal(Str str_a, Str str_b, StrCmpFlags flags);

Nothing str_clean(Str str);

static U16
safe_cast_u16(U32 x) {
  runtime_assert(x <= MAX_U16);
  U16 result = (U16)x;
  return result;
}

// static U32
// safe_cast_u32(U64 x) {
//   runtime_assert(x <= MAX_U32);
//   U32 result = (U32)x;
//   return result;
// }

// static I32
// safe_cast_s32(I64 x) {
//   runtime_assert(x <= MAX_I32);
//   I32 result = (I32)x;
//   return result;
// }

embed fn Bool
is_white_space_char(U8 c) {
  return (c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\f' ||
          c == '\v');
}

embed fn Bool
is_upper_case_char(U8 c) {
  return ('A' <= c && c <= 'Z');
}

embed fn Bool
is_lower_case_char(U8 c) {
  return ('a' <= c && c <= 'z');
}

embed fn Bool
is_alpha_char(U8 c) {
  return is_upper_case_char(c) || is_lower_case_char(c);
}

embed fn Bool
is_slash_char(U8 c) {
  return (c == '/' || c == '\\');
}

embed fn Bool
is_digit_char(U8 c) {
  return ('0' <= (c) && (c) <= '9');
}

embed fn U8
to_lower_char(U8 c) {
  return is_upper_case_char(c) ? c + 32 : c;
}

embed fn U8
to_upper_char(U8 c) {
  return is_lower_case_char(c) ? c - 32 : c;
}

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_STRING_IMPLEMENTATION

mount_slave_profiling_context();

fn Str
str(U8* zstr) {
  start_profiling(1);

  assert(zstr != 0);
  // assert(strlen(zstr) > 0);

  Str result = {.zstr = zstr, .length = (Sz)str_length(zstr)};

  end_profiling();
  return result;
}

fn WStr
wstr(U16* zstr) {
  start_profiling(1);

  assert(zstr != 0);
  // assert(strlen(zstr) > 0);

  WStr result = {.zstr = zstr, .length = (Sz)wstr_length(zstr)};

  end_profiling();
  return result;
}

fn UStr
ustr(U32* zstr) {
  start_profiling(1);

  assert(zstr != 0);
  // assert(strlen(zstr) > 0);

  UStr result = {.zstr = zstr, .length = (Sz)ustr_length(zstr)};

  end_profiling();
  return result;
}

fn Str
str_from_wstr(Arena* arena, WStr in) {
  Str result = {0};
  if (in.length) {
    U64 cap = in.length * 3;
    U8* str = arena_push_array_0_init(arena, U8, cap + 1);
    U16* ptr = in.zstr;
    U16* opl = ptr + in.length;
    U64 size = 0;
    StrUnicodeDecode consume;
    for (; ptr < opl; ptr += consume.inc) {
      consume = wstr_decode(ptr, opl - ptr);
      size += str_encode(str + size, consume.codepoint);
    }
    str[size] = 0;
    arena_pop(arena, (cap - size));
    result = str_raw(str, size);
  }
  return result;
}

fn WStr
wstr_from_str(Arena* arena, Str in) {
  WStr result = {0};
  if (in.length) {
    U64 cap = in.length * 2;
    U16* str = arena_push_array_0_init(arena, U16, cap + 1);
    U8* ptr = in.zstr;
    U8* opl = ptr + in.length;
    U64 size = 0;
    StrUnicodeDecode consume;
    for (; ptr < opl; ptr += consume.inc) {
      consume = str_decode(ptr, opl - ptr);
      size += wstr_encode(str + size, consume.codepoint);
    }
    str[size] = 0;
    arena_pop(arena, (cap - size) * 2);
    result = wstr_raw(str, size);
  }
  return result;
}

fn Str
str_from_ustr(Arena* arena, UStr in) {
  Str result = {0};
  if (in.length) {
    U64 cap = in.length * 4;
    U8* str = arena_push_array_0_init(arena, U8, cap + 1);
    U32* ptr = in.zstr;
    U32* opl = ptr + in.length;
    U64 size = 0;
    for (; ptr < opl; ptr += 1) {
      size += str_encode(str + size, *ptr);
    }
    str[size] = 0;
    arena_pop(arena, (cap - size));
    result = str_raw(str, size);
  }
  return result;
}

fn UStr
ustr_from_str(Arena* arena, Str in) {
  UStr result = {0};
  if (in.length) {
    U64 cap = in.length;
    U32* str = arena_push_array_0_init(arena, U32, cap + 1);
    U8* ptr = in.zstr;
    U8* opl = ptr + in.length;
    U64 size = 0;
    StrUnicodeDecode consume;
    for (; ptr < opl; ptr += consume.inc) {
      consume = str_decode(ptr, opl - ptr);
      str[size] = consume.codepoint;
      size += 1;
    }
    str[size] = 0;
    arena_pop(arena, (cap - size) * 4);
    result = ustr_raw(str, size);
  }
  return result;
}

fn U32
str_encode(U8* str, U32 codepoint) {
  U32 inc = 0;
  if (codepoint <= 0x7F) {
    str[0] = (U8)codepoint;
    inc = 1;
  } else if (codepoint <= 0x7FF) {
    str[0] = (0x00000003 << 6) | ((codepoint >> 6) & 0x0000001f);
    str[1] = (1 << 7) | (codepoint & 0x0000003f);
    inc = 2;
  } else if (codepoint <= 0xFFFF) {
    str[0] = (0x00000007 << 5) | ((codepoint >> 12) & 0x0000000f);
    str[1] = (1 << 7) | ((codepoint >> 6) & 0x0000003f);
    str[2] = (1 << 7) | (codepoint & 0x0000003f);
    inc = 3;
  } else if (codepoint <= 0x10FFFF) {
    str[0] = (0x0000000f << 4) | ((codepoint >> 18) & 0x00000007);
    str[1] = (1 << 7) | ((codepoint >> 12) & 0x0000003f);
    str[2] = (1 << 7) | ((codepoint >> 6) & 0x0000003f);
    str[3] = (1 << 7) | (codepoint & 0x0000003f);
    inc = 4;
  } else {
    str[0] = '?';
    inc = 1;
  }
  return inc;
}

fn U32
wstr_encode(U16* str, U32 codepoint) {
  U32 inc = 1;
  if (codepoint == MAX_U32) {
    str[0] = (U16)'?';
  } else if (codepoint < 0x10000) {
    str[0] = (U16)codepoint;
  } else {
    U32 v = codepoint - 0x10000;
    str[0] = safe_cast_u16(0xD800 + (v >> 10));
    str[1] = safe_cast_u16(0xDC00 + (v & 0x000003ff));
    inc = 2;
  }
  return inc;
}

fn StrUnicodeDecode
str_decode(U8* str, U64 max) {
  StrUnicodeDecode result = {1, MAX_U32};
  U8 byte = str[0];
  U8 byte_class = str_utf8_class[byte >> 3];
  switch (byte_class) {
    case 1: {
      result.codepoint = byte;
    } break;
    case 2: {
      if (1 < max) {
        U8 cont_byte = str[1];
        if (str_utf8_class[cont_byte >> 3] == 0) {
          result.codepoint = (byte & 0x0000001f) << 6;
          result.codepoint |= (cont_byte & 0x0000003f);
          result.inc = 2;
        }
      }
    } break;
    case 3: {
      if (2 < max) {
        U8 cont_byte[2] = {str[1], str[2]};
        if (str_utf8_class[cont_byte[0] >> 3] == 0 &&
            str_utf8_class[cont_byte[1] >> 3] == 0) {
          result.codepoint = (byte & 0x0000000f) << 12;
          result.codepoint |= ((cont_byte[0] & 0x0000003f) << 6);
          result.codepoint |= (cont_byte[1] & 0x0000003f);
          result.inc = 3;
        }
      }
    } break;
    case 4: {
      if (3 < max) {
        U8 cont_byte[3] = {str[1], str[2], str[3]};
        if (str_utf8_class[cont_byte[0] >> 3] == 0 &&
            str_utf8_class[cont_byte[1] >> 3] == 0 &&
            str_utf8_class[cont_byte[2] >> 3] == 0) {
          result.codepoint = (byte & 0x00000007) << 18;
          result.codepoint |= ((cont_byte[0] & 0x0000003f) << 12);
          result.codepoint |= ((cont_byte[1] & 0x0000003f) << 6);
          result.codepoint |= (cont_byte[2] & 0x0000003f);
          result.inc = 4;
        }
      }
    }
  }
  return result;
}

fn StrUnicodeDecode
wstr_decode(U16* str, U64 max) {
  StrUnicodeDecode result = {1, MAX_U32};
  result.codepoint = str[0];
  result.inc = 1;
  if (max > 1 && 0xD800 <= str[0] && str[0] < 0xDC00 && 0xDC00 <= str[1] &&
      str[1] < 0xE000) {
    result.codepoint =
        ((str[0] - 0xD800) << 10) | ((str[1] - 0xDC00) + 0x10000);
    result.inc = 2;
  }
  return result;
}

fn Sz
str_length(const U8* zstr) {
  start_profiling(1);

  const U8* start = zstr;
  for (; *zstr; zstr++)
    ;
  Sz length = (Sz)(zstr - start);

  end_profiling();
  return length;
}

fn Sz
wstr_length(const U16* zstr) {
  start_profiling(1);

  const U16* start = zstr;
  for (; *zstr; zstr++)
    ;
  Sz length = (Sz)(zstr - start);

  end_profiling();
  return length;
}

fn Sz
ustr_length(const U32* zstr) {
  start_profiling(1);

  const U32* start = zstr;
  for (; *zstr; zstr++)
    ;
  Sz length = (Sz)(zstr - start);

  end_profiling();
  return length;
}

fn Str
str_raw(RawPtr rptr, Sz length) {
  start_profiling(1);

  assert(rptr != 0);

  Str result = {.zstr = (U8*)rptr, .length = length};

  end_profiling();
  return result;
}

fn WStr
wstr_raw(RawPtr rptr, Sz length) {
  start_profiling(1);

  assert(rptr != 0);

  WStr result = {.zstr = (U16*)rptr, .length = length};

  end_profiling();
  return result;
}

fn UStr
ustr_raw(RawPtr rptr, Sz length) {
  start_profiling(1);

  assert(rptr != 0);

  UStr result = {.zstr = (U32*)rptr, .length = length};

  end_profiling();
  return result;
}

fn Str
str_clone(Arena* arena, Str str) {
  start_profiling(1);

  assert(arena != 0);
  assert(str.zstr != 0);

  U8* copy =
      arena_push(arena, sizeof(I8) * (str.length + 1), alignof(I8), TRUE);
  copy_memory(copy, str.zstr, str.length);
  Str result = {.zstr = copy, .length = str.length};

  end_profiling();
  return result;
}

fn Str
str_slice(Arena* arena, Str str, Sz from, Sz to) {
  start_profiling(1);

  assert(arena != 0);
  assert(str.zstr != 0);
  assert(str.length > 0);
  assert(from <= to);
  assert(from <= str.length);
  assert(to <= str.length);

  Sz new_length = to - from;
  U8* copy =
      arena_push(arena, sizeof(I8) * (new_length + 1), alignof(I8), TRUE);

  copy_memory(copy, str.zstr + from, new_length);
  Str result = {.zstr = copy, .length = new_length};

  end_profiling();
  return result;
}

fn Str
str_join(Arena* arena, Str s1, Str s2, U8 separator) {
  start_profiling(1);

  assert(arena != 0);
  assert(s1.zstr != 0);
  assert(s1.length > 0);
  assert(s2.zstr != 0);
  assert(s2.length > 0);
  assert(separator != 0);

  Sz length = s1.length + s2.length + 1; /*/*/
  U8* str =
      arena_push(arena, sizeof(I8) * (length + 1 /*0*/), alignof(I8), TRUE);

  memcpy(str, s1.zstr, s1.length);
  str[s1.length] = separator;
  memcpy(str + s1.length + 1, s2.zstr, s2.length);

  end_profiling();
  return (Str){.zstr = str, .length = length};
}

fn Nothing
str_reset(Str* ptr) {
  start_profiling(1);

  zero_memory((RawPtr)ptr->zstr, ptr->length);
  ptr->length = 0;

  end_profiling();
}

fn Bool
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

fn I64
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

fn I64
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

fn Bool
str_equal(Str str_a, Str str_b, StrCmpFlags flags) {
  start_profiling(1);

  assert(str_a.zstr != 0);
  assert(str_a.length > 0);
  assert(str_b.zstr != 0);
  assert(str_b.length > 0);

  Bool result = FALSE;

  if (str_a.length == str_b.length && flags == 0) {
    result = is_memory_equal(str_a.zstr, str_b.zstr, str_b.length);
  } else if (str_a.length == str_b.length ||
             (flags & STR_CMP_CASE_RIGHT_SIDE_SLOPPY)) {
    Bool case_insensitive = (flags & STR_CMP_CASE_INSENSITIVE);
    Bool slash_insensitive = (flags & STR_CMP_CASE_SLASH_INSENSITIVE);
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

fn Nothing
str_clean(Str str) {
  str.zstr = 0;
  str.length = 0;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_STRING_IMPLEMENTATION */
#endif /* SEPI_STRING_H */
