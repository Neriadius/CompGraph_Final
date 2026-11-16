#ifndef INTERIOR_MODELS_H
#define INTERIOR_MODELS_H

// Procedural geometry of the globe interior, built from primitives into one mesh:
// snow ground, fire pit, seats, and props around the hut (HutProps.cpp).

// The hut, trees, snowman and Typhoeus are OBJ models (models/, drawn by Interior.cpp).

#include <vector>
#include <glm/glm.hpp>

#include "MeshBuilder.h"

// Colour palette shared by the interior models
namespace palette
{
	Material Snow();
	Material PackedSnow();
	Material Bark();
	Material LogEnds();
	Material Stone(const glm::vec3& color = glm::vec3(0.50f, 0.50f, 0.54f));
	Material Metal();
	Material Plain(const glm::vec3& color, float spec = 0.06f, float shininess = 12.0f);
}

// Where the lights sit, filled while the geometry is built
struct InteriorAnchors
{
	std::vector<glm::vec3> windowLights;     // just outside the hut's window
	glm::vec3 lantern = glm::vec3(0.0f);
	glm::vec3 fire = glm::vec3(0.0f);        // centre of the fire pit on the ground
	glm::vec3 chimneyTop = glm::vec3(0.0f);
};

// y of the visible snow surface at (x, z), globe space
float GroundY(float x, float z);

void BuildGround(MeshBuilder& mb);
void BuildHutProps(MeshBuilder& mb, InteriorAnchors& anchors);   // HutProps.cpp
glm::mat4 HutMatrix();                                            // hut model -> globe space
void BuildFirePit(MeshBuilder& mb, InteriorAnchors& anchors);
void BuildSeats(MeshBuilder& mb);

#endif
