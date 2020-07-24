//CSCI 5611 OpenGL Animation Tutorial 
//A 1D bouncing ball
//Modified by Yuxuan Huang into a particle-based interactive water simulation

//Running on Mac OSX
//  Download the SDL2 Framework from here: https://www.libsdl.org/download-2.0.php
//  Open the .dmg and move the file SDL2.Framework into the directory /Library/Frameworks/
//  Make sure you place this cpp file in the same directory with the "glad" folder and the "glm" folder
//  g++ Bounce.cpp glad/glad.c -framework OpenGL -framework SDL2; ./a.out

//Running on Windows
//  Download the SDL2 *Development Libararies* from here: https://www.libsdl.org/download-2.0.php
//  Place SDL2.dll, the 3 .lib files, and the include directory in locations known to MSVC
//  Add both Bounce.cpp and glad/glad.c to the project file
//  Compile and run

//Running on Ubuntu
//  sudo apt-get install libsdl2-2.0-0 libsdl2-dev
//  Make sure you place this cpp file in the same directory with the "glad" folder and the "glm" folder
//  g++ Bounce.cpp glad/glad.c -lGL -lSDL; ./a.out

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

#include "ParticleSystem.h"
#include "../../Tools/FileLoader.h"
#include "../../Tools/ExportTools.h"
#include "../../Tools/UserControl.h"

using namespace std;

int screen_width, screen_height;
float aspect; //aspect ratio (needs to be updated if the window is resized)
bool fullscreen;
bool saveOutput; //Make to true to save out your animation

// Shader sources
const GLchar* vertexSource;
const GLchar* fragmentSource;

// Camera Spec
glm::vec3 cam_loc, look_at, up;

// particle system
ParticleSystem water;

// sphere spec
glm::vec3 sph_loc, sph_color;
float sph_rad;
int sph_vert; // number of vertices for sphere

//Index of where to model, view, and projection matricies are stored on the GPU
GLint uniModel, uniView, uniProj, uniColor;

void init();
void update(float dt, GLint shader1, GLint shader2, GLint vao1, GLint vao2);
void computePhysics(float dt);
void set_camera();
void draw_particles(float dt);
void draw_sphere(float dt);

int main(int argc, char* argv[]) {

    //======================= Initializations and Window Setup =================================

    init();

    SDL_Init(SDL_INIT_VIDEO);  //Initialize Graphics (for OpenGL)

    //Ask SDL to get a recent version of OpenGL (3.2 or greater)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    //Create a window (offsetx, offsety, width, height, flags)
    SDL_Window* window = SDL_CreateWindow("Sphere Water Interaction", 100, 100, screen_width, screen_height, SDL_WINDOW_OPENGL);
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
    loadShader(vertexShader, "../ParticleSystems/Shader/vertexshader.txt");

    //Load the vertex Shader for particles
    GLuint ptc_vertexShader = glCreateShader(GL_VERTEX_SHADER);
    loadShader(ptc_vertexShader, "../ParticleSystems/Shader/ptc_vertexshader.txt");

    //Load the default fragment Shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    loadShader(fragmentShader, "../ParticleSystems/Shader/fragmentshader.txt");

    //Load the fragment Shader for particles
    GLuint ptc_fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    loadShader(ptc_fragmentShader, "../ParticleSystems/Shader/ptc_fragmentshader.txt");


    //Join the vertex and fragment shaders together into one program

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glBindFragDataLocation(shaderProgram, 0, "outColor"); // set output
    glLinkProgram(shaderProgram); //run the linker

    GLuint ptc_shaderProgram = glCreateProgram();
    glAttachShader(ptc_shaderProgram, ptc_vertexShader);
    glAttachShader(ptc_shaderProgram, ptc_fragmentShader);
    glBindFragDataLocation(ptc_shaderProgram, 0, "outColor"); // set output
    glLinkProgram(ptc_shaderProgram); //run the linker

    //============================ Model Setup ======================================

    float ptc_vert[] = {
        // X      Y     Z
          0.0f, 0.0f, 0.0f
    };


    std::vector< float > vertices;
    std::vector< float > uvs; // Won't be used at the moment.
    std::vector< float > normals;

    loadobj("../ParticleSystems/Assets/sphere.obj", vertices, uvs, normals);
    sph_vert = vertices.size();

    //============================ Buffer Setup ======================================

    //Build a Vertex Array Object. This stores the VBO and attribute mappings in one object
    GLuint vao, vao_sph;
    glGenVertexArrays(1, &vao); //Create a VAO for particles
    glGenVertexArrays(1, &vao_sph); // VAO for sphere object


    glBindVertexArray(vao); //Bind the first VAO to the current context

    //Allocate memory on the graphics card to store geometry (vertex buffer object)
    GLuint vbo;
    glGenBuffers(1, &vbo);  //Create 1 buffer called vbo

    glBindBuffer(GL_ARRAY_BUFFER, vbo); //Set the vbo as the active array buffer (Only one buffer can be active at a time)
    glBufferData(GL_ARRAY_BUFFER, sizeof(ptc_vert), ptc_vert, GL_STATIC_DRAW); //upload vertices to vbo
    //GL_STATIC_DRAW means we won't change the geometry, GL_DYNAMIC_DRAW = geometry changes infrequently
    //GL_STREAM_DRAW = geom. changes frequently.  This effects which types of GPU memory is used

    //Tell OpenGL how to set fragment shader input (for particles)
    GLint posAttrib = glGetAttribLocation(ptc_shaderProgram, "position");
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    //Attribute, vals/attrib., type, normalized?, stride, offset
    //Binds to VBO current GL_ARRAY_BUFFER 
    glEnableVertexAttribArray(posAttrib);


    glBindVertexArray(vao_sph);//Bind the second VAO to the current context

    GLuint vbo_sph[2];
    glGenBuffers(2, vbo_sph);  //Create 1 buffer called vbo
    glBindBuffer(GL_ARRAY_BUFFER, vbo_sph[0]);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
    //Tell OpenGL how to set fragment shader input (for sphere)
    GLint sph_posAttrib = glGetAttribLocation(shaderProgram, "position");
    glVertexAttribPointer(sph_posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    //Attribute, vals/attrib., type, normalized?, stride, offset
    //Binds to VBO current GL_ARRAY_BUFFER 
    glEnableVertexAttribArray(sph_posAttrib);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_sph[1]);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), &normals[0], GL_STATIC_DRAW);

    GLint sph_normalAttrib = glGetAttribLocation(shaderProgram, "inNormal");
    glVertexAttribPointer(sph_normalAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glEnableVertexAttribArray(sph_normalAttrib);


    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glDisable(GL_CULL_FACE);


    //Event Loop (Loop forever processing each event as fast as possible)
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
            if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_f) //If "f" is pressed
                fullscreen = !fullscreen;
            SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0); //Set to full screen 
            move_obj(windowEvent, sph_loc, look_at - cam_loc, up, 0.2);
        }

        // Clear the screen to default color
        glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (!saveOutput) dt = (SDL_GetTicks() / 1000.f) - lastTime;
        if (dt > .1) dt = .1; //Have some max dt
        lastTime = SDL_GetTicks() / 1000.f;
        if (saveOutput) dt += .07; //Fix framerate at 14 FPS

        update(dt, ptc_shaderProgram, shaderProgram, vao, vao_sph);
        printf("FPS: %i \n", int(1 / dt));

        if (saveOutput) Win2PPM(screen_width, screen_height);


        SDL_GL_SwapWindow(window); //Double buffering
    }

    //Clean Up
    glDeleteProgram(ptc_shaderProgram);
    glDeleteShader(ptc_fragmentShader);
    glDeleteShader(ptc_vertexShader);

    glDeleteProgram(shaderProgram);
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);

    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, vbo_sph);

    glDeleteVertexArrays(1, &vao);
    glDeleteVertexArrays(1, &vao_sph);

    SDL_GL_DeleteContext(context);
    SDL_Quit();
    return 0;
}

void init() {
    screen_width = 800;
    screen_height = 600;
    saveOutput = false;
    fullscreen = false;

    cam_loc = glm::vec3(20.f, 20.f, 1.8f);
    look_at = glm::vec3(0.0f, 0.0f, 0.0f);
    up = glm::vec3(0.0f, 0.0f, 1.0f);

    water = ParticleSystem(5000, 3.0f, 0.0f, 150000, glm::vec3(0, 5, 5), 1.0f, axis::X, 10.0f, 10.0f, glm::vec3(0.4f, 0.9f, 1.0f));

    sph_loc = glm::vec3(0.0f, 0.0f, 0.0f);
    sph_rad = 1.0f;
    sph_color = glm::vec3(1.0f, 0.0f, 0.0f);
};

void update(float dt, GLint shader1, GLint shader2, GLint vao1, GLint vao2) {

    set_camera();

    //Where to model, view, and projection matricies are stored on the GPU
    uniModel = glGetUniformLocation(shader1, "model");
    uniView = glGetUniformLocation(shader1, "view");
    uniProj = glGetUniformLocation(shader1, "proj");
    glUseProgram(shader1); //Set the particle shader to active
    glBindVertexArray(vao1);
    draw_particles(dt);

    set_camera();
    uniModel = glGetUniformLocation(shader2, "model");
    uniView = glGetUniformLocation(shader2, "view");
    uniProj = glGetUniformLocation(shader2, "proj");
    glUseProgram(shader2);
    glBindVertexArray(vao2);
    draw_sphere(dt);
    computePhysics(dt);
}

void computePhysics(float dt) {
    water.update(dt, ParticleSystem::particle_type::fluid, sph_loc, sph_rad);
    printf("Particle Count: %i \n", water.Pos.size());
}

void set_camera() {
    //Set the Camera view paramters (FOV, aspect ratio, etc.)
    glm::mat4 proj = glm::perspective(3.14f / 4, aspect, .1f, 100.0f); //FOV, aspect, near, far
    glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

    //Set the Camera Position and Orientation
    glm::mat4 view = glm::lookAt(cam_loc, look_at, up); // camera location, look at point, up direction
    glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

    glPointSize(8.0f);
}

void draw_particles(float dt) {
    // iterate through all particles
    for (int i = 0; i < water.Pos.size(); i++) {
        glm::mat4 model = glm::mat4();
        model = glm::translate(model, water.Pos[i]);
        model = glm::scale(model, glm::vec3(1));
        glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3f(uniColor, water.Clr[i].r, water.Clr[i].g, water.Clr[i].b);
        glDrawArrays(GL_POINTS, 0, 1); //(Primitives, Which VBO, Number of vertices)
    }

}

void draw_sphere(float dt) {
    glm::mat4 model = glm::mat4();
    model = glm::translate(model, sph_loc);
    model = glm::scale(model, glm::vec3(sph_rad));
    glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3f(uniColor, sph_color.r, sph_color.g, sph_color.b);
    glDrawArrays(GL_TRIANGLES, 0, sph_vert / 3); //(Primitives, Which VBO, Number of vertices)
}