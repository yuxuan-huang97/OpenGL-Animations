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

	for (int i = 0; i < length; i++) {
		for (int j = 0; j < width; j++) {
			pos.push_back(glm::vec3(upperleft.x - i * restlen, upperleft.y + j * restlen, upperleft.z));
			normal.push_back(glm::vec3(0.0f));
			vel.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
		}
	}

	wind_v = glm::vec3(0.0f); // no wind initially
	//wind_v = glm::vec3(-10.0f, -10.0f, 0.0f);
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

vector<int> Cloth::get_index() {
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

vector<float> Cloth::get_uv() {
	vector<float> uvs;
	for (int i = 0; i < length; i++) {
		for (int j = 0; j < width; j++) {
			uvs.push_back(i / float(length - 1)); // u
			//uvs.push_back((width - j) / float(width - 1)); // v
			uvs.push_back(j / float(width - 1)); // v
		}
	}
	return uvs;
}

vector<float> Cloth::get_normal() {
	vector<float> result;
	glm::vec3 n;
	for (int i = 0; i < length; i++) {
		for (int j = 0; j < width; j++) {
			n = glm::normalize(normal[i * width + j]);
			result.push_back(n.x);
			result.push_back(n.y);
			result.push_back(n.z);
		}
	}
	return result;
}

void Cloth::update(float total_dt, int substep, glm::vec3 obs_loc, float obs_rad) {

	vector<glm::vec3> vforce; // forces in the vertical strings
	vector<glm::vec3> hforce; // forces in horizontal strings
	vector<glm::vec3> gforce; // drag force
	#pragma omp parallel for
	for (int i = 0; i < width * length; i++) gforce.push_back(glm::vec3(0.0f));
	bool update_normals = false;

	// compute forces
	float stringF; // elastic force in the string
	float dampF; // damping force in the string

	glm::vec3 delta_p;
	float v1, v2;

	float dt = total_dt / substep;
	
	for (int step = 0; step < substep; step++) {

		// reset all the forces (& normals)
		vforce.clear();
		hforce.clear();
		for (int i = 0; i < width * length; i++) gforce[i] = glm::vec3(0.0f);

		if (step == substep - 1) update_normals = true; // only update normals in the final substep

		// vertical
		#pragma omp parallel for
		for (int i = 0; i < length; i++) {
			for (int j = 0; j < width - 1; j++) {
				delta_p = pos[i * width + j + 1] - pos[i * width + j];
				float len = glm::length(delta_p); // len is the distance between two vertical conjunctions
				stringF = -k * (len - restlen);

				delta_p = glm::normalize(delta_p); // delta_p is now the unit direction
				v1 = glm::dot(vel[i * width + j], delta_p);
				v2 = glm::dot(vel[i * width + j + 1], delta_p);
				dampF = kv * (v1 - v2);

				vforce.push_back((stringF + dampF) * delta_p);
			}
		}

		// horizontal
		#pragma omp parallel for
		for (int i = 0; i < length - 1; i++) {
			for (int j = 0; j < width; j++) {
				delta_p = pos[(i + 1) * width + j] - pos[i * width + j];
				float len = glm::length(delta_p); // len is the distance between two vertical conjunctions
				stringF = -k * (len - restlen);

				delta_p = glm::normalize(delta_p); // delta_p is now the unit direction
				v1 = glm::dot(vel[i * width + j], delta_p);
				v2 = glm::dot(vel[(i + 1) * width + j], delta_p);
				dampF = kv * (v1 - v2);

				hforce.push_back((stringF + dampF) * delta_p);
				//hforce.push_back((stringF) * delta_p);
			}
		}

		// drag (& normal)
		drag(gforce, update_normals);

		// Eulerian integration & collision detection
		#pragma omp parallel for
		for (int i = 0; i < length; i++) { // for each conjunctions
			for (int j = 0; j < width; j++) {
				if (j == 0) {
					if (i == 0 || i == length/3 || i == 2*length/3 || i == length - 1) continue; // exclude the pins
				}
				// compute the acceleration
				glm::vec3 acc(0.0f);
				// string force
				// force from upper string
				if (j > 0) {
					acc += vforce[i * (width - 1) + j - 1];
				}
				// force from lower string
				if (j < width - 1) {
					acc -= vforce[i * (width - 1) + j];
				}
				// force from left string
				if (i > 0) {
					acc += hforce[(i - 1) * width + j];
				}
				// force from right string
				if (i < length - 1) {
					acc -= hforce[i * width + j];
				}
				// convert force to acceleration
				acc = acc * 0.5f / mass;

				// drag
				acc += gforce[i * width + j] / mass;

				// gravity
				acc.z += gravity;

				// update speed and position
				vel[i * width + j] += acc * dt;
				pos[i * width + j] += vel[i * width + j] * dt;

				// collision detection
				glm::vec3 n = pos[i * width + j] - obs_loc;
				if (glm::length(n) <= obs_rad + 0.1) { // collided

					float alpha = 0.1f;
					n = glm::normalize(n);
					pos[i * width + j] = obs_loc + (obs_rad + 0.2f) * n;

					float vns = glm::dot(vel[i * width + j], n); // velocity parallel to normal
					glm::vec3 vtemp = -(1 + alpha) * vns * n;
					vel[i * width + j] += vtemp;
				}
			}
		}

	}

}

void Cloth::drag(vector<glm::vec3>& gforce, bool comp_normal) {
	float c = 2.0f;
	if (comp_normal) { // if we need to update normals
		#pragma omp parallel for
		for (int i = 0; i < width * length; i++) { // we reset all the normals first
			normal[i] = glm::vec3(0.0f);
		}
	}
	#pragma omp parallel for
	for (int i = 0; i < length - 1; i++) {
		for (int j = 0; j < width - 1; j++) {

			// find the indices
			int ind0 = i * width + j;
			int ind1 = i * width + j + 1;
			int ind2 = (i + 1) * width + j + 1;
			int ind3 = (i + 1) * width + j;

			// compute average vel for triangles
			glm::vec3 vtmp = vel[ind0] + vel[ind2];
			glm::vec3 v0 = (vtmp + vel[ind1]) / 3.0f - wind_v; // average vel for the 1st triangle
			glm::vec3 v1 = (vtmp + vel[ind3]) / 3.0f - wind_v; // ... for the 2nd triangle

			// compute normal for triangles
			vtmp = pos[ind2] - pos[ind0]; // diagonal vector
			glm::vec3 n0 = glm::cross(pos[ind1] - pos[ind0], vtmp); // unnormalized normal
			glm::vec3 n1 = glm::cross(vtmp, pos[ind3] - pos[ind0]);

			// compute the final force
			glm::vec3 f0 = -0.5f * c * (glm::length(v0) * glm::dot(v0, n0) / (2.0f * glm::length(n0))) * n0; // force on each triangle
			glm::vec3 f1 = -0.5f * c * (glm::length(v1) * glm::dot(v1, n1) / (2.0f * glm::length(n1))) * n1;
			f0 /= 3.0f; // force per vertex
			f1 /= 3.0f;
			gforce[ind0] += f0;
			gforce[ind1] += f0;
			gforce[ind2] += f0;
			gforce[ind0] += f1;
			gforce[ind2] += f1;
			gforce[ind3] += f1;

			if (comp_normal) { // update the normals if we need to
				n0 = glm::normalize(n0);
				n1 = glm::normalize(n1);
				normal[ind0] += n0; // no need to /3 because it has not been normalized anyway
				normal[ind1] += n0; // we will normalize it in the get_normal() call
				normal[ind2] += n0;
				normal[ind0] += n1;
				normal[ind2] += n1;
				normal[ind3] += n1;
			} 
		}
	}
}

void Cloth::set_wind(glm::vec3 new_speed) {
	wind_v = new_speed;
}