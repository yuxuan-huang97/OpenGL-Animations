// 1D and 2D shallow water template
// by Yuxuan Huang

#define GLM_FORCE_RADIANS
#include "../../../glm/glm.hpp"
#include "../../../glm/gtc/matrix_transform.hpp"
#include "../../../glm/gtc/type_ptr.hpp"
#include "ShallowWater.h"

// ============= 1D shallow water =====================

shallow1d::shallow1d() {
	g = 10.0f;
	n = 50;
	dx = 0.1f;
	b_cond = boundary_condition::free;
	length = 10.f;
	width = 10.f;
	height = 0.f;
	scale = 50.f;
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
	for (int i = 0; i < n - 1; i++) {
		h.push_back(1.0f);
		uh.push_back(0.0f);
		hm.push_back(0.0f);
		uhm.push_back(0.0f);
	}
	h.push_back(1.0f);
	uh.push_back(0.0f);
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
        uhm[i] = (uh[i] + uh[i + 1]) / 2.0 - (dt / 2.0) * \
            ((uh[i + 1] * uh[i + 1]) / h[i + 1] + 0.5 * g * h[i + 1] * h[i + 1] - \
            (uh[i] * uh[i]) / h[i] - 0.5 * g * h[i] * h[i]) / dx;
    }
    // fullstep
    float damp = 0.1;
    for (int i = 0; i < n - 2; i++) {
        h[i + 1] -= dt * (uhm[i + 1] - uhm[i]) / dx;
        uh[i + 1] -= dt * (damp * uh[i + 1] + \
            (uhm[i + 1] * uhm[i + 1]) / hm[i + 1] + 0.5 * g * hm[i + 1] * hm[i + 1] - \
            uhm[i] * uhm[i] / hm[i] - 0.5 * g * hm[i] * hm[i]) / dx;
    }
	// boundary conditions
	set_boundary();
	

	// update buffers
	update_vertex();
	update_normal();
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
	float left = -length / 2.0;
	float near = width / 2.0;
	float step = length / (n - 1);
	for (int i = 0; i < n; i++) {
		// near vertex
		vertices[6 * i] = near;
		vertices[6 * i + 1] = left + i * step;
		vertices[6 * i + 2] = height + h[i];
		// far vertex
		vertices[6 * i + 3] = -near;
		vertices[6 * i + 4] = left + i * step;
		vertices[6 * i + 5] = height + h[i];
	}
}

void shallow1d::update_normal() {
	// normals of boundary vertices
	glm::vec3 ntmp = glm::normalize(glm::vec3(0.f, vertices[2] - vertices[8], vertices[7] - vertices[1])); // deltax(0), -deltaz, deltay
	normals[1] = ntmp.y; // x component is always 0
	normals[2] = ntmp.z;
	normals[4] = ntmp.y;
	normals[5] = ntmp.z;
	ntmp = glm::normalize(glm::vec3(0.f, vertices[6 * n - 10] - vertices[6 * n - 4], vertices[6 * n - 5] - vertices[6 * n - 11]));
	normals[6 * n - 5] = ntmp.y;
	normals[6 * n - 4] = ntmp.z;
	normals[6 * n - 2] = ntmp.y;
	normals[6 * n - 1] = ntmp.z;
	// normals of other vertices
	for (int i = 1; i < n - 1; i++) {
		ntmp = glm::normalize(glm::vec3(0.f, vertices[6 * i - 4] - vertices[6 * i + 8], vertices[6 * i + 7] - vertices[6 * i - 5]));
		normals[6 * i + 1] = ntmp.y;
		normals[6 * i + 2] = ntmp.z;
		normals[6 * i + 4] = ntmp.y;
		normals[6 * i + 5] = ntmp.z;
	}
}

void shallow1d::set_h(int index, float value) {
	h[index] = value;
}

void shallow1d::set_uh(int index, float value) {
	uh[index] = value;
}

void shallow1d::set_boundary(boundary_condition bc) {
	b_cond = bc;
}


// ============= 2D shallow water =====================

shallow2d::shallow2d() {
	g = 10.0f;
	nx = 50;
	ny = 50;
	dx = 0.1f;
	dy = dx;
	b_cond = boundary_condition::free;
	length = 10.f;
	width = 10.f;
	height = 0.f;
	scale = 50.f;
	init();
}

shallow2d::shallow2d(int x_divisions, int y_divisions, float width_of_cell, float gravity, boundary_condition boundary, \
	float len, float wid, float hi, float s) {
	g = gravity;
	nx = x_divisions;
	ny = y_divisions;
	dx = width_of_cell;
	dy = dx;
	b_cond = boundary;
	length = len;
	width = wid;
	height = hi;
	scale = s;
	init();
}


void shallow2d::init() {
	for (int i = 0; i < (nx - 1) * (ny - 1); i++) {
		h.push_back(1.0f);
		uh.push_back(0.0f);
		vh.push_back(0.0f);
		xhm.push_back(0.0f);
		yhm.push_back(0.0f);
		uhm.push_back(0.0f);
		vhm.push_back(0.0f);
	}
	for (int i = 0; i < nx + ny - 1; i++) {
		h.push_back(1.0f);
		uh.push_back(0.0f);
		vh.push_back(0.0f);
	}
	init_buffer();
}

void shallow2d::init_buffer() {
	// indices are fixed after initialization
	for (int i = 0; i < nx - 1; i++) { // rows
		for (int j = 0; j < ny - 1; j++) { // columns
			// first triangle
			indices.push_back(nx * i + j);
			indices.push_back(nx * i + j + 1);
			indices.push_back(nx * (i + 1) + j);
			//second triangle
			indices.push_back(nx * i + j + 1);
			indices.push_back(nx * (i + 1) + j + 1);
			indices.push_back(nx * (i + 1) + j);
		}
	}
	// vertices and normals change at each frame
	for (int i = 0; i < 3 * nx * ny; i++) { // nx * ny vertices -> 3 * nx * ny floats
		vertices.push_back(0.0f);
		normals.push_back(0.0f);
	}
}

void shallow2d::waveUpdate(float dt) {

	// halfstep
	for (int i = 0; i < nx - 1; i++) { // for x
		xhm[i] = (h[i] + h[i + 1]) / 2.0 - (dt / 2.0) * (uh[i + 1] - uh[i]) / dx;

		uhm[i] = (uh[i] + uh[i + 1]) / 2.0 - (dt / 2.0) * \
			((uh[i + 1] * uh[i + 1]) / h[i + 1] + 0.5 * g * h[i + 1] * h[i + 1] - \
			(uh[i] * uh[i]) / h[i] - 0.5 * g * h[i] * h[i]) / dx;
	}
	for (int i = 0; i < ny - 1; i++) { // for y
		yhm[i] = (h[i] + h[i + 1]) / 2.0 - (dt / 2.0) * (vh[i + 1] - vh[i]) / dy;

		vhm[i] = (vh[i] + vh[i + 1]) / 2.0 - (dt / 2.0) * \
			((vh[i + 1] * vh[i + 1]) / h[i + 1] + 0.5 * g * h[i + 1] * h[i + 1] - \
			(vh[i] * vh[i]) / h[i] - 0.5 * g * h[i] * h[i]) / dy;
	}
	// fullstep
	float damp = 0.1;
	for (int i = 0; i < nx - 2; i++) {
		for (int j = 0; j < ny - 2; j++) {
			h[(i + 1) * ny + j + 1] -= dt * (((uhm[i + 1] - uhm[i]) / dx + (vhm[j + 1] - vhm[j]) / dy));
			uh[i + 1] -= dt * (damp * uh[i + 1] + \
				(uhm[i + 1] * uhm[i + 1]) / xhm[i + 1] + 0.5 * g * xhm[i + 1] * xhm[i + 1] - \
				uhm[i] * uhm[i] / xhm[i] - 0.5 * g * xhm[i] * xhm[i]) / dx + (uhm[i + 1] * vhm[j + 1] /\
					xhm[i + 1] - uhm[i] * vhm[i] / xhm[i]) / dy;
			vh[i + 1] -= dt * (damp * vh[i + 1] + \
				(uhm[i + 1] * vhm[j + 1] / yhm[i + 1] - uhm[i] * vhm[i] / yhm[i]) / dx + \
				(vhm[i + 1] * vhm[i + 1]) / yhm[i + 1] + 0.5 * g * yhm[i + 1] * yhm[i + 1] - \
				vhm[i] * vhm[i] / yhm[i] - 0.5 * g * yhm[i] * yhm[i]) / dy;
		}
	}
	// boundary conditions
	set_boundary();


	// update buffers
	update_vertex();
	update_normal();
}

void shallow2d::set_boundary(){
	switch (b_cond) {
	case boundary_condition::periodic:
		for (int i = 0; i < nx; i++) {
			h[i] = h[(ny - 2) * nx + i];
			h[(ny - 1) * nx + i] = h[i + nx];
			uh[i] = uh[(ny - 2) * nx + i];
			uh[(ny - 1) * nx + i] = uh[i + nx];
			vh[i] = vh[(ny - 2) * nx + i];
			vh[(ny - 1) * nx + i] = vh[i + nx];
		}
		for (int i = 1; i < ny - 1; i++) {
			h[i * nx] = h[i * nx + 1];
			h[(i + 1) * nx - 1] = h[(i + 1) * nx - 2];
			uh[i * nx] = uh[i * nx + 1];
			uh[(i + 1) * nx - 1] = uh[(i + 1) * nx - 2];
			vh[i * nx] = vh[i * nx + 1];
			vh[(i + 1) * nx - 1] = vh[(i + 1) * nx - 2];
		}

	case boundary_condition::reflective:
		for (int i = 0; i < nx; i++) {
			h[i] = h[i + nx];
			h[(ny - 1) * nx + i] = h[(ny - 2) * nx + i];
			uh[i] = uh[i + nx];
			uh[(ny - 1) * nx + i] = uh[(ny - 2) * nx + i];
			vh[i] = h[i + nx];
			vh[(ny - 1) * nx + i] = vh[(ny - 2) * nx + i];
		}
		for (int i = 1; i < ny - 1; i++) {
			h[i * nx] = h[i * nx + 1];
			h[(i + 1) * nx - 1] = h[(i + 1) * nx - 2];
			uh[i * nx] = -uh[i * nx + 1];
			uh[(i + 1) * nx - 1] = -uh[(i + 1) * nx - 2];
			vh[i * nx] = -vh[i * nx + 1];
			vh[(i + 1) * nx - 1] = -vh[(i + 1) * nx - 2];
		}
		break;
	default: // default is free
		for (int i = 0; i < nx; i++) {
			h[i] = h[i + nx];
			h[(ny - 1) * nx + i] = h[(ny - 2) * nx + i];
			uh[i] = uh[i + nx];
			uh[(ny - 1) * nx + i] = uh[(ny - 2) * nx + i];
			vh[i] = h[i + nx];
			vh[(ny - 1) * nx + i] = vh[(ny - 2) * nx + i];
		}
		for (int i = 1; i < ny - 1; i++) {
			h[i * nx] = h[i * nx + 1];
			h[(i + 1) * nx - 1] = h[(i + 1) * nx - 2];
			uh[i * nx] = uh[i * nx + 1];
			uh[(i + 1) * nx - 1] = uh[(i + 1) * nx - 2];
			vh[i * nx] = vh[i * nx + 1];
			vh[(i + 1) * nx - 1] = vh[(i + 1) * nx - 2];
		}
	}
}

void shallow2d::update_vertex() {
	float left = -length / 2.0;
	float near = width / 2.0;
	float xstep = length / (nx - 1);
	float ystep = length / (ny - 1);
	for (int i = 0; i < nx; i++) {
		for (int j = 0; j < ny; j++) {
			vertices[3 * (i * ny + j)] = near - i * xstep;
			vertices[3 * (i * ny + j) + 1] = left + j * xstep;
			vertices[3 * (i * ny + j) + 2] = height + h[i * ny + j];
		}
		/*
		// near vertex
		vertices[6 * i] = near;
		vertices[6 * i + 1] = left + i * step;
		vertices[6 * i + 2] = height + h[i];
		// far vertex
		vertices[6 * i + 3] = -near;
		vertices[6 * i + 4] = left + i * step;
		vertices[6 * i + 5] = height + h[i];
		*/
	}
}

void shallow2d::update_normal() {
	return;
}

void shallow2d::set_h(int x, int y, float value) {
	h[x * ny + y] = value;
}

void shallow2d::set_uh(int x, int y, float value) {
	uh[x * ny + y] = value;
}

void shallow2d::set_boundary(boundary_condition bc) {
	b_cond = bc;
}