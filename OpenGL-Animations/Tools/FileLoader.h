// An obj loader that assists the loading process of obj models into OpenGL
// written by Yuxuan Huang

#include <vector>
#include <string>
#include "../../glm/glm.hpp"
#include "../../glm/gtc/matrix_transform.hpp"
#include "../../glm/gtc/type_ptr.hpp"

using namespace std;

bool loadobj(const char* path, std::vector < float >& out_vertices,
    std::vector < float >& out_uvs,
    std::vector < float >& out_normals);

