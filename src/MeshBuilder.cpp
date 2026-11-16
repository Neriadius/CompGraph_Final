#include "MeshBuilder.h"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

#include "VAO.h"
#include "VBO.h"
#include "EBO.h"

namespace
{
	const float PI = 3.14159265358979f;

	// Position and normal transform of one primitive.
	// Normals use the inverse-transpose so non-uniform scaling (ellipsoids) stays correct.
	struct Xf
	{
		glm::mat4 m;
		glm::mat3 n;
		explicit Xf(const glm::mat4& xf) : m(xf), n(glm::transpose(glm::inverse(glm::mat3(xf)))) {}
		glm::vec3 P(const glm::vec3& p) const { return glm::vec3(m * glm::vec4(p, 1.0f)); }
		glm::vec3 N(const glm::vec3& v) const { return glm::normalize(n * v); }
	};
}

GLuint MeshBuilder::AddVertex(const glm::vec3& p, const glm::vec3& n, const glm::vec2& uv, const Material& m)
{
	GLuint index = VertexCount();
	vertices.insert(vertices.end(), {
		p.x, p.y, p.z,
		n.x, n.y, n.z,
		m.color.r, m.color.g, m.color.b,
		uv.x, uv.y,
		m.spec, m.shininess, (float)m.pattern, m.patternAmount,
		m.windowEmit, m.fireEmit });
	return index;
}

void MeshBuilder::Box(const glm::mat4& xf, const glm::vec3& h, const Material& m)
{
	Xf t(xf);
	struct Face { glm::vec3 n, u, v; };    // cross(u, v) = n -> counter-clockwise seen from outside
	static const Face faces[6] = {
		{ { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } },
		{ { -1, 0, 0 }, { 0, 0, 1 }, { 0, 1, 0 } },
		{ { 0, 1, 0 }, { 0, 0, 1 }, { 1, 0, 0 } },
		{ { 0, -1, 0 }, { 1, 0, 0 }, { 0, 0, 1 } },
		{ { 0, 0, 1 }, { 1, 0, 0 }, { 0, 1, 0 } },
		{ { 0, 0, -1 }, { 0, 1, 0 }, { 1, 0, 0 } },
	};
	for (const Face& f : faces)
	{
		float hn = glm::dot(glm::abs(f.n), h);
		float hu = glm::dot(glm::abs(f.u), h);
		float hv = glm::dot(glm::abs(f.v), h);
		glm::vec3 c = f.n * hn;
		glm::vec3 n = t.N(f.n);
		GLuint a = AddVertex(t.P(c - f.u * hu - f.v * hv), n, { -hu, -hv }, m);
		GLuint b = AddVertex(t.P(c + f.u * hu - f.v * hv), n, { hu, -hv }, m);
		GLuint d = AddVertex(t.P(c + f.u * hu + f.v * hv), n, { hu, hv }, m);
		GLuint e = AddVertex(t.P(c - f.u * hu + f.v * hv), n, { -hu, hv }, m);
		indices.insert(indices.end(), { a, b, d, a, d, e });
	}
}

void MeshBuilder::Cylinder(const glm::mat4& xf, float rBottom, float rTop, float height, int slices,
	const Material& side, const Material* caps)
{
	Xf t(xf);
	const float slope = (rBottom - rTop) / height;      // makes the normal tilt up on cones
	const float around = PI * (rBottom + rTop);         // average circumference (uv.x in world units)

	GLuint first = VertexCount();
	for (int j = 0; j <= slices; j++)
	{
		float a = 2.0f * PI * j / slices;
		float c = std::cos(a), s = std::sin(a);
		glm::vec3 n = t.N(glm::vec3(c, slope, s));
		float u = around * j / slices;
		AddVertex(t.P({ rBottom * c, 0.0f, rBottom * s }), n, { u, 0.0f }, side);
		AddVertex(t.P({ rTop * c, height, rTop * s }), n, { u, height }, side);
	}
	for (int j = 0; j < slices; j++)
	{
		GLuint a = first + 2 * j, b = a + 1, c = a + 2, d = a + 3;
		indices.insert(indices.end(), { a, b, d, a, d, c });
	}

	if (!caps) return;
	for (int cap = 0; cap < 2; cap++)
	{
		float r = cap ? rTop : rBottom;
		if (r <= 0.0f) continue;
		float y = cap ? height : 0.0f;
		glm::vec3 n = t.N({ 0.0f, cap ? 1.0f : -1.0f, 0.0f });
		GLuint centre = AddVertex(t.P({ 0.0f, y, 0.0f }), n, { 0.0f, 0.0f }, *caps);
		GLuint ring = VertexCount();
		for (int j = 0; j < slices; j++)
		{
			float a = 2.0f * PI * j / slices;
			glm::vec2 p(r * std::cos(a), r * std::sin(a));
			AddVertex(t.P({ p.x, y, p.y }), n, p, *caps);
		}
		for (int j = 0; j < slices; j++)
			AddTriangle(centre, ring + j, ring + (j + 1) % slices);
	}
}

void MeshBuilder::Log(const glm::mat4& xf, float radius, float length, const Material& bark,
	const Material& ends, int slices)
{
	// Cylinder along +Y turned to lie along +X, centred on its middle.
	glm::mat4 m = glm::rotate(xf, -0.5f * PI, glm::vec3(0, 0, 1));
	m = glm::translate(m, glm::vec3(0.0f, -0.5f * length, 0.0f));
	Cylinder(m, radius, radius, length, slices, bark, &ends);
}

void MeshBuilder::Sphere(const glm::mat4& xf, float radius, int stacks, int slices, const Material& m,
	float yMin, float yMax)
{
	Xf t(xf);
	const float phi0 = std::acos(glm::clamp(yMax, -1.0f, 1.0f));   // from the top
	const float phi1 = std::acos(glm::clamp(yMin, -1.0f, 1.0f));

	GLuint first = VertexCount();
	for (int i = 0; i <= stacks; i++)
	{
		float phi = phi0 + (phi1 - phi0) * i / stacks;
		for (int j = 0; j <= slices; j++)
		{
			float th = 2.0f * PI * j / slices;
			glm::vec3 n(std::sin(phi) * std::cos(th), std::cos(phi), std::sin(phi) * std::sin(th));
			AddVertex(t.P(n * radius), t.N(n), { th * radius, phi * radius }, m);
		}
	}
	for (int i = 0; i < stacks; i++)
		for (int j = 0; j < slices; j++)
		{
			GLuint a = first + i * (slices + 1) + j;
			GLuint b = a + slices + 1;
			indices.insert(indices.end(), { a, a + 1, b, a + 1, b + 1, b });
		}

	// Close a cut sphere with a flat disc so it is solid from every side
	if (yMin > -1.0f)
	{
		float y = yMin * radius, r = std::sqrt(std::max(0.0f, radius * radius - y * y));
		glm::vec3 n = t.N({ 0, -1, 0 });
		GLuint c = AddVertex(t.P({ 0, y, 0 }), n, { 0, 0 }, m);
		GLuint ring = VertexCount();
		for (int j = 0; j < slices; j++)
		{
			float th = 2.0f * PI * j / slices;
			AddVertex(t.P({ r * std::cos(th), y, r * std::sin(th) }), n, { r * std::cos(th), r * std::sin(th) }, m);
		}
		for (int j = 0; j < slices; j++) AddTriangle(c, ring + (j + 1) % slices, ring + j);
	}
}

void MeshBuilder::Torus(const glm::mat4& xf, float R, float r, int rings, int sides, const Material& m)
{
	Xf t(xf);
	GLuint first = VertexCount();
	for (int i = 0; i <= rings; i++)
	{
		float u = 2.0f * PI * i / rings;
		for (int j = 0; j <= sides; j++)
		{
			float v = 2.0f * PI * j / sides;
			glm::vec3 n(std::cos(v) * std::cos(u), std::sin(v), std::cos(v) * std::sin(u));
			glm::vec3 p((R + r * std::cos(v)) * std::cos(u), r * std::sin(v), (R + r * std::cos(v)) * std::sin(u));
			AddVertex(t.P(p), t.N(n), { u * R, v * r }, m);
		}
	}
	for (int i = 0; i < rings; i++)
		for (int j = 0; j < sides; j++)
		{
			GLuint a = first + i * (sides + 1) + j;
			GLuint b = a + sides + 1;
			indices.insert(indices.end(), { a, b, b + 1, a, b + 1, a + 1 });
		}
}

void MeshBuilder::Grid(int rows, int cols, bool wrapCols, const std::function<glm::vec3(int, int)>& pos,
	const std::function<glm::vec2(int, int)>& uv, const Material& m, bool flipNormals)
{
	std::vector<glm::vec3> p((size_t)rows * cols), nrm((size_t)rows * cols, glm::vec3(0.0f));
	for (int i = 0; i < rows; i++)
		for (int j = 0; j < cols; j++)
			p[(size_t)i * cols + j] = pos(i, j);

	std::vector<GLuint> local;
	const int jEnd = wrapCols ? cols : cols - 1;
	for (int i = 0; i < rows - 1; i++)
		for (int j = 0; j < jEnd; j++)
		{
			// Winding chosen so that rows running outwards / down a slope with columns
			// turning from +X towards +Z give normals pointing out of the surface.
			GLuint a = i * cols + j, b = i * cols + (j + 1) % cols;
			GLuint c = a + cols, d = b + cols;
			local.insert(local.end(), { a, d, c, a, b, d });
		}

	// Area-weighted face normals summed per vertex
	for (size_t k = 0; k < local.size(); k += 3)
	{
		glm::vec3 fn = glm::cross(p[local[k + 1]] - p[local[k]], p[local[k + 2]] - p[local[k]]);
		nrm[local[k]] += fn; nrm[local[k + 1]] += fn; nrm[local[k + 2]] += fn;
	}

	GLuint first = VertexCount();
	for (int i = 0; i < rows; i++)
		for (int j = 0; j < cols; j++)
		{
			size_t k = (size_t)i * cols + j;
			glm::vec3 n = glm::length(nrm[k]) > 0.0f ? glm::normalize(nrm[k]) : glm::vec3(0, 1, 0);
			AddVertex(p[k], flipNormals ? -n : n, uv(i, j), m);
		}
	for (GLuint idx : local) indices.push_back(first + idx);
}

void MeshBuilder::Facet(size_t firstIndex)
{
	std::vector<GLuint> old(indices.begin() + firstIndex, indices.end());
	indices.resize(firstIndex);
	const int F = FLOATS_PER_VERTEX;

	for (size_t k = 0; k + 2 < old.size(); k += 3)
	{
		glm::vec3 p[3], avg(0.0f);
		for (int v = 0; v < 3; v++)
		{
			const GLfloat* src = &vertices[(size_t)old[k + v] * F];
			p[v] = glm::vec3(src[0], src[1], src[2]);
			avg += glm::vec3(src[3], src[4], src[5]);
		}
		glm::vec3 fn = glm::cross(p[1] - p[0], p[2] - p[0]);
		if (glm::length(fn) < 1e-12f) continue;
		fn = glm::normalize(fn);
		if (glm::dot(fn, avg) < 0.0f) fn = -fn;          // keep facing the same way as the smooth normals

		for (int v = 0; v < 3; v++)
		{
			std::vector<GLfloat> copy(vertices.begin() + (size_t)old[k + v] * F,
				vertices.begin() + (size_t)old[k + v] * F + F);
			copy[3] = fn.x; copy[4] = fn.y; copy[5] = fn.z;
			indices.push_back(VertexCount());
			vertices.insert(vertices.end(), copy.begin(), copy.end());
		}
	}
}

glm::mat4 AlignY(const glm::vec3& from, const glm::vec3& to, float* length)
{
	glm::vec3 d = to - from;
	float len = glm::length(d);
	if (length) *length = len;
	glm::vec3 y = len > 0.0f ? d / len : glm::vec3(0, 1, 0);
	glm::vec3 up = std::fabs(y.y) < 0.99f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
	glm::vec3 x = glm::normalize(glm::cross(up, y));
	glm::vec3 z = glm::cross(x, y);
	return glm::mat4(glm::vec4(x, 0), glm::vec4(y, 0), glm::vec4(z, 0), glm::vec4(from, 1));
}

void GpuMesh::Upload(const MeshBuilder& mesh)
{
	count = (GLsizei)mesh.indices.size();
	vao = new VAO();
	vao->Bind();
	VBO vbo(const_cast<GLfloat*>(mesh.vertices.data()), (GLsizeiptr)(mesh.vertices.size() * sizeof(GLfloat)));
	EBO ebo(const_cast<GLuint*>(mesh.indices.data()), (GLsizeiptr)(mesh.indices.size() * sizeof(GLuint)));

	const GLsizeiptr stride = MeshBuilder::FLOATS_PER_VERTEX * sizeof(GLfloat);
	vao->LinkAttrib(vbo, 0, 3, GL_FLOAT, stride, (void*)0);
	vao->LinkAttrib(vbo, 1, 3, GL_FLOAT, stride, (void*)(3 * sizeof(GLfloat)));
	vao->LinkAttrib(vbo, 2, 3, GL_FLOAT, stride, (void*)(6 * sizeof(GLfloat)));
	vao->LinkAttrib(vbo, 3, 2, GL_FLOAT, stride, (void*)(9 * sizeof(GLfloat)));
	vao->LinkAttrib(vbo, 4, 4, GL_FLOAT, stride, (void*)(11 * sizeof(GLfloat)));
	vao->LinkAttrib(vbo, 5, 2, GL_FLOAT, stride, (void*)(15 * sizeof(GLfloat)));
	vao->Unbind();
}

void GpuMesh::Draw() const
{
	if (!vao || count == 0) return;
	vao->Bind();
	glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0);
}
