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

vector<int> Cloth::index() {
	vector<int> index_buffer;
	for (int i = 0; i < length - 1; i++) {
		for (int j = 0; j < width - 1; j++) {
			// first triangle index
			index_buffer.push_back(i * width);
			index_buffer.push_back(i * width + 1);
			index_buffer.push_back((i + 1) * width + 1);
			// second triangle index
			index_buffer.push_back(i * width);
			index_buffer.push_back((i + 1) * width + 1);
			index_buffer.push_back((i + 1) * width);
		}
	}
	return index_buffer;
}

void Cloth::update(float dt) {
	return;
}