#include <animation/AnimationManagerActor.h>
#include <scene/Component.h>
#include <functional>



AnimationChannelInstance::AnimationChannelInstance(AnimationChannel& channelRef, Component& channelComp):
	ChannelRef(channelRef), ChannelComp(channelComp)
{
}

void AnimationChannelInstance::Restart()
{
	PosKeysLeft.clear();
	RotKeysLeft.clear();
	ScaleKeysLeft.clear();

	for (int i = 0; i < static_cast<int>(ChannelRef.PosKeys.size() - 1); i++)
		PosKeysLeft.push_back(std::make_unique<Interpolator<glm::vec3>>(Interpolator<glm::vec3>(Interpolation((i == 0) ? (ChannelRef.PosKeys[0]->Time) : (0.0f), (float)ChannelRef.PosKeys[i + 1]->Time - (float)ChannelRef.PosKeys[i]->Time), ChannelRef.PosKeys[i]->Value, ChannelRef.PosKeys[i + 1]->Value)));
	if (!ChannelRef.PosKeys.empty())
		PosKeysLeft.push_back(std::make_unique<Interpolator<glm::vec3>>(Interpolator<glm::vec3>(Interpolation(ChannelRef.PosKeys.back()->Time, 0.0f, InterpolationType::CONSTANT, false, AnimBehaviour::STOP, AnimBehaviour::REPEAT), ChannelRef.PosKeys.back()->Value, ChannelRef.PosKeys.back()->Value)));

	for (int i = 0; i < static_cast<int>(ChannelRef.RotKeys.size() - 1); i++)
		RotKeysLeft.push_back(std::make_unique<Interpolator<glm::quat>>(Interpolator<glm::quat>(Interpolation((i == 0) ? (ChannelRef.RotKeys[0]->Time) : (0.0f), (float)ChannelRef.RotKeys[i + 1]->Time - (float)ChannelRef.RotKeys[i]->Time), ChannelRef.RotKeys[i]->Value, ChannelRef.RotKeys[i + 1]->Value)));
	if (!ChannelRef.RotKeys.empty())
		RotKeysLeft.push_back(std::make_unique<Interpolator<glm::quat>>(Interpolator<glm::quat>(Interpolation(ChannelRef.RotKeys.back()->Time, 0.0f, InterpolationType::CONSTANT, false, AnimBehaviour::STOP, AnimBehaviour::REPEAT), ChannelRef.RotKeys.back()->Value, ChannelRef.RotKeys.back()->Value)));

	for (int i = 0; i < static_cast<int>(ChannelRef.ScaleKeys.size() - 1); i++)
		ScaleKeysLeft.push_back(std::make_unique<Interpolator<glm::vec3>>(Interpolator<glm::vec3>(Interpolation((i == 0) ? (ChannelRef.ScaleKeys[0]->Time) : (0.0f), (float)ChannelRef.ScaleKeys[i + 1]->Time - (float)ChannelRef.ScaleKeys[i]->Time), ChannelRef.ScaleKeys[i]->Value, ChannelRef.ScaleKeys[i + 1]->Value)));
	if (!ChannelRef.ScaleKeys.empty())
		ScaleKeysLeft.push_back(std::make_unique<Interpolator<glm::vec3>>(Interpolator<glm::vec3>(Interpolation(ChannelRef.ScaleKeys.back()->Time, 0.0f, InterpolationType::CONSTANT, false, AnimBehaviour::STOP, AnimBehaviour::REPEAT), ChannelRef.ScaleKeys.back()->Value, ChannelRef.ScaleKeys.back()->Value)));
}

void AnimationChannelInstance::Stop()
{
	PosKeysLeft.clear();
	RotKeysLeft.clear();
	ScaleKeysLeft.clear();
}

bool UpdateKey(InterpolatorBase& key, float& deltaTime)
{
	//std::cout << "updating key\n";
	float prevT = key.GetInterp()->GetT();
	key.Update(deltaTime);
	if (key.GetHasEnded())
	{
		if (key.GetInterp()->GetDuration() > 0.0f)
			deltaTime -= (1.0f - prevT) * key.GetInterp()->GetDuration();	//subtract used time
		return true;
	}

	deltaTime = 0.0f;
	return false;
}

template <typename T> void UpdateKeys(std::deque<std::unique_ptr<Interpolator<T>>>& keys, float deltaTime, std::function<void(const T&)> setValFunc)
{
	float timeLeft = deltaTime;
	while (!keys.empty())
	{
		float prevTimeLeft = timeLeft;
		bool pop = UpdateKey(*keys.front(), timeLeft);
		setValFunc(keys.front()->GetCurrentValue());

		if (pop)
			keys.pop_front();
		else
			break;
	}
}

bool AnimationChannelInstance::Update(float deltaTime)
{
	UpdateKeys<glm::vec3>(PosKeysLeft, deltaTime, [this](const glm::vec3& vec) {ChannelComp.GetTransform().SetPosition(vec); });
	UpdateKeys<glm::quat>(RotKeysLeft, deltaTime, [this](const glm::quat& q) {ChannelComp.GetTransform().SetRotation(q); });
	UpdateKeys<glm::vec3>(ScaleKeysLeft, deltaTime, [this](const glm::vec3& vec) {ChannelComp.GetTransform().SetScale(vec); });
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
	Anim(anim), AnimRootComp(animRootComp), TimePassed(0.0f)
{
	std::function<void(Component&)> boneFinderFunc = [this, &boneFinderFunc](Component& comp) {
		auto found = std::find_if(Anim.Channels.begin(), Anim.Channels.end(), [&comp](const std::shared_ptr<AnimationChannel>& channel) { return channel->Name == comp.GetName(); });
		if (found != Anim.Channels.end())
			ChannelInstances.push_back(std::make_unique<AnimationChannelInstance>(AnimationChannelInstance(**found, comp)));

		for (auto it : comp.GetChildren())
			boneFinderFunc(*it);
	};

	boneFinderFunc(AnimRootComp);
}

Animation& AnimationInstance::GetAnimation()
{
	return Anim;
}

void AnimationInstance::Update(float deltaTime)
{
	bool finished = true;
	float minT = 1.0f;

	for (auto& it : ChannelInstances)
		if (it->Update(deltaTime))
			finished = false;

	TimePassed += deltaTime;

	if (TimePassed > GetAnimation().Duration)
	{
		float nextIterationTime = TimePassed - GetAnimation().Duration;
		Restart();
		Update(nextIterationTime);
	}
}

void AnimationInstance::Stop()
{
	for (auto& it : ChannelInstances)
		it->Stop();
}

void AnimationInstance::Restart()
{
	for (auto& it : ChannelInstances)
		it->Restart();
	TimePassed = 0.0f;
}

AnimationManagerComponent::AnimationManagerComponent(GameScene& scene, const std::string& name):
	Component(scene, name, Transform()),
	CurrentAnim(nullptr)
{
}

void AnimationManagerComponent::AddAnimationInstance(AnimationInstance&& animInstance)
{
	AnimInstances.push_back(std::make_unique<AnimationInstance>(std::move(animInstance)));
}

void AnimationManagerComponent::Update(float deltaTime)
{
	//for (std::unique_ptr<AnimationInstance>& it : AnimInstances)
		//it->Update(deltaTime);
	if (CurrentAnim)
		CurrentAnim->Update(deltaTime);
}

void AnimationManagerComponent::SelectAnimation(AnimationInstance* anim)
{
	if (CurrentAnim)
		CurrentAnim->Stop();

	CurrentAnim = anim;
	std::cout << "Started anim " + CurrentAnim->GetAnimation().Name + ". Nr of channels: " << CurrentAnim->GetAnimation().Channels.size() << '\n';

	if (CurrentAnim)
		CurrentAnim->Restart();
}

#include <UI/UICanvasActor.h>
#include <UI/UICanvasField.h>
#include <scene/UIButtonActor.h>

void AnimationManagerComponent::GetEditorDescription(UIActor& editorParent, GameScene& editorScene)
{
	Component::GetEditorDescription(editorParent, editorScene);

	UICanvasField& field = AddFieldToCanvas("Animations", editorParent);

	float posX = 0.0f;
	for (auto& it : AnimInstances)
	{
		UIButtonActor& button = field.CreateChild(UIButtonActor(editorScene, it->GetAnimation().Name, it->GetAnimation().Name, [this, &it]() { SelectAnimation(it.get()); }));
		button.GetTransform()->SetPosition(glm::vec2(posX += 3.0f, 0.0f));
	}
}
