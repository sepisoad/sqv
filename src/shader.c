#include <stdlib.h>
#include <stdio.h>
#include <GL/glew.h>

// Function to read a file's contents into a string
static char* readFile(const char* filePath) {
    FILE* file = fopen(filePath, "r");
    if (!file) {
        printf("ERROR: Could not open file %s\n", filePath);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(length + 1);
    if (!buffer) {
        printf("ERROR: Memory allocation failed for file %s\n", filePath);
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, length, file);
    buffer[length] = '\0'; // Null-terminate the string
    fclose(file);
    return buffer;
}

// Function to compile a shader from a file
static GLuint compileShaderFromFile(GLenum type, const char* filePath) {
    char* source = readFile(filePath);
    if (!source) return 0;

    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, (const char**)&source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("ERROR: Shader Compilation Failed\n%s\n", infoLog);
    }

    free(source); // Free the file contents after use
    return shader;
}

// Function to create the shader program from files
GLuint createShaderProgramFromFiles(const char* vertex_path, const char* fragment_path) {
    GLuint vertex_shader = compileShaderFromFile(GL_VERTEX_SHADER, vertex_path);
    GLuint fragment_shader = compileShaderFromFile(GL_FRAGMENT_SHADER, fragment_path);

    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    GLint success;
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shader_program, 512, NULL, infoLog);
        printf("ERROR: Shader Program Linking Failed\n%s\n", infoLog);
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return shader_program;
}