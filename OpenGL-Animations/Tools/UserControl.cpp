#include "UserControl.h"


void move_obj(SDL_Event event, glm::vec3& pos, glm::vec3 view, glm::vec3 up, float speed) {
    //float speed = 0.2;
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