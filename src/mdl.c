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

  m->iskin = 0; // hardcode!
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

void mdl_render_frame(int n, const mdl_model *mdl) {
  // int i, j;
  // float s, t;
  // vec3_t v;
  // struct mdl_vertex_t *pvert;

  // if ((n < 0) || (n > mdl->header.num_frames - 1))
  //   return;

  // glBindTexture(GL_TEXTURE_2D, mdl->tex_id[mdl->iskin]);

  // /* Draw the model */
  // glBegin(GL_TRIANGLES);
  // /* Draw each triangle */
  // for (i = 0; i < mdl->header.num_tris; ++i) {
  //   /* Draw each vertex */
  //   for (j = 0; j < 3; ++j) {
  //     pvert = &mdl->frames[n].frame.verts[mdl->triangles[i].vertex[j]];

  //     /* Compute texture coordinates */
  //     s = (GLfloat)mdl->texcoords[mdl->triangles[i].vertex[j]].s;
  //     t = (GLfloat)mdl->texcoords[mdl->triangles[i].vertex[j]].t;

  //     if (!mdl->triangles[i].facesfront &&
  //         mdl->texcoords[mdl->triangles[i].vertex[j]].onseam) {
  //       s += mdl->header.skinwidth * 0.5f; /* Backface */
  //     }

  //     /* Scale s and t to range from 0.0 to 1.0 */
  //     s = (s + 0.5) / mdl->header.skinwidth;
  //     t = (t + 0.5) / mdl->header.skinheight;

  //     /* Pass texture coordinates to OpenGL */
  //     glTexCoord2f(s, t);

  //     /* Normal vector */
  //     glNormal3fv(anorms_table[pvert->normalIndex]);

  //     /* Calculate real vertex position */
  //     v[0] = (mdl->header.scale[0] * pvert->v[0]) + mdl->header.translate[0];
  //     v[1] = (mdl->header.scale[1] * pvert->v[1]) + mdl->header.translate[1];
  //     v[2] = (mdl->header.scale[2] * pvert->v[2]) + mdl->header.translate[2];

  //     glVertex3fv(v);
  //   }
  // }
  // glEnd();
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