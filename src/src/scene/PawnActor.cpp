#include "PawnActor.h"
#include <UI/UICanvasActor.h>
#include <UI/UICanvasField.h>
#include <scene/BoneComponent.h>
#include <animation/AnimationManagerComponent.h>
#include <UI/UIListActor.h>
#include <scene/UIButtonActor.h>

#include <scene/UIInputBoxActor.h>

namespace GEE
{
	PawnActor::PawnActor(GameScene& scene, Actor* parentActor, const std::string& name) :
		Actor(scene, parentActor, name),
		PreAnimBonePos(Vec3f(0.0f)),
		//RootBone(nullptr),
		AnimManager(nullptr),
		UpdateAnimsListInEditor(nullptr),
		AnimIndex(0),
		SpeedPerSec(1.0f),
		CurrentTargetPos(Vec3f(0.0f)),
		PathIndex(0),
		Gun(nullptr),
		PlayerTarget(nullptr)
	{
	}

	void PawnActor::OnStart()
	{
		Actor::OnStart();
	}

	void PawnActor::Update(float deltaTime)
	{
		Actor::Update(deltaTime);

		if (IsBeingKilled() || !AnimManager)
			return;

		if (AnimManager->IsBeingKilled())
		{
			AnimManager = nullptr;
			return;
		}
		if (Gun && Gun->IsBeingKilled())
			Gun = nullptr;


		/*		Vec3f velocity = RootBone->GetTransform().GetWorldTransform().RotationRef * glm::abs(RootBone->GetTransform().PositionRef - PreAnimBonePos);
			velocity.y = 0.0f;
			//velocity.x = 0.0f;*/

		if (CurrentTargetPos != Vec3f(0.0f))
		{
			if (glm::distance(CurrentTargetPos, GetTransform()->GetWorldTransform().GetPos()) < 0.05f)
			{
				CurrentTargetPos = Vec3f(0.0f);
				if (AnimManager->GetCurrentAnim())
					AnimManager->GetCurrentAnim()->Stop();
				PathIndex = (++PathIndex) % 4;
				return;
			}

			if (!AnimManager->GetCurrentAnim())
				AnimManager->SelectAnimation(AnimManager->GetAnimInstance(AnimIndex));

			Vec3f posDir = CurrentTargetPos - GetTransform()->GetWorldTransform().GetPos();
			posDir.y = 0.0f;
			posDir = glm::normalize(posDir);
			if (PlayerTarget)
			{
				Vec3f playerDir = glm::normalize(PlayerTarget->GetTransform()->GetWorldTransform().GetPos() - GetTransform()->GetWorldTransform().GetPos());
				float distance = glm::distance(PlayerTarget->GetTransform()->GetWorldTransform().GetPos(), GetTransform()->GetWorldTransform().GetPos());
	
				if (Gun && glm::dot(playerDir, GetTransform()->GetRot() * Vec3f(0.0f, 0.0f, -1.0f)) > glm::cos(glm::radians(30.0f)) && distance < 3.0f)
				{
					Gun->FireWeapon();
					//Gun->GetTransform()->SetRotationWorld(quatFromDirectionVec(-playerDir));
					posDir = glm::normalize(Vec3f(playerDir.x, 0.0f, playerDir.z));

				}
			}

			GetTransform()->SetRotation(quatFromDirectionVec(-posDir));

			Vec3f velocity = GetTransform()->GetWorldTransform().GetRot() * Vec3f(0.0f, 0.0f, -1.0f * SpeedPerSec * deltaTime);

			GetTransform()->Move(velocity);
		}

		MoveAlongPath();
	}

	void PawnActor::GetEditorDescription(EditorDescriptionBuilder descBuilder)
	{
		Actor::GetEditorDescription(descBuilder);

		descBuilder.AddField("Anim Manager").GetTemplates().ObjectInput<Component, AnimationManagerComponent>(*GetRoot(), [this](AnimationManagerComponent* animManager) {AnimManager = animManager; UpdateAnimsListInEditor(animManager); });
		descBuilder.AddField("Target position").GetTemplates().VecInput(CurrentTargetPos);

		dynamic_cast<UIInputBoxActor*>(descBuilder.GetDescriptionParent().FindActor("VecBox0"))->SetRetrieveContentEachFrame(true);
		dynamic_cast<UIInputBoxActor*>(descBuilder.GetDescriptionParent().FindActor("VecBox1"))->SetRetrieveContentEachFrame(true);
		dynamic_cast<UIInputBoxActor*>(descBuilder.GetDescriptionParent().FindActor("VecBox2"))->SetRetrieveContentEachFrame(true);

		descBuilder.AddField("Gun").GetTemplates().ObjectInput<Actor, GunActor>(*this, Gun);
		descBuilder.AddField("Player target").GetTemplates().ObjectInput<Actor, Actor>(*Scene.GetRootActor(), PlayerTarget);

		UIInputBoxActor& speedInputBox = descBuilder.AddField("Speed").CreateChild<UIInputBoxActor>("SpeedInputBox");
		speedInputBox.SetOnInputFunc([this](float val) { SpeedPerSec = val; }, [this]()->float { return SpeedPerSec; });

		UICanvasField& animsField = descBuilder.AddField("Anim List");
		UIAutomaticListActor& animsList = animsField.CreateChild<UIAutomaticListActor>("Animations list", Vec3f(3.0f, 0.0f, 0.0f));

		UpdateAnimsListInEditor = [this, &animsList, descBuilder](AnimationManagerComponent* animManager) mutable {
			for (auto& it : animsList.GetChildren())
				it->MarkAsKilled();

			if (animManager)
				for (int i = 0; i < animManager->GetAnimInstancesCount(); i++)
					animsList.CreateChild<UIButtonActor>("Anim button", animManager->GetAnimInstance(i)->GetAnimation().Localization.Name, [this, i]() {AnimIndex = i; });
			animsList.Refresh();
		};

		if (AnimManager)
			UpdateAnimsListInEditor(AnimManager);
	}

	void PawnActor::MoveToPosition(const Vec3f& worldPos)
	{
		CurrentTargetPos = worldPos;
	}

	void PawnActor::MoveAlongPath()
	{
		switch (PathIndex)
		{
		case 0: MoveToPosition(Vec3f(-5.0f, 0.0f, 0.0f)); break;
		case 1: MoveToPosition(Vec3f(-15.0f, 0.0f, 0.0f)); break;
		case 2: MoveToPosition(Vec3f(-15.0f, 0.0f, -20.0f)); break;
		case 3: MoveToPosition(Vec3f(-5.0f, 0.0f, -20.0f)); break;
		}
	}
}