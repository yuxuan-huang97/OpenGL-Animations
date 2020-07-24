//CSCI 5611 OpenGL Animation Tutorial 
//A 1D bouncing ball
//Modified by Yuxuan Huang into a particle-based firework simulation

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

// uvsphere and icosphere spec
float sph_rad, ico_rad;
int sph_vert, ico_vert; // number of vertices for sphere and environment

//Index of where to model, view, and projection matricies are stored on the GPU
GLint uniModel, uniView, uniProj, uniColor;

void init();
void update(float dt);
void computePhysics(float dt);
void set_camera();
void draw_particles(float dt);
void draw_sphere(float dt);
void draw_ico(float dt);

class firework {
public:
    glm::vec3 position;
    int stage; // 0 for launch, 1 for explosion

    vector<glm::vec3> sphere_loc; // sphere for larger particles
    vector<glm::vec3> ico_loc; // icosphere for smaller particles (tails)
    vector<glm::vec3> sphere_vel; // velocity of the spheres
    vector<glm::vec3> sphere_col; // color of the spheres

    vector<float> sphere_life; // life of the spheres
    vector<float> sphere_life_ini; // initial life of the spheres (only used in the explosion stage)

    firework() { // default firework
        position = glm::vec3(0.0f, 0.0f, 0.0f);
        stage = 0;
        sphere_loc.push_back(position);
        sphere_vel.push_back(glm::vec3(0.0f, 0.0f, 10.0f)); // straight up
        sphere_col.push_back(glm::vec3(1.0f, 1.0f, 1.0f)); // white

        expl_count = 50;
        r_life = 1.2f;
        p_life = 1.0f;
        color_type = int(3 * static_cast <float> (rand()) / static_cast <float> (RAND_MAX));

        sphere_life.push_back(r_life);
    }

    firework(glm::vec3 pos, float rlife, float plife, int explc) { // customized firework
        position = pos;
        stage = 0;
        sphere_loc.push_back(position);
        sphere_vel.push_back(glm::vec3(0.0f + noise(0.1f), 0.0f + noise(0.1f), 10.0f + noise(0.1f)));
        sphere_col.push_back(glm::vec3(1.0f, 1.0f, 1.0f));

        expl_count = explc;
        r_life = rlife;
        p_life = plife;
        color_type = int(3 * static_cast <float> (rand()) / static_cast <float> (RAND_MAX));

        sphere_life.push_back(rlife);
    }

    void update(float dt) {
        for (int i = 0; i < sphere_loc.size(); i++) {
            sphere_loc[i] += (sphere_vel[i] * dt);
            sphere_vel[i].z += (g * dt);
            sphere_life[i] -= dt;
        }
        // transition to explosion stage
        if (stage == 0) {
            if (sphere_life[0] <= 0) {
                explode();
            }
        }
    }

private:
    float g = -9.8f;
    int expl_count; // number of particles generated in explosion
    float r_life; // rocket life
    float p_life; // particle life

    int color_type;

    // explosion creates a number of smaller spheres
    void explode() {
        stage = 1; // switch to explosion stage

        for (int i = 0; i < expl_count; i++) {
            sphere_loc.push_back(sphere_loc[0]);
            sphere_vel.push_back(5.0f * sample_vel());
            sphere_life.push_back(p_life + noise(0.1f));
        }
        generate_color();
        // delete the first item (the original sphere) in each vector
        sphere_loc[0] = sphere_loc[expl_count];
        sphere_loc.pop_back();
        sphere_vel[0] = sphere_vel[expl_count];
        sphere_vel.pop_back();
        sphere_col[0] = sphere_col[expl_count];
        sphere_col.pop_back();
        sphere_life[0] = sphere_life[expl_count];
        sphere_life.pop_back();
    }

    // generate random float in [-0.5, +0.5] * scale
    float noise(float scale) {
        return scale * static_cast <float> (rand()) / static_cast <float> (RAND_MAX) - 0.5;
    }

    // sample velocity on a sphere
    glm::vec3 sample_vel() {
        float theta = 2 * M_PI * static_cast <float> (rand()) / static_cast <float> (RAND_MAX);  // 0 - 2PI
        float phi = M_PI * static_cast <float> (rand()) / static_cast <float> (RAND_MAX); // 0 - PI
        float noise = 1 - (0.25f * static_cast <float> (rand()) / static_cast <float> (RAND_MAX)); // 0.75 - 1
        return noise * glm::vec3(cos(theta) * sin(phi), sin(theta) * sin(phi) , cos(phi));
    }

    // sample color
    void generate_color() {
        glm::vec3 col0;
        glm::vec3 col1;
        switch (color_type) {
        case 0:
            col0 = glm::vec3(1.0f, 0.0f, 0.0f); // red
            col1 = glm::vec3(0.0f, 0.0f, 1.0f); // blue
            break;
        case 1:
            col0 = glm::vec3(0.0f, 1.0f, 1.0f); // cyan
            col1 = glm::vec3(1.0f, 1.0f, 0.0f); // yellow
            break;
        default:
            col0 = glm::vec3(1.0f, 0.0f, 1.0f); // magenta
            col1 = glm::vec3(0.0f, 1.0f, 0.0f); // green
        }
        for (int i = 0; i < expl_count; i++) {
            if (noise(1.0f) > 0) sphere_col.push_back(col0);
            else sphere_col.push_back(col1);
        }
    }
};

firework fw0;

int main(int argc, char* argv[]) {

    init();

    //======================= Initializations and Window Setup =================================

    SDL_Init(SDL_INIT_VIDEO);  //Initialize Graphics (for OpenGL)

    //Ask SDL to get a recent version of OpenGL (3.2 or greater)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    //Create a window (offsetx, offsety, width, height, flags)
    SDL_Window* window = SDL_CreateWindow("Firework Simulation", 100, 100, screen_width, screen_height, SDL_WINDOW_OPENGL);
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

    init();

    //Load the default vertex Shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    loadShader(vertexShader, "../ParticleSystems/Shader/vertexshader.txt");

    //Load the default fragment Shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    loadShader(fragmentShader, "../ParticleSystems/Shader/fragmentshader.txt");


    //Join the vertex and fragment shaders together into one program

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glBindFragDataLocation(shaderProgram, 0, "outColor"); // set output
    glLinkProgram(shaderProgram); //run the linker

    glUseProgram(shaderProgram);

    //============================ Model Setup ======================================

    std::vector< float > vertices;
    std::vector< float > uvs; // Won't be used at the moment.
    std::vector< float > normals;
    loadobj("../ParticleSystems/Assets/sphere.obj", vertices, uvs, normals);
    sph_vert = vertices.size();
    loadobj("../ParticleSystems/Assets/icosphere.obj", vertices, uvs, normals); // append to the vector (does not matter since both objects are using the same format)
    ico_vert = vertices.size();

    //============================ Buffer Setup ======================================

    //Build a Vertex Array Object. This stores the VBO and attribute mappings in one object
    GLuint vao;
    glGenVertexArrays(1, &vao); 
    glBindVertexArray(vao);

    GLuint vbo[2];
    glGenBuffers(2, vbo);  //Create 1 buffer called vbo

    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
    //Tell OpenGL how to set fragment shader input (for sphere)
    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    //Attribute, vals/attrib., type, normalized?, stride, offset
    //Binds to VBO current GL_ARRAY_BUFFER 
    glEnableVertexAttribArray(posAttrib);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), &normals[0], GL_STATIC_DRAW);

    GLint normalAttrib = glGetAttribLocation(shaderProgram, "inNormal");
    glVertexAttribPointer(normalAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glEnableVertexAttribArray(normalAttrib);


    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    uniModel = glGetUniformLocation(shaderProgram, "model");
    uniView = glGetUniformLocation(shaderProgram, "view");
    uniProj = glGetUniformLocation(shaderProgram, "proj");

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
        }

        // Clear the screen to default color
        glClearColor(0.1f, 0.1f, 0.1f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (!saveOutput) dt = (SDL_GetTicks() / 1000.f) - lastTime;
        if (dt > .1) dt = .1; //Have some max dt
        lastTime = SDL_GetTicks() / 1000.f;
        if (saveOutput) dt += .07; //Fix framerate at 14 FPS

        update(dt);

        printf("FPS: %i \n", int(1 / dt));

        if (saveOutput) Win2PPM(screen_width, screen_height);


        SDL_GL_SwapWindow(window); //Double buffering
    }

    //Clean Up

    glDeleteProgram(shaderProgram);
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);

    glDeleteBuffers(1, vbo);

    glDeleteVertexArrays(1, &vao);

    SDL_GL_DeleteContext(context);
    SDL_Quit();
    return 0;
}

void init() {
    screen_width = 800;
    screen_height = 600;
    saveOutput = false;
    fullscreen = false;

    cam_loc = glm::vec3(15.f, 15.f, 1.8f);
    look_at = glm::vec3(0.0f, 0.0f, 0.0f);
    up = glm::vec3(0.0f, 0.0f, 1.0f);

    sph_rad = 0.1f;
    fw0 = firework();
};

void update(float dt) {

    set_camera();

    draw_sphere(dt);

    computePhysics(dt);
}

void computePhysics(float dt) {
    fw0.update(dt);
    //printf("Particle Count: %i \n", fire.Pos.size());
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

void draw_sphere(float dt) {
    // draw every sphere in firework0's list
    for (int i = 0; i < fw0.sphere_loc.size(); i++) {
        glm::mat4 model = glm::mat4();
        model = glm::translate(model, fw0.sphere_loc[i]);
        model = glm::scale(model, glm::vec3(sph_rad));
        glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3f(uniColor, fw0.sphere_col[i].r, fw0.sphere_col[i].g, fw0.sphere_col[i].b);
        glDrawArrays(GL_TRIANGLES, 0, sph_vert / 3); //(Primitives, starting index, Number of vertices)
    }
}

