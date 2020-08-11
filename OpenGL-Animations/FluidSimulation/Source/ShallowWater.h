// 1D and 2D shallow water template
// by Yuxuan Huang

#include <vector>

using namespace std;

enum class boundary_condition{periodic, free, reflective};

class shallow1d {
public:
	int n; // n cells (surface division)

	// buffers for rendering
	vector<float> vertices;
	vector<float> normals;
	vector<int> indices;

	shallow1d();
	shallow1d(int num_of_divisions, float width_of_cell, float gravity, boundary_condition boundary,\
		float length, float width, float height, float scale);

	void waveUpdate(float dt);

	void set_h(int index, float value);
	void set_uh(int index, float value);
	void set_boundary(boundary_condition new_boundary_condition);

private:
	float g; // gravity

	float dx; // horizontal width of each cell
	vector<float> h; // height vector
	vector<float> uh; // momentum vector
	vector<float> hm; // midpoint height vector
	vector<float> uhm; // midpoint momentum vector

	boundary_condition b_cond; // boundary condition

	// render parameters
	float length, width, height; // length width height
	float scale; // scale of the waves

	void init();
	void init_buffer();

	void set_boundary(); // adjust the boundary cells according to the boundary condition type

	// rendering info update
	void update_vertex();
	void update_normal();
};


class shallow2d {
public:
	int nx, ny; // nx*ny cells (surface division)

	// buffers for rendering
	vector<float> vertices;
	vector<float> normals;
	vector<int> indices;

	shallow2d();
	shallow2d(int x_divisions, int y_divisions, float width_of_cell, float gravity, boundary_condition boundary, \
		float length, float width, float height, float scale);

	void waveUpdate(float dt, int substep);

	void set_h(int x_index, int y_index, float value);
	void set_uh(int x_index, int y_index, float value);
	void set_boundary(boundary_condition new_boundary_condition);

private:
	float g; // gravity

	float dx; // x width of each cell
	float dy; // y width of each cell
	vector<float> h; // height vector
	vector<float> uh; // momentum vector in x direction
	vector<float> vh; // momentum vector in y direction
	vector<float> xhm; // midpoint height vector in x direction
	vector<float> yhm; // midpoint height vector in y direction
	vector<float> uhmx; // midpoint u momentum vector in x direction
	vector<float> uhmy; // midpoint u momentum vector in y direction
	vector<float> vhmx; // midpoint v momentum vector in x direction
	vector<float> vhmy; // midpoint v momentum vector in y direction

	boundary_condition b_cond; // boundary condition

	// render parameters
	float length, width, height; // length width height
	float scale; // scale of the waves

	void init();
	void init_buffer();

	void set_boundary(); // adjust the boundary cells according to the boundary condition type

	// rendering info update
	void update_vertex();
	void update_normal();
};