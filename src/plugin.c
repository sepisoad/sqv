#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_POINTS 11

Camera camera;
Vector3 cubePosition;
Vector3 cubeSize;
Ray ray;
RayCollision collision;
Texture texture;
float angle;

void DrawCubeTexture(Texture2D, Vector3, float, float, float, Color);
void DrawCubeTextureRec(Texture2D, Rectangle, Vector3, float, float, float,
                        Color);

void plugin_init() {
  camera = (Camera){0};
  cubePosition = (Vector3){0.0f, 1.0f, 0.0f};
  cubeSize = (Vector3){2.0f, 2.0f, 2.0f};
  ray = (Ray){0};
  collision = (RayCollision){0};

  camera.position = (Vector3){10.0f, 10.0f, 10.0f};
  camera.target = (Vector3){0.0f, 0.0f, 0.0f};
  camera.up = (Vector3){0.0f, 1.0f, 0.0f};
  camera.fovy = 45.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  texture = LoadTexture("res/textures/container2.png");
  angle = 0.0f;
}

void plugin_kill() { UnloadTexture(texture); }

void plugin_main(int screen_width, int screen_height) {
  BeginDrawing();
  ClearBackground(RAYWHITE);
  BeginMode3D(camera);
  DrawCubeTexture(texture, (Vector3){-2.0f, 2.0f, 0.0f}, 2.0f, 4.0f, 2.0f,
                  WHITE);
  DrawCubeTextureRec(texture,
                     (Rectangle){0.0f, texture.height / 2.0f,
                                 texture.width / 2.0f, texture.height / 2.0f},
                     (Vector3){2.0f, 1.0f, 0.0f}, 2.0f, 2.0f, 2.0f, WHITE);
  DrawGrid(10, 1.0f);
  EndMode3D();
  DrawFPS(10, 10);
  EndDrawing();
}

void DrawCubeTexture(Texture2D texture, Vector3 position, float width,
                     float height, float length, Color color) {
  float x = position.x;
  float y = position.y;
  float z = position.z;

  // Set desired texture to be enabled while drawing following vertex data
  rlSetTexture(texture.id);

  // Vertex data transformation can be defined with the commented lines,
  // but in this example we calculate the transformed vertex data directly when
  // calling rlVertex3f()
  // rlPushMatrix();
  // NOTE: Transformation is applied in inverse order (scale -> rotate ->
  // translate)
  // rlTranslatef(2.0f, 0.0f, 0.0f);
  // rlRotatef(45, 0, 1, 0);
  // rlScalef(2.0f, 2.0f, 2.0f);

  rlBegin(RL_QUADS);
  rlColor4ub(color.r, color.g, color.b, color.a);
  // Front Face
  rlNormal3f(0.0f, 0.0f, 1.0f);  // Normal Pointing Towards Viewer
  rlTexCoord2f(0.0f, 0.0f);
  rlVertex3f(x - width / 2, y - height / 2,
             z + length / 2);  // Bottom Left Of The Texture and Quad
  rlTexCoord2f(1.0f, 0.0f);
  rlVertex3f(x + width / 2, y - height / 2,
             z + length / 2);  // Bottom Right Of The Texture and Quad
  rlTexCoord2f(1.0f, 1.0f);
  rlVertex3f(x + width / 2, y + height / 2,
             z + length / 2);  // Top Right Of The Texture and Quad
  rlTexCoord2f(0.0f, 1.0f);
  rlVertex3f(x - width / 2, y + height / 2,
             z + length / 2);  // Top Left Of The Texture and Quad
  // Back Face
  rlNormal3f(0.0f, 0.0f, -1.0f);  // Normal Pointing Away From Viewer
  rlTexCoord2f(1.0f, 0.0f);
  rlVertex3f(x - width / 2, y - height / 2,
             z - length / 2);  // Bottom Right Of The Texture and Quad
  rlTexCoord2f(1.0f, 1.0f);
  rlVertex3f(x - width / 2, y + height / 2,
             z - length / 2);  // Top Right Of The Texture and Quad
  rlTexCoord2f(0.0f, 1.0f);
  rlVertex3f(x + width / 2, y + height / 2,
             z - length / 2);  // Top Left Of The Texture and Quad
  rlTexCoord2f(0.0f, 0.0f);
  rlVertex3f(x + width / 2, y - height / 2,
             z - length / 2);  // Bottom Left Of The Texture and Quad
  // Top Face
  rlNormal3f(0.0f, 1.0f, 0.0f);  // Normal Pointing Up
  rlTexCoord2f(0.0f, 1.0f);
  rlVertex3f(x - width / 2, y + height / 2,
             z - length / 2);  // Top Left Of The Texture and Quad
  rlTexCoord2f(0.0f, 0.0f);
  rlVertex3f(x - width / 2, y + height / 2,
             z + length / 2);  // Bottom Left Of The Texture and Quad
  rlTexCoord2f(1.0f, 0.0f);
  rlVertex3f(x + width / 2, y + height / 2,
             z + length / 2);  // Bottom Right Of The Texture and Quad
  rlTexCoord2f(1.0f, 1.0f);
  rlVertex3f(x + width / 2, y + height / 2,
             z - length / 2);  // Top Right Of The Texture and Quad
  // Bottom Face
  rlNormal3f(0.0f, -1.0f, 0.0f);  // Normal Pointing Down
  rlTexCoord2f(1.0f, 1.0f);
  rlVertex3f(x - width / 2, y - height / 2,
             z - length / 2);  // Top Right Of The Texture and Quad
  rlTexCoord2f(0.0f, 1.0f);
  rlVertex3f(x + width / 2, y - height / 2,
             z - length / 2);  // Top Left Of The Texture and Quad
  rlTexCoord2f(0.0f, 0.0f);
  rlVertex3f(x + width / 2, y - height / 2,
             z + length / 2);  // Bottom Left Of The Texture and Quad
  rlTexCoord2f(1.0f, 0.0f);
  rlVertex3f(x - width / 2, y - height / 2,
             z + length / 2);  // Bottom Right Of The Texture and Quad
  // Right face
  rlNormal3f(1.0f, 0.0f, 0.0f);  // Normal Pointing Right
  rlTexCoord2f(1.0f, 0.0f);
  rlVertex3f(x + width / 2, y - height / 2,
             z - length / 2);  // Bottom Right Of The Texture and Quad
  rlTexCoord2f(1.0f, 1.0f);
  rlVertex3f(x + width / 2, y + height / 2,
             z - length / 2);  // Top Right Of The Texture and Quad
  rlTexCoord2f(0.0f, 1.0f);
  rlVertex3f(x + width / 2, y + height / 2,
             z + length / 2);  // Top Left Of The Texture and Quad
  rlTexCoord2f(0.0f, 0.0f);
  rlVertex3f(x + width / 2, y - height / 2,
             z + length / 2);  // Bottom Left Of The Texture and Quad
  // Left Face
  rlNormal3f(-1.0f, 0.0f, 0.0f);  // Normal Pointing Left
  rlTexCoord2f(0.0f, 0.0f);
  rlVertex3f(x - width / 2, y - height / 2,
             z - length / 2);  // Bottom Left Of The Texture and Quad
  rlTexCoord2f(1.0f, 0.0f);
  rlVertex3f(x - width / 2, y - height / 2,
             z + length / 2);  // Bottom Right Of The Texture and Quad
  rlTexCoord2f(1.0f, 1.0f);
  rlVertex3f(x - width / 2, y + height / 2,
             z + length / 2);  // Top Right Of The Texture and Quad
  rlTexCoord2f(0.0f, 1.0f);
  rlVertex3f(x - width / 2, y + height / 2,
             z - length / 2);  // Top Left Of The Texture and Quad
  rlEnd();
  // rlPopMatrix();

  rlSetTexture(0);
}

// Draw cube with texture piece applied to all faces
void DrawCubeTextureRec(Texture2D texture, Rectangle source, Vector3 position,
                        float width, float height, float length, Color color) {
  float x = position.x;
  float y = position.y;
  float z = position.z;
  float texWidth = (float)texture.width;
  float texHeight = (float)texture.height;

  // Set desired texture to be enabled while drawing following vertex data
  rlSetTexture(texture.id);

  // We calculate the normalized texture coordinates for the desired
  // texture-source-rectangle It means converting from (tex.width, tex.height)
  // coordinates to [0.0f, 1.0f] equivalent
  rlBegin(RL_QUADS);
  rlColor4ub(color.r, color.g, color.b, color.a);

  // Front face
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

  // Back face
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

  // Top face
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

  // Bottom face
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

  // Right face
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

  // Left face
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