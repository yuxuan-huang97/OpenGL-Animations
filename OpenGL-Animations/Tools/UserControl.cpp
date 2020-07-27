#include "UserControl.h"


void move_obj(SDL_Event event, glm::vec3& pos, glm::vec3 view, glm::vec3 up, float speed) {
    
    glm::vec3 translation;

    view = glm::normalize(view);
    up = glm::normalize(up);
    glm::vec3 left = glm::cross(up, view);

    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
        case SDLK_UP: translation = view; break;
        case SDLK_DOWN: translation = view * (-1.0f); break;
        case SDLK_d: translation = left * (-1.0f); break;
        case SDLK_a: translation = left; break;
        case SDLK_w: translation = up; break;
        case SDLK_s: translation = up * (-1.0f); break;
        default: break;
        }
    }
    translation *= speed;
    pos += translation;
}

void move_camera(SDL_Event event, glm::vec3& pos, glm::vec3& lookat, glm::vec3 up, float rotspeed, float movspeed) {

    // vector preprocessing
    glm::vec3 view = lookat - pos;
    view = glm::normalize(view);
    up = glm::normalize(up);
    glm::vec3 left = glm::cross(up, view);

    // mouse movement event for camera rotation
    if (event.type == SDL_MOUSEMOTION) {
        float rotX = 0;
        float rotY = 0;

        // obtain mouse movement data and convert to radians
        rotX += event.motion.xrel;
        rotY += event.motion.yrel;
        rotX = rotX * M_PI / 180.0f;
        rotY = rotY * M_PI / 180.0f;

        rotate(rotX, rotY, rotspeed, view);

        //printf("X: %i, Y: %i \n", event.motion.x, event.motion.y);
    }

    lookat = pos + view;

    // keyboard event for camera movement
    bool forward = false, backward = false, leftward = false, rightward = false, upward = false, downward = false;
    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
        case SDLK_w: forward = true; break;
        case SDLK_a: leftward = true; break;
        case SDLK_s: backward = true; break;
        case SDLK_d: rightward = true; break;
        case SDLK_e: upward = true; break;
        case SDLK_c: downward = true; break;
        default: break;
        }
    }
    
    else if (event.type == SDL_KEYUP) {
        switch (event.key.keysym.sym) {
        case SDLK_w: forward = false; break;
        case SDLK_a: leftward = false; break;
        case SDLK_s: backward = false; break;
        case SDLK_d: rightward = false; break;
        case SDLK_e: upward = false; break;
        case SDLK_c: downward = false; break;
        default: break;
        }
    }

    if (forward) {
        pos += view * movspeed;
        lookat += view * movspeed;
    }
    else if (backward) {
        pos -= view * movspeed;
        lookat -= view * movspeed;
    }

    if (leftward) {
        pos += left * movspeed;
        lookat += left * movspeed;
    }
    else if (rightward) {
        pos -= left * movspeed;
        lookat -= left * movspeed;
    }

    if (upward) {
        pos += up * movspeed;
        lookat += up * movspeed;
    }
    else if (downward) {
        pos -= up * movspeed;
        lookat -= up * movspeed;
    }
}

void bound_rotate(SDL_Window* window, glm::vec3 pos, glm::vec3& lookat) {
    
    glm::vec3 view = lookat - pos;

    int x, y, w, h;
    float rotX = 0;
    float rotY = 0;
    float speed = 0.003;
    int tolerance = 10;
    SDL_GetMouseState(&x, &y);
    SDL_GetWindowSize(window, &w, &h);

    if (x <= tolerance) rotX = -1.0f;
    else if (x >= w - tolerance - 1) rotX = 1.0f;
    if (y <= tolerance) rotY = -1.0f;
    else if (y >= h - tolerance - 1) rotY = 1.0f;
    rotate(rotX, rotY, speed, view);

    lookat = pos + view;
}

void grab_obj(SDL_Event event, glm::vec3 pos, glm::vec3 lookat, glm::vec3 objloc,\
    float& relative_dis, float& relativeX, float& relativeY, bool& grabbed) {
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_g) {
        grabbed = !grabbed;
        if (grabbed) {
            glm::vec3 view = glm::normalize(lookat - pos);
            glm::vec3 obj = glm::normalize(objloc - pos);

            relative_dis = glm::length(objloc - pos);
            relativeX = atan2(obj.y, obj.x) - atan2(view.y, view.x);
            relativeY = acos(obj.z) - acos(view.z);


            //printf("object grabbed\nRelativeX: %f, RelativeY: %f, Dis: %f\n", relativeX*180/3.14, relativeY*180/3.14, relative_dis);
        }
        //else printf("object released\n");
    }
}

void rotate(float rotX, float rotY, float speed, glm::vec3& target) {
    // convert to spherical coordinate
    float theta = atan2(target.y, target.x); // -PI - +PI
    float phi = acos(target.z); // 0 - PI

    // rotate in spherical coordinate
    theta -= speed * rotX;
    phi += speed * rotY;

    if (theta < -M_PI) theta += 2 * M_PI;
    else if (theta > M_PI) theta -= 2 * M_PI;
    if (phi < 0) phi = 0;
    else if (phi > M_PI) phi = M_PI;

    // convert back to cartesian coordinate
    target.x = cos(theta) * sin(phi);
    target.y = sin(theta) * sin(phi);
    target.z = cos(phi);
}
