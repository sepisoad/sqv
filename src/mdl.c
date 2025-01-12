#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sokol_gfx.h>

#include "mdl.h"

extern uint8_t mdl_palette[256][3];

void mld_make_texture_from_skin(sg_bindings b, int n, const mdl_model *m) {
  const mdl_header hdr = m->header;
  const uint8_t chnl = 4; // 4 channels
  const size_t dtsize = hdr.skinwidth * hdr.skinheight * chnl;

  uint8_t *p = (uint8_t *)malloc(dtsize);

  /* Convert indexed 8 bits texture to RGBA 32 bits */
  for (int i = 0; i < hdr.skinwidth * hdr.skinheight; ++i) {
    p[(i * 4) + 0] = mdl_palette[m->skins[n].data[i]][0];
    p[(i * 4) + 1] = mdl_palette[m->skins[n].data[i]][1];
    p[(i * 4) + 2] = mdl_palette[m->skins[n].data[i]][2];
    p[(i * 4) + 3] = 1; // 4th alpha channel is always 1
  }

  sg_init_image(
      b.images[0],
      &(sg_image_desc){.width = hdr.skinwidth,
                       .height = hdr.skinheight,
                       .pixel_format = SG_PIXELFORMAT_RGBA8,
                       .data.subimage[0][0] = {.ptr = p, .size = dtsize}});
  free(p);
}

int mdl_read_model(sg_bindings b, const char *path, mdl_model *m) {
  FILE *fp;

  fp = fopen(path, "rb");
  if (!fp) {
    fprintf(stderr, "error: couldn't open \"%s\"!\n", path);
    return 0;
  }

  fread(&m->header, 1, sizeof(mdl_header), fp);
  if ((m->header.ident != 1330660425) || (m->header.version != 6)) {
    fprintf(stderr, "Error: bad version or identifier\n");
    fclose(fp);
    return 0;
  }

  const mdl_header h = m->header;
  m->skins = (mdl_skin *)malloc(sizeof(mdl_skin) * h.num_skins);
  m->texcoords = (mdl_texcoord *)malloc(sizeof(mdl_texcoord) * h.num_verts);
  m->triangles = (mdl_triangle *)malloc(sizeof(mdl_triangle) * h.num_tris);
  m->frames = (mdl_frame *)malloc(sizeof(mdl_frame) * h.num_frames);
  m->tex_id = (uint32_t *)malloc(sizeof(uint32_t) * h.num_skins);

  const size_t skinsz = h.skinwidth * h.skinheight;
  const size_t skinbufsz = sizeof(uint8_t) * skinsz;
  for (int i = 0; i < h.num_skins; ++i) {
    m->skins[i].data = (uint8_t *)malloc(skinbufsz);
    fread(&m->skins[i].group, sizeof(int), 1, fp);
    fread(m->skins[i].data, sizeof(uint8_t), skinsz, fp);

    mld_make_texture_from_skin(b, i, m);
    m->tex_id[i] = b.images[0].id; // SEPI: hmmm!

    free(m->skins[i].data);
    m->skins[i].data = NULL;
  }

  fread(m->texcoords, sizeof(mdl_texcoord), h.num_verts, fp);
  fread(m->triangles, sizeof(mdl_triangle), h.num_tris, fp);

  const size_t vertxsz = sizeof(mdl_vertex) * h.num_verts;
  for (int i = 0; i < h.num_frames; ++i) {
    m->frames[i].frame.verts = (mdl_vertex *)malloc(vertxsz);
    fread(&m->frames[i].type, sizeof(int), 1, fp);
    fread(&m->frames[i].frame.bboxmin, sizeof(mdl_vertex), 1, fp);
    fread(&m->frames[i].frame.bboxmax, sizeof(mdl_vertex), 1, fp);
    fread(m->frames[i].frame.name, sizeof(char), 16, fp);
    fread(m->frames[i].frame.verts, sizeof(mdl_vertex), h.num_verts, fp);
  }

  fclose(fp);
  return 1;
}

void mdl_assemble_buffer(int n, const mdl_model *m, float *ver_buf,
                         int *ind_buf) {
  const mdl_header h = m->header;
  if ((n < 0) || (n > h.num_frames - 1))
    return;

  int ver_idx = 0;
  int ind_idx = 0;
  float s, t;
  mdl_vertex *tri_vert;

  for (int i = 0; i < h.num_tris; ++i) {
    for (int j = 0; j < 3; ++j) {
      int tri_idx = m->triangles[i].vertex[j];

      tri_vert = &m->frames[n].frame.verts[tri_idx];

      s = (float)m->texcoords[tri_idx].s;
      t = (float)m->texcoords[tri_idx].t;
      if (!m->triangles[i].facesfront && m->texcoords[tri_idx].onseam) {
        s += h.skinwidth * 0.5f; /* Backface */
      }
      s = (s + 0.5) / h.skinwidth;
      t = (t + 0.5) / h.skinheight;

      ver_buf[ver_idx + 0] = (h.scale[0] * tri_vert->v[0]) + h.translate[0];
      ver_buf[ver_idx + 1] = (h.scale[1] * tri_vert->v[1]) + h.translate[1];
      ver_buf[ver_idx + 2] = (h.scale[2] * tri_vert->v[2]) + h.translate[2];
      ver_buf[ver_idx + 3] = s;
      ver_buf[ver_idx + 4] = t;
      ver_idx += 5;

      ind_buf[ind_idx] = tri_idx;
      ind_idx++;
    }
  }
}

void mdl_free(mdl_model *m) {
  int i;

  if (m->skins) {
    free(m->skins);
    m->skins = NULL;
  }

  if (m->texcoords) {
    free(m->texcoords);
    m->texcoords = NULL;
  }

  if (m->triangles) {
    free(m->triangles);
    m->triangles = NULL;
  }

  if (m->tex_id) {
    // SEPI: i suppose sokol free GPU texture mem, need to double check!
    // glDeleteTextures(m->header.num_skins, m->tex_id);

    free(m->tex_id);
    m->tex_id = NULL;
  }

  if (m->frames) {
    for (i = 0; i < m->header.num_frames; ++i) {
      free(m->frames[i].frame.verts);
      m->frames[i].frame.verts = NULL;
    }

    free(m->frames);
    m->frames = NULL;
  }
}