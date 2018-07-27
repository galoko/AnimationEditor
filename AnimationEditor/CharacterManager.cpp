#include "CharacterManager.hpp"

#include "PhysicsManager.hpp"

void CharacterManager::Initialize(void) {

	Char = new Character();
}

Character* CharacterManager::GetCharacter(void) {
	return Char;
}

void CharacterManager::Serialize(CharacterSerializedState& State)
{
	Character* Char = GetCharacter();

	State.Position = Char->Position;

	State.Bones.clear();

	for (Bone* Bone : Char->Bones) 
		State.Bones.push_back({ Bone->Name, quat_cast(Bone->Rotation) });
}

void CharacterManager::Deserialize(CharacterSerializedState& State)
{
	Character* Char = GetCharacter();

	Char->Position = State.Position;

	for (SerializedBone SerializedBone : State.Bones) {

		Bone* Bone = Char->FindBone(SerializedBone.Name);
		if (Bone == nullptr)
			continue;

		Bone->Rotation = mat4_cast(SerializedBone.Rotation);
	}

	PhysicsManager::GetInstance().SyncWorldWithCharacter();
}