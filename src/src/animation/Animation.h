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
		static Time AnimTime;
	};


	class Interpolation
	{
	public:
		Interpolation(Time begin, Time end, InterpolationType type = InterpolationType::Linear, bool fadeAway = false, AnimBehaviour before = AnimBehaviour::STOP, AnimBehaviour after = AnimBehaviour::STOP);
		[[nodiscard]] bool IsChanging() const;
		[[nodiscard]] double GetT() const;
		[[nodiscard]] Time GetDuration() const;

		void SetOnUpdateFunc(std::function<bool(double)> func)
		{
			OnUpdateFunc = std::move(func);
		}

		void SetT(double t);

		void Reset(Time begin = -1.0f, Time end = -1.0f);
		void Inverse();	//this method essentially changes the direction of the interpolation. When you inverse an Interpolation, the interpolation function and time become inversed, so TValue increases at the same pace
		/**
		 * @brief Updates the TValue value and calls OnUpdateFunc, if it exists.
		 * @param deltaTime: The difference in time between the last update call and this one. This engine uses a constant time-step, so unless you change anything it should be safe to assume that deltaTime will always be constant.
		 * @return: a boolean indicating whether this Interpolation has finished. If OnUpdateFunc exists, it returns the value of it. If it doesn't, it returns true if the interpolation is not changing.
		*/
		bool UpdateT(Time deltaTime);

		template <class ValType> ValType InterpolateValues(ValType y1, ValType y2);

	private:
		Time CurrentTime;
		Time Begin;
		Time End;
		double TValue;

		InterpolationType Type;
		bool FadeAway;	//used to inverse the interpolation function

		AnimBehaviour BeforeBehaviour, AfterBehaviour;

		std::function<bool(Time)> OnUpdateFunc;
	};

	class InterpolatorBase
	{
	public:
		virtual bool GetHasEnded() = 0;
		virtual SharedPtr<Interpolation> GetInterp() = 0;
		virtual void ResetMinVal() = 0;
		virtual void Update(Time deltaTime) = 0;
		virtual void UpdateInterpolatedValPtr() = 0;	//Call to update the interpolated value ptr (if passed)
		virtual ~InterpolatorBase() = default;
	};

	template <class ValType>
	class Interpolator : public InterpolatorBase
	{
		SharedPtr<Interpolation> Interp;
		ValType MinVal;
		ValType MaxVal;
		ValType LastInterpResult;

		bool HasBegun;
		bool HasEnded;
		bool UpdateMinValOnBegin;	//if true, at the beginning of interpolation MinVal will be set to the value of the variable that InterpolatedValPtr points to
		ValType* InterpolatedValPtr;	//optional pointer; if set, the Update method automatically updates the variable that this pointer points to

	public:
		Interpolator(Time begin, Time end, ValType min, ValType max, InterpolationType type, bool fadeAway = false, AnimBehaviour before = AnimBehaviour::STOP, AnimBehaviour after = AnimBehaviour::STOP, bool updateMinOnBegin = false, ValType* valPtr = nullptr);
		Interpolator(Interpolation&& interp, ValType min, ValType max, bool updateMinOnBegin = true, ValType* valPtr = nullptr);
		Interpolator(SharedPtr<Interpolation> interp, ValType min, ValType max, bool updateMinOnBegin = true, ValType* valPtr = nullptr);

		bool GetHasEnded() override;
		SharedPtr<Interpolation> GetInterp() override;
		ValType GetCurrentValue();
		void SetValPtr(ValType* valPtr);

		void SetMinVal(ValType val);	//these 3 methods are not recommended for use during actual interpolation
		void ResetMinVal() override;	//set MinVal to the current value
		void SetMaxVal(ValType val);

		void Inverse();		//make the interpolator "go another way" (interpolate to the starting point). Useful for stuff like camera interpolation

		void Update(Time deltaTime) override;
		void UpdateInterpolatedValPtr() override;
	};


	struct AnimationKey
	{
		Time KeyTime;
		AnimationKey(Time time) :
			KeyTime(time)
		{
		}
	};

	struct AnimationVecKey : public AnimationKey
	{
		Vec3f Value;
		AnimationVecKey(Time time, Vec3f value) :
			AnimationKey(time),
			Value(value)
		{
		}
	};

	struct AnimationQuatKey : public AnimationKey
	{
		Quatf Value;
		AnimationQuatKey(Time time, Quatf value) :
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
		AnimationChannel(aiNodeAnim*, double tickPerSecond);
	};

	struct Animation
	{
		std::vector<SharedPtr<AnimationChannel>> Channels;
		struct AnimationLoc : public HTreeObjectLoc	//exact localization of the animation
		{
			std::string Name;
			AnimationLoc(HTreeObjectLoc treeObjectLoc, const String& name) : HTreeObjectLoc(treeObjectLoc), Name(name) {}
		} Localization;
		Time Duration;

		Animation(const Hierarchy::Tree& tree, aiAnimation*);
	};

	Vec3f aiToGlm(const aiVector3D&);
	Quatf aiToGlm(const aiQuaternion&);
}