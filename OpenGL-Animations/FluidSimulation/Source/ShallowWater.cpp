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
	
	for (int i = 0; i < nx * ny; i++) {
		h.push_back(1.0f);
		uh.push_back(0.0f);
		vh.push_back(0.0f);
	}
	for (int i = 0; i < (nx - 1) * (ny - 2); i++) {
		xhm.push_back(1.0f);
		uhmx.push_back(0.0f);
		vhmx.push_back(0.0f);
	}
	for (int i = 0; i < (nx - 2) * (ny - 1); i++) {
		yhm.push_back(1.0f);
		uhmy.push_back(0.0f);
		vhmy.push_back(0.0f);
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
	// for x
	//printf("x:\n");
	for (int i = 0; i < nx - 1; i++) {
		for (int j = 0; j < ny - 2; j++) {
			int ind = i * (ny - 2) + j;
			int indx = i * ny + j + 1;
			int next_x = (i + 1) * ny + j + 1;
			
			xhm[ind] = (h[indx] + h[next_x]) / 2.0 - (dt / 2.0) * (uh[next_x] - uh[indx]) / dx;
			uhmx[ind] = (uh[indx] + uh[next_x]) / 2.0 - (dt / 2.0) * 
				((uh[next_x] * uh[next_x]) / h[next_x] + 0.5 * g * h[next_x] * h[next_x] - 
				(uh[indx] * uh[indx]) / h[indx] - 0.5 * g * h[indx] * h[indx]) / dx;
			vhmx[ind] = (vh[indx] + vh[next_x]) / 2.0 - (dt / 2.0) * 
				((uh[next_x] * vh[next_x]) / h[next_x]  - (uh[indx] * vh[indx]) / h[indx]) / dx;
			/*
			float num = (vh[indx] + vh[next_x]) / 2.0 - (dt / 2.0) *
				((uh[next_x] * vh[next_x]) / h[next_x] - (uh[indx] * vh[indx]) / h[indx]) / dx;
			
			printf("%f\n", uhmx[ind]);*/
		}
	} 
	// for y
	//printf("y:\n");
	for (int j = 0; j < ny - 1; j++) {
		for (int i = 0; i < nx - 2; i++) {
			int ind = i * (ny - 1) + j;
			int indy = (i + 1) * ny + j;
			//int next_y = indy + 1;
			yhm[ind] = (h[indy] + h[indy + 1]) / 2.0 - (dt / 2.0) * (vh[indy + 1] - vh[indy]) / dy;
			vhmy[ind] = (vh[indy] + vh[indy + 1]) / 2.0 - (dt / 2.0) * 
				((vh[indy + 1] * vh[indy + 1]) / h[indy + 1] + 0.5 * g * h[indy + 1] * h[indy + 1] - 
				(vh[indy] * vh[indy]) / h[indy] - 0.5 * g * h[indy] * h[indy]) / dy;
			uhmy[ind] = (uh[indy] + uh[indy + 1]) / 2.0 - (dt / 2.0) * 
				((vh[indy + 1] * uh[indy + 1]) / h[indy + 1] - (vh[indy] * uh[indy]) / h[indy]) / dy;
			
			/*float num = (uh[indy] + uh[indy + 1]) / 2.0 - (dt / 2.0) *
				((vh[indy + 1] * uh[indy + 1]) / h[indy + 1] - (vh[indy] * uh[indy]) / h[indy]) / dy;
			
			printf("%f\n", vhmy[ind]);*/
		}
	}
	//printf("\n");
	
	// fullstep
	float damp = 0.2;
	for (int i = 0; i < nx - 2; i++) {
		for (int j = 0; j < ny - 2; j++) {

			// indices
			int index = (i + 1) * ny + j + 1;
			int x_mid = (i + 1) * (ny - 2) + j;
			int x_prev = i * (ny - 2) + j;
			int y_mid = i * (ny - 1) + j + 1;
			int y_prev = i * (ny - 1) + j;

			//update
			h[index] -= dt * ((uhmx[x_mid] - uhmx[x_prev]) / dx + (vhmy[y_mid] - vhmy[y_prev]) / dy);
			uh[index] -= dt * (damp * uh[index] + 
				(uhmx[x_mid] * uhmx[x_mid] / xhm[x_mid] + 0.5 * g * xhm[x_mid] * xhm[x_mid] - 
				uhmx[x_prev] * uhmx[x_prev] / xhm[x_prev] - 0.5 * g * xhm[x_prev] * xhm[x_prev]) / dx + 
				(uhmy[y_mid] * vhmy[y_mid] / yhm[y_mid] - uhmy[y_prev] * vhmy[y_prev] / yhm[y_prev]) / dy); // this line potentially buggy
			vh[index] -= dt * (damp * vh[index] + 
				(uhmx[x_mid] * vhmx[x_mid] / xhm[x_mid] - uhmx[x_prev] * vhmx[x_prev] / xhm[x_prev]) / dx + 
				(vhmy[y_mid] * vhmy[y_mid] / yhm[y_mid] + 0.5 * g * yhm[y_mid] * yhm[y_mid] - 
				vhmy[y_prev] * vhmy[y_prev] / yhm[y_prev] - 0.5 * g * yhm[y_prev] * yhm[y_prev]) / dy);
			//printf("%f\n%f\n\n", uhmy[y_mid], uhmy[y_prev]);

		}
	}
	//printf("\n");
	// boundary conditions
	set_boundary();


	// update buffers
	update_vertex();
	update_normal();
}

// potentially problematic
void shallow2d::set_boundary(){
	switch (b_cond) {
	case boundary_condition::periodic:
		for (int i = 0; i < ny; i++) {
			h[i] = h[ny * (nx - 2) + i];
			h[ny * (nx - 1) + i] = h[i + ny];
			uh[i] = uh[ny * (nx - 2) + i];
			uh[ny * (nx - 1) + i] = uh[i + ny];
			vh[i] = vh[ny * (nx - 2) + i];
			vh[ny * (nx - 1) + i] = vh[i + ny];
		}
		for (int i = 1; i < ny - 1; i++) {
			h[i * ny] = h[(i + 1) * ny - 2];
			h[(i + 1) * ny - 1] = h[i * ny + 1];
			uh[i * ny] = uh[(i + 1) * ny - 2];
			uh[(i + 1) * ny - 1] = uh[i * ny + 1];
			vh[i * ny] = vh[(i + 1) * ny - 2];
			vh[(i + 1) * ny - 1] = vh[i * ny + 1];
		}

	case boundary_condition::reflective:
		for (int i = 0; i < ny; i++) {
			h[i] = h[i + ny];
			h[ny * (nx - 1) + i] = h[ny * (nx - 2) + i];
			uh[i] = uh[i + ny];
			uh[ny * (nx - 1) + i] = -uh[ny * (nx - 2) + i];
			vh[i] = vh[i + ny];
			vh[ny * (nx - 1) + i] = -vh[ny * (nx - 2) + i];
		}
		for (int i = 1; i < ny - 1; i++) {
			h[i * ny] = h[i * ny + 1];
			h[(i + 1) * ny - 1] = h[(i + 1) * ny - 2];
			uh[i * ny] = uh[i * ny + 1];
			uh[(i + 1) * ny - 1] = -uh[(i + 1) * ny - 2];
			vh[i * ny] = vh[i * ny + 1];
			vh[(i + 1) * ny - 1] = -vh[(i + 1) * ny - 2];
		}
		break;
	default: // default is free
		for (int i = 0; i < ny; i++) {
			h[i] = h[i + ny];
			h[ny * (nx - 1) + i] = h[ny * (nx - 2) + i];
			uh[i] = uh[i + ny];
			uh[ny * (nx - 1) + i] = uh[ny * (nx - 2) + i];
			vh[i] = vh[i + ny];
			vh[ny * (nx - 1) + i] = vh[ny * (nx - 2) + i];
		}
		for (int i = 1; i < ny - 1; i++) {
			h[i * ny] = h[i * ny + 1];
			h[(i + 1) * ny - 1] = h[(i + 1) * ny - 2];
			uh[i * ny] = uh[i * ny + 1];
			uh[(i + 1) * ny - 1] = uh[(i + 1) * ny - 2];
			vh[i * ny] = vh[i * ny + 1];
			vh[(i + 1) * ny - 1] = vh[(i + 1) * ny - 2];
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
	}
}

void shallow2d::update_normal() {
	// clear existing normals
	#pragma omp parallel for
	for (int i = 0; i < normals.size(); i++) normals[i] = 0.0f;
	// compute new normals
	for (int i = 0; i < nx - 1; i++) {
		for (int j = 0; j < ny - 1; j++) {
			// find indices
			int ind0 = 3 * (i * ny + j);
			int ind1 = 3 * (i * ny + j + 1);
			int ind2 = 3 * ((i + 1) * ny + j + 1);
			int ind3 = 3 * ((i + 1) * ny + j);
			// look up vertices
			glm::vec3 vert0(vertices[ind0], vertices[ind0 + 1], vertices[ind0 + 2]);
			glm::vec3 vert1(vertices[ind1], vertices[ind1 + 1], vertices[ind1 + 2]);
			glm::vec3 vert2(vertices[ind2], vertices[ind2 + 1], vertices[ind2 + 2]);
			glm::vec3 vert3(vertices[ind3], vertices[ind3 + 1], vertices[ind3 + 2]);
			// compute vectors
			glm::vec3 v0 = glm::normalize(vert0 - vert3);
			glm::vec3 v1 = glm::normalize(vert1 - vert3);
			glm::vec3 v2 = glm::normalize(vert2 - vert3);
			// compute normals
			glm::vec3 n0 = glm::cross(v0, v1);
			glm::vec3 n1 = glm::cross(v1, v2);
			// assign normals
			normals[ind0] += n0.x; normals[ind0 + 1] += n0.y; normals[ind0 + 2] += n0.z;
			normals[ind1] += n0.x; normals[ind1 + 1] += n0.y; normals[ind1 + 2] += n0.z;
			normals[ind3] += n0.x; normals[ind3 + 1] += n0.y; normals[ind3 + 2] += n0.z;
			normals[ind1] += n1.x; normals[ind1 + 1] += n1.y; normals[ind1 + 2] += n1.z;
			normals[ind2] += n1.x; normals[ind2 + 1] += n1.y; normals[ind2 + 2] += n1.z;
			normals[ind3] += n1.x; normals[ind3 + 1] += n1.y; normals[ind3 + 2] += n1.z;
		}
	}
	// normalize normals
	for (int i = 0; i < nx * ny; i++) {
		glm::vec3 tmp(normals[3 * i], normals[3 * i + 1], normals[3 * i + 2]);
		tmp = glm::normalize(tmp);
		normals[3 * i] = tmp.x; normals[3 * i + 1] = tmp.y; normals[3 * i + 2] = tmp.z;
	}
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