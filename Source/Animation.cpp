#include "Animation.h"

Interpolation::Interpolation(float begin, float end, InterpolationType type, AnimBehaviour before, AnimBehaviour after)
{
	Reset(begin, end);

	Type = type;
	BeforeBehaviour = before;
	AfterBehaviour = after;
}

bool Interpolation::IsChanging()
{
	if (T == 0.0f || T == 1.0f)
		return false;

	return true;
}

float Interpolation::GetT()
{
	return T;
}

void Interpolation::Reset(float begin, float end)
{
	if (begin != -1.0f)
		Begin = begin;
	if (end != -1.0f)
		End = end;

	CurrentTime = -Begin;
	T = 0.0f;
}

void Interpolation::UpdateT(float deltaTime)
{
	CurrentTime += deltaTime;

	switch (Type)
	{
	case LINEAR:
		T = abs(CurrentTime) / End;	//inverselerp(Begin.x, End.x, x)
		break;

	case QUADRATIC:
		T = pow(CurrentTime / End, 2.0f);
		break;

	case CONSTANT:
	default:
		if (CurrentTime >= End)
			T = 1.0f;
	}

	if (CurrentTime < 0.0f)
		T *= -1.0f;

	T = AdjustTToBehaviours(T, BeforeBehaviour, AfterBehaviour);
}

template<class ValType> ValType Interpolation::InterpolateValues(ValType y1, ValType y2)
{
	switch (Type)
	{
	case LINEAR:
	case QUADRATIC:
		return glm::mix(y1, y2, T);

	case CONSTANT:
	default:
		if (T >= 1.0f)
			return y2;
		return y1;
	}
}

/*
	=========================================================
	=========================================================
	=========================================================
*/

template <class ValType> Interpolator<ValType>::Interpolator(float begin, float end, ValType min, ValType max, InterpolationType type, AnimBehaviour before, AnimBehaviour after, ValType* valPtr):
	Interpolator(new Interpolation(begin, end, type, before, after), min, max, valPtr)
{
}

template <class ValType> Interpolator<ValType>::Interpolator(Interpolation* interp, ValType min, ValType max, ValType* valPtr)
{
	Interp = interp;
	MinVal = min;
	MaxVal = max;
	InterpolatedValPtr = valPtr;
}

template <class ValType> Interpolation* Interpolator<ValType>::GetInterp()
{
	return Interp;
}


template <class ValType> void Interpolator<ValType>::SetValPtr(ValType* valPtr)
{
	InterpolatedValPtr = valPtr;
}


template <class ValType> ValType Interpolator<ValType>::Update(float deltaTime)
{
	Interp->UpdateT(deltaTime);
	ValType result = Interp->InterpolateValues(MinVal, MaxVal);

	if (InterpolatedValPtr)
		*InterpolatedValPtr = result;

	return result;
}

/*
	=========================================================
	=========================================================
	=========================================================
*/

template float Interpolation::InterpolateValues<float>(float y1, float y2);
template glm::vec3 Interpolation::InterpolateValues<glm::vec3>(glm::vec3 y1, glm::vec3 y2);

template class Interpolator<glm::vec3>;

float AdjustTToBehaviours(float T, AnimBehaviour before, AnimBehaviour after)
{
	AnimBehaviour behaviours[2] = { before, after };

	for (int i = 0; i <= 1; i++)
	{
		if ((i == 0 && T > 0.0f) || (i == 1 && T < 1.0f))
			continue;

		switch (behaviours[i])
		{
		case STOP: return (i == 0) ? (0.0f) : (1.0f);	//the condition can be simplified to static_cast<float>(i) but i don't think that's readable
		case REPEAT: return fmod(T, 1.0f);
		default:
		case EXTRAPOLATE: return T;
		}
	}

	return T;
}