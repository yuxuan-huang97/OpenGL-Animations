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

#define STB_IMAGE_IMPLEMENTATION
#include "../../stb_image.h"

using namespace std;

int screen_width, screen_height;
float aspect; //aspect ratio (needs to be updated if the window is resized)

// Camera Spec
glm::vec3 cam_loc, look_at, up;

//Index of where to model, view, and projection matricies are stored on the GPU
GLint uniModel, uniView, uniProj, uniColor;

// cloth specs
Cloth cloth;
vector<float> vertices;
vector<float> normals;
vector<int> indices;

// sphere data
glm::vec3 sph_loc, sph_color;
float sph_rad;
int sph_vert; // number of verts for sphere
vector<float> svertices;
vector<float> suvs;
vector<float> snormals;

// user interaction variables
bool grabbed;
float relative_dis, relativeX, relativeY;

// wind parameter
glm::vec3 wind_dir;
float windspeed;

GLuint shaderProgram;
GLuint uvshaderProgram;
void init();
void update(float dt, GLuint vbo[], GLuint vbo_sph[]);
void draw_cloth();
void draw_sphere();
void set_camera();
void set_wind(SDL_Event event, Cloth &cloth);
GLuint load_texture(const char* filePath);

GLuint vao_cloth, vao_sph;
GLuint texture;

bool play = false;

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

    //Load the default vertex Shader
    GLuint uvvertexShader = glCreateShader(GL_VERTEX_SHADER);
    loadShader(uvvertexShader, "../ClothSimulation/Shader/uvvertexshader.txt");

    //Load the default fragment Shader
    GLuint uvfragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    loadShader(uvfragmentShader, "../ClothSimulation/Shader/uvfragmentshader.txt");

    //Join the vertex and fragment shaders together into one program

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glBindFragDataLocation(shaderProgram, 0, "outColor"); // set output
    glLinkProgram(shaderProgram); //run the linker

    uvshaderProgram = glCreateProgram();
    glAttachShader(uvshaderProgram, uvvertexShader);
    glAttachShader(uvshaderProgram, uvfragmentShader);
    glBindFragDataLocation(uvshaderProgram, 0, "outColor"); // set output
    glLinkProgram(uvshaderProgram); //run the linker

    //glUseProgram(shaderProgram);
    //============================ Model Setup =======================================
    
    loadobj("../ClothSimulation/Assets/sphere.obj", svertices, suvs, snormals);
    sph_vert = svertices.size();
    
    texture = load_texture("../ClothSimulation/Assets/UMN.jpg");
    //============================ Buffer Setup ======================================

    //Build a Vertex Array Object. This stores the VBO and attribute mappings in one object
    glGenVertexArrays(1, &vao_cloth); // VAO for cloth
    glGenVertexArrays(1, &vao_sph); // VAO for sphere
    glBindVertexArray(vao_sph); //Bind the sphere VAO to the current context

    //Allocate memory on the graphics card to store geometry (vertex buffer object)
    GLuint vbo_cloth[3], vbo_sph[2];
    glGenBuffers(3, vbo_cloth);
    glGenBuffers(2, vbo_sph);


    glBindBuffer(GL_ARRAY_BUFFER, vbo_sph[0]); //Set the vbo as the active array buffer (Only one buffer can be active at a time)
    glBufferData(GL_ARRAY_BUFFER, svertices.size() * sizeof(float), &svertices[0], GL_STATIC_DRAW); //upload vertices to vbo

    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(posAttrib);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_sph[1]); //Set the vbo as the active array buffer (Only one buffer can be active at a time)
    glBufferData(GL_ARRAY_BUFFER, snormals.size() * sizeof(float), &snormals[0], GL_STATIC_DRAW); //upload vertices to vbo

    GLint normalAttrib = glGetAttribLocation(shaderProgram, "inNormal");
    glVertexAttribPointer(normalAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normalAttrib);


    glBindVertexArray(vao_cloth); //Bind the cloth VAO to the current context

    vector<float> uvs = cloth.get_uv();
    glBindBuffer(GL_ARRAY_BUFFER, vbo_cloth[2]); //Set the vbo as the active array buffer (Only one buffer can be active at a time)
    glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(float), &uvs[0], GL_STATIC_DRAW); //upload vertices to vbo

    GLint uvAttrib = glGetAttribLocation(uvshaderProgram, "inUV");
    glVertexAttribPointer(uvAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(uvAttrib);

    GLuint ebo; // Element buffer object
    glGenBuffers(1, &ebo);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), &indices[0], GL_STREAM_DRAW);

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
            if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_SPACE)
                play = true; //play

            set_wind(windowEvent, cloth);

            move_camera(windowEvent, cam_loc, look_at, up, 0.3f, 0.3f);
            grab_obj(windowEvent, cam_loc, look_at, sph_loc, relative_dis, relativeX, relativeY, grabbed);
        }
        bound_rotate(window, cam_loc, look_at, 0.01f);

        // Clear the screen to default color
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        dt = (SDL_GetTicks() / 1000.f) - lastTime;
        if (dt > .1) dt = .1; //Have some max dt
        lastTime = SDL_GetTicks() / 1000.f;

        update(0.035, vbo_cloth, vbo_sph);

        //printf("FPS: %i \n", int(1 / dt));

        SDL_GL_SwapWindow(window); //Double buffering
    }

    //Clean Up
    glDeleteProgram(shaderProgram);
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);

    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, vbo_cloth);
    glDeleteBuffers(1, vbo_sph);
    glDeleteVertexArrays(1, &vao_cloth);
    glDeleteVertexArrays(1, &vao_sph);

    SDL_GL_DeleteContext(context);
    SDL_Quit();
    return 0;
}

void init() {
    screen_width = 800;
    screen_height = 600;

    cam_loc = glm::vec3(30.0f, 30.0f, 30.0f);
    look_at = glm::vec3(0.0f, 0.0f, 5.0f);
    up = glm::vec3(0.0f, 0.0f, 1.0f);

    cloth = Cloth(30, 30, -20.0f, 0.5f, 1.0f, 15000.0f, 800.0f);
    indices = cloth.get_index();

    sph_loc = glm::vec3(0.0f, 10.0f, 10.0f);
    sph_rad = 2.5f;
    sph_color = glm::vec3(1.0f, 1.0f, 1.0f);

    grabbed = false;

    wind_dir = glm::vec3(0.0f);
    windspeed = 0.0f;
}

void update(float dt, GLuint vbo[], GLuint vbo_sph[]) {
    
    if (play) cloth.update(dt, 70, sph_loc, sph_rad);
    vertices = cloth.vertex_buffer();
    normals = cloth.get_normal();

    if (grabbed) {
        sph_loc = glm::normalize(look_at - cam_loc);
        rotate(-relativeX, relativeY, 1.0f, sph_loc);
        sph_loc *= relative_dis;
        sph_loc += cam_loc;
    }

    set_camera();
    uniModel = glGetUniformLocation(uvshaderProgram, "model");
    uniView = glGetUniformLocation(uvshaderProgram, "view");
    uniProj = glGetUniformLocation(uvshaderProgram, "proj");

    glBindVertexArray(vao_cloth);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); // vertex positions
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STREAM_DRAW);
    GLint posAttrib = glGetAttribLocation(uvshaderProgram, "position");
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(posAttrib);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]); // vertex normals
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), &normals[0], GL_STREAM_DRAW);
    GLint normalAttrib = glGetAttribLocation(uvshaderProgram, "inNormal");
    glVertexAttribPointer(normalAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normalAttrib);

    glUseProgram(uvshaderProgram);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    draw_cloth();


    set_camera();
    uniModel = glGetUniformLocation(shaderProgram, "model");
    uniView = glGetUniformLocation(shaderProgram, "view");
    uniProj = glGetUniformLocation(shaderProgram, "proj");
    uniColor = glGetUniformLocation(shaderProgram, "inColor");

    glBindVertexArray(vao_sph);

    glUseProgram(shaderProgram);
    
    draw_sphere();

}

void draw_cloth() {
    glm::mat4 model = glm::mat4();
    model = glm::translate(model, glm::vec3(0.0f));
    model = glm::scale(model, glm::vec3(1.0f));
    glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3f(uniColor, 1.0f, 1.0f, 1.0f);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, (void*)0); //(Primitives, count, type, offset)
}

void draw_sphere() {
    glm::mat4 model = glm::mat4();
    model = glm::translate(model, sph_loc);
    model = glm::scale(model, glm::vec3(sph_rad));
    glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3f(uniColor, sph_color.r, sph_color.g, sph_color.b);
    glDrawArrays(GL_TRIANGLES, 0, sph_vert / 3); //(Primitives, starting index, Number of vertices)
}

void set_camera() {
    //Set the Camera view paramters (FOV, aspect ratio, etc.)
    glm::mat4 proj = glm::perspective(3.14f / 4, aspect, .1f, 1000.0f); //FOV, aspect, near, far
    glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

    //Set the Camera Position and Orientation
    glm::mat4 view = glm::lookAt(cam_loc, look_at, up); // camera location, look at point, up direction
    glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));
}

void set_wind(SDL_Event event, Cloth& cloth) {
    if (event.type == SDL_KEYUP) {
        switch (event.key.keysym.sym) {
        case SDLK_LEFT: wind_dir = glm::vec3(0.0f, -1.0f, 0.0f); break;
        case SDLK_RIGHT: wind_dir = glm::vec3(-1.0f, 0.0f, 0.0f); break;
        case SDLK_UP: windspeed += 1.0f; break;
        case SDLK_DOWN: windspeed -= 1.0f; break;
        case SDLK_0: windspeed = 0.0f; break;
        default: break;
        }
        cloth.set_wind(wind_dir * windspeed);
    }
}

// texture mapping related function
GLuint load_texture(const char* filePath) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load and generate the texture
    int width, height, nrChannels;
    unsigned char* data = stbi_load(filePath, &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
    return texture;
}