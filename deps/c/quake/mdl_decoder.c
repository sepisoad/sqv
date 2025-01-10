#include "mdl_decoder.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

  // mdl->texture_id =
  //     (uint32_t*)malloc(sizeof(uint32_t) * mdl->header.skins_count);

  mdl->is_kin = 0;

  for (i = 0; i < mdl->header.skins_count; ++i) {
    mdl->skins[i].data = (uint8_t*)malloc(
        sizeof(uint8_t) * mdl->header.skin_width * mdl->header.skin_height);

    fread(&mdl->skins[i].is_grouped, sizeof(int), 1, fp);
    fread(mdl->skins[i].data, sizeof(uint8_t),
          mdl->header.skin_width * mdl->header.skin_height, fp);

    // mdl->texture_id[i] = make_texture_from_skin(i, mdl);

    // TODO: memory leak, try to use raylib here
    // free(mdl->skins[i].data);
    // mdl->skins[i].data = NULL;
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
    // free(mdl->texture_id); //TODO: hmm free only if pointer
    // mdl->texture_id = NULL; //TODO: hmm free only if pointer
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

void animate(int start, int end, int* frame, float* interp) {
  if ((*frame < start) || (*frame > end)) *frame = start;

  if (*interp >= 1.0f) {
    /* Move to next frame */
    *interp = 0.0f;
    (*frame)++;

    if (*frame >= end) *frame = start;
  }
}
