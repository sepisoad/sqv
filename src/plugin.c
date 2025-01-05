#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_POINTS 11

Camera camera;
Vector2 cubeScreenPosition;
Vector3 cubePosition;
Texture texture;
bool is_cursor_enable;
float angle;

void DrawCubeTexture(Texture2D, Vector3, float, float, float);
void DrawCubeTextureRec(Texture2D, Rectangle, Vector3, float, float, float);

void plugin_init() {
  camera = (Camera){0};

  cubePosition = (Vector3){0.0f, 1.0f, 0.0f};
  cubeScreenPosition = (Vector2){0.0f, 0.0f};

  camera.position = (Vector3){10.0f, 10.0f, 10.0f};
  camera.target = (Vector3){0.0f, 0.0f, 0.0f};
  camera.up = (Vector3){0.0f, 1.0f, 0.0f};
  camera.fovy = 45.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  texture = LoadTexture("res/textures/container2.png");

  angle = 0.0f;
  is_cursor_enable = true;
  // EnableCursor();
  //
}

void plugin_kill() { UnloadTexture(texture); }

void plugin_main(int screen_width, int screen_height) {
  if (IsKeyReleased(KEY_ESCAPE)) {
    is_cursor_enable = !is_cursor_enable;
    if (is_cursor_enable) {
      EnableCursor();

    } else {
      DisableCursor();
    }
  }

  UpdateCamera(&camera, CAMERA_THIRD_PERSON);
  cubeScreenPosition = GetWorldToScreen(
      (Vector3){cubePosition.x, cubePosition.y + 2.5f, cubePosition.z}, camera);

  BeginDrawing();
  ClearBackground(RAYWHITE);
  BeginMode3D(camera);
  DrawCubeTexture(texture, (Vector3){0.0f, 0.0f, 0.0f}, 4.0f, 4.0f, 4.0f);
  // DrawCubeTextureRec(
  //     texture, (Rectangle){0.0f, texture.height, texture.width,
  //     texture.height}, (Vector3){0.0f, 0.0f, 0.0f}, 4.0f, 4.0f, 4.0f);
  DrawGrid(10, 1.0f);
  EndMode3D();
  DrawFPS(10, 10);
  EndDrawing();
}

void DrawCubeTexture(Texture2D texture, Vector3 position, float width,
                     float height, float length) {
  Color color = GREEN;
  float x = position.x;
  float y = position.y;
  float z = position.z;

  rlSetTexture(texture.id);

  rlBegin(RL_QUADS);
  rlColor4ub(color.r, color.g, color.b, color.a);

  rlNormal3f(0.0f, 0.0f, 1.0f);
  rlTexCoord2f(0.0f, 0.0f);
  rlVertex3f(x - width / 2, y - height / 2, z + length / 2);
  rlTexCoord2f(1.0f, 0.0f);
  rlVertex3f(x + width / 2, y - height / 2, z + length / 2);
  rlTexCoord2f(1.0f, 1.0f);
  rlVertex3f(x + width / 2, y + height / 2, z + length / 2);
  rlTexCoord2f(0.0f, 1.0f);
  rlVertex3f(x - width / 2, y + height / 2, z + length / 2);

  rlNormal3f(0.0f, 0.0f, -1.0f);
  rlTexCoord2f(1.0f, 0.0f);
  rlVertex3f(x - width / 2, y - height / 2, z - length / 2);
  rlTexCoord2f(1.0f, 1.0f);
  rlVertex3f(x - width / 2, y + height / 2, z - length / 2);
  rlTexCoord2f(0.0f, 1.0f);
  rlVertex3f(x + width / 2, y + height / 2, z - length / 2);
  rlTexCoord2f(0.0f, 0.0f);
  rlVertex3f(x + width / 2, y - height / 2, z - length / 2);

  rlNormal3f(0.0f, 1.0f, 0.0f);
  rlTexCoord2f(0.0f, 1.0f);
  rlVertex3f(x - width / 2, y + height / 2, z - length / 2);
  rlTexCoord2f(0.0f, 0.0f);
  rlVertex3f(x - width / 2, y + height / 2, z + length / 2);
  rlTexCoord2f(1.0f, 0.0f);
  rlVertex3f(x + width / 2, y + height / 2, z + length / 2);
  rlTexCoord2f(1.0f, 1.0f);
  rlVertex3f(x + width / 2, y + height / 2, z - length / 2);

  rlNormal3f(0.0f, -1.0f, 0.0f);
  rlTexCoord2f(1.0f, 1.0f);
  rlVertex3f(x - width / 2, y - height / 2, z - length / 2);
  rlTexCoord2f(0.0f, 1.0f);
  rlVertex3f(x + width / 2, y - height / 2, z - length / 2);
  rlTexCoord2f(0.0f, 0.0f);
  rlVertex3f(x + width / 2, y - height / 2, z + length / 2);
  rlTexCoord2f(1.0f, 0.0f);
  rlVertex3f(x - width / 2, y - height / 2, z + length / 2);

  rlNormal3f(1.0f, 0.0f, 0.0f);
  rlTexCoord2f(1.0f, 0.0f);
  rlVertex3f(x + width / 2, y - height / 2, z - length / 2);
  rlTexCoord2f(1.0f, 1.0f);
  rlVertex3f(x + width / 2, y + height / 2, z - length / 2);
  rlTexCoord2f(0.0f, 1.0f);
  rlVertex3f(x + width / 2, y + height / 2, z + length / 2);
  rlTexCoord2f(0.0f, 0.0f);
  rlVertex3f(x + width / 2, y - height / 2, z + length / 2);

  rlNormal3f(-1.0f, 0.0f, 0.0f);
  rlTexCoord2f(0.0f, 0.0f);
  rlVertex3f(x - width / 2, y - height / 2, z - length / 2);
  rlTexCoord2f(1.0f, 0.0f);
  rlVertex3f(x - width / 2, y - height / 2, z + length / 2);
  rlTexCoord2f(1.0f, 1.0f);
  rlVertex3f(x - width / 2, y + height / 2, z + length / 2);
  rlTexCoord2f(0.0f, 1.0f);
  rlVertex3f(x - width / 2, y + height / 2, z - length / 2);
  rlEnd();

  rlSetTexture(0);
}

// Draw cube with texture piece applied to all faces
void DrawCubeTextureRec(Texture2D texture, Rectangle source, Vector3 position,
                        float width, float height, float length) {
  Color color = WHITE;
  float x = position.x;
  float y = position.y;
  float z = position.z;
  float texWidth = (float)texture.width;
  float texHeight = (float)texture.height;

  rlSetTexture(texture.id);

  rlBegin(RL_QUADS);
  rlColor4ub(color.r, color.g, color.b, color.a);

  rlNormal3f(0.0f, 0.0f, 1.0f);
  rlTexCoord2f(source.x / texWidth, (source.y + source.height) / texHeight);
  rlVertex3f(x - width / 2, y - height / 2, z + length / 2);
  rlTexCoord2f((source.x + source.width) / texWidth,
               (source.y + source.height) / texHeight);
  rlVertex3f(x + width / 2, y - height / 2, z + length / 2);
  rlTexCoord2f((source.x + source.width) / texWidth, source.y / texHeight);
  rlVertex3f(x + width / 2, y + height / 2, z + length / 2);
  rlTexCoord2f(source.x / texWidth, source.y / texHeight);
  rlVertex3f(x - width / 2, y + height / 2, z + length / 2);

  rlNormal3f(0.0f, 0.0f, -1.0f);
  rlTexCoord2f((source.x + source.width) / texWidth,
               (source.y + source.height) / texHeight);
  rlVertex3f(x - width / 2, y - height / 2, z - length / 2);
  rlTexCoord2f((source.x + source.width) / texWidth, source.y / texHeight);
  rlVertex3f(x - width / 2, y + height / 2, z - length / 2);
  rlTexCoord2f(source.x / texWidth, source.y / texHeight);
  rlVertex3f(x + width / 2, y + height / 2, z - length / 2);
  rlTexCoord2f(source.x / texWidth, (source.y + source.height) / texHeight);
  rlVertex3f(x + width / 2, y - height / 2, z - length / 2);

  rlNormal3f(0.0f, 1.0f, 0.0f);
  rlTexCoord2f(source.x / texWidth, source.y / texHeight);
  rlVertex3f(x - width / 2, y + height / 2, z - length / 2);
  rlTexCoord2f(source.x / texWidth, (source.y + source.height) / texHeight);
  rlVertex3f(x - width / 2, y + height / 2, z + length / 2);
  rlTexCoord2f((source.x + source.width) / texWidth,
               (source.y + source.height) / texHeight);
  rlVertex3f(x + width / 2, y + height / 2, z + length / 2);
  rlTexCoord2f((source.x + source.width) / texWidth, source.y / texHeight);
  rlVertex3f(x + width / 2, y + height / 2, z - length / 2);

  rlNormal3f(0.0f, -1.0f, 0.0f);
  rlTexCoord2f((source.x + source.width) / texWidth, source.y / texHeight);
  rlVertex3f(x - width / 2, y - height / 2, z - length / 2);
  rlTexCoord2f(source.x / texWidth, source.y / texHeight);
  rlVertex3f(x + width / 2, y - height / 2, z - length / 2);
  rlTexCoord2f(source.x / texWidth, (source.y + source.height) / texHeight);
  rlVertex3f(x + width / 2, y - height / 2, z + length / 2);
  rlTexCoord2f((source.x + source.width) / texWidth,
               (source.y + source.height) / texHeight);
  rlVertex3f(x - width / 2, y - height / 2, z + length / 2);

  rlNormal3f(1.0f, 0.0f, 0.0f);
  rlTexCoord2f((source.x + source.width) / texWidth,
               (source.y + source.height) / texHeight);
  rlVertex3f(x + width / 2, y - height / 2, z - length / 2);
  rlTexCoord2f((source.x + source.width) / texWidth, source.y / texHeight);
  rlVertex3f(x + width / 2, y + height / 2, z - length / 2);
  rlTexCoord2f(source.x / texWidth, source.y / texHeight);
  rlVertex3f(x + width / 2, y + height / 2, z + length / 2);
  rlTexCoord2f(source.x / texWidth, (source.y + source.height) / texHeight);
  rlVertex3f(x + width / 2, y - height / 2, z + length / 2);

  rlNormal3f(-1.0f, 0.0f, 0.0f);
  rlTexCoord2f(source.x / texWidth, (source.y + source.height) / texHeight);
  rlVertex3f(x - width / 2, y - height / 2, z - length / 2);
  rlTexCoord2f((source.x + source.width) / texWidth,
               (source.y + source.height) / texHeight);
  rlVertex3f(x - width / 2, y - height / 2, z + length / 2);
  rlTexCoord2f((source.x + source.width) / texWidth, source.y / texHeight);
  rlVertex3f(x - width / 2, y + height / 2, z + length / 2);
  rlTexCoord2f(source.x / texWidth, source.y / texHeight);
  rlVertex3f(x - width / 2, y + height / 2, z - length / 2);

  rlEnd();

  rlSetTexture(0);
}