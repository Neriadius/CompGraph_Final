#ifndef MESH_BUILDER_H
#define MESH_BUILDER_H

// Builds meshes out of primitives (box, cylinder/cone, sphere, lathe, prism, torus, grid)
// for the interior of the globe (Person 3). Every primitive takes a transform `xf`
// (local -> mesh space), so parts can be placed, rotated and scaled freely and many parts
// end up in ONE vertex buffer = one draw call.
//
// Vertex layout (17 floats), matches shaders/interior.vert:
//   0 position(3)  1 normal(3)  2 colour(3)  3 uv(2)
//   4 material(4) = specular strength, shininess, pattern id, pattern amount
//   5 emission(2) = window-light weight, fire-light weight

#include <functional>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

class VAO;

// Procedural surface patterns, computed in shaders/interior.frag (no texture files)
enum Pattern
{
	PAT_NONE = 0,
	PAT_BARK = 1,      // grain lines along the log (uv.y = along the log)
	PAT_RINGS = 2,     // tree rings on cut log ends (uv = position on the cap)
	PAT_SNOW = 3,      // soft variation + glitter
	PAT_STONE = 4,     // mottled stone
	PAT_PLANKS = 5,    // vertical boards (uv.x across the boards)
	PAT_NEEDLES = 6,   // pine needles: streaks down the cone
	PAT_KNIT = 7,      // knitted wool (hat, scarf, mittens)
};

struct Material
{
	glm::vec3 color = glm::vec3(0.8f);
	float spec = 0.08f;          // specular strength
	float shininess = 16.0f;
	int pattern = PAT_NONE;
	float patternAmount = 0.0f;
	float windowEmit = 0.0f;     // glows with the hut lights (L)
	float fireEmit = 0.0f;       // glows with the campfire flicker (F)
};

class MeshBuilder
{
public:
	static const int FLOATS_PER_VERTEX = 17;

	std::vector<GLfloat> vertices;
	std::vector<GLuint> indices;

	GLuint VertexCount() const { return (GLuint)(vertices.size() / FLOATS_PER_VERTEX); }

	GLuint AddVertex(const glm::vec3& p, const glm::vec3& n, const glm::vec2& uv, const Material& m);
	void AddTriangle(GLuint a, GLuint b, GLuint c) { indices.insert(indices.end(), { a, b, c }); }

	// Box centred on the origin of xf, half sizes h. uv = face coordinates in world units.
	void Box(const glm::mat4& xf, const glm::vec3& h, const Material& m);

	// Cylinder / cone / frustum along +Y from y=0 to y=height.
	// caps: optional material for the end discs (nullptr = open ends).
	void Cylinder(const glm::mat4& xf, float rBottom, float rTop, float height, int slices,
		const Material& side, const Material* caps = nullptr);

	// Log lying along +X, centred on the xf origin: bark side + tree-ring ends.
	void Log(const glm::mat4& xf, float radius, float length, const Material& bark, const Material& ends,
		int slices = 12);

	// UV sphere; scale xf for ellipsoids. yMin/yMax (-1..1) cut it (e.g. 0..1 = upper half).
	void Sphere(const glm::mat4& xf, float radius, int stacks, int slices, const Material& m,
		float yMin = -1.0f, float yMax = 1.0f);

	// Surface of revolution around +Y; profile points are (radius, y) from bottom to top.
	void Lathe(const glm::mat4& xf, const std::vector<glm::vec2>& profile, int slices, const Material& m);

	// Convex polygon in the XY plane extruded along Z from -depth/2 to +depth/2.
	void Prism(const glm::mat4& xf, const std::vector<glm::vec2>& polygon, float depth, const Material& m,
		const Material* sides = nullptr);

	// Torus around +Y (ring radius R, tube radius r).
	void Torus(const glm::mat4& xf, float R, float r, int rings, int sides, const Material& m);

	// Grid of rows x cols points from pos(i, j). Normals are averaged from the faces.
	// wrapCols: column cols-1 connects back to column 0 (closed ring).
	// Orientation: rows going outwards (or down a cone) and columns turning from +X to +Z
	// give outward normals; flipNormals reverses them.
	void Grid(int rows, int cols, bool wrapCols, const std::function<glm::vec3(int, int)>& pos,
		const std::function<glm::vec2(int, int)>& uv, const Material& m, bool flipNormals = false);

	// Turns the triangles added since `firstIndex` into flat-shaded facets (low-poly look).
	void Facet(size_t firstIndex);
};

// Frame at `from` whose +Y axis points to `to` (for limbs, leaning logs, cylinders between
// two points). Returns the distance through `length`.
glm::mat4 AlignY(const glm::vec3& from, const glm::vec3& to, float* length = nullptr);

// The uploaded mesh: one VAO, one glDrawElements.
struct GpuMesh
{
	VAO* vao = nullptr;
	GLsizei count = 0;

	void Upload(const MeshBuilder& mesh);
	void Draw() const;
};

#endif
