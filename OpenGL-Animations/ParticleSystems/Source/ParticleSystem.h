// A Particle System Class by Yuxuan Huang

#include <vector>
#define GLM_FORCE_RADIANS
#include "../../glm/glm.hpp"
#include "../../glm/gtc/matrix_transform.hpp"
#include "../../glm/gtc/type_ptr.hpp"

using namespace std;

enum class axis {X, Y, Z};

class ParticleSystem
{	

public:
	enum particle_type { fluid, smoke };

	vector<glm::vec3> Pos;
	vector<glm::vec3> Clr;

	ParticleSystem(float gr, float ls, float lsptb, int mpc, glm::vec3 pos, float sr, axis n, float vel, float vptb, glm::vec3 col);

	void update(float dt, particle_type t, glm::vec3 obs_loc, float obs_rad); // update in a fluid-like behavior

private:
	// particle system global parameters
	float genRate;
	float lifespan;
	float lfspan_ptb; // lifespan perturbation
	int max_ptc_ct; // maximum particle count
	glm::vec3 src_pos; // source position
	float src_radius; // source radius
	axis src_normal; // source normal (which axis the source is facing)
	float ini_vel; // initial velocity (only scalar value because default direction is perpendicular to the source
	float vel_ptb; // velocity perturbation (in angles)
	glm::vec3 ini_clr; // initial color

	// lists of information for each particle
	vector<glm::vec3> Vel;
	vector<float> Life;

	// private functions for particle generation and update
	glm::vec3 sampleSource(); // sample the particle source

	glm::vec3 sampleVelocity(); // sample initial velocity

	float sampleLifespan(); // sample lifespan

	void spawnParticles(float dt); // spawn particles per timestep
	
	void spawnOneParticle(); // spawn a paticle

	void removeParticles(); // remove the dead particles per timestep
};

