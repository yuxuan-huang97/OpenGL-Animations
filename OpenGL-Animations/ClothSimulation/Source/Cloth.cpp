// A string-based simulated cloth template
// written by Yuxuan Huang

#include "Cloth.h"

// default cloth
Cloth::Cloth() {
	length = 30;
	width = 30;
	gravity = -9.8f;
	restlen = 5.0f;
	mass = 5.0f;
	k = 1000;
	kv = 100;

	init();
}

// customized cloth
Cloth::Cloth(int l, int w, float g, float len, float m, float k_p, float kv_p) {
	length = l;
	width = w;
	gravity = g;
	restlen = len;
	mass = m;
	k = k_p;
	kv = kv_p;

	init();
}

void Cloth::init() {
	glm::vec3 upperleft((length - 1) * restlen / 2.0f, 0.0f, (width - 1) * restlen); //coordinate of the upper left corner

	for (int i = 0; i < width; i++) {
		for (int j = 0; j < length; j++) {
			pos.push_back(glm::vec3(upperleft.x - i * restlen, upperleft.y, upperleft.z - j * restlen));
			vel.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
		}
	}
}

vector<float> Cloth::vertex_buffer() {
	vector<float> vertex_buffer;
	for (int i = 0; i < pos.size(); i++) {
		vertex_buffer.push_back(pos[i].x);
		vertex_buffer.push_back(pos[i].y);
		vertex_buffer.push_back(pos[i].z);
	}
	return vertex_buffer;
}

vector<int> Cloth::index() {
	vector<int> index_buffer;
	for (int i = 0; i < length - 1; i++) {
		for (int j = 0; j < width - 1; j++) {
			// first triangle index
			index_buffer.push_back(i * width + j);
			index_buffer.push_back(i * width + 1 + j);
			index_buffer.push_back((i + 1) * width + 1 + j);
			// second triangle index
			index_buffer.push_back(i * width + j);
			index_buffer.push_back((i + 1) * width + 1 + j);
			index_buffer.push_back((i + 1) * width + j);
		}
	}
	return index_buffer;
}

void Cloth::update(float total_dt, int substep) {
	vector<glm::vec3> vforce; // forces in the vertical strings

	// compute forces
	float stringF; // elastic force in the string
	float dampF; // damping force in the string

	glm::vec3 delta_p;
	float v1, v2;

	float dt = total_dt / substep;
	
	for (int step = 0; step < substep; step++) {
		// vertical
		for (int i = 0; i < length; i++) {
			for (int j = 0; j < width - 1; j++) {
				delta_p = pos[i * width + j + 1] - pos[i * width + j];
				float len = glm::length(delta_p); // len is the distance between two vertical conjunctions
				stringF = -k * (len - restlen);
				glm::normalize(delta_p); // delta_p is now the unit direction
				v1 = glm::dot(vel[i * width + j], delta_p);
				v2 = glm::dot(vel[i * width + j + 1], delta_p);
				dampF = -kv * (v1 - v2);

				vforce.push_back((stringF + dampF) * delta_p);
			}
		}

		// horizontal

		// Eulerian integration
		for (int i = 0; i < length; i++) { // for each conjunctions
			for (int j = 0; j < width; j++) {
				//if ((i == 0 && j == 0) || (i == length - 1 && j == 0)) continue; // exclude the pins
				if (j == 0) continue; // exclude the pins
				// compute the acceleration
				glm::vec3 acc(0.0f);
				// force from upper string
				if (j > 0) {
					acc += vforce[i * (width - 1) + j - 1];
				}
				// force from lower string
				if (j < width - 1) {
					acc -= vforce[i * (width - 1) + j];
				}
				// convert force to acceleration
				acc = acc * 0.5f / mass;
				acc.z += gravity;
				// update speed and position
				vel[i * width + j] += acc * dt;
				pos[i * width + j] += vel[i * width + j] * dt;

			}
		}

		// collision detection
	}

}