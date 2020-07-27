// A number of user control functions,
// including camera control and interaction with objects


#include <SDL.h>
#include <SDL_opengl.h>
#include <math.h>

#define GLM_FORCE_RADIANS
#include "../../glm/glm.hpp"
#include "../../glm/gtc/matrix_transform.hpp"
#include "../../glm/gtc/type_ptr.hpp"

using namespace std;

// move object around by pressing keys
void move_obj(SDL_Event event, glm::vec3& pos, glm::vec3 view, glm::vec3 up, float speed); 
// pos is the position of the object to be moved
// view is the vector pointing from eye to focus

// grab objects by attaching it to the camera
void grab_obj(SDL_Event event, glm::vec3 pos, glm::vec3 lookat, glm::vec3 objloc, \
    float& relative_dis, float& relativeX, float& relativeY, bool& grabbed);

// move camera around by pressing keys and rotate by moving the cursor
void move_camera(SDL_Event event, glm::vec3& pos, glm::vec3& lookat, glm::vec3 up, float rotspeed, float movspeed);

// rotate the camera when cursor is close to the boundaries
// needs to be called in every frame to be effective
void bound_rotate(SDL_Window* window, glm::vec3 pos, glm::vec3& lookat);

void rotate(float rotX, float rotY, float speed, glm::vec3& target);