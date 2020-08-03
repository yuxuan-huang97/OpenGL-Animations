// A string-based simulated cloth template
// written by Yuxuan Huang

#define GLM_FORCE_RADIANS
#include "../../../glm/glm.hpp"
#include "../../../glm/gtc/matrix_transform.hpp"
#include "../../../glm/gtc/type_ptr.hpp"

#include <vector>

using namespace std;

class Cloth {

public:
	int length, width;
	vector<glm::vec3> pos; // position of every conjunction

	Cloth();

	Cloth(int length, int width, float gravity, float restlen, float mass, float k, float kv);

	void update(float dt, int substep, glm::vec3 obs_loc, float obs_rad);

	vector<float> vertex_buffer(); // return the positions of each vertex in vbo format
	vector<float> get_normal(); // returns the normals of each vertex
	vector<int> get_index(); // return the index buffer of the cloth

private:
	float gravity;
	float restlen, mass;
	float k, kv;
	glm::vec3 wind_v;

	vector<glm::vec3> vel; // velocity of every conjunction
	vector<glm::vec3> normal; // (unnormalized) normal of every conjunction

	void init();
	void drag(vector<glm::vec3> &dragforce, bool compute_normal); // compute drag force (and normals of each vertex along the way)
};