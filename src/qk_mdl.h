#ifndef QK_MDL_HEADER_
#define QK_MDL_HEADER_

#include <stdint.h>

typedef float qk_vectorf;

typedef qk_vectorf qk_vector3f[3];

typedef enum {
  QK_ST__UNKNOWN = -1,

  QK_ST_SYNC = 0,
  QK_ST_RAND,
  QK_ST_FRAMETIME,

  QK_ST__MAX = 255,
} qk_synctype;

typedef enum {
  QK_TEXFLG__UNKNOWN = -1,

  QK_TEXFLG_NONE = 0x0000,
  QK_TEXFLG_MIPMAP = 0x0001,
  QK_TEXFLG_LINEAR = 0x0002,
  QK_TEXFLG_NEAREST = 0x0004,
  QK_TEXFLG_ALPHA = 0x0008,
  QK_TEXFLG_PAD = 0x0010,
  QK_TEXFLG_PERSIST = 0x0020,
  QK_TEXFLG_OVERWRITE = 0x0040,
  QK_TEXFLG_NOPICMIP = 0x0080,
  QK_TEXFLG_FULLBRIGHT = 0x0100,
  QK_TEXFLG_NOBRIGHT = 0x0200,
  QK_TEXFLG_CONCHARS = 0x0400,
  QK_TEXFLG_ARRAY = 0x0800,
  QK_TEXFLG_CUBEMAP = 0x1000,
  QK_TEXFLG_BINDLESS = 0x2000,
  QK_TEXFLG_ALPHABRIGHT = 0x4000,
  QK_TEXFLG_CLAMP = 0x8000,

  QK_TEXFLG__MAX = 0x8000 // same as QK_TEXFLG_CLAMP!
} qk_texture_flags;

typedef enum {
  QK_SKIN__UNKNOWN = -1,

  QK_SKIN_SINGLE = 0,
  QK_SKIN_GROUP,

  QK_SKIN__MAX = 255
} qk_skintype;

typedef struct {
  int32_t magic_codes;
  int32_t version;
  qk_vector3f scale;
  qk_vector3f origin;
  float bounding_radius;
  qk_vector3f eye_position;
  int32_t skins_count;
  int32_t skin_width;
  int32_t skin_height;
  int32_t vertices_count;
  int32_t triangles_count;
  int32_t frames_count;
  qk_synctype sync_type;
  int32_t flags;
  float size;
} qk_header;

typedef struct {
  uint32_t width;
  uint32_t height;
  uint8_t *pixels;
} qk_skin;

typedef struct {
  qk_header header;
  qk_skin *skins;
} qk_mdl;

#endif // QK_MDL_HEADER_
