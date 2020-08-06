// 1D and 2D shallow water template
// by Yuxuan Huang

#include "ShallowWater.h"

shallow1d::shallow1d() {
	g = 10.0f;
	n = 50;
	dx = 0.1f;
	b_cond = boundary_condition::free;
	init();
}

shallow1d::shallow1d(int num_of_divisions, float width_of_cell, float gravity, boundary_condition boundary,\
	float len, float wid, float hi, float s) {
	g = gravity;
	n = num_of_divisions;
	dx = width_of_cell;
	b_cond = boundary;
	length = len;
	width = wid;
	height = hi;
	scale = s;
	init();
}

void shallow1d::init() {
	for (int i = 0; i < n; i++) {
		h.push_back(0.0f);
		uh.push_back(0.0f);
		hm.push_back(0.0f);
		uhm.push_back(0.0f);
	}
	init_buffer();
}

void shallow1d::init_buffer() {
	// indices are fixed after initialization
	for (int i = 0; i < n - 1; i++) {
		// first triangle
		indices.push_back(2 * i);
		indices.push_back(2 * i + 2);
		indices.push_back(2 * i + 1);
		// second triangle
		indices.push_back(2 * i + 2);
		indices.push_back(2 * i + 3);
		indices.push_back(2 * i + 1);
	}
	// vertices and normals change at each frame
	for (int i = 0; i < 6 * n; i++) { // 2n vertices -> 3 * 2n = 6n floats
		vertices.push_back(0.0f);
		normals.push_back(0.0f);
	}
}

void shallow1d::waveUpdate(float dt) {

    // halfstep
    for (int i = 0; i < n - 1; i++) {
        hm[i] = (h[i] + h[i + 1]) / 2.0 - (dt / 2.0) * (uh[i + 1] - uh[i]) / dx;
        uhm[i] = (uh[i] + uh[i + 1]) / 2.0 - (dt / 2.0) *
            ((uh[i + 1] * uh[i + 1]) / h[i + 1] + 0.5 * g * h[i + 1] * h[i + 1] -
            (uh[i] * uh[i]) / h[i] - 0.5 * g * h[i] * h[i]) / dx;
    }
    // fullstep
    float damp = 0.1;
    for (int i = 0; i < n - 2; i++) {
        h[i + 1] -= dt * (uhm[i + 1] - uhm[i]) / dx;
        uh[i + 1] -= dt * (damp * uh[i + 1] +
            (uhm[i + 1] * uhm[i + 1]) / hm[i + 1] + 0.5 * g * hm[i + 1] * hm[i + 1] -
            uhm[i] * uhm[i] / hm[i] - 0.5 * g * hm[i] * hm[i]) / dx;
    }
	// boundary conditions
	set_boundary();

	// update buffers

}

void shallow1d::set_boundary() {
	switch (b_cond) {
	case boundary_condition::periodic:
		h[0] = h[n - 2];
		h[n - 1] = h[1];
		uh[0] = uh[n - 2];
		uh[n - 1] = uh[1];
		break;
	case boundary_condition::reflective:
		h[0] = h[1];
		h[n - 1] = h[n - 2];
		uh[0] = -uh[1];
		uh[n - 1] = -uh[n - 2];
		break;
	default: // default is free
		h[0] = h[1];
		h[n - 1] = h[n - 2];
		uh[0] = uh[1];
		uh[n - 1] = uh[n - 2];
	}
}

void shallow1d::update_vertex() {

}

void shallow1d::update_normal() {

}

/*
the direction of gravity?
*/