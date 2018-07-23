#pragma once

#include <windows.h>

#include <glm/glm.hpp>

#include "Character.hpp"

using namespace glm;

typedef class AnimationManager {
private:
	AnimationManager(void) { };

	struct {
		
		int IsActive;
		Bone* Bone;
		vec3 LocalPoint, WorldDestPoint;

	} InverseKinematicTask;

	void PhysicsPreSolve(void);

	void SetupInverseKinematicMass(Bone* PickedBone);
public:
	static AnimationManager& GetInstance(void) {
		static AnimationManager Instance;

		return Instance;
	}

	AnimationManager(AnimationManager const&) = delete;
	void operator=(AnimationManager const&) = delete;

	void Initialize(void);
	void Tick(double dt);

	void InverseKinematic(Bone* Bone, vec3 LocalPoint, vec3 WorldDestPoint);
	void InverseKinematic(vec3 WorldDestPoint);
	void CancelInverseKinematic(void);

} AnimationManager;