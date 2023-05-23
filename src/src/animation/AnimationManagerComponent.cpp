#include <animation/AnimationManagerComponent.h>
#include <scene/hierarchy/HierarchyTree.h>
#include <scene/Component.h>
#include <functional>

#include <UI/UICanvasActor.h>
#include <UI/UICanvasField.h>
#include <scene/UIButtonActor.h>

#include "utility/Serialisation.h"


namespace GEE
{
	AnimationChannelInstance::AnimationChannelInstance(AnimationChannel& channelRef, Component& channelComp) :
		ChannelRef(channelRef), ChannelComp(channelComp), IsValid(true)
	{
	}

	void AnimationChannelInstance::Restart()
	{
		PosKeysLeft.clear();
		RotKeysLeft.clear();
		ScaleKeysLeft.clear();

		for (int i = 0; i < static_cast<int>(ChannelRef.PosKeys.size() - 1); i++)
			PosKeysLeft.push_back(MakeUnique<Interpolator<Vec3f>>(Interpolator<Vec3f>(Interpolation((i == 0) ? (ChannelRef.PosKeys[0]->KeyTime) : (0.0), ChannelRef.PosKeys[i + 1]->KeyTime - ChannelRef.PosKeys[i]->KeyTime), ChannelRef.PosKeys[i]->Value, ChannelRef.PosKeys[i + 1]->Value)));
		if (!ChannelRef.PosKeys.empty())
			PosKeysLeft.push_back(MakeUnique<Interpolator<Vec3f>>(Interpolator<Vec3f>(Interpolation(ChannelRef.PosKeys.back()->KeyTime, 0.0, InterpolationType::Constant, false, AnimBehaviour::STOP, AnimBehaviour::REPEAT), ChannelRef.PosKeys.back()->Value, ChannelRef.PosKeys.back()->Value)));

		for (int i = 0; i < static_cast<int>(ChannelRef.RotKeys.size() - 1); i++)
			RotKeysLeft.push_back(MakeUnique<Interpolator<Quatf>>(Interpolator<Quatf>(Interpolation((i == 0) ? (ChannelRef.RotKeys[0]->KeyTime) : (0.0), ChannelRef.RotKeys[i + 1]->KeyTime - ChannelRef.RotKeys[i]->KeyTime), ChannelRef.RotKeys[i]->Value, ChannelRef.RotKeys[i + 1]->Value)));
		if (!ChannelRef.RotKeys.empty())
			RotKeysLeft.push_back(MakeUnique<Interpolator<Quatf>>(Interpolator<Quatf>(Interpolation(ChannelRef.RotKeys.back()->KeyTime, 0.0, InterpolationType::Constant, false, AnimBehaviour::STOP, AnimBehaviour::REPEAT), ChannelRef.RotKeys.back()->Value, ChannelRef.RotKeys.back()->Value)));

		for (int i = 0; i < static_cast<int>(ChannelRef.ScaleKeys.size() - 1); i++)
			ScaleKeysLeft.push_back(MakeUnique<Interpolator<Vec3f>>(Interpolator<Vec3f>(Interpolation((i == 0) ? (ChannelRef.ScaleKeys[0]->KeyTime) : (0.0), ChannelRef.ScaleKeys[i + 1]->KeyTime - ChannelRef.ScaleKeys[i]->KeyTime), ChannelRef.ScaleKeys[i]->Value, ChannelRef.ScaleKeys[i + 1]->Value)));
		if (!ChannelRef.ScaleKeys.empty())
			ScaleKeysLeft.push_back(MakeUnique<Interpolator<Vec3f>>(Interpolator<Vec3f>(Interpolation(ChannelRef.ScaleKeys.back()->KeyTime, 0.0, InterpolationType::Constant, false, AnimBehaviour::STOP, AnimBehaviour::REPEAT), ChannelRef.ScaleKeys.back()->Value, ChannelRef.ScaleKeys.back()->Value)));
	}

	void AnimationChannelInstance::Stop()
	{
		PosKeysLeft.clear();
		RotKeysLeft.clear();
		ScaleKeysLeft.clear();
	}

	bool UpdateKey(InterpolatorBase& key, Time& deltaTime)
	{
		auto prevT = key.GetInterp()->GetT();
		key.Update(deltaTime);
		if (key.GetHasEnded())
		{
			if (key.GetInterp()->GetDuration() > 0.0)
				deltaTime -= (1.0 - prevT) * key.GetInterp()->GetDuration();	//subtract used time
			return true;
		}

		deltaTime = 0.0;
		return false;
	}

	template <typename T> void UpdateKeys(std::deque<UniquePtr<Interpolator<T>>>& keys, Time deltaTime, std::function<void(const T&)> setValFunc)
	{
		auto timeLeft = deltaTime;
		while (!keys.empty())
		{
			auto prevTimeLeft = timeLeft;
			bool pop = UpdateKey(*keys.front(), timeLeft);
			setValFunc(keys.front()->GetCurrentValue());

			if (pop)
				keys.pop_front();
			else
				break;
		}
	}

	bool AnimationChannelInstance::Update(Time deltaTime)
	{
		if (!IsValid)
			return false;
		if (ChannelComp.IsBeingKilled())
		{
			IsValid = false;
			Stop();
			return false;
		}

		UpdateKeys<Vec3f>(PosKeysLeft, deltaTime, [this](const Vec3f& vec) {ChannelComp.GetTransform().SetPosition(vec); });
		UpdateKeys<Quatf>(RotKeysLeft, deltaTime, [this](const Quatf& q) {ChannelComp.GetTransform().SetRotation(q); });
		UpdateKeys<Vec3f>(ScaleKeysLeft, deltaTime, [this](const Vec3f& vec) {ChannelComp.GetTransform().SetScale(vec); });
		/*float timeLeft = deltaTime;
		while (!PosKeysLeft.empty())
		{
			float prevTimeLeft = timeLeft;
			bool pop = UpdateKey(*PosKeysLeft.front(), timeLeft);
			ChannelComp.GetTransform().SetPosition(PosKeysLeft.front()->GetCurrentValue());

			if (pop)
				PosKeysLeft.pop_front();
			else
				break;
		}

		timeLeft = deltaTime;
		while (!RotKeysLeft.empty())
		{
			bool pop = UpdateKey(*RotKeysLeft.front(), timeLeft);
			ChannelComp.GetTransform().SetRotation(RotKeysLeft.front()->GetCurrentValue());

			if (pop)
				RotKeysLeft.pop_front();
			else
				break;
		}

		timeLeft = deltaTime;
		while (!ScaleKeysLeft.empty())
		{
			bool pop = UpdateKey(*ScaleKeysLeft.front(), timeLeft);
			ChannelComp.GetTransform().SetScale(ScaleKeysLeft.front()->GetCurrentValue());

			if (pop)
				ScaleKeysLeft.pop_front();
			else
				break;
		}
		/*
		if (!PosKeysLeft.empty())
		{
			PosKeysLeft.front()->Update(deltaTime);
			if (PosKeysLeft.front()->GetHasEnded())
			{
				//float timeLeft = (PosKeysLeft.front()->GetInterp()->GetT() - 1.0f) / PosKeysLeft.front()->GetInterp()->GetDuration() << '\n';
				PosKeysLeft.pop_front();
			}

			ChannelComp.GetTransform().SetPosition(PosKeysLeft.front()->GetCurrentValue());
		}

		if (!RotKeysLeft.empty())
		{
			RotKeysLeft.front()->Update(deltaTime);
			if (RotKeysLeft.front()->GetHasEnded())
				RotKeysLeft.pop_front();

			ChannelComp.GetTransform().SetRotation(RotKeysLeft.front()->GetCurrentValue());
		}

		if (!ScaleKeysLeft.empty())
		{
			ScaleKeysLeft.front()->Update(deltaTime);
			if (ScaleKeysLeft.front()->GetHasEnded())
				ScaleKeysLeft.pop_front();

			ChannelComp.GetTransform().SetScale(ScaleKeysLeft.front()->GetCurrentValue());
		}
		*/
		return !(PosKeysLeft.empty() && RotKeysLeft.empty() && ScaleKeysLeft.empty());
	}

	AnimationInstance::AnimationInstance(Animation& anim, Component& animRootComp) :
		Anim(anim), AnimRootComp(animRootComp), TimePassed(0.0f), IsValid(true)
	{
		std::function<void(Component&)> boneFinderFunc = [this, &boneFinderFunc](Component& comp) {
			auto found = std::find_if(Anim.Channels.begin(), Anim.Channels.end(), [&comp](const SharedPtr<AnimationChannel>& channel) { return channel->Name == comp.GetName(); });
			if (found != Anim.Channels.end())
				ChannelInstances.push_back(MakeUnique<AnimationChannelInstance>(AnimationChannelInstance(**found, comp)));

			for (auto it : comp.GetChildren())
				boneFinderFunc(*it);
		};

		boneFinderFunc(AnimRootComp);
	}

	Animation::AnimationLoc AnimationInstance::GetLocalization() const
	{
		return Anim.Localization;
	}

	Animation& AnimationInstance::GetAnimation() const
	{
		return Anim;
	}

	bool AnimationInstance::HasFinished() const
	{
		return TimePassed > GetAnimation().Duration;
	}

	void AnimationInstance::Update(Time deltaTime)
	{
		if (!IsValid)
			return;
		if (AnimRootComp.IsBeingKilled())
		{
			IsValid = false;
			Stop();
			return;
		}

		bool finished = true;

		for (auto& it : ChannelInstances)
			if (it->Update(deltaTime))
				finished = false;

		TimePassed += deltaTime;

		/*if (HasFinished())
		{
			float nextIterationTime = TimePassed - GetAnimation().Duration;
			Restart();
			Update(nextIterationTime);
		}*/
	}

	void AnimationInstance::Stop()
	{
		for (auto& it : ChannelInstances)
			it->Stop();
		TimePassed = GetAnimation().Duration;
	}

	void AnimationInstance::Restart()
	{
		for (auto& it : ChannelInstances)
			it->Restart();
		TimePassed = 0.0;
	}

	template<typename Archive>
	void AnimationInstance::Save(Archive& archive) const
	{
		archive(cereal::make_nvp("AnimHierarchyTreePath", GetLocalization().GetTreeName()), cereal::make_nvp("AnimName", GetLocalization().Name), cereal::make_nvp("RootCompName", AnimRootComp.GetName()), cereal::make_nvp("RootCompActorName", AnimRootComp.GetActor().GetName()));
	}

	template<typename Archive>
	void AnimationInstance::load_and_construct(Archive& archive, cereal::construct<AnimationInstance>& construct)
	{
		std::cout << "a tera instancje animacji\n";
		std::string animHierarchyTreePath, animName, rootCompName, rootCompActorName;
		archive(cereal::make_nvp("AnimHierarchyTreePath", animHierarchyTreePath), cereal::make_nvp("AnimName", animName), cereal::make_nvp("RootCompName", rootCompName), cereal::make_nvp("RootCompActorName", rootCompActorName));

		Animation* anim = GameManager::Get().FindHierarchyTree(animHierarchyTreePath)->FindAnimation(animName);
		Component* comp = GameManager::DefaultScene->FindActor(rootCompActorName)->GetRoot()->GetComponent<Component>(rootCompName);

		if (!anim)
			throw Exception(std::string("ERROR: Cannot find anim " + animName + " in hierarchy tree " + animHierarchyTreePath).c_str());
		if (!comp)
			throw Exception(std::string("ERROR: Cannot find anim root component " + rootCompName + " in actor " + rootCompActorName).c_str());
		

		construct(*anim, *comp);
	}

	template <typename Archive>
	void AnimationManagerComponent::Save(Archive& archive) const
	{
		archive(cereal::make_nvp("AnimInstances", cereal::defer(AnimInstances)));
		archive(cereal::make_nvp("CurrentAnimName", std::string((CurrentAnim) ? (CurrentAnim->GetLocalization().Name) : (""))), cereal::base_class<Component>(this));
	}

	template <typename Archive>
	void AnimationManagerComponent::Load(Archive& archive)
	{
		std::cout << "loading animationmanagercomponent\n";
		std::string currentAnimName;

		archive(cereal::make_nvp("AnimInstances", cereal::defer(AnimInstances)));

		archive(cereal::make_nvp("CurrentAnimName", currentAnimName), cereal::base_class<Component>(this));

		// erase unloaded anim instances post load
		GetScene().AddPostLoadLambda([this]() mutable {	AnimInstances.erase(std::remove_if(AnimInstances.begin(), AnimInstances.end(), [](const UniquePtr<AnimationInstance>& animInstance)-> bool { return (animInstance.get() == nullptr); }), AnimInstances.end());	});

		if (!currentAnimName.empty())
			for (auto& it : AnimInstances)
				if (it->GetLocalization().Name == currentAnimName)
					SelectAnimation(it.get());
	}

	template void AnimationInstance::Save<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&) const;
	template void AnimationInstance::load_and_construct<cereal::JSONInputArchive>(cereal::JSONInputArchive&, cereal::construct<AnimationInstance>&);

	AnimationManagerComponent::AnimationManagerComponent(Actor& actor, Component* parentComp, const std::string& name) :
		Component(actor, parentComp, name, Transform()),
		CurrentAnim(nullptr)
	{
	}

	AnimationInstance* AnimationManagerComponent::GetAnimInstance(int index)
	{
		if (index > static_cast<int>(GetAnimInstancesCount()) - 1)
			return nullptr;

		return AnimInstances[index].get();
	}

	unsigned AnimationManagerComponent::GetAnimInstancesCount() const
	{
		return static_cast<unsigned int>(AnimInstances.size());
	}

	AnimationInstance* AnimationManagerComponent::GetCurrentAnim()
	{
		return CurrentAnim;
	}

	void AnimationManagerComponent::AddAnimationInstance(AnimationInstance&& animInstance)
	{
		AnimInstances.push_back(MakeUnique<AnimationInstance>(std::move(animInstance)));
	}

	void AnimationManagerComponent::Update(Time dt)
	{
		//for (UniquePtr<AnimationInstance>& it : AnimInstances)
			//it->Update(deltaTime);
		if (CurrentAnim)
		{
			CurrentAnim->Update(dt);
			if (CurrentAnim->HasFinished())
				SelectAnimation(nullptr);
		}

	}

	void AnimationManagerComponent::SelectAnimation(AnimationInstance* anim)
	{
		if (CurrentAnim)
			CurrentAnim->Stop();

		CurrentAnim = anim;
		if (CurrentAnim)
			std::cout << "Started anim " + CurrentAnim->GetAnimation().Localization.Name + ". Nr of channels: " << CurrentAnim->GetAnimation().Channels.size() << '\n';
		else
			std::cout << "Selected nullptr animation.\n";

		if (CurrentAnim)
			CurrentAnim->Restart();
	}

	void AnimationManagerComponent::GetEditorDescription(ComponentDescriptionBuilder descBuilder)
	{
		Component::GetEditorDescription(descBuilder);

		UICanvasField& animField = descBuilder.AddField("Animations");

		float posX = 0.0f;
		for (auto& it : AnimInstances)
		{
			std::cout << "%%%: " << it.get() << '\n';
			UIButtonActor& button = animField.CreateChild<UIButtonActor>(it->GetAnimation().Localization.Name, it->GetAnimation().Localization.Name, [this, &it]() { SelectAnimation(it.get()); });
			button.GetTransform()->SetPosition(Vec2f(posX, 0.0f));
			posX += 3.0f;
		}
	}

}
GEE_SERIALIZATION_INST_DEFAULT(GEE::AnimationManagerComponent);