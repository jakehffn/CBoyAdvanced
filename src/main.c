#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef __EMSCRIPTEN__

#include <emscripten.h>
#include <GLES3/gl3.h>
#include <GLES3/gl2ext.h>

#else 
// GLEW must come before OpenGL
#include <GL\glew.h>

#endif

#include <SDL.h>
#include <SDL_opengl.h>

#include "video/window.h"
#include "video/shader.h"

#define WIDTH 160
#define HEIGHT 144

const char* vertex_shader_source = "#version 300 es\n"
    "precision highp float;\n"
    "layout (location = 0) in vec4 vertex;\n"  // <vec2 position, vec2 texCoords>
    "out vec2 TexCoords;\n"
    "void main() {\n"
        "TexCoords = vertex.zw;\n"
        "gl_Position = vec4(2.0f*vertex.x - 1.0f, 1.0f - 2.0f*vertex.y, 0.0, 1.0);\n"
    "}\0";

const char* fragment_shader_source = "#version 300 es\n"
    "precision highp float;\n"
    "in vec2 TexCoords;\n"
    "out vec4 color;\n"
    "uniform sampler2D screenTexture;\n"
    "void main() {\n"
        "color = vec4(texture(screenTexture, vec2(TexCoords.x, TexCoords.y)).rgb, 1.0f);\n"
        // "color = vec4(1.0f*TexCoords.x, 0.6f*TexCoords.y, 1.0f, 1.0f);\n"
    "}\0";

const float quadVertexData[] = { 
        // pos      // tex
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 
    
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f
};

Window window = NULL;
Shader shader = NULL;
char texture_data[WIDTH*HEIGHT*4];
GLuint VAO = -1;
GLuint screen_texture = -1;

void createScreenTexture() {

    for (int xx = 0; xx < WIDTH; xx++) {

        for (int yy = 0; yy < HEIGHT; yy++) {

            int cell_start = (yy*WIDTH + xx)*3;

            texture_data[cell_start] = 0x00;
            texture_data[cell_start + 1] = 0x00;
            texture_data[cell_start + 2] = 0x00;

            if (xx/8 % 3 == 0) {
                texture_data[cell_start] = 0xFF;
            }

            if (xx/8 % 3 == 1) {
                texture_data[cell_start+1] = 0xFF;
            }

            if (xx/8 % 3 == 2) {
                texture_data[cell_start+2] = 0xFF;
            }
        }
    }

    glGenTextures(1, &screen_texture);
    glBindTexture(GL_TEXTURE_2D, screen_texture);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, texture_data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  
}

void createQuadVAO() {

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    // The verticies will never change so the buffer ID is not saved
    GLuint vertexBuffer;
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertexData), quadVertexData, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    // Free bound buffers
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0); 
}

void drawToCanvas() {

    // shader = createShader(vertex_shader_source, fragment_shader_source);
    // compileShader(shader);
    glClear(GL_COLOR_BUFFER_BIT);
    useShader(shader);

    glBindVertexArray(VAO);
    glBindTexture(GL_TEXTURE_2D, screen_texture);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    // glBindVertexArray(0);
    // glUseProgram(0);
    swapWindow(window);

    // destroyShader(shader);
}

int main(int argv, char** args) {

    window = createWindow(WIDTH, HEIGHT);

    printf("Here 1\n");

    if (!initWindow(window)) {

        printf("Failed to initialize SDL\n");

    } else {


        printf("SDL initialized\n");

        initShader(&shader, vertex_shader_source, fragment_shader_source);
        createScreenTexture();

        glClearColor(0.5, 0.2, 0.5, 1.0);
        createQuadVAO();
        
        #ifdef __EMSCRIPTEN__

            printf("On the web!\n");

            emscripten_set_main_loop(drawToCanvas, 0, 1);

        #else

            bool quit = false;
            SDL_Event event;

            while (!quit) {

                // Poll for events
                while (SDL_PollEvent(&event)) {
                    switch (event.type) {
                        case SDL_QUIT:
                            // User clicked the close button, exit loop
                            printf("Quit event received\n");
                            quit = true;
                            break;

                        case SDL_WINDOWEVENT:

                            if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                                glViewport(0, 0, event.window.data1, event.window.data2);
                            }
                            break;

                        case SDL_KEYDOWN:
                            // User pressed a key
                            printf("Key pressed: %s\n", SDL_GetKeyName(event.key.keysym.sym));
                            break;

                        case SDL_MOUSEBUTTONDOWN:
                            // User clicked the mouse button
                            printf("Mouse button %d clicked at (%d,%d)\n", event.button.button, event.button.x, event.button.y);
                            break;
                            
                        default:
                            // Ignore other events
                            break;
                    }
                }

                // printf("Loop\n");

                drawToCanvas();
            }

        #endif
    }  

    printf("Program end\n");

    glDeleteBuffers(1, &VAO);
    glDeleteTextures(1, &screen_texture);

    destroyWindow(window);
    destroyShader(shader);

    // free(shader);
    // free(window);

    return 0;
}