#pragma once
#include "Utility.h"
#include <glfw/glfw3.h>

enum AnimBehaviour
{
	STOP,
	EXTRAPOLATE,
	REPEAT
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
	Interpolation(float begin, float end, InterpolationType type, bool fadeAway = false, AnimBehaviour before = AnimBehaviour::STOP, AnimBehaviour after = AnimBehaviour::STOP);
	bool IsChanging();
	float GetT();

	void Reset(float begin = -1.0f, float end = -1.0f);
	void Inverse();	//this method essentially changes the direction of the interpolation. When you inverse an Interpolation, the interpolation function and time become inversed, so T increases at the same pace
	void UpdateT(float deltaTime);

	template <class ValType> ValType InterpolateValues(ValType y1, ValType y2);
};

template <class ValType>
class Interpolator
{
	std::shared_ptr<Interpolation> Interp;
	ValType MinVal;
	ValType MaxVal;
	ValType LastInterpResult;

	bool HasBegun;
	bool HasEnded;
	bool UpdateMinValOnBegin;	//if true, at the beginning of interpolation MinVal will be set to the value of the variable that InterpolatedValPtr points to
	ValType* InterpolatedValPtr;	//optional pointer; if set, the Update method automatically updates the variable that this pointer points to

public:
	Interpolator(float begin, float end, ValType min, ValType max, InterpolationType type, bool fadeAway = false, AnimBehaviour before = AnimBehaviour::STOP, AnimBehaviour after = AnimBehaviour::STOP, bool updateMinOnBegin = true, ValType* valPtr = nullptr);
	Interpolator(std::shared_ptr<Interpolation> interp, ValType min, ValType max, bool updateMinOnBegin = true, ValType* valPtr = nullptr);

	bool GetHasEnded();
	std::shared_ptr<Interpolation> GetInterp();
	ValType GetCurrentValue();
	void SetValPtr(ValType* valPtr);

	void SetMinVal(ValType val);	//these 2 methods are not recommended for use during actual interpolation
	void SetMaxVal(ValType val);

	void Inverse();		//make the interpolator "go another way" (interpolate to the starting point). Useful for stuff like camera interpolation

	ValType Update(float deltaTime);
};