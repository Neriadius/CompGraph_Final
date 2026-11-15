#ifndef INTERIOR_MODELS_H
#define INTERIOR_MODELS_H

// Static geometry of the globe interior (Person 3), all built from primitives into one mesh:
// snow ground, pine trees, log hut (HutModel.cpp), fire pit, seats and props.
// Built once at start-up in globe space (radius 1, centre at the origin).

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
	Material Planks(const glm::vec3& color);
	Material Stone(const glm::vec3& color = glm::vec3(0.50f, 0.50f, 0.54f));
	Material Needles();
	Material Metal();
	Material Plain(const glm::vec3& color, float spec = 0.06f, float shininess = 12.0f);
}

// Where the lights sit, filled while the geometry is built (globe space, no shake)
struct InteriorAnchors
{
	std::vector<glm::vec3> windowLights;     // just outside each hut window
	glm::vec3 lantern = glm::vec3(0.0f);
	glm::vec3 fire = glm::vec3(0.0f);        // centre of the fire pit on the ground
	glm::vec3 chimneyTop = glm::vec3(0.0f);
};

// y of the visible snow surface at (x, z), globe space
float GroundY(float x, float z);

void BuildGround(MeshBuilder& mb);
void BuildTrees(MeshBuilder& mb);
void BuildHut(MeshBuilder& mb, InteriorAnchors& anchors);        // HutModel.cpp
void BuildFirePit(MeshBuilder& mb, InteriorAnchors& anchors);
void BuildSeats(MeshBuilder& mb);

#endif
