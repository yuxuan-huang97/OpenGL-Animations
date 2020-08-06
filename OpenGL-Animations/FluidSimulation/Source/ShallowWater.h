// 1D and 2D shallow water template
// by Yuxuan Huang

#include <vector>

using namespace std;

enum class boundary_condition{periodic, free, reflective};

class shallow1d {
public:
	// buffers for rendering
	vector<float> vertices;
	vector<float> normals;
	vector<int> indices;

	shallow1d();
	shallow1d(int num_of_divisions, float width_of_cell, float gravity, boundary_condition boundary, float length, float width, float height);

	void waveUpdate(float dt);

private:
	float g; // gravity

	int n; // n cells (surface division)
	float dx; // horizontal width of each cell
	vector<float> h; // height vector
	vector<float> uh; // momentum vector
	vector<float> hm; // midpoint height vector
	vector<float> uhm; // midpoint momentum vector

	boundary_condition b_cond; // boundary condition

	// render parameters
	float length, width, height; // length width height

	void init();
	void init_buffer();

	void set_boundary(); // adjust the boundary cells according to the boundary condition type
};