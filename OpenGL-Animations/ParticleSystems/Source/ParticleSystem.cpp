#define _USE_MATH_DEFINES

#include "ParticleSystem.h"
#include <cmath>
#include <ctime>
#include <cstdlib>

float g = -9.8;

ParticleSystem::ParticleSystem() { // default particle system

	// initializing global parameters for particle system
	genRate = 2500; // generation rate
	lifespan = 2; //lifespan
	lfspan_ptb = 0; //lifespan perturbation
	max_ptc_ct = 20000; // maximum particle count
	src_pos = glm::vec3(0.0f, 0.0f, 0.0f); // source position
	src_radius = 1.0f; // source radius
	src_normal = axis::Z; // source normal
	ini_vel = 1.0f;  // initial velocity
	vel_ptb = 0.0f; // velocity perturbation
	ini_clr = glm::vec3(0.0f, 0.0f, 0.0f); // initial color

	Pos.reserve(max_ptc_ct);
	Vel.reserve(max_ptc_ct);
	Life.reserve(max_ptc_ct);
	Clr.reserve(max_ptc_ct);
}

ParticleSystem::ParticleSystem(float gr, float ls, float lsptb, int mpc, glm::vec3 pos, float sr, axis n, float vel, float vptb, glm::vec3 clr) {
	// initializing global parameters for particle system
	genRate = gr; // generation rate
	lifespan = ls; //lifespan
	lfspan_ptb = lsptb; //lifespan perturbation
	max_ptc_ct = mpc; // maximum particle count
	src_pos = pos; // source position
	src_radius = sr; // source radius
	src_normal = n; // source normal
	ini_vel = vel;  // initial velocity
	vel_ptb = vptb; // velocity perturbation
	ini_clr = clr; // initial color

	Pos.reserve(max_ptc_ct);
	Vel.reserve(max_ptc_ct);
	Life.reserve(max_ptc_ct);
	Clr.reserve(max_ptc_ct);

}

// Update the particles in a fluid-like/smoke-like behavior
void ParticleSystem::update(float dt, ParticleSystem::particle_type t, glm::vec3 obs_loc, float obs_rad) {
	removeParticles(); // remove the dead particles
	spawnParticles(dt); // spawn new particles
	
	for (int i = 0; i < Pos.size(); i++) { // update pos, vel, and life (and color)
		Pos[i] += Vel[i] * dt;
		if (t == ParticleSystem::particle_type::fluid) Vel[i] += glm::vec3(0, 0, g) * dt;
		else if (t == ParticleSystem::particle_type::smoke) {
			Vel[i] += glm::vec3(0, 0, 10) * dt;
			Vel[i].x *= 0.8;
			Vel[i].y *= 0.8;
			Clr[i].g = Life[i] / lifespan;
		};
		Life[i] -= dt;


		//collision detection
		glm::vec3 dir = Pos[i] - obs_loc;
		float dis = glm::length(dir);
		//printf("Distance: %f \n", dis);
		if (dis <= obs_rad) { // collide
			// return to valid state
			dir /= dis;
			Pos[i] = obs_loc + dir * (obs_rad + 0.01f);
			//if fluid particle then reflect
			if (t == ParticleSystem::particle_type::fluid) {
				float tmp = glm::dot(Vel[i], dir);
				Vel[i] -= 2 * tmp * dir;
				Vel[i] *= 0.5f;
			}
		}
	}
}


void ParticleSystem::spawnParticles(float dt) {
	// determine how many particles to spawn per dt
	float ppt = genRate * dt;

	// spawn the integral parts of particles
	for (int i = 0; i < int(ppt); i++) {
		if (Pos.size() < max_ptc_ct)  spawnOneParticle();
	}
	
	// spawn the "fractional part"
	if (src_radius * static_cast <float> (rand()) / static_cast <float> (RAND_MAX) < ppt - int(ppt) && (Pos.size() < max_ptc_ct))
		spawnOneParticle();
}

void ParticleSystem::spawnOneParticle() {

	// randomly initialize properties of a single particle
	Pos.push_back(sampleSource()); 
	Vel.push_back(sampleVelocity());
	Life.push_back(sampleLifespan());
	Clr.push_back(ini_clr);

}

void ParticleSystem::removeParticles() {

	vector<int> ind; // indices of dead particles
	// Cycle through the list of lifespan and remove the dead particles
	#pragma omp parallel for
	for (int i = Life.size() - 1; i >= 0; i--) {
		if (Life[i] <= 0) ind.push_back(i); // the indices are recorded in reversed order
	}
	
	for (int i = 0; i < ind.size(); i++) {
		int last = Pos.size() - 1;
		Pos[ind[i]] = Pos[last];
		Pos.pop_back();
		Vel[ind[i]] = Vel[last];
		Vel.pop_back();
		Life[ind[i]] = Life[last];
		Life.pop_back();
		Clr[ind[i]] = Clr[last];
		Clr.pop_back();
	}
}

glm::vec3 ParticleSystem::sampleSource() {

	glm::vec3 pos = src_pos;
	float theta = 2 * M_PI * static_cast <float> (rand()) / static_cast <float> (RAND_MAX);  // 0 - 2PI
	float r = sqrt(src_radius * static_cast <float> (rand()) / static_cast <float> (RAND_MAX)); // 0 - src_radius

	float v1 = r * cos(theta); // displacement from the center of the source
	float v2 = r * sin(theta);

	// determine final spawn location
	switch (src_normal) {
	case axis::X:
		pos += glm::vec3(0, v2, v1);break;
	case axis::Y:
		pos += glm::vec3(v1, 0, v2); break;
	default:
		pos += glm::vec3(v1, v2, 0);
	}
	return pos;
}

glm::vec3 ParticleSystem::sampleVelocity() {

	glm::vec3 vel;

	float theta = vel_ptb * M_PI * static_cast <float> (rand()) / (static_cast <float> (180 * RAND_MAX)); // 0 - MaxAngle
	float phi = 2 * M_PI * static_cast <float> (rand()) / static_cast <float> (RAND_MAX); // 0 - 2PI

	float v1 = cos(theta);
	float v2 = sin(theta) * cos(phi);
	float v3 = sin(theta) * sin(phi);

	switch (src_normal) {
	case axis::X:
		vel = glm::vec3(v1, v2, v3); break;
	case axis::Y:
		vel = glm::vec3(v3, v1, v2); break;
	default:
		vel = glm::vec3(v2, v3, v1);
	}

	vel *= ini_vel;
	return vel;
}

float ParticleSystem::sampleLifespan() {
	float ls = 2 * static_cast <float> (rand()) / static_cast <float> (RAND_MAX) - 0.5; // (-1) - 1
	ls *= lfspan_ptb;
	ls += lifespan;
	return ls;
}
