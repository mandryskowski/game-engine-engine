#include <animation/Animation.h>
#include <assimp/scene.h>

namespace GEE
{
	float DUPA::AnimTime = 9999.0f;

	Interpolation::Interpolation(Time begin, Time end, InterpolationType type, bool fadeAway, AnimBehaviour before, AnimBehaviour after):
		OnUpdateFunc(nullptr)
	{
		Reset(begin, end);

		Type = type;
		FadeAway = fadeAway;

		BeforeBehaviour = before;
		AfterBehaviour = after;
	}

	bool Interpolation::IsChanging()
	{
		if (CompType == 0.0f || CompType == 1.0f)
			return false;

		return true;
	}

	float Interpolation::GetT()
	{
		return CompType;
	}

	Time Interpolation::GetDuration()
	{
		return End;
	}

	void Interpolation::Reset(Time begin, Time end)
	{
		if (begin != -1.0f)
			Begin = begin;
		if (end != -1.0f)
			End = end;

		CurrentTime = -Begin;
		CompType = 0.0f;
	}

	void Interpolation::Inverse()
	{
		FadeAway = !FadeAway;
		CurrentTime = End - CurrentTime;
	}

	bool Interpolation::UpdateT(Time deltaTime)
	{
		CurrentTime += deltaTime;

		if (AfterBehaviour == STOP && CurrentTime > End)
			CurrentTime = End;
		if (CurrentTime < 0.0f && -CurrentTime > Begin)
			CurrentTime = -Begin;
		else if ((BeforeBehaviour == REPEAT && CurrentTime < 0.0f) || (AfterBehaviour == REPEAT && CurrentTime > End))
			CurrentTime = fmod(CurrentTime, End);

		////////////////////////////////////////////////////////////////////

		if (Type == Constant && CurrentTime >= End)
			CompType = 1.0f;
		else
		{
			float exponent = static_cast<float>(Type);	//Linear = 1, Quadratic = 2, Cubic = 3, etc.
			CompType = (FadeAway) ? (1.0f - pow(1.0f - CurrentTime / End, exponent)) : (pow(CurrentTime / End, exponent));
		}

		////////////////////////////////////////////////////////////////////

		if (BeforeBehaviour == STOP && CompType < 0.0f)
			CompType = 0.0f;

		if (OnUpdateFunc)
			return OnUpdateFunc(CompType);
		return !IsChanging();
	}

	template<class ValType> ValType Interpolation::InterpolateValues(ValType y1, ValType y2)
	{
		return glm::mix(y1, y2, CompType);
	}

	template<> Quatf Interpolation::InterpolateValues<Quatf>(Quatf y1, Quatf y2)
	{
		return glm::slerp(y1, y2, CompType);
	}

	/*
		=========================================================
		=========================================================
		=========================================================
	*/

	template <class ValType> Interpolator<ValType>::Interpolator(float begin, float end, ValType min, ValType max, InterpolationType type, bool fadeAway, AnimBehaviour before, AnimBehaviour after, bool updateMinOnBegin, ValType* valPtr) :
		Interpolator(MakeShared<Interpolation>(begin, end, type, fadeAway, before, after), min, max, updateMinOnBegin, valPtr)
	{
	}

	template<class ValType>
	Interpolator<ValType>::Interpolator(Interpolation&& interp, ValType min, ValType max, bool updateMinOnBegin, ValType* valPtr) :
		Interpolator(MakeShared<Interpolation>(interp), min, max, updateMinOnBegin, valPtr)
	{
	}

	template <class ValType> Interpolator<ValType>::Interpolator(SharedPtr<Interpolation> interp, ValType min, ValType max, bool updateOnBegin, ValType* valPtr) :
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

	template <class ValType> SharedPtr<Interpolation> Interpolator<ValType>::GetInterp()
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
		if (!HasBegun && (Interp->IsChanging() || Interp->GetT() > 0.0f))
		{
			if (InterpolatedValPtr)
			{
				if (UpdateMinValOnBegin)
					MinVal = *InterpolatedValPtr;
				else
					*InterpolatedValPtr = MinVal;
			}

			LastInterpResult = MinVal;
			HasBegun = true;
		}

		Interp->UpdateT(deltaTime);
		ValType result = Interp->InterpolateValues(MinVal, MaxVal);
		bool change = true;

		if (auto cast = dynamic_cast<Interpolator<float>*>(this))
			;// std::cout << "TVAL: " << Interp->GetT() << ", VAL: " << glm::mix(cast->MinVal, cast->MaxVal, Interp->GetT()) << ", MIN: " << cast->MinVal << ", MAX: " << cast->MaxVal << '\n';

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

	template<class ValType> void Interpolator<ValType>::UpdateInterpolatedValPtr()
	{
		if (InterpolatedValPtr)
			*InterpolatedValPtr = Interp->InterpolateValues(MinVal, MaxVal);
	}

	Animation::Animation(const HierarchyTemplate::HierarchyTreeT& tree, aiAnimation* anim) :
		Localization(tree, anim->mName.C_Str()), Duration(anim->mDuration / ((anim->mTicksPerSecond != 0.0f) ? (anim->mTicksPerSecond) : (1.0f)))
	{
		for (int i = 0; i < static_cast<int>(anim->mNumChannels); i++)
		{
			Channels.push_back(MakeShared<AnimationChannel>(AnimationChannel(anim->mChannels[i], anim->mTicksPerSecond)));
		}
	}

	AnimationChannel::AnimationChannel(aiNodeAnim* aiChannel, float ticksPerSecond) :
		Name(aiChannel->mNodeName.C_Str())
	{
		for (int i = 0; i < static_cast<int>(aiChannel->mNumPositionKeys); i++)
			PosKeys.push_back(MakeShared<AnimationVecKey>(AnimationVecKey(aiChannel->mPositionKeys[i].mTime / ticksPerSecond, aiToGlm(aiChannel->mPositionKeys[i].mValue))));
		for (int i = 0; i < static_cast<int>(aiChannel->mNumRotationKeys); i++)
			RotKeys.push_back(MakeShared<AnimationQuatKey>(AnimationQuatKey(aiChannel->mRotationKeys[i].mTime / ticksPerSecond, aiToGlm(aiChannel->mRotationKeys[i].mValue))));
		for (int i = 0; i < static_cast<int>(aiChannel->mNumScalingKeys); i++)
			ScaleKeys.push_back(MakeShared<AnimationVecKey>(AnimationVecKey(aiChannel->mScalingKeys[i].mTime / ticksPerSecond, aiToGlm(aiChannel->mScalingKeys[i].mValue))));
	}


	Vec3f aiToGlm(const aiVector3D& aiVec)
	{
		return Vec3f(aiVec.x, aiVec.y, aiVec.z);
	}

	Quatf aiToGlm(const aiQuaternion& aiQuat)
	{
		return Quatf(aiQuat.w, aiQuat.x, aiQuat.y, aiQuat.z);
	}

	/*
		=========================================================
		=========================================================
		=========================================================
	*/

	template float Interpolation::InterpolateValues<float>(float y1, float y2);
	template Vec3f Interpolation::InterpolateValues<Vec3f>(Vec3f y1, Vec3f y2);

	template class Interpolator<float>;
	template class Interpolator<Vec3f>;
	template class Interpolator<Quatf>;
}