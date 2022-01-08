#pragma once
#include <utility/Utility.h>
#include <game/GameManager.h> //for HTreeObjectLoc
#include <assimp/types.h>

struct aiNodeAnim;
struct aiAnimation;


namespace GEE
{
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

	typedef float Time;


	class Interpolation
	{
		Time CurrentTime;
		Time Begin;
		Time End;
		float CompType;

		InterpolationType Type;
		bool FadeAway;	//used to inverse the interpolation function

		AnimBehaviour BeforeBehaviour, AfterBehaviour;

		std::function<bool(float)> OnUpdateFunc;

	public:
		Interpolation(Time begin, Time end, InterpolationType type = InterpolationType::Linear, bool fadeAway = false, AnimBehaviour before = AnimBehaviour::STOP, AnimBehaviour after = AnimBehaviour::STOP);
		bool IsChanging();
		float GetT();
		Time GetDuration();

		void SetOnUpdateFunc(std::function<bool(Time)> func)
		{
			OnUpdateFunc = std::move(func);
		}

		void Reset(Time begin = -1.0f, Time end = -1.0f);
		void Inverse();	//this method essentially changes the direction of the interpolation. When you inverse an Interpolation, the interpolation function and time become inversed, so CompType increases at the same pace
		/**
		 * @brief Updates the CompType value and calls OnUpdateFunc, if it exists.
		 * @param deltaTime: The difference in time between the last update call and this one. This engine uses a constant time-step, so unless you change anything it should be safe to assume that deltaTime will always be constant.
		 * @return: a boolean indicating whether this Interpolation has finished. If OnUpdateFunc exists, it returns the value of it. If it doesn't, it returns true if the interpolation is not changing.
		*/
		bool UpdateT(Time deltaTime);

		template <class ValType> ValType InterpolateValues(ValType y1, ValType y2);
	};

	class InterpolatorBase
	{
	public:
		virtual bool GetHasEnded() = 0;
		virtual SharedPtr<Interpolation> GetInterp() = 0;
		virtual void ResetMinVal() = 0;
		virtual void Update(float deltaTime) = 0;
		virtual void UpdateInterpolatedValPtr() = 0;	//Call to update the interpolated value ptr (if passed)
	};

	template <class ValType>
	class Interpolator : public InterpolatorBase
	{
	public: //TODO erase this
		SharedPtr<Interpolation> Interp;
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
		Interpolator(SharedPtr<Interpolation> interp, ValType min, ValType max, bool updateMinOnBegin = true, ValType* valPtr = nullptr);

		virtual bool GetHasEnded() override;
		virtual SharedPtr<Interpolation> GetInterp() override;
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
		Vec3f Value;
		AnimationVecKey(float time, Vec3f value) :
			AnimationKey(time),
			Value(value)
		{
		}
	};

	struct AnimationQuatKey : public AnimationKey
	{
		Quatf Value;
		AnimationQuatKey(float time, Quatf value) :
			AnimationKey(time),
			Value(value)
		{
		}
	};

	struct AnimationChannel
	{
		std::string Name;
		std::vector<SharedPtr<AnimationVecKey>> PosKeys;
		std::vector<SharedPtr<AnimationQuatKey>> RotKeys;
		std::vector<SharedPtr<AnimationVecKey>> ScaleKeys;
		AnimationChannel(aiNodeAnim*, float tickPerSecond);
	};

	struct Animation
	{
		std::vector<SharedPtr<AnimationChannel>> Channels;
		struct AnimationLoc : public HTreeObjectLoc	//exact localization of the animation
		{
			std::string Name;
			AnimationLoc(HTreeObjectLoc treeObjectLoc, const std::string& name) : HTreeObjectLoc(treeObjectLoc), Name(name) {}
		} Localization;
		float Duration;

		Animation(const HierarchyTemplate::HierarchyTreeT& tree, aiAnimation*);
	};

	Vec3f aiToGlm(const aiVector3D&);
	Quatf aiToGlm(const aiQuaternion&);
}