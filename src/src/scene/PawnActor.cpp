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
		AnimManager(nullptr),
		//RootBone(nullptr),
		WalkingAnimIndex(0),
		IdleAnimIndex(0),
		PreAnimBonePos(Vec3f(0.0f)),
		State(Idle),
		Health(100.0f),
		RespawnTime(5.0f),
		DeathTime(0.0f),
		PawnSound(nullptr),
		SoundCooldown(10.0f),
		LastSoundTime(0.0f),
		CurrentTargetPos(Vec3f(0.0f)),
		SpeedPerSec(1.0f),
		PathIndex(0),
		Gun(nullptr),
		PlayerTarget(nullptr)
	{
	}

	void PawnActor::OnStart()
	{
		Actor::OnStart();
	}

	void PawnActor::Update(Time dt)
	{
		Actor::Update(dt);

		if (State == PawnState::Dying && GameHandle->GetProgramRuntime() - DeathTime >= RespawnTime)
			Respawn();

		if (State == PawnState::Respawning && AnimManager && (!AnimManager->GetCurrentAnim() || AnimManager->GetCurrentAnim()->HasFinished()))
			State = PawnState::Idle;

		if (IsBeingKilled() || !AnimManager || (State == PawnState::Dying || State == PawnState::Respawning))
			return;

		// Take care of dangling pointers
		if (AnimManager->IsBeingKilled())
		{
			AnimManager = nullptr;
			return;
		}
		if (Gun && Gun->IsBeingKilled())
			Gun = nullptr;

		if (PawnSound && State == PawnState::Walking)
			PawnSound->Play();

		if (!AnimManager->GetCurrentAnim())
		{
			if (State == PawnState::Walking)
				AnimManager->SelectAnimation(AnimManager->GetAnimInstance(WalkingAnimIndex));
			else if (State == PawnState::Idle)
				AnimManager->SelectAnimation(AnimManager->GetAnimInstance(IdleAnimIndex));
		}
		else
		{
			if ((State == PawnState::Walking && AnimManager->GetCurrentAnim() != AnimManager->GetAnimInstance(WalkingAnimIndex)) ||
				(State == PawnState::Idle && AnimManager->GetCurrentAnim() != AnimManager->GetAnimInstance(IdleAnimIndex)))
				AnimManager->GetCurrentAnim()->Stop();
				
		}

		Vec3f rotDir(0.0f);
		if (PlayerTarget)
		{
			Vec3f playerDir = glm::normalize(PlayerTarget->GetTransform()->GetWorldTransform().GetPos() - GetTransform()->GetWorldTransform().GetPos());
			float distance = glm::distance(PlayerTarget->GetTransform()->GetWorldTransform().GetPos(), GetTransform()->GetWorldTransform().GetPos());

			//std::cout << "REH| Dot value: " << glm::dot(playerDir, GetTransform()->GetRot() * Vec3f(0.0f, 0.0f, 1.0f)) << '\n';
			//std::cout << "REH| Dist: " << distance << '\n';

			if (Gun && glm::dot(playerDir, GetTransform()->GetRot() * Vec3f(0.0f, 0.0f, 1.0f)) > glm::cos(glm::radians(60.0f)) && distance < 3.0f)
			{
				//std::cout << "REH| Shot!\n";
				Gun->FireWeapon();
				//Gun->GetTransform()->SetRotationWorld(quatFromDirectionVec(-playerDir));
				rotDir = -glm::normalize(Math::CutYAxis(playerDir));

			}
		}

		if (rotDir != Vec3f(0.0f))
			GetTransform()->SetRotation(quatFromDirectionVec(rotDir));

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
				AnimManager->SelectAnimation(AnimManager->GetAnimInstance(WalkingAnimIndex));

			Vec3f posDir = glm::normalize(Math::CutYAxis(CurrentTargetPos - GetTransform()->GetWorldTransform().GetPos()));


			//GetTransform()->SetRotation(quatFromDirectionVec(posDir));

			Vec3f velocity = GetTransform()->GetWorldTransform().GetRot() * Vec3f(0.0f, 0.0f, -1.0f * SpeedPerSec * dt);

			GetTransform()->Move(velocity);
		}

		//MoveAlongPath();
	}

	void PawnActor::GetEditorDescription(EditorDescriptionBuilder descBuilder)
	{
		Actor::GetEditorDescription(descBuilder);

		descBuilder.AddField("Anim Manager").GetTemplates().ObjectInput<Component, AnimationManagerComponent>(*GetRoot(), [this, descBuilder](AnimationManagerComponent* animManager) mutable { AnimManager = animManager; descBuilder.RefreshActor(); });
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

		UICanvasField& walkingAnimField = descBuilder.AddField("Walking Anim List");
		UICanvasField& idleAnimsField = descBuilder.AddField("Idle Anim List");

		auto updateAnimsListFunc = [this, descBuilder](UIAutomaticListActor& animList, AnimationManagerComponent* animManager, int& indexRef) mutable {
			for (auto& it : animList.GetChildren())
				it->MarkAsKilled();

			if (animManager)
				for (int i = 0; i < static_cast<int>(animManager->GetAnimInstancesCount()); i++)
					animList.CreateChild<UIButtonActor>("Anim button", animManager->GetAnimInstance(i)->GetAnimation().Localization.Name, [this, i, &indexRef]() { indexRef = i; });
			animList.Refresh();
		};

		if (AnimManager)
		{
			updateAnimsListFunc(walkingAnimField.CreateChild<UIAutomaticListActor>("Weird animations list", Vec3f(3.0f, 0.0f, 0.0f)), AnimManager, WalkingAnimIndex);
			updateAnimsListFunc(idleAnimsField.CreateChild<UIAutomaticListActor>("Idle animations list", Vec3f(3.0f, 0.0f, 0.0f)), AnimManager, IdleAnimIndex);
		}
	}

	void PawnActor::InflictDamage(float damage, const Vec3f& optionalDirection)
	{
		Health = glm::max(Health - damage, 0.0f);

		if (State != PawnState::Dying && Health <= 0.0f)
		{
			DeathTime = GameHandle->GetProgramRuntime();
			State = PawnState::Dying;

			if (AnimManager)
			{
				if (AnimManager->GetCurrentAnim())
					AnimManager->GetCurrentAnim()->Stop();

				AnimManager->SelectAnimation(AnimManager->GetAnimInstance(0));
			}

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