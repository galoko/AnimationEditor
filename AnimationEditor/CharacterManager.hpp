#pragma once

#include <glm/glm.hpp>

#include "Character.hpp"
#include "SerializationManager.hpp"

using namespace glm;

typedef class CharacterManager {
private:
	CharacterManager(void) { };

	Character* Char;

	uint32 AnimationTimestamp;
public:
	static CharacterManager& GetInstance(void) {
		static CharacterManager Instance;

		return Instance;
	}

	CharacterManager(CharacterManager const&) = delete;
	void operator=(CharacterManager const&) = delete;

	void Initialize(void);
	Character* GetCharacter(void);

	void Serialize(CharacterSerializedState& State);
	void Deserialize(CharacterSerializedState& State);

} CharacterManager;