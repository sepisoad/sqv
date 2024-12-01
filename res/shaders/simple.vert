#version 410 core

layout (location = 0) in vec3 aPos;    // Vertex position
layout (location = 1) in vec3 aColor;  // Vertex color

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vertexColor;  // Pass color to fragment shader

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vertexColor = aColor;  // Pass through color
}
