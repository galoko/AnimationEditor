#include "CharacterManager.hpp"

void CharacterManager::Initialize(void) {

	Char = new Character();
}

Character* CharacterManager::GetCharacter(void) {
	return Char;
}