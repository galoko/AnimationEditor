#pragma once

#include <glm/glm.hpp>

#include "Character.hpp"
#include "SerializationManager.hpp"

using namespace glm;

typedef class CharacterManager {
private:
	CharacterManager(void) { };

	Character* Char;
public:
	static CharacterManager& GetInstance(void) {
		static CharacterManager Instance;

		return Instance;
	}

	CharacterManager(CharacterManager const&) = delete;
	void operator=(CharacterManager const&) = delete;

	void Initialize(void);
	Character* GetCharacter(void);

	uint32 AnimationTimestamp;

	void Serialize(CharacterSerializedState& State);
	void Deserialize(CharacterSerializedState& State);

} CharacterManager;