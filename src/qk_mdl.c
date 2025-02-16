#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../deps/log.h"

#ifdef DEBUG
#include "../deps/stb_image_write.h"
#endif

#include "qk_mdl.h"
#include "sqv_err.h"
#include "utils.h"

const int MAGICCODE     = (('O' << 24) + ('P' << 16) + ('D' << 8) + 'I');
const int MD0VERSION    = 6;
const int MAXSKINHEIGHT = 480;
const int MAXVERTICES   = 2000;
const int MAXTRIANGLES  = 4096;
const int MAXSKINS      = 32;

extern const uint8_t _qk_palette[256][3];
extern const float   _qk_normals[162][3];

static sqv_err load_image(
    uintptr_t mem, uint8_t** pixels, size_t size, uint32_t width,
    uint32_t height
) {
  uint8_t* indices = (uint8_t*)mem;
  *pixels          = (uint8_t*)malloc(size * 4); // SEPI: mem 4 => r, g, b, a
  makesure(*pixels != NULL, "failed to allocate memory for skin image");

  for (size_t i = 0, j = 0; i < size; i++, j += 4) {
    uint32_t index   = indices[i];
    (*pixels)[j + 0] = _qk_palette[index][0]; // red
    (*pixels)[j + 1] = _qk_palette[index][1]; // green
    (*pixels)[j + 2] = _qk_palette[index][2]; // blue
    (*pixels)[j + 3] = 255; // alpha, always opaque
  }

  return SQV_SUCCESS;
}

static uintptr_t
load_skins(const qk_header* hdr, qk_skin** skins, uintptr_t mem) {
  qk_skintype* skin_type = NULL;
  uint32_t     width     = hdr->skin_width;
  uint32_t     height    = hdr->skin_height;
  uint32_t     skin_size = width * height;

  *skins = (qk_skin*)malloc(sizeof(qk_skin) * hdr->skins_count);
  makesure(*skins != NULL, "failed to allocate memory for skins");

  for (size_t i = 0; i < hdr->skins_count; i++) {
    skin_type = (qk_skintype*)mem;
    if (*skin_type == QK_SKIN_SINGLE) {
      uint8_t* pixels = NULL;
      mem += sizeof(qk_skintype);
      sqv_err err = load_image(mem, &pixels, skin_size, width, height);
      makesure(err == SQV_SUCCESS, "load_image() failed");

#ifdef DEBUG
      char skin_name[1024] = { 0 };
      sprintf(skin_name, ".ignore/skin_%zu.png", i);
      stbi_write_png(skin_name, width, height, 4, pixels, width * 4);
#endif

      (*skins)[i].image   = sg_alloc_image();
      (*skins)[i].sampler = sg_make_sampler(&(sg_sampler_desc) {
          .min_filter = SG_FILTER_LINEAR,
          .mag_filter = SG_FILTER_LINEAR,
      });
      sg_init_image((*skins)[i].image, &(sg_image_desc){
        .width = width,
        .height = height,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .data.subimage[0][0] = {
            .ptr = pixels,
            .size = (size_t)(width * height * 4),
        }
      });
      free(pixels);
      mem += skin_size;
    } else {
      /*
       * this is not implemented yet, and may never be implemented!
       */
      makesure(false, "load_skin() does not support multi skin YET!");
    }
  }

cleanup:
  return mem;
}

static uintptr_t load_raw_texture_coordinates(
    const qk_header* hdr, qk_raw_texcoord** texcoords, uintptr_t mem
) {
  *texcoords
      = (qk_raw_texcoord*)malloc(sizeof(qk_raw_texcoord) * hdr->vertices_count);
  makesure(
      *texcoords != NULL, "failed to allocate memory for texture coordinates"
  );

  for (size_t i = 0; i < hdr->vertices_count; i++) {
    qk_raw_texcoord* ptr   = (qk_raw_texcoord*)mem;
    (*texcoords)[i].onseam = endian_i32(ptr->onseam);
    (*texcoords)[i].s      = endian_i32(ptr->s);
    (*texcoords)[i].t      = endian_i32(ptr->t);
    mem += sizeof(qk_raw_texcoord);
  }

  return mem;
}

static uintptr_t load_raw_triangles_indices(
    const qk_header* hdr, qk_raw_triangles_idx** trisidx, uintptr_t mem
) {
  int32_t count = hdr->triangles_count;
  *trisidx
      = (qk_raw_triangles_idx*)malloc(sizeof(qk_raw_triangles_idx) * count);
  makesure(*trisidx != NULL, "failed to allocate memory for triangle pixels");

  for (size_t i = 0; i < count; i++) {
    qk_raw_triangles_idx* ptr = (qk_raw_triangles_idx*)mem;
    (*trisidx)[i].frontface   = endian_i32(ptr->frontface);
    for (size_t j = 0; j < 3; j++) {
      (*trisidx)[i].vertices_idx[j] = endian_i32(ptr->vertices_idx[j]);
    }
    mem += sizeof(qk_raw_triangles_idx);
  }

  return mem;
}

static uintptr_t load_frame_single(
    const qk_header* hdr, qk_raw_frame* frm, uint32_t* posidx, uintptr_t mem
) {
  qk_raw_frame_single* raw = (qk_raw_frame_single*)mem;

  strncpy(frm->name, raw->name, sizeof(raw->name));
  frm->poses_count = 1;
  frm->first_pose  = (*posidx);
  (*posidx)++;

  for (uint8_t i = 0; i < 3; i++) {
    frm->bbox_min.vertex[i] = raw->bbox_min.vertex[i];
    frm->bbox_max.vertex[i] = raw->bbox_max.vertex[i];
  }

  frm->raw_vertices_ptr = (qk_raw_triangle_vertex*)(raw + 1);
  mem = (uintptr_t)(frm->raw_vertices_ptr + hdr->vertices_count);

  return mem;
}

static uintptr_t load_frames_group(
    const qk_header* hdr, qk_raw_frame* frm, uint32_t* posidx, uintptr_t mem
) {
  qk_raw_frames_group* raw = (qk_raw_frames_group*)mem;

  frm->poses_count = raw->frames_count; /* don't let the naming confuse you */
  frm->first_pose  = (*posidx);

  for (uint8_t i = 0; i < 3; i++) {
    frm->bbox_min.vertex[i] = raw->bbox_min.vertex[i];
    frm->bbox_max.vertex[i] = raw->bbox_max.vertex[i];
  }

  float* interval_ptr = (float*)(raw + 1);
  frm->interval       = endian_f32(*interval_ptr);

  interval_ptr += frm->poses_count;
  mem = (uintptr_t)interval_ptr;

  for (uint32_t i = 0; i < frm->poses_count; i++) {
    frm->raw_vertices_ptr
        = (qk_raw_triangle_vertex*)((qk_raw_frame_single*)mem + 1);
    (*posidx)++;

    mem = (uintptr_t)((qk_raw_triangle_vertex*)((qk_raw_frame_single*)mem + 1)
                      + hdr->vertices_count);
  }

  return mem;
}

static uintptr_t
load_raw_frames(const qk_header* hdr, qk_raw_frame** frms, uintptr_t mem) {
  uint32_t posidx = 0;
  *frms = (qk_raw_frame*)malloc(sizeof(qk_raw_frame) * hdr->frames_count);

  for (uint32_t i = 0; i < hdr->frames_count; i++) {
    qk_frametype ft = endian_i32(*(qk_frametype*)mem);
    mem += sizeof(qk_frametype);
    if (ft == QK_FT_SINGLE) {
      mem = load_frame_single(hdr, &(*frms)[i], &posidx, mem);
    } else {
      mem = load_frames_group(hdr, &(*frms)[i], &posidx, mem);
    }
  }

  return mem;
}

static sqv_err calc_bounds(qk_mdl* mdl) {
  // TODO: implement this
  return SQV_SUCCESS;
}

static sqv_err make_display_lists(qk_mdl* mdl) {
  // qk_raw_header      hdr = mdl->raw_header;
  // qk_raw_triangles_idx* tip = mdl->triangles_idx;
  // qk_raw_triangle_vertex* tvp = mdl->triangles_vertices;
  // uint32_t vc = hdr.vertices_count;
  // uint32_t fc = hdr.frames_count;

  // SEPI: mem
  // tvp = (qk_raw_triangle_vertex*)malloc(sizeof(qk_raw_triangle_vertex) * vc *
  // fc); assert(tvp != NULL);

  // for (size_t i = 0; i < fc; i++) {
  //   for (size_t j = 0; j < vc; j++) {
  //     // tvp[i * hdr.vertices_count + j] = tip[i][j];
  //     // TODO:
  //   }
  // }

  return SQV_SUCCESS;
}

static void load_mdl_buffer(const char* path, uint8_t** buf) {
  FILE* f = fopen(path, "rb");
  makesure(f != NULL, "invalid mdl file");

  fseek(f, 0, SEEK_END);
  size_t fsize = ftell(f);
  rewind(f);

  *buf = (uint8_t*)malloc(sizeof(uint8_t) * fsize);
  makesure(*buf != NULL, "failed to allocate memory to mdl file");

  size_t rsize = fread(*buf, 1, fsize, f);
  makesure(
      rsize == fsize, "read size '%zu' did not match the file size '%zu'",
      rsize, fsize
  );

  if (f) {
    fclose(f);
  }
}

/*  *********************
 *  MDL FORMAT
 *  *********************
 *
 *  Header (84 bytes) [x]
 *  =========
 *  Skins data (skins_count) [x]
 *  =========
 *  TexCoords data (vertices_count) [x]
 *  =========
 *  Triangles data (triangles_count) [x]
 *  =========
 *  Frames info [@]
 *  ---------
 *  Frames data (frames count) [@]
 */

sqv_err qk_load_mdl(const char* path, qk_mdl* _mdl_) {
  sqv_err  err = SQV_SUCCESS;
  uint8_t* buf = NULL;

  load_mdl_buffer(path, &buf);

  uintptr_t      mem         = (uintptr_t)buf;
  qk_raw_header* rhdr        = (qk_raw_header*)buf;
  int32_t        mdl_mcode   = endian_i32(rhdr->magic_codes);
  int32_t        mdl_version = endian_i32(rhdr->version);
  int32_t        mdl_flags   = endian_i32(rhdr->flags);
  int32_t        mdl_size    = endian_f32(rhdr->size);

  qk_header hdr = {
    .radius          = endian_f32(rhdr->bounding_radius),
    .skin_width      = endian_i32(rhdr->skin_width),
    .skin_height     = endian_i32(rhdr->skin_height),
    .skins_count     = endian_i32(rhdr->skins_count),
    .vertices_count  = endian_i32(rhdr->vertices_count),
    .triangles_count = endian_i32(rhdr->triangles_count),
    .frames_count    = endian_i32(rhdr->frames_count),
    .scale           = { .X = endian_f32(rhdr->scale[0]),
                         .Y = endian_f32(rhdr->scale[1]),
                         .Z = endian_f32(rhdr->scale[2]) },
    .origin          = { .X = endian_f32(rhdr->origin[0]),
                         .Y = endian_f32(rhdr->origin[1]),
                         .Z = endian_f32(rhdr->origin[2]) },
    .eye             = { .X = endian_f32(rhdr->eye_position[0]),
                         .Y = endian_f32(rhdr->eye_position[1]),
                         .Z = endian_f32(rhdr->eye_position[2]) },
  };

  makesure(mdl_mcode == MAGICCODE, "wrong magic code");
  makesure(mdl_version == MD0VERSION, "wrong version");
  makesure(hdr.skin_height <= MAXSKINHEIGHT, "invalid skin height");
  makesure(hdr.vertices_count <= MAXVERTICES, "invalid vertices count ");
  makesure(hdr.triangles_count <= MAXTRIANGLES, "invalid triangles count");
  makesure(hdr.skins_count <= MAXSKINS, "invalid skins count");
  makesure(hdr.vertices_count > 0, "invalid vertices count");
  makesure(hdr.triangles_count > 0, "invalid triangles count");
  makesure(hdr.frames_count > 0, "invalid frames count");
  makesure(hdr.skins_count > 0, "invalid skins count");

  /* TODO: free these memories */
  qk_skin*              skins          = NULL;
  qk_raw_texcoord*      raw_tex_coords = NULL;
  qk_raw_triangles_idx* raw_tris_idx   = NULL;
  qk_raw_frame*         raw_frames     = NULL;

  mem += sizeof(qk_raw_header);
  mem = load_skins(&hdr, &skins, mem);
  mem = load_raw_texture_coordinates(&hdr, &raw_tex_coords, mem);
  mem = load_raw_triangles_indices(&hdr, &raw_tris_idx, mem);
  mem = load_raw_frames(&hdr, &raw_frames, mem);

  // SEPI: VVVVVVVVVV
  err = calc_bounds(_mdl_);
  makesure(err == SQV_SUCCESS, "calc_bound() failed");

  err = make_display_lists(_mdl_);
  makesure(err == SQV_SUCCESS, "make_display_lists() failed");

cleanup:
  if (buf) {
    free(buf);
  }

  return err;
}

sqv_err qk_init(void) { return SQV_SUCCESS; }

sqv_err qk_deinit(qk_mdl* mdl) {
  if (mdl->skins) {
    free(mdl->skins);
    mdl->skins = NULL;
  }

  /* TODO: clean up*/
  // if (mdl->texcoords) {
  //   free(mdl->texcoords);
  //   mdl->texcoords = NULL;
  // }

  // if (mdl->triangles_idx) {
  //   free(mdl->triangles_idx);
  //   mdl->triangles_idx = NULL;
  // }

  return SQV_SUCCESS;
}
