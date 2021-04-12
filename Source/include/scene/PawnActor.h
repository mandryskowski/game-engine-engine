#pragma once
#include <scene/GunActor.h>
#include <animation/AnimationManagerActor.h>
#include <functional>

class PawnActor : public Actor
{
public:
	PawnActor(GameScene&, const std::string& name);
	virtual void OnStart() override;
	virtual void Update(float deltaTime);
	virtual void GetEditorDescription(EditorDescriptionBuilder) override;
	void MoveToPosition(const glm::vec3& worldPos);
	void MoveAlongPath();
private:
	//Component* RootBone;
	AnimationManagerComponent* AnimManager;
	int AnimIndex;
	glm::vec3 PreAnimBonePos;

	glm::vec3 CurrentTargetPos;
	float SpeedPerSec;

	int PathIndex;
	glm::vec3 Waypoints[4];

	GunActor* Gun;

	std::function<void(AnimationManagerComponent*)> UpdateAnimsListInEditor;
};