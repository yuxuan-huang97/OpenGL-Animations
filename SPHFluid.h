// 2D and 3D SPH Fluid template
// Yuxuan Huang

#define GLM_FORCE_RADIANS
#include "../../../glm/glm.hpp"
#include "../../../glm/gtc/matrix_transform.hpp"
#include "../../../glm/gtc/type_ptr.hpp"

#include <vector>

using namespace std;

class SPH_2D
{
private:
	int n; // number of particles
	float mass; // mass of each particle
	float r; // smoothing radius
	float g; // gravity
	
	vector<glm::vec2> position; // positions of the particles
	vector<glm::vec2> position_old; // previous positions of the particles
	
};

