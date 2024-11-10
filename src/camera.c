#include <math.h>

// Function to create a perspective projection matrix
void createPerspectiveMatrix(float* matrix, float fov, float aspect, float near, float far) {
    float tanHalfFov = tanf(fov / 2.0f);
    for (int i = 0; i < 16; ++i) matrix[i] = 0.0f;
    matrix[0] = 1.0f / (aspect * tanHalfFov);
    matrix[5] = 1.0f / tanHalfFov;
    matrix[10] = -(far + near) / (far - near);
    matrix[11] = -1.0f;
    matrix[14] = -(2.0f * far * near) / (far - near);
}

// Function to create a simple rotation matrix around the Y-axis
void createRotationMatrix(float* matrix, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    matrix[0] = c;   matrix[1] = 0.0f; matrix[2] = -s;  matrix[3] = 0.0f;
    matrix[4] = 0.0f; matrix[5] = 1.0f; matrix[6] = 0.0f; matrix[7] = 0.0f;
    matrix[8] = s;   matrix[9] = 0.0f; matrix[10] = c;  matrix[11] = 0.0f;
    matrix[12] = 0.0f; matrix[13] = 0.0f; matrix[14] = 0.0f; matrix[15] = 1.0f;
}

// Function to create a simple view matrix to move the camera back
void createViewMatrix(float* matrix) {
    for (int i = 0; i < 16; ++i) matrix[i] = 0.0f;
    matrix[0] = 1.0f;
    matrix[5] = 1.0f;
    matrix[10] = 1.0f;
    matrix[14] = -5.0f; // Move the camera back along the Z-axis
    matrix[15] = 1.0f;
}