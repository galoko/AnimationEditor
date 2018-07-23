#include "CharacterManager.hpp"

void CharacterManager::Initialize(void) {

	Char = new Character();
	Char->FindBone(L"Chest")->IsLocked = true;
}

Character* CharacterManager::GetCharacter(void) {
	return Char;
}