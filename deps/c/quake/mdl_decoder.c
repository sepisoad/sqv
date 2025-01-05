#include "mdl_decoder.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

vector anorms_table[162] = {
#include "mdl_anorms.h"
};

unsigned char quake_palette[256][3] = {
#include "mdl_colormap.h"
};

quake_model mdlfile;

uint8_t make_texture_from_skin(int n, const quake_model* mdl) {
  int i;
  uint8_t id;
  uint8_t* pixels =
      (uint8_t*)malloc(mdl->header.skin_width * mdl->header.skin_height * 3);

  for (i = 0; i < mdl->header.skin_width * mdl->header.skin_height; ++i) {
    pixels[(i * 3) + 0] = quake_palette[mdl->skins[n].data[i]][0];
    pixels[(i * 3) + 1] = quake_palette[mdl->skins[n].data[i]][1];
    pixels[(i * 3) + 2] = quake_palette[mdl->skins[n].data[i]][2];
  }

  // /* Generate OpenGL texture */
  // glGenTextures(1, &id);
  // glBindTexture(GL_TEXTURE_2D, id);

  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, mdl->header.skin_width,
  //                   mdl->header.skin_height, GL_RGB, GL_UNSIGNED_BYTE,
  //                   pixels);

  /* OpenGL has its own copy of image data */
  free(pixels);
  return id;
}

int quake_read_model(const char* filename, quake_model* mdl) {
  FILE* fp;
  int i;

  fp = fopen(filename, "rb");
  if (!fp) {
    fprintf(stderr, "error: couldn't open \"%s\"!\n", filename);
    return 0;
  }

  fread(&mdl->header, 1, sizeof(header), fp);

  if ((mdl->header.ident != 1330660425) || (mdl->header.version != 6)) {
    fprintf(stderr, "Error: bad version or identifier\n");
    fclose(fp);
    return 0;
  }

  mdl->skins = (skin*)malloc(sizeof(skin) * mdl->header.skins_count);
  mdl->texture_coordinates = (texture_coordinate*)malloc(
      sizeof(texture_coordinate) * mdl->header.vertices_count);
  mdl->triangles =
      (triangle*)malloc(sizeof(triangle) * mdl->header.triangles_count);
  mdl->frames = (frame*)malloc(sizeof(frame) * mdl->header.frames_count);
  mdl->texture_id = (uint8_t*)malloc(sizeof(uint8_t) * mdl->header.skins_count);

  mdl->is_kin = 0;

  for (i = 0; i < mdl->header.skins_count; ++i) {
    mdl->skins[i].data = (uint8_t*)malloc(
        sizeof(uint8_t) * mdl->header.skin_width * mdl->header.skin_height);

    fread(&mdl->skins[i].is_grouped, sizeof(int), 1, fp);
    fread(mdl->skins[i].data, sizeof(uint8_t),
          mdl->header.skin_width * mdl->header.skin_height, fp);

    mdl->texture_id[i] = make_texture_from_skin(i, mdl);

    free(mdl->skins[i].data);
    mdl->skins[i].data = NULL;
  }

  fread(mdl->texture_coordinates, sizeof(texture_coordinate),
        mdl->header.vertices_count, fp);
  fread(mdl->triangles, sizeof(triangle), mdl->header.triangles_count, fp);

  for (i = 0; i < mdl->header.frames_count; ++i) {
    mdl->frames[i].frame.vertices =
        (vertex*)malloc(sizeof(vertex) * mdl->header.vertices_count);

    fread(&mdl->frames[i].type, sizeof(int), 1, fp);
    fread(&mdl->frames[i].frame.bouding_box_min, sizeof(vertex), 1, fp);
    fread(&mdl->frames[i].frame.bouding_box_max, sizeof(vertex), 1, fp);
    fread(mdl->frames[i].frame.name, sizeof(char), 16, fp);
    fread(mdl->frames[i].frame.vertices, sizeof(vertex),
          mdl->header.vertices_count, fp);
  }

  fclose(fp);
  return 1;
}

void quake_free_model(quake_model* mdl) {
  int i;

  if (mdl->skins) {
    free(mdl->skins);
    mdl->skins = NULL;
  }

  if (mdl->texture_coordinates) {
    free(mdl->texture_coordinates);
    mdl->texture_coordinates = NULL;
  }

  if (mdl->triangles) {
    free(mdl->triangles);
    mdl->triangles = NULL;
  }

  if (mdl->texture_id) {
    // glDeleteTextures(mdl->header.skins_count, mdl->texture_id);
    free(mdl->texture_id);
    mdl->texture_id = NULL;
  }

  if (mdl->frames) {
    for (i = 0; i < mdl->header.frames_count; ++i) {
      free(mdl->frames[i].frame.vertices);
      mdl->frames[i].frame.vertices = NULL;
    }

    free(mdl->frames);
    mdl->frames = NULL;
  }
}

// void RenderFrame(int n, const quake_model* mdl) {
//   int i, j;
//   GLfloat s, t;
//   vector v;
//   vertex* pvert;

//   if ((n < 0) || (n > mdl->header.frames_count - 1)) return;

//   glBindTexture(GL_TEXTURE_2D, mdl->texture_id[mdl->is_kin]);

//   glBegin(GL_TRIANGLES);
//   for (i = 0; i < mdl->header.triangles_count; ++i) {
//     for (j = 0; j < 3; ++j) {
//       pvert = &mdl->frames[n].frame.vertices[mdl->triangles[i].vertex[j]];

//       s = (GLfloat)mdl->texcoords[mdl->triangles[i].vertex[j]].s;
//       t = (GLfloat)mdl->texcoords[mdl->triangles[i].vertex[j]].t;

//       if (!mdl->triangles[i].is_front_front &&
//           mdl->texcoords[mdl->triangles[i].vertex[j]].is_on_seam) {
//         s += mdl->header.skin_width * 0.5f; /* Backface */
//       }

//       s = (s + 0.5) / mdl->header.skin_width;
//       t = (t + 0.5) / mdl->header.skin_height;

//       glTexCoord2f(s, t);
//       glNormal3fv(anorms_table[pvert->normal_index]);

//       v[0] = (mdl->header.model_scale[0] * pvert->v[0]) +
//              mdl->header.model_translate[0];
//       v[1] = (mdl->header.model_scale[1] * pvert->v[1]) +
//              mdl->header.model_translate[1];
//       v[2] = (mdl->header.model_scale[2] * pvert->v[2]) +
//              mdl->header.model_translate[2];

//       glVertex3fv(v);
//     }
//   }
//   glEnd();
// }

// void RenderFrameItp(int n, float interp, const quake_model* mdl) {
//   int i, j;
//   GLfloat s, t;
//   vector norm, v;
//   GLfloat *n_curr, *n_next;
//   vertex *pvert1, *pvert2;

//   /* Check if n is in a valid range */
//   if ((n < 0) || (n > mdl->header.frames_count)) return;

//   /* Enable quake_model's texture */
//   glBindTexture(GL_TEXTURE_2D, mdl->texture_id[mdl->is_kin]);

//   /* Draw the quake_model */
//   glBegin(GL_TRIANGLES);
//   /* Draw each triangle */
//   for (i = 0; i < mdl->header.triangles_count; ++i) {
//     /* Draw each vertex */
//     for (j = 0; j < 3; ++j) {
//       pvert1 = &mdl->frames[n].frame.vertices[mdl->triangles[i].vertex[j]];
//       pvert2 = &mdl->frames[n +
//       1].frame.vertices[mdl->triangles[i].vertex[j]];

//       /* Compute texture coordinates */
//       s = (GLfloat)mdl->texcoords[mdl->triangles[i].vertex[j]].s;
//       t = (GLfloat)mdl->texcoords[mdl->triangles[i].vertex[j]].t;

//       if (!mdl->triangles[i].is_front_front &&
//           mdl->texcoords[mdl->triangles[i].vertex[j]].is_on_seam) {
//         s += mdl->header.skin_width * 0.5f; /* Backface */
//       }

//       /* Scale s and t to range from 0.0 to 1.0 */
//       s = (s + 0.5) / mdl->header.skin_width;
//       t = (t + 0.5) / mdl->header.skin_height;

//       /* Pass texture coordinates to OpenGL */
//       glTexCoord2f(s, t);

//       /* Interpolate normals */
//       n_curr = anorms_table[pvert1->normal_index];
//       n_next = anorms_table[pvert2->normal_index];

//       norm[0] = n_curr[0] + interp * (n_next[0] - n_curr[0]);
//       norm[1] = n_curr[1] + interp * (n_next[1] - n_curr[1]);
//       norm[2] = n_curr[2] + interp * (n_next[2] - n_curr[2]);

//       glNormal3fv(norm);

//       /* Interpolate vertices */
//       v[0] = mdl->header.model_scale[0] *
//                  (pvert1->v[0] + interp * (pvert2->v[0] - pvert1->v[0])) +
//              mdl->header.model_translate[0];
//       v[1] = mdl->header.model_scale[1] *
//                  (pvert1->v[1] + interp * (pvert2->v[1] - pvert1->v[1])) +
//              mdl->header.model_translate[1];
//       v[2] = mdl->header.model_scale[2] *
//                  (pvert1->v[2] + interp * (pvert2->v[2] - pvert1->v[2])) +
//              mdl->header.model_translate[2];

//       glVertex3fv(v);
//     }
//   }
//   glEnd();
// }

void animate(int start, int end, int* frame, float* interp) {
  if ((*frame < start) || (*frame > end)) *frame = start;

  if (*interp >= 1.0f) {
    /* Move to next frame */
    *interp = 0.0f;
    (*frame)++;

    if (*frame >= end) *frame = start;
  }
}

void quake_mdl_init(const char* filename) {
  // GLfloat lightpos[] = {5.0f, 10.0f, 0.0f, 1.0f};

  /* Initialize OpenGL context */
  // glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
  // glShadeModel(GL_SMOOTH);

  // glEnable(GL_DEPTH_TEST);
  // glEnable(GL_TEXTURE_2D);
  // glEnable(GL_LIGHTING);
  // glEnable(GL_LIGHT0);

  // glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

  /* Load MDL quake_model file */
  if (!quake_read_model(filename, &mdlfile)) exit(EXIT_FAILURE);
}

void quake_mdl_cleanup() { quake_free_model(&mdlfile); }

// void reshape(int w, int h) {
//   if (h == 0)
//     h = 1;

//   glViewport(0, 0, (GLsizei)w, (GLsizei)h);

//   glMatrixMode(GL_PROJECTION);
//   glLoadIdentity();
//   gluPerspective(45.0, w / (GLdouble)h, 0.1, 1000.0);

//   glMatrixMode(GL_MODELVIEW);
//   glLoadIdentity();
// }

// void display() {
//   static int n = 0;
//   static float interp = 0.0;
//   static double curent_time = 0;
//   static double last_time = 0;

//   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//   glLoadIdentity();

//   last_time = curent_time;
//   curent_time = (double)glutGet(GLUT_ELAPSED_TIME) / 1000.0;

//   /* Animate quake_model from frames 0 to frames_count-1 */
//   interp += 10 * (curent_time - last_time);
//   animate(0, mdlfile.header.frames_count - 1, &n, &interp);

//   glTranslatef(0.0f, 0.0f, -100.0f);
//   glRotatef(-90.0f, 1.0, 0.0, 0.0);
//   glRotatef(-90.0f, 0.0, 0.0, 1.0);

//   /* Draw the quake_model */
//   if (mdlfile.header.frames_count > 1)
//     RenderFrameItp(n, interp, &mdlfile);
//   else
//     RenderFrame(n, &mdlfile);

//   glutSwapBuffers();
//   glutPostRedisplay();
// }
