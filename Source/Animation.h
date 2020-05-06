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
	AnimBehaviour BeforeBehaviour, AfterBehaviour;

public:
	Interpolation(float begin, float end, InterpolationType type, AnimBehaviour before = AnimBehaviour::STOP, AnimBehaviour after = AnimBehaviour::STOP);
	bool IsChanging();
	float GetT();
	void Reset(float begin = -1.0f, float end = -1.0f);
	void UpdateT(float deltaTime);
	template <class ValType> ValType InterpolateValues(ValType y1, ValType y2);	//TODO: Byc moze przeniesc te funkcje do Interpolator (zobaczyc czy Type tak naprawde wplywa na interpolacje, np nwm czy musze brac logarytm10 w tej funkcji)
};

template <class ValType>
class Interpolator
{
	Interpolation* Interp;
	ValType MinVal;
	ValType MaxVal;
	ValType* InterpolatedValPtr;	//optional pointer; if set, the Update method automatically updates the variable that this pointer points to

public:
	Interpolator(float begin, float end, ValType min, ValType max, InterpolationType type, AnimBehaviour before = AnimBehaviour::STOP, AnimBehaviour after = AnimBehaviour::STOP, ValType* valPtr = nullptr);
	Interpolator(Interpolation* interp, ValType min, ValType max, ValType* valPtr = nullptr);
	Interpolation* GetInterp();
	void SetValPtr(ValType* valPtr);
	ValType Update(float deltaTime);
};

float AdjustTToBehaviours(float T, AnimBehaviour before, AnimBehaviour after);