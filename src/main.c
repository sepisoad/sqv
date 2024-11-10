#include <SDL2/SDL.h>
#include <stdbool.h>

#include "app.h"
#include "shader.h"
#include "camera.h"

int main(int argc, char* argv[]) {
    app_t app;
    sqv_app_init(&app);

    // SEPI: next step would be to load a MDL file try to render it with the following logic
   
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    float vertices[] = {
        // Positions
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f
    };

    unsigned int indices[] = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        0, 1, 5, 5, 4, 0,
        3, 2, 6, 6, 7, 3,
        0, 3, 7, 7, 4, 0,
        1, 2, 6, 6, 5, 1
    };

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glEnable(GL_DEPTH_TEST);

    // Enable wireframe mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    bool quit = false;
    SDL_Event event;
    float angle = 0.0f;
    float aspectRatio = 800.0f / 600.0f;
    float projection[16];
    createPerspectiveMatrix(projection, 45.0f * (M_PI / 180.0f), aspectRatio, 0.1f, 100.0f);

    while (!quit) {
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
        }

        angle += 0.01f;
        float model[16];
        createRotationMatrix(model, angle);
        float view[16];
        createViewMatrix(view);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUniformMatrix4fv(glGetUniformLocation(app.shader_program, "model"), 1, GL_FALSE, model);
        glUniformMatrix4fv(glGetUniformLocation(app.shader_program, "view"), 1, GL_FALSE, view);
        glUniformMatrix4fv(glGetUniformLocation(app.shader_program, "projection"), 1, GL_FALSE, projection);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        SDL_GL_SwapWindow(app.window);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    sqv_app_clean(&app);

    return 0;
}
