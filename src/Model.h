#ifndef MODEL_H
#define MODEL_H

// A textured model loaded from an OBJ file (tinyobjloader) with PNG textures (stb_image),
// drawn with the interior lighting (shaders/model.vert + shaders/interior.frag)

// A material can be:
//   - textured (map_Kd) or plain colour (Kd)
//   - cut-out (map_d): pixels with alpha < 0.5 are discarded (pine branches, bare bush)
//   - lamp-lit glass (name contains "Window"): glows with the hut lights (L key)
//   - snow (name contains "Snow"): gets the snow glitter of the interior shader

// Animation by morphing: LoadMorph() reads a second OBJ with the same mesh in another pose;
// the vertex shader blends the two poses with the uniform morph (0..1).

#include <string>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

class Model
{
public:
	bool Load(const std::string& objPath);
	bool LoadMorph(const std::string& objPath);     // second pose, same mesh

	// Program must be the model shader, already active with camera + lights set
	void Draw(GLuint program, const glm::mat4& model, float morph = 0.0f) const;

	glm::vec3 boundsMin = glm::vec3(0.0f), boundsMax = glm::vec3(0.0f);

private:
	struct Part
	{
		GLuint vao = 0, vbo = 0;
		GLsizei count = 0;
		GLuint texture = 0;
		glm::vec3 color = glm::vec3(0.8f);
		float spec = 0.08f, shininess = 16.0f;
		bool cutout = false, windowLit = false, snow = false;
		size_t firstFloat = 0;                       // where this part starts in `vertices`
	};

	static const int FLOATS = 14;                    // pos(3) normal(3) uv(2) pos2(3) normal2(3)
	std::vector<Part> parts;
	std::vector<float> vertices;                     // kept until the morph pose is loaded
	std::string folder;

	GLuint LoadTexture(const std::string& file);
	void Upload(Part& part);
};

#endif
