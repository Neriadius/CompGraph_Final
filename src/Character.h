#ifndef CHARACTER_H
#define CHARACTER_H

// Little person sitting on a log by the campfire (Person 3).
//
// Transformation hierarchy (each part is its own mesh, built around its pivot):
//
//   root (seat on the ground, facing the fire)
//    |- body: hips, legs, boots                      (static)
//    '- torso      = root  * T(hips)     * R(lean)   (+ breathing scale on the torso mesh only)
//        |- head   = torso * T(neck)     * R(turn, nod)
//        |- upperArm[s] = torso * T(shoulder) * R(swing)
//        '-    forearm[s] = upperArm * T(elbow) * R(rub hands)
//
// Moving a parent moves everything below it: the torso lean carries the head and both arms.

#include <functional>
#include <glm/glm.hpp>

#include "MeshBuilder.h"

class Character
{
public:
	void Init();

	// setModel uploads one model matrix (+ its normal matrix) to the active shader.
	void Draw(const glm::mat4& root, float time, const std::function<void(const glm::mat4&)>& setModel) const;

private:
	GpuMesh body, torso, head;
	GpuMesh upperArm[2], forearm[2];     // [0] = left (-X), [1] = right (+X)
};

#endif
