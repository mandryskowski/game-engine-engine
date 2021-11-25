#pragma once
#include <animation/Animation.h>
#include <scene/Component.h>
#include <deque>

namespace GEE
{
	struct AnimationChannelInstance
	{
		AnimationChannel& ChannelRef;
		Component& ChannelComp;

		bool IsValid;

		std::deque<UniquePtr<Interpolator<Vec3f>>> PosKeysLeft, ScaleKeysLeft;
		std::deque<UniquePtr<Interpolator<Quatf>>> RotKeysLeft;

		AnimationChannelInstance(AnimationChannel&, Component&);
		void Restart();
		void Stop();
		float GetTimeLeft()
		{
			float timeLeft = 0.0f;
			for (auto& it : PosKeysLeft)
				timeLeft += (1.0f - it->GetInterp()->GetT()) * it->GetInterp()->GetDuration();

			return timeLeft;
		}
		bool Update(float);	//Returns true if animation is taking place, false otherwise

	};

	class AnimationInstance
	{
		Animation& Anim;
		Component& AnimRootComp;
		std::vector<UniquePtr<AnimationChannelInstance>> ChannelInstances;
		float TimePassed;

		bool IsValid;

	public:
		AnimationInstance(Animation&, Component&);

		Animation::AnimationLoc GetLocalization() const;
		Animation& GetAnimation() const;
		bool HasFinished() const;
		void Update(float);
		void Stop();
		void Restart();

		template <typename Archive> void Save(Archive& archive) const
		{
			archive(cereal::make_nvp("AnimHierarchyTreePath", GetLocalization().GetTreeName()), cereal::make_nvp("AnimName", GetLocalization().Name), cereal::make_nvp("RootCompName", AnimRootComp.GetName()), cereal::make_nvp("RootCompActorName", AnimRootComp.GetActor().GetName()));
		}
		template <typename Archive> static void load_and_construct(Archive& archive, cereal::construct<AnimationInstance>& construct)
		{
			std::cout << "a tera instancje animacji\n";
			std::string animHierarchyTreePath, animName, rootCompName, rootCompActorName;
			archive(cereal::make_nvp("AnimHierarchyTreePath", animHierarchyTreePath), cereal::make_nvp("AnimName", animName), cereal::make_nvp("RootCompName", rootCompName), cereal::make_nvp("RootCompActorName", rootCompActorName));

			Animation* anim = GameManager::Get().FindHierarchyTree(animHierarchyTreePath)->FindAnimation(animName);
			Component* comp = GameManager::DefaultScene->FindActor(rootCompActorName)->GetRoot()->GetComponent<Component>(rootCompName);

			if (!anim)
			{
				std::cout << "ERROR: Cannot find anim " << animName << " in hierarchy tree " << animHierarchyTreePath << '\n';
				exit(-42069);
			}
			if (!comp)
			{
				std::cout << "ERROR: Cannot find anim root component " << rootCompName << " in actor " << rootCompActorName << '\n';
				exit(-42069);
			}

			construct(*anim, *comp);
		}
	};

	class AnimationManagerComponent : public Component
	{
		std::vector<UniquePtr<AnimationInstance>> AnimInstances;
		AnimationInstance* CurrentAnim;

	public:
		AnimationManagerComponent(Actor&, Component* parentComp, const std::string& name);

		AnimationInstance* GetAnimInstance(int index);
		int GetAnimInstancesCount() const;
		AnimationInstance* GetCurrentAnim();

		void AddAnimationInstance(AnimationInstance&&);

		virtual void Update(float) override;
		void SelectAnimation(AnimationInstance*);

		virtual void GetEditorDescription(EditorDescriptionBuilder) override;

		template <typename Archive> void Save(Archive& archive) const
		{
			archive(cereal::make_nvp("AnimInstances", cereal::defer(AnimInstances)), cereal::make_nvp("CurrentAnimName", std::string((CurrentAnim) ? (CurrentAnim->GetLocalization().Name) : (""))), cereal::base_class<Component>(this));
		}
		template <typename Archive> void Load(Archive& archive)
		{
			std::cout << "robie animationmanagercomponent\n";
			std::string currentAnimName;
			archive(cereal::make_nvp("AnimInstances", cereal::defer(AnimInstances)), cereal::make_nvp("CurrentAnimName", currentAnimName), cereal::base_class<Component>(this));

			if (!currentAnimName.empty())
				for (auto& it : AnimInstances)
					if (it->GetLocalization().Name == currentAnimName)
						SelectAnimation(it.get());
		}
	};


}

GEE_POLYMORPHIC_SERIALIZABLE_COMPONENT(GEE::Component, GEE::AnimationManagerComponent)