#include "Animation.h"

Interpolation::Interpolation(float begin, float end, InterpolationType type, bool fadeAway, AnimBehaviour before, AnimBehaviour after)
{
	Reset(begin, end);

	Type = type;
	FadeAway = fadeAway;

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

void Interpolation::Inverse()
{
	FadeAway = !FadeAway;
	CurrentTime = End - CurrentTime;
}

void Interpolation::UpdateT(float deltaTime)
{
	CurrentTime += deltaTime;

	if (AfterBehaviour == STOP && CurrentTime > End)
		CurrentTime = End;
	else if ((BeforeBehaviour == REPEAT && CurrentTime < 0.0f) || (AfterBehaviour == REPEAT && CurrentTime > End))
		CurrentTime = fmod(CurrentTime, End);

	////////////////////////////////////////////////////////////////////

	if (Type == CONSTANT && CurrentTime >= End)
		T = 1.0f;
	else
	{
		float exponent = static_cast<float>(Type);	//LINEAR = 1, QUADRATIC = 2, CUBIC = 3, etc.
		T = (FadeAway) ? (1.0f - pow(1.0f - CurrentTime / End, exponent)) : (pow(CurrentTime / End, exponent));
	}

	////////////////////////////////////////////////////////////////////

	if (BeforeBehaviour == STOP && T < 0.0f)
		T = 0.0f;
}

template<class ValType> ValType Interpolation::InterpolateValues(ValType y1, ValType y2)
{
	return glm::mix(y1, y2, T);
}

/*
	=========================================================
	=========================================================
	=========================================================
*/

template <class ValType> Interpolator<ValType>::Interpolator(float begin, float end, ValType min, ValType max, InterpolationType type, bool fadeAway, AnimBehaviour before, AnimBehaviour after, bool updateMinOnBegin, ValType* valPtr):
	Interpolator(std::make_shared<Interpolation>(begin, end, type, fadeAway, before, after), min, max, updateMinOnBegin, valPtr)
{
}

template <class ValType> Interpolator<ValType>::Interpolator(std::shared_ptr<Interpolation> interp, ValType min, ValType max, bool updateOnBegin, ValType* valPtr):
	Interp(interp),
	MinVal(min),
	MaxVal(max),
	LastInterpResult(MinVal),
	HasBegun(false),
	HasEnded(false),
	UpdateMinValOnBegin(updateOnBegin),
	InterpolatedValPtr(valPtr)
{
}

template <class ValType> bool Interpolator<ValType>::GetHasEnded()
{
	return HasEnded;
}

template <class ValType> std::shared_ptr<Interpolation> Interpolator<ValType>::GetInterp()
{
	return Interp;
}

template <class ValType> ValType Interpolator<ValType>::GetCurrentValue()
{
	return LastInterpResult;
}

template <class ValType> void Interpolator<ValType>::SetValPtr(ValType* valPtr)
{
	InterpolatedValPtr = valPtr;
}

template <class ValType> void Interpolator<ValType>::SetMinVal(ValType val)
{
	MinVal = val;
}

template <class ValType> void Interpolator<ValType>::ResetMinVal()
{
	if (InterpolatedValPtr)
		MinVal = *InterpolatedValPtr;
	else
		MinVal = LastInterpResult;
}

template <class ValType> void Interpolator<ValType>::SetMaxVal(ValType val)
{
	MaxVal = val;
}

template <class ValType> void Interpolator<ValType>::Inverse()
{
	Interp->Inverse();
	std::swap(MinVal, MaxVal);	//we swap the min and max values, because we're now interpolating in the other direction
}


template <class ValType> void Interpolator<ValType>::Update(float deltaTime)
{
	if (!HasBegun && Interp->IsChanging() && InterpolatedValPtr)
	{
		if (UpdateMinValOnBegin)
			MinVal = *InterpolatedValPtr;
		else
			*InterpolatedValPtr = MinVal;

		LastInterpResult = MinVal;
		HasBegun = true;
	}

	Interp->UpdateT(deltaTime);
	ValType result = Interp->InterpolateValues(MinVal, MaxVal);
	bool change = true;

	if (!Interp->IsChanging())
	{
		if (HasBegun && !HasEnded)
		{
			result = MaxVal;
			HasEnded = true;
		}
		else
			change = false;
	}


	if (InterpolatedValPtr && change)
		*InterpolatedValPtr += result - LastInterpResult;	//we subtract the last result from the current result, because if two interpolations of the same variable happen at once, we can't just assign the result, we have to add delta

	LastInterpResult = result;
}

/*
	=========================================================
	=========================================================
	=========================================================
*/

template float Interpolation::InterpolateValues<float>(float y1, float y2);
template glm::vec3 Interpolation::InterpolateValues<glm::vec3>(glm::vec3 y1, glm::vec3 y2);

template class Interpolator<float>;
template class Interpolator<glm::vec3>;
template class Interpolator<glm::quat>;