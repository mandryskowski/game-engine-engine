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
		PawnSound(nullptr),
		LastSoundTime(0.0f),
		SoundCooldown(10.0f),
		UpdateAnimsListInEditor(nullptr),
		AnimIndex(0),
		SpeedPerSec(1.0f),
		CurrentTargetPos(Vec3f(0.0f)),
		PathIndex(0),
		Gun(nullptr),
		PlayerTarget(nullptr),
		Health(100.0f),
		RespawnTime(5.0f),
		DeathTime(0.0f),
		State(PawnState::Idle)
	{
	}

	void PawnActor::OnStart()
	{
		Actor::OnStart();
	}

	void PawnActor::Update(float deltaTime)
	{
		Actor::Update(deltaTime);

		if (State == PawnState::Dying && GameHandle->GetProgramRuntime() - DeathTime >= RespawnTime)
			Respawn();

		if (State == PawnState::Respawning && AnimManager && (!AnimManager->GetCurrentAnim() || AnimManager->GetCurrentAnim()->HasFinished()))
			State = PawnState::Idle;

		if (IsBeingKilled() || !AnimManager || (State == PawnState::Dying || State == PawnState::Respawning))
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

			Vec3f posDir = glm::normalize(Math::CutYAxis(CurrentTargetPos - GetTransform()->GetWorldTransform().GetPos()));

			if (PlayerTarget)
			{
				Vec3f playerDir = glm::normalize(PlayerTarget->GetTransform()->GetWorldTransform().GetPos() - GetTransform()->GetWorldTransform().GetPos());
				float distance = glm::distance(PlayerTarget->GetTransform()->GetWorldTransform().GetPos(), GetTransform()->GetWorldTransform().GetPos());
	
				if (Gun && glm::dot(playerDir, GetTransform()->GetRot() * Vec3f(0.0f, 0.0f, -1.0f)) > glm::cos(glm::radians(30.0f)) && distance < 3.0f)
				{
					Gun->FireWeapon();
					//Gun->GetTransform()->SetRotationWorld(quatFromDirectionVec(-playerDir));
					posDir = glm::normalize(Math::CutYAxis(playerDir));

				}
				if (PawnSound && !PawnSound->IsPlaying() && distance < 3.0f && (GameHandle->GetProgramRuntime() - LastSoundTime) > SoundCooldown)
				{
					PawnSound->Play();
					LastSoundTime = GameHandle->GetProgramRuntime() + PawnSound->GetCurrentSoundBuffer().Duration;
				}

				if (distance < 1.0f)
					InflictDamage(30.0f * deltaTime);

			}

			GetTransform()->SetRotation(quatFromDirectionVec(posDir));

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

		descBuilder.AddField("Sound source").GetTemplates().ObjectInput<Component, Audio::SoundSourceComponent>(*GetRoot(), [this](Audio::SoundSourceComponent* soundSource) { PawnSound = soundSource; });

		UIInputBoxActor& speedInputBox = descBuilder.AddField("Speed").CreateChild<UIInputBoxActor>("SpeedInputBox");
		speedInputBox.SetOnInputFunc([this](float val) { SpeedPerSec = val; }, [this]()->float { return SpeedPerSec; });

		descBuilder.AddField("Speed").GetTemplates().SliderUnitInterval([](float) {});

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

	void PawnActor::InflictDamage(float damage, const Vec3f& optionalDirection)
	{
		Health = glm::max(Health - damage, 0.0f);

		if (State != PawnState::Dying && Health <= 0.0f)
		{
			DeathTime = GameHandle->GetProgramRuntime();
			State = PawnState::Dying;

			if (AnimManager->GetCurrentAnim())
				AnimManager->GetCurrentAnim()->Stop();

			AnimManager->SelectAnimation(AnimManager->GetAnimInstance(0));
			if (PawnSound)
			{
				PawnSound->Play();
			}

			if (optionalDirection != Vec3f(0.0f))
				GetTransform()->SetRotation(quatFromDirectionVec(glm::normalize(optionalDirection)));
		}
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
	void PawnActor::Respawn()
	{
		Health = 100.0f;
		State = PawnState::Respawning;

		if (!AnimManager)
			return;

		if (AnimManager->GetCurrentAnim())
			AnimManager->GetCurrentAnim()->Stop();

		AnimManager->SelectAnimation(AnimManager->GetAnimInstance(2));
	}
}