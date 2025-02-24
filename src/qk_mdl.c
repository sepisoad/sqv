#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../deps/vector.h"
#include "../deps/list.h"
#include "../deps/log.h"

#ifdef DEBUG
#include "../deps/stb_image_write.h"
#endif

#include "qk_mdl.h"
#include "sqv_err.h"
#include "utils.h"

#define MAX_STATIC_MEM 8192

const int MAGICCODE = (('O' << 24) + ('P' << 16) + ('D' << 8) + 'I');
const int MD0VERSION = 6;
const int MAXSKINHEIGHT = 480;
const int MAXVERTICES = 2000;
const int MAXTRIANGLES = 4096;
const int MAXSKINS = 32;

extern const uint8_t _qk_palette[256][3];
extern const float _qk_normals[162][3];

static sqv_err load_image(
    uintptr_t mem, uint8_t** pixels, size_t size, uint32_t width,
    uint32_t height
) {
  uint8_t* indices = (uint8_t*)mem;
  *pixels = (uint8_t*)malloc(size * 4); // SEPI: mem 4 => r, g, b, a
  makesure(*pixels != NULL, "malloc failed");

  for (size_t i = 0, j = 0; i < size; i++, j += 4) {
    uint32_t index = indices[i];
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
  uint32_t width = hdr->skin_width;
  uint32_t height = hdr->skin_height;
  uint32_t skin_size = width * height;

  *skins = (qk_skin*)malloc(sizeof(qk_skin) * hdr->skins_count);
  makesure(*skins != NULL, "malloc failed");

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

      (*skins)[i].image = sg_alloc_image();
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
    const qk_header* hdr, qk_raw_texcoord** coords, uintptr_t mem
) {
  *coords
      = (qk_raw_texcoord*)malloc(sizeof(qk_raw_texcoord) * hdr->vertices_count);
  makesure(*coords != NULL, "malloc failedes");

  for (size_t i = 0; i < hdr->vertices_count; i++) {
    qk_raw_texcoord* ptr = (qk_raw_texcoord*)mem;
    (*coords)[i].onseam = endian_i32(ptr->onseam);
    (*coords)[i].s = endian_i32(ptr->s);
    (*coords)[i].t = endian_i32(ptr->t);
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
  makesure(*trisidx != NULL, "malloc failed");

  for (size_t i = 0; i < count; i++) {
    qk_raw_triangles_idx* ptr = (qk_raw_triangles_idx*)mem;
    (*trisidx)[i].frontface = endian_i32(ptr->frontface);
    for (size_t j = 0; j < 3; j++) {
      (*trisidx)[i].vertices_idx[j] = endian_i32(ptr->vertices_idx[j]);
    }
    mem += sizeof(qk_raw_triangles_idx);
  }

  return mem;
}

static uintptr_t load_frame_single(
    const qk_header* hdr, List* frms, uint32_t* posidx, uintptr_t mem
) {
  qk_raw_frame_single* snl = (qk_raw_frame_single*)mem;
  qk_raw_frame frm;

  strncpy(frm.name, snl->name, sizeof(snl->name));
  frm.first_pose = (*posidx);
  (*posidx)++;

  for (uint8_t i = 0; i < 3; i++) {
    frm.bbox_min.vertex[i] = snl->bbox_min.vertex[i];
    frm.bbox_max.vertex[i] = snl->bbox_max.vertex[i];
  }

  frm.raw_vertices_ptr = list_create(sizeof(qk_raw_triangle_vertex*), NULL);
  qk_raw_triangle_vertex* ptr = (qk_raw_triangle_vertex*)(snl + 1);

  list_push_back(frm.raw_vertices_ptr, ptr);
  list_push_back(frms, &frm);

  mem = (uintptr_t)(ptr + hdr->vertices_count);
  return mem;
}

static uintptr_t load_frames_group(
    const qk_header* hdr, List* frms, uint32_t* posidx, uintptr_t mem
) {
  qk_raw_frames_group* grp = (qk_raw_frames_group*)mem;
  qk_raw_frame frm;

  frm.first_pose = (*posidx);

  for (uint8_t i = 0; i < 3; i++) {
    frm.bbox_min.vertex[i] = grp->bbox_min.vertex[i];
    frm.bbox_max.vertex[i] = grp->bbox_max.vertex[i];
  }

  float* interval_ptr = (float*)(grp + 1);
  frm.interval = endian_f32(*interval_ptr);

  interval_ptr += grp->frames_count;
  mem = (uintptr_t)interval_ptr;

  frm.raw_vertices_ptr = list_create(sizeof(qk_raw_triangle_vertex*), NULL);
  for (uint32_t i = 0; i < grp->frames_count; i++) {
    qk_raw_triangle_vertex* ptr
        = (qk_raw_triangle_vertex*)((qk_raw_frame_single*)mem + 1);
    list_push_back(frm.raw_vertices_ptr, ptr);

    mem = (uintptr_t)(ptr + hdr->vertices_count);
    (*posidx)++;
  }

  list_push_back(frms, &frm);

  return mem;
}

static uintptr_t load_raw_frames(qk_header* hdr, List* frms, uintptr_t mem) {
  uint32_t poses_count = 0;

  for (uint32_t i = 0; i < hdr->frames_count; i++) {
    qk_frametype ft = endian_i32(*(qk_frametype*)mem);
    mem += sizeof(qk_frametype);
    if (ft == QK_FT_SINGLE) {
      mem = load_frame_single(hdr, frms, &poses_count, mem);
    } else {
      // example: progs/flame2.mdl
      mem = load_frames_group(hdr, frms, &poses_count, mem);
    }
  }

  hdr->poses_count = poses_count;
  return mem;
}

static sqv_err calc_bounds(qk_header* hdr) {
  // TODO: implement this
  return SQV_SUCCESS;
}

static uint32_t build_strip_length(
    const qk_header* hdr, const qk_raw_triangles_idx* tris, int32_t used[],
    int32_t sverts[], int32_t stris[], uint32_t* count, uint32_t sidx,
    uint32_t stv
) {
  const qk_raw_triangles_idx* last = tris + sidx;
  const qk_raw_triangles_idx* check = NULL;

  used[sidx] = 2;
  sverts[0] = last->vertices_idx[(stv) % 3];
  sverts[1] = last->vertices_idx[(stv + 1) % 3];
  sverts[2] = last->vertices_idx[(stv + 2) % 3];

  stris[0] = sidx;
  (*count) = 1;

  int32_t m1 = last->vertices_idx[(stv + 2) % 3];
  int32_t m2 = last->vertices_idx[(stv + 1) % 3];
  int32_t triidx = 0;

nexttri:
  for (triidx = sidx + 1, check = tris + (sidx + 1);
       triidx < hdr->triangles_count; triidx++, check++) {
    if (check->frontface != last->frontface) continue;
    for (uint32_t k = 0; k < 3; k++) {
      if (check->vertices_idx[k] != m1) continue;
      if (check->vertices_idx[(k + 1) % 3] != m2) continue;
      if (used[triidx]) goto done;
      if ((*count) & 1)
        m2 = check->vertices_idx[(k + 2) % 3];
      else
        m1 = check->vertices_idx[(k + 2) % 3];

      sverts[(*count) + 2] = check->vertices_idx[(k + 2) % 3];
      stris[(*count)] = triidx;
      *count = (*count) + 1;

      used[triidx] = 2;
      goto nexttri;
    }
  }
done:
  for (triidx = sidx + 1; triidx < hdr->triangles_count; triidx++)
    if (used[triidx] == 2) used[triidx] = 0;

  return *count;
}

static uint32_t build_fan_length(
    const qk_header* hdr, const qk_raw_triangles_idx* tris, int32_t used[],
    int32_t sverts[], int32_t stris[], uint32_t* count, uint32_t sidx,
    uint32_t stv
) {
  const qk_raw_triangles_idx* last = tris + sidx;
  const qk_raw_triangles_idx* check = NULL;

  used[sidx] = 2;
  sverts[0] = last->vertices_idx[(stv) % 3];
  sverts[1] = last->vertices_idx[(stv + 1) % 3];
  sverts[2] = last->vertices_idx[(stv + 2) % 3];

  stris[0] = sidx;
  (*count) = 1;

  int32_t m1 = last->vertices_idx[(stv + 0) % 3];
  int32_t m2 = last->vertices_idx[(stv + 2) % 3];
  int32_t triidx = 0;

nexttri:
  for (triidx = sidx + 1, check = tris + (sidx + 1);
       triidx < hdr->triangles_count; triidx++, check++) {
    if (check->frontface != last->frontface) continue;
    for (uint32_t k = 0; k < 3; k++) {
      if (check->vertices_idx[k] != m1) continue;
      if (check->vertices_idx[(k + 1) % 3] != m2) continue;

      if (used[triidx]) goto done;

      m2 = check->vertices_idx[(k + 2) % 3];

      sverts[(*count) + 2] = m2;
      stris[*count] = triidx;
      *(count) = (*count) + 1;

      used[triidx] = 2;
      goto nexttri;
    }
  }
done:
  for (triidx = sidx + 1; triidx < hdr->triangles_count; triidx++)
    if (used[triidx] == 2) used[triidx] = 0;

  return *count;
}

static sqv_err build_triangles(
    const qk_header* hdr, const qk_raw_texcoord* coords,
    const qk_raw_triangles_idx* tris
) {
  int32_t is_used[MAX_STATIC_MEM] = { 0 }; // TODO: duh!
  int32_t best_vertices[MAX_STATIC_MEM] = { 0 }; // TODO: duh!
  int32_t best_triangles[MAX_STATIC_MEM] = { 0 }; // TODO: duh!
  int32_t strip_vertices[MAX_STATIC_MEM] = { 0 }; // TODO: duh!
  int32_t strip_triangles[MAX_STATIC_MEM] = { 0 }; // TODO: duh!
  int32_t commands[MAX_STATIC_MEM * 7 + 1] = { 0 }; // TODO: duh!
  int32_t vertex_order[MAX_STATIC_MEM * 3] = { 0 }; // TODO: duh!
  uint32_t best_length;
  uint32_t best_type;
  uint32_t commands_count = 0;
  uint32_t orders_count = 0;
  uint32_t strips_count = 0;

  for (uint32_t i = 0; i < hdr->triangles_count; i++) {
    if (is_used[i]) continue;

    best_length = 0;
    best_type = 0;

    for (uint32_t type = 0; type < 2; type++) {
      for (uint32_t start_vec = 0; start_vec < 3; start_vec++) {
        uint32_t length;
        if (type == 1) {
          length = build_strip_length(
              hdr, tris, is_used, strip_vertices, strip_triangles,
              &strips_count, i, start_vec
          );

        } else {
          length = build_fan_length(
              hdr, tris, is_used, strip_vertices, strip_triangles,
              &strips_count, i, start_vec
          );
        }
        if (length > best_length) {
          best_type = type;
          best_length = length;

          for (uint32_t j = 0; j < best_length + 2; j++) {
            best_vertices[j] = strip_vertices[j];
          }

          for (uint32_t j = 0; j < best_length; j++) {
            best_triangles[j] = strip_triangles[j];
          }
        }
      }

      for (uint32_t j = 0; j < best_length; j++)
        is_used[best_triangles[j]] = 1;

      if (best_type == 1)
        commands[commands_count++] = (best_length + 2);
      else
        commands[commands_count++] = -(best_length + 2);

      for (uint32_t j = 0; j < best_length + 2; j++) {
        int tmp;

        int32_t k = best_vertices[j];
        vertex_order[orders_count++] = k;

        int32_t s = coords[k].s;
        int32_t t = coords[k].t;

        if (!tris[best_triangles[0]].frontface && coords[k].onseam)
          s += hdr->skin_width / 2; // on back side
        s = (s + 0.5) / hdr->skin_width;
        t = (t + 0.5) / hdr->skin_height;

        memcpy(&tmp, &s, 4);
        commands[commands_count++] = tmp;
        memcpy(&tmp, &t, 4);
        commands[commands_count++] = tmp;
      }
    }
  }

  commands[commands_count++] = 0;

  // allverts += numorder;
  // alltris += pheader->numtris;

  return SQV_SUCCESS;
}

static sqv_err make_display_lists(
    const qk_header* hdr, const List* raw_frames, const qk_raw_texcoord* coords,
    const qk_raw_triangles_idx* tris
) {
  uint32_t frames_count = list_length(raw_frames);
  makesure(frames_count == hdr->frames_count, "invalid frames count");

  build_triangles(hdr, coords, tris);

  qk_triangle* triangles = (qk_triangle*)malloc(
      sizeof(qk_triangle) * hdr->poses_count * hdr->vertices_count
  );
  makesure(triangles != NULL, "malloc failed");

  uintptr_t* ptrs = (uintptr_t*)malloc(sizeof(uintptr_t*) * frames_count);
  makesure(ptrs != NULL, "malloc failed");

  Node* ptr = list_begin(raw_frames);
  for (uint32_t i = 0; i < frames_count; i++) {
    ptrs[i] = (uintptr_t)ptr->value;
    ptr = ptr->next;
  }

  for (uint32_t i = 0; i < hdr->poses_count; i++) {
    qk_raw_triangle_vertex* rawtri = (qk_raw_triangle_vertex*)(ptrs + i);
    const float* normal = _qk_normals[rawtri->normal_idx];
    triangles[i]                   = (qk_triangle
    ) { .normal = { .X = normal[0], .Y = normal[1], .Z = normal[2], },
                          .vertex = { .X = rawtri->vertex[0],
                                      .Y = rawtri->vertex[1],
                                      .Z = rawtri->vertex[2], }, };
  }

  free(ptrs);

  return SQV_SUCCESS;
}

static void load_mdl_buffer(const char* path, uint8_t** buf) {
  FILE* f = fopen(path, "rb");
  makesure(f != NULL, "invalid mdl file");

  fseek(f, 0, SEEK_END);
  size_t fsize = ftell(f);
  rewind(f);

  *buf = (uint8_t*)malloc(sizeof(uint8_t) * fsize);
  makesure(*buf != NULL, "malloc failed");

  size_t rsize = fread(*buf, 1, fsize, f);
  makesure(
      rsize == fsize, "read size '%zu' did not match the file size '%zu'",
      rsize, fsize
  );

  if (f) {
    fclose(f);
  }
}

sqv_err qk_load_mdl(const char* path, qk_mdl* _) {
  sqv_err err = SQV_SUCCESS;
  uint8_t* buf = NULL;

  load_mdl_buffer(path, &buf);

  uintptr_t mem = (uintptr_t)buf;
  qk_raw_header* rhdr = (qk_raw_header*)buf;
  int32_t mdl_mcode = endian_i32(rhdr->magic_codes);
  int32_t mdl_version = endian_i32(rhdr->version);
  int32_t mdl_flags = endian_i32(rhdr->flags);
  int32_t mdl_size = endian_f32(rhdr->size);

  qk_header hdr = {
    .radius = endian_f32(rhdr->bounding_radius),
    .skin_width = endian_i32(rhdr->skin_width),
    .skin_height = endian_i32(rhdr->skin_height),
    .skins_count = endian_i32(rhdr->skins_count),
    .vertices_count = endian_i32(rhdr->vertices_count),
    .triangles_count = endian_i32(rhdr->triangles_count),
    .frames_count = endian_i32(rhdr->frames_count),
    .scale = { .X = endian_f32(rhdr->scale[0]),
               .Y = endian_f32(rhdr->scale[1]),
               .Z = endian_f32(rhdr->scale[2]) },
    .origin = { .X = endian_f32(rhdr->origin[0]),
                .Y = endian_f32(rhdr->origin[1]),
                .Z = endian_f32(rhdr->origin[2]) },
    .eye = { .X = endian_f32(rhdr->eye_position[0]),
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
  qk_skin* skins = NULL;
  qk_raw_texcoord* raw_tex_coords = NULL;
  qk_raw_triangles_idx* raw_tris_idx = NULL;
  List* raw_frames = list_create(sizeof(qk_raw_frame), NULL);

  mem += sizeof(qk_raw_header);
  mem = load_skins(&hdr, &skins, mem);
  mem = load_raw_texture_coordinates(&hdr, &raw_tex_coords, mem);
  mem = load_raw_triangles_indices(&hdr, &raw_tris_idx, mem);
  mem = load_raw_frames(&hdr, raw_frames, mem);

  err = calc_bounds(&hdr);
  makesure(err == SQV_SUCCESS, "calc_bound() failed");

  err = make_display_lists(&hdr, raw_frames, raw_tex_coords, raw_tris_idx);
  makesure(err == SQV_SUCCESS, "make_display_lists() failed");

cleanup:
  if (buf) {
    free(buf);
  }

  if (raw_tex_coords) {
    free(raw_tex_coords);
  }

  if (raw_tris_idx) {
    free(raw_tris_idx);
  }

  Node* node = list_begin(raw_frames);
  for (uint32_t i = 0; i < hdr.frames_count; i++) {
    qk_raw_frame* frame = (qk_raw_frame*)node->value;
    List* list = frame->raw_vertices_ptr;
    list_deallocate(list);

    node = node->next;
  }

  if (raw_frames) {
    list_deallocate(raw_frames);
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
