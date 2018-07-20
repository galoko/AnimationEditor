#pragma once

#include "Character.hpp"

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
} CharacterManager;