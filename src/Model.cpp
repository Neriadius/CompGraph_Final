#include "Model.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <glm/gtc/type_ptr.hpp>

#include "../libs/tiny_obj_loader.h"
#include "../libs/stb_image.h"

namespace
{
	bool Contains(const std::string& s, const char* what) { return s.find(what) != std::string::npos; }

	// Reads an OBJ into one triangle list per material: pos(3) normal(3) uv(2)
	bool ReadObj(const std::string& path, tinyobj::ObjReader& reader)
	{
		tinyobj::ObjReaderConfig config;
		config.triangulate = true;
		config.vertex_color = false;
		if (!reader.ParseFromFile(path, config))
		{
			std::cerr << "Model: cannot read " << path << ": " << reader.Error() << std::endl;
			return false;
		}
		if (!reader.Warning().empty()) std::cerr << "Model: " << reader.Warning();
		return true;
	}

	// Triangles in file order, grouped by material id (same order for both morph poses)
	std::map<int, std::vector<float>> Triangles(const tinyobj::ObjReader& reader)
	{
		const tinyobj::attrib_t& a = reader.GetAttrib();
		std::map<int, std::vector<float>> byMaterial;
		for (const tinyobj::shape_t& shape : reader.GetShapes())
		{
			const tinyobj::mesh_t& mesh = shape.mesh;
			for (size_t f = 0; f < mesh.num_face_vertices.size(); f++)
			{
				std::vector<float>& out = byMaterial[mesh.material_ids[f]];
				for (int k = 0; k < 3; k++)
				{
					const tinyobj::index_t& i = mesh.indices[3 * f + k];
					for (int c = 0; c < 3; c++) out.push_back(a.vertices[3 * i.vertex_index + c]);
					for (int c = 0; c < 3; c++) out.push_back(i.normal_index >= 0 ? a.normals[3 * i.normal_index + c] : (c == 1 ? 1.0f : 0.0f));
					out.push_back(i.texcoord_index >= 0 ? a.texcoords[2 * i.texcoord_index] : 0.0f);
					out.push_back(i.texcoord_index >= 0 ? a.texcoords[2 * i.texcoord_index + 1] : 0.0f);
				}
			}
		}
		return byMaterial;
	}
}

bool Model::Load(const std::string& objPath)
{
	tinyobj::ObjReader reader;
	if (!ReadObj(objPath, reader)) return false;
	size_t slash = objPath.find_last_of("/\\");
	folder = slash == std::string::npos ? "" : objPath.substr(0, slash + 1);

	const std::vector<tinyobj::material_t>& mats = reader.GetMaterials();
	boundsMin = glm::vec3(1e9f);
	boundsMax = glm::vec3(-1e9f);

	for (auto& [id, tris] : Triangles(reader))
	{
		Part part;
		if (id >= 0 && id < (int)mats.size())
		{
			const tinyobj::material_t& m = mats[id];
			part.color = glm::vec3(m.diffuse[0], m.diffuse[1], m.diffuse[2]);
			if (!m.diffuse_texname.empty()) part.texture = LoadTexture(m.diffuse_texname);
			part.cutout = !m.alpha_texname.empty();
			part.windowLit = Contains(m.name, "Window");
			part.snow = Contains(m.name, "Snow") || Contains(m.name, "snow");
			if (part.windowLit) { part.spec = 0.9f; part.shininess = 90.0f; }
			if (part.snow) { part.spec = 0.18f; part.shininess = 30.0f; }
		}
		// pos/normal/uv + a copy as the second pose (overwritten by LoadMorph)
		part.firstFloat = vertices.size();
		for (size_t v = 0; v + 8 <= tris.size(); v += 8)
		{
			vertices.insert(vertices.end(), tris.begin() + v, tris.begin() + v + 8);
			vertices.insert(vertices.end(), tris.begin() + v, tris.begin() + v + 6);
			glm::vec3 p(tris[v], tris[v + 1], tris[v + 2]);
			boundsMin = glm::min(boundsMin, p);
			boundsMax = glm::max(boundsMax, p);
		}
		part.count = (GLsizei)(tris.size() / 8);
		parts.push_back(part);
	}
	for (Part& p : parts) Upload(p);
	std::cout << "Model: " << objPath << " (" << parts.size() << " parts)" << std::endl;
	return !parts.empty();
}

bool Model::LoadMorph(const std::string& objPath)
{
	tinyobj::ObjReader reader;
	if (!ReadObj(objPath, reader)) return false;
	auto byMaterial = Triangles(reader);
	size_t p = 0;
	for (auto& [id, tris] : byMaterial)
	{
		(void)id;
		if (p >= parts.size() || (GLsizei)(tris.size() / 8) != parts[p].count)
		{
			std::cerr << "Model: " << objPath << " does not match the first pose" << std::endl;
			return false;
		}
		float* dst = &vertices[parts[p].firstFloat];
		for (size_t v = 0, k = 0; v + 8 <= tris.size(); v += 8, k++)
			std::copy(tris.begin() + v, tris.begin() + v + 6, dst + k * FLOATS + 8);   // second pose slot
		glBindBuffer(GL_ARRAY_BUFFER, parts[p].vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(parts[p].count * FLOATS * sizeof(float)), dst);
		p++;
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	return true;
}

void Model::Upload(Part& part)
{
	glGenVertexArrays(1, &part.vao);
	glGenBuffers(1, &part.vbo);
	glBindVertexArray(part.vao);
	glBindBuffer(GL_ARRAY_BUFFER, part.vbo);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(part.count * FLOATS * sizeof(float)),
		&vertices[part.firstFloat], GL_STATIC_DRAW);
	const GLsizei stride = FLOATS * sizeof(float);
	const int sizes[5] = { 3, 3, 2, 3, 3 };
	size_t offset = 0;
	for (int a = 0; a < 5; a++)
	{
		glVertexAttribPointer(a, sizes[a], GL_FLOAT, GL_FALSE, stride, (void*)(offset * sizeof(float)));
		glEnableVertexAttribArray(a);
		offset += sizes[a];
	}
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GLuint Model::LoadTexture(const std::string& file)
{
	int w, h, n;
	stbi_set_flip_vertically_on_load(1);            // OBJ texture coordinates start at the bottom
	unsigned char* data = stbi_load((folder + file).c_str(), &w, &h, &n, 4);
	if (!data)
	{
		std::cerr << "Model: cannot load texture " << folder + file << std::endl;
		return 0;
	}
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(data);
	return tex;
}

void Model::Draw(GLuint program, const glm::mat4& model, float morph) const
{
	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
	glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix3fv(glGetUniformLocation(program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
	glUniform1f(glGetUniformLocation(program, "morph"), morph);
	glUniform1i(glGetUniformLocation(program, "albedoMap"), 0);
	glActiveTexture(GL_TEXTURE0);

	for (const Part& p : parts)
	{
		const float pattern = p.snow ? 3.0f : 0.0f;                  // PAT_SNOW: glitter
		glUniform3fv(glGetUniformLocation(program, "baseColor"), 1, glm::value_ptr(p.color));
		glUniform4f(glGetUniformLocation(program, "material"), p.spec, p.shininess, pattern, p.snow ? 1.0f : 0.0f);
		glUniform2f(glGetUniformLocation(program, "emission"), p.windowLit ? 1.0f : 0.0f, 0.0f);
		glUniform1i(glGetUniformLocation(program, "useTexture"), p.texture ? 1 : 0);
		glUniform1i(glGetUniformLocation(program, "alphaCutout"), p.cutout ? 1 : 0);
		glBindTexture(GL_TEXTURE_2D, p.texture);
		glBindVertexArray(p.vao);
		glDrawArrays(GL_TRIANGLES, 0, p.count);
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}
