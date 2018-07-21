#pragma once

typedef class AnimationManager {
private:
	AnimationManager(void) { };
public:
	static AnimationManager& GetInstance(void) {
		static AnimationManager Instance;

		return Instance;
	}

	AnimationManager(AnimationManager const&) = delete;
	void operator=(AnimationManager const&) = delete;
} AnimationManager;