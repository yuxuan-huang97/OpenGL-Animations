// OpenGL Cloth Simulation
// by Yuxuan Huang

#include <iostream>
#include <fstream>
#include <string>

#include "../../../glad/glad.h"  //Include order can matter here
#ifdef __APPLE__
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#else
#include <SDL.h>
#include <SDL_opengl.h>
#endif
#include <cstdio>

#define GLM_FORCE_RADIANS
#include "../../../glm/glm.hpp"
#include "../../../glm/gtc/matrix_transform.hpp"
#include "../../../glm/gtc/type_ptr.hpp"

#include "../../Tools/FileLoader.h"
#include "../../Tools/UserControl.h"
#include "Cloth.h"

using namespace std;

int screen_width, screen_height;
float aspect; //aspect ratio (needs to be updated if the window is resized)

// Camera Spec
glm::vec3 cam_loc, look_at, up;

//Index of where to model, view, and projection matricies are stored on the GPU
GLint uniModel, uniView, uniProj, uniColor;

// cloth specs
Cloth cloth;
vector<int> index;

void init();
void update(float dt, GLuint ebo);
void draw_cloth();
void set_camera();

int main(int argc, char* args[]) {

    //======================= Initializations and Window Setup =================================
    init();

    SDL_Init(SDL_INIT_VIDEO);  //Initialize Graphics (for OpenGL)

    //Ask SDL to get a recent version of OpenGL (3.2 or greater)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    //Create a window (offsetx, offsety, width, height, flags)
    SDL_Window* window = SDL_CreateWindow("Cloth Simulation", 100, 100, screen_width, screen_height, SDL_WINDOW_OPENGL);
    aspect = screen_width / (float)screen_height; //aspect ratio (needs to be updated if the window is resized)

    //The above window cannot be resized which makes some code slightly easier.
    //Below show how to make a full screen window or allow resizing
    //SDL_Window* window = SDL_CreateWindow("My OpenGL Program", 0, 0, screen_width, screen_height, SDL_WINDOW_FULLSCREEN|SDL_WINDOW_OPENGL);
    //SDL_Window* window = SDL_CreateWindow("My OpenGL Program", 100, 100, screen_width, screen_height, SDL_WINDOW_RESIZABLE|SDL_WINDOW_OPENGL);
    //SDL_Window* window = SDL_CreateWindow("My OpenGL Program",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,0,0,SDL_WINDOW_FULLSCREEN_DESKTOP|SDL_WINDOW_OPENGL); //Boarderless window "fake" full screen

    //Create a context to draw in
    SDL_GLContext context = SDL_GL_CreateContext(window);

    if (gladLoadGLLoader(SDL_GL_GetProcAddress)) {
        printf("\nOpenGL loaded\n");
        printf("Vendor:   %s\n", glGetString(GL_VENDOR));
        printf("Renderer: %s\n", glGetString(GL_RENDERER));
        printf("Version:  %s\n\n", glGetString(GL_VERSION));
    }
    else {
        printf("ERROR: Failed to initialize OpenGL context.\n");
        return -1;
    }

    //============================ Shader Setup ======================================

    //Load the default vertex Shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    loadShader(vertexShader, "../ClothSimulation/Shader/vertexshader.txt");

    //Load the default fragment Shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    loadShader(fragmentShader, "../ClothSimulation/Shader/fragmentshader.txt");


    //Join the vertex and fragment shaders together into one program

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glBindFragDataLocation(shaderProgram, 0, "outColor"); // set output
    glLinkProgram(shaderProgram); //run the linker

    glUseProgram(shaderProgram);

    uniModel = glGetUniformLocation(shaderProgram, "model");
    uniView = glGetUniformLocation(shaderProgram, "view");
    uniProj = glGetUniformLocation(shaderProgram, "proj");
    uniColor = glGetUniformLocation(shaderProgram, "inColor");

    //============================ Buffer Setup ======================================

    //Build a Vertex Array Object. This stores the VBO and attribute mappings in one object
    GLuint vao;
    glGenVertexArrays(1, &vao); // VAO for particles
    glBindVertexArray(vao); //Bind the above created VAO to the current context

    //Allocate memory on the graphics card to store geometry (vertex buffer object)
    GLuint vbo;
    glGenBuffers(1, &vbo);  //Create 1 buffer called vbo

    glBindBuffer(GL_ARRAY_BUFFER, vbo); //Set the vbo as the active array buffer (Only one buffer can be active at a time)
    glBufferData(GL_ARRAY_BUFFER, sizeof(cloth.pos), &cloth.pos[0], GL_STREAM_DRAW); //upload vertices to vbo
    //GL_STATIC_DRAW means we won't change the geometry, GL_DYNAMIC_DRAW = geometry changes infrequently
    //GL_STREAM_DRAW = geom. changes frequently.  This effects which types of GPU memory is used

    GLuint ebo; // Element buffer object
    glGenBuffers(1, &ebo);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ARRAY_BUFFER, index.size() * sizeof(int), &index[0], GL_STREAM_DRAW);


    //Tell OpenGL how to set fragment shader input (for particles)
    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    //Attribute, vals/attrib., type, normalized?, stride, offset
    //Binds to VBO current GL_ARRAY_BUFFER 
    glEnableVertexAttribArray(posAttrib);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    SDL_Event windowEvent;
    bool quit = false;
    float lastTime = SDL_GetTicks() / 1000.f;
    float dt = 0;
    while (!quit) {
        while (SDL_PollEvent(&windowEvent)) {
            if (windowEvent.type == SDL_QUIT) quit = true; //Exit event loop
          //List of keycodes: https://wiki.libsdl.org/SDL_Keycode - You can catch many special keys
          //Scancode referes to a keyboard position, keycode referes to the letter (e.g., EU keyboards)
            if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE)
                quit = true; //Exit event loop
            move_camera(windowEvent, cam_loc, look_at, up, 0.1f, 0.3f);
        }
        bound_rotate(window, cam_loc, look_at, 0.01f);

        // Clear the screen to default color
        glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (dt > .1) dt = .1; //Have some max dt
        lastTime = SDL_GetTicks() / 1000.f;

        update(dt, ebo);

        SDL_GL_SwapWindow(window); //Double buffering
    }

    //Clean Up

    SDL_GL_DeleteContext(context);
    SDL_Quit();
    return 0;
}

void init() {
    screen_width = 800;
    screen_height = 600;

    cloth = Cloth();
    index = cloth.index();
}

void update(float dt, GLuint ebo) {
    set_camera();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    draw_cloth();
    return;
}

void draw_cloth() {
    glm::mat4 model = glm::mat4();
    model = glm::translate(model, glm::vec3(0.0f));
    model = glm::scale(model, glm::vec3(1.0f));
    glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3f(uniColor, 1.0f, 0.0f, 0.0f);
    glDrawElements(GL_TRIANGLES, index.size(), GL_UNSIGNED_INT, (void*)0); //(Primitives, count, type, offset)
}

void set_camera() {
    //Set the Camera view paramters (FOV, aspect ratio, etc.)
    glm::mat4 proj = glm::perspective(3.14f / 4, aspect, .1f, 100.0f); //FOV, aspect, near, far
    glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

    //Set the Camera Position and Orientation
    glm::mat4 view = glm::lookAt(cam_loc, look_at, up); // camera location, look at point, up direction
    glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));
}