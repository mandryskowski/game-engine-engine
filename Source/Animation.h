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
	void UpdateT(float deltaTime);

	template <class ValType> ValType InterpolateValues(ValType y1, ValType y2);	//TODO: Byc moze przeniesc te funkcje do Interpolator (zobaczyc czy Type tak naprawde wplywa na interpolacje, np nwm czy musze brac logarytm10 w tej funkcji)
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
	void SetValPtr(ValType* valPtr);

	void SetMinVal(ValType val);	//these 2 methods are not recommended for use during actual interpolation
	void SetMaxVal(ValType val);

	ValType Update(float deltaTime);
};