#pragma once
#include <animation/Animation.h>
#include <scene/Component.h>
#include <deque>

struct AnimationChannelInstance
{
	AnimationChannel& ChannelRef;
	Component& ChannelComp;

	std::deque<std::unique_ptr<Interpolator<glm::vec3>>> PosKeysLeft, ScaleKeysLeft;
	std::deque<std::unique_ptr<Interpolator<glm::quat>>> RotKeysLeft;

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
	std::vector<std::unique_ptr<AnimationChannelInstance>> ChannelInstances;
	float TimePassed;

public:
	AnimationInstance(Animation&, Component&);

	Animation& GetAnimation();
	void Update(float);
	void Stop();
	void Restart();
};

class AnimationManagerComponent : public Component
{
	std::vector<std::unique_ptr<AnimationInstance>> AnimInstances;
	AnimationInstance* CurrentAnim;

public:
	AnimationManagerComponent(GameScene& scene, const std::string& name);

	void AddAnimationInstance(AnimationInstance&&);

	virtual void Update(float) override;
	void SelectAnimation(AnimationInstance*);

	virtual void GetEditorDescription(UIActor& canvas, GameScene& editorScene) override;
};