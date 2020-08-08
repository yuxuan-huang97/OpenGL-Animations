// OpenGL 1D Shallow Water Equation Simulator
// by Yuxuan Huang

#include <iostream>
#include <fstream>
#include <string>

#include "../../../glad/glad.h"  //Include order can matter here

#include <SDL.h>
#include <SDL_opengl.h>
#include <cstdio>

#define GLM_FORCE_RADIANS
#include "../../../glm/glm.hpp"
#include "../../../glm/gtc/matrix_transform.hpp"
#include "../../../glm/gtc/type_ptr.hpp"

#include "../../Tools/FileLoader.h"
#include "../../Tools/UserControl.h"
#include "ShallowWater.h"

using namespace std;

int screen_width, screen_height;
float aspect; //aspect ratio (needs to be updated if the window is resized)

// Camera Spec
glm::vec3 cam_loc, look_at, up;

//Index of where to model, view, and projection matricies are stored on the GPU
GLint uniModel, uniView, uniProj, uniColor;

// shallow water 1D
//shallow1d water;
shallow1d water(70, 0.1f, 10.f, boundary_condition::free, 10.f, 10.f, 0.f, 50);

GLuint shaderProgram;
GLuint vbo[2];

void init();
void update(float dt);
void draw();
void set_camera();
void perturbation(SDL_Event event);

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
    loadShader(vertexShader, "../FluidSimulation/Shader/vertexshader.txt");

    //Load the default fragment Shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    loadShader(fragmentShader, "../FluidSimulation/Shader/fragmentshader.txt");


    //Join the vertex and fragment shaders together into one program

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glBindFragDataLocation(shaderProgram, 0, "outColor"); // set output
    glLinkProgram(shaderProgram); //run the linker

    glUseProgram(shaderProgram);

    uniModel = glGetUniformLocation(shaderProgram, "model");
    uniView = glGetUniformLocation(shaderProgram, "view");
    uniProj = glGetUniformLocation(shaderProgram, "proj");
    uniColor = glGetUniformLocation(shaderProgram, "inColor");

    //============================ Model Setup ======================================
    //vertices = { 3.0f,0.0f,0.0f ,-3.0f,0.0f,0.0f ,0.0f,0.0f,3.0f };

    //============================ Buffer Setup ======================================

    //Build a Vertex Array Object. This stores the VBO and attribute mappings in one object
    GLuint vao;
    glGenVertexArrays(1, &vao); // VAO for particles
    glBindVertexArray(vao); //Bind the above created VAO to the current context

    //Allocate memory on the graphics card to store geometry (vertex buffer object)
    glGenBuffers(2, vbo);  //Create 1 buffer called vbo

    GLuint ebo; // Element buffer object
    glGenBuffers(1, &ebo);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, water.indices.size() * sizeof(int), &water.indices[0], GL_STATIC_DRAW);
    



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
            perturbation(windowEvent);
            //move_camera(windowEvent, cam_loc, look_at, up, 0.1f, 0.3f);
        }

        // Clear the screen to default color
        glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //if (dt > .1) dt = .1; //Have some max dt
        dt = 0.01f;
        lastTime = SDL_GetTicks() / 1000.f;

        update(dt);

        SDL_GL_SwapWindow(window); //Double buffering
    }

    //Clean Up
    glDeleteProgram(shaderProgram);
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);

    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(2, vbo);
    glDeleteVertexArrays(1, &vao);

    SDL_GL_DeleteContext(context);
    SDL_Quit();
    return 0;
}

void init() {
    screen_width = 800;
    screen_height = 600;

    cam_loc = glm::vec3(11.5f, 0.f, 8.f);
    look_at = glm::vec3(0.0f, 0.0f, 1.0f);
    up = glm::vec3(0.0f, 0.0f, 1.0f);

}

void update(float dt) {
    water.waveUpdate(dt);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, water.vertices.size() * sizeof(float), &water.vertices[0], GL_STREAM_DRAW); //upload vertices to vbo

    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(posAttrib);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, water.normals.size() * sizeof(float), &water.normals[0], GL_STREAM_DRAW); //upload normals to vbo

    GLint normAttrib = glGetAttribLocation(shaderProgram, "inNormal");
    glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normAttrib);

    set_camera();
    draw();
    return;
}

void draw() {
    glm::mat4 model = glm::mat4();
    model = glm::translate(model, glm::vec3(0.0f));
    model = glm::scale(model, glm::vec3(1.0f));
    glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
    //glUniform3f(uniColor, 0.34f, 0.78f, 1.0f);
    glUniform3f(uniColor, 0.0f, 0.0f, 1.0f);
    glDrawElements(GL_TRIANGLES, water.indices.size(), GL_UNSIGNED_INT, (void*)0); //(Primitives, count, type, offset)
}

void set_camera() {
    //Set the Camera view paramters (FOV, aspect ratio, etc.)
    glm::mat4 proj = glm::perspective(3.14f / 4, aspect, .1f, 100.0f); //FOV, aspect, near, far
    glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

    //Set the Camera Position and Orientation
    glm::mat4 view = glm::lookAt(cam_loc, look_at, up); // camera location, look at point, up direction
    glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));
}

void perturbation(SDL_Event event) {
    if (event.type == SDL_KEYUP) {
        switch (event.key.keysym.sym) {
        case SDLK_UP: water.set_uh(water.n / 2, 3.0f); break;
        case SDLK_LEFT: water.set_uh(water.n / 3, 3.0f); break;
        case SDLK_RIGHT: water.set_uh(2 * water.n / 3, 3.0f); break;
        case SDLK_f: water.set_boundary(boundary_condition::free); break;
        case SDLK_p: water.set_boundary(boundary_condition::periodic); break;
        case SDLK_r: water.set_boundary(boundary_condition::reflective); break;
        }
    }
}