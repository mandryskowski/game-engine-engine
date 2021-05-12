#pragma once
#include <utility/Utility.h>
#include <game/GameManager.h> //for HTreeObjectLoc
#include <glfw/glfw3.h>
#include "assimp/types.h"

struct aiNodeAnim;
struct aiAnimation;

enum AnimBehaviour
{
	STOP,
	EXTRAPOLATE,
	REPEAT
};

struct DUPA
{
	static float AnimTime;
};


class Interpolation
{
	float CurrentTime;
	float Begin;
	float End;
	float T;

	InterpolationType Type;
	bool FadeAway;	//used to inverse the interpolation function

	AnimBehaviour BeforeBehaviour, AfterBehaviour;

public:
	Interpolation(float begin, float end, InterpolationType type = InterpolationType::LINEAR, bool fadeAway = false, AnimBehaviour before = AnimBehaviour::STOP, AnimBehaviour after = AnimBehaviour::STOP);
	bool IsChanging();
	float GetT();
	float GetDuration();

	void Reset(float begin = -1.0f, float end = -1.0f);
	void Inverse();	//this method essentially changes the direction of the interpolation. When you inverse an Interpolation, the interpolation function and time become inversed, so T increases at the same pace
	void UpdateT(float deltaTime);

	template <class ValType> ValType InterpolateValues(ValType y1, ValType y2);
};

class InterpolatorBase
{
public:
	virtual bool GetHasEnded() = 0;
	virtual std::shared_ptr<Interpolation> GetInterp() = 0;
	virtual void ResetMinVal() = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void UpdateInterpolatedValPtr() = 0;	//Call to update the interpolated value ptr (if passed)
};

template <class ValType>
class Interpolator: public InterpolatorBase
{
public: //TODO erase this
	std::shared_ptr<Interpolation> Interp;
	ValType MinVal;
	ValType MaxVal;
	ValType LastInterpResult;

	bool HasBegun;
	bool HasEnded;
	bool UpdateMinValOnBegin;	//if true, at the beginning of interpolation MinVal will be set to the value of the variable that InterpolatedValPtr points to
	ValType* InterpolatedValPtr;	//optional pointer; if set, the Update method automatically updates the variable that this pointer points to

public:
	Interpolator(float begin, float end, ValType min, ValType max, InterpolationType type, bool fadeAway = false, AnimBehaviour before = AnimBehaviour::STOP, AnimBehaviour after = AnimBehaviour::STOP, bool updateMinOnBegin = false, ValType* valPtr = nullptr);
	Interpolator(Interpolation&& interp, ValType min, ValType max, bool updateMinOnBegin = true, ValType* valPtr = nullptr);
	Interpolator(std::shared_ptr<Interpolation> interp, ValType min, ValType max, bool updateMinOnBegin = true, ValType* valPtr = nullptr);

	virtual bool GetHasEnded() override;
	virtual std::shared_ptr<Interpolation> GetInterp() override;
	ValType GetCurrentValue();
	void SetValPtr(ValType* valPtr);

	void SetMinVal(ValType val);	//these 3 methods are not recommended for use during actual interpolation
	virtual void ResetMinVal() override;	//set MinVal to the current value
	void SetMaxVal(ValType val);

	void Inverse();		//make the interpolator "go another way" (interpolate to the starting point). Useful for stuff like camera interpolation

	void Update(float deltaTime) override;
	virtual void UpdateInterpolatedValPtr() override;
};


struct AnimationKey
{
	float Time;
	AnimationKey(float time) :
		Time(time)
	{
	}
};

struct AnimationVecKey : public AnimationKey
{
	glm::vec3 Value;
	AnimationVecKey(float time, glm::vec3 value) :
		AnimationKey(time),
		Value(value)
	{
	}
};

struct AnimationQuatKey : public AnimationKey
{
	glm::quat Value;
	AnimationQuatKey(float time, glm::quat value) :
		AnimationKey(time),
		Value(value)
	{
	}
};

struct AnimationChannel
{
	std::string Name;
	std::vector<std::shared_ptr<AnimationVecKey>> PosKeys;
	std::vector<std::shared_ptr<AnimationQuatKey>> RotKeys;
	std::vector<std::shared_ptr<AnimationVecKey>> ScaleKeys;
	AnimationChannel(aiNodeAnim*, float tickPerSecond);
};

struct Animation
{
	std::vector<std::shared_ptr<AnimationChannel>> Channels;
	struct AnimationLoc: public HTreeObjectLoc	//exact localization of the animation
	{
		std::string Name;
		AnimationLoc(HTreeObjectLoc treeObjectLoc, const std::string& name) : HTreeObjectLoc(treeObjectLoc), Name(name) {}
	} Localization;
	float Duration;

	Animation(const HierarchyTemplate::HierarchyTreeT& tree, aiAnimation*);
};

glm::vec3 aiToGlm(const aiVector3D&);
glm::quat aiToGlm(const aiQuaternion&);