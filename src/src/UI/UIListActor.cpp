#include <UI/UIListActor.h>
#include <scene/Component.h>
#include <math/Transform.h>

namespace GEE
{
	UIListActor::UIListActor(GameScene& scene, Actor* parentActor, const std::string& name) :
		UIActorDefault(scene, parentActor, name),
		bExpanded(true)
	{
	}

	UIListActor::UIListActor(UIListActor&& listActor) :
		UIActorDefault(std::move(listActor)),
		bExpanded(listActor.bExpanded)
	{
	}

	void UIListActor::Refresh()
	{
		for (auto& element: ListElements)
		{
			if (UIListActor* listCast = dynamic_cast<UIListActor*>(&element.GetActorRef()))
				listCast->Refresh();
		}

		MoveElements();
	}

	Vec3f UIListActor::GetListOffset()
	{
		Vec3f offset(0.0f);
		for (auto& it : ListElements)
			offset += it.GetElementOffset();

		return Mat3f(GetTransform()->GetMatrix()) * offset;
	}

	Vec3f UIListActor::GetListBegin()
	{
		//if (ListElements.empty())
			return Vec3f(0.0f);

		//return ListElements.front().GetActorRef().GetTransform()->GetPos() - ListElements.front().GetCenterOffset();
	}

	const UIListElement UIListActor::GetListElement(unsigned int index) const
	{
		GEE_CORE_ASSERT(index < ListElements.size());
		return ListElements[index];
	}

	int UIListActor::GetListElementCount() const
	{
		CleanupDeadElements();
		return static_cast<int>(ListElements.size());
	}

	Boxf<Vec2f> UIListActor::GetBoundingBoxIncludingChildren(bool world) const
	{
		if (bExpanded)
			return UIActorDefault::GetBoundingBoxIncludingChildren(world);

		const Boxf<Vec2f> thisBoundingBox = GetBoundingBox(world);
		Vec2f canvasLeftDownMin(thisBoundingBox.Position - thisBoundingBox.Size);
		Vec2f canvasRightUpMax(thisBoundingBox.Position + thisBoundingBox.Size);

		for (auto it : ChildUIElements)
		{
			Boxf<Vec2f> bBox = it->GetBoundingBoxIncludingChildren();
			if (std::find_if(ListElements.begin(), ListElements.end(), [&](const UIListElement& listElement) {return (&listElement.GetUIElementRef()) == it; }) != ListElements.end())
				continue;

			canvasLeftDownMin = glm::min(canvasLeftDownMin, bBox.Position - bBox.Size);
			canvasRightUpMax = glm::max(canvasRightUpMax, bBox.Position + bBox.Size);
		}

		return Boxf<Vec2f>::FromMinMaxCorners(canvasLeftDownMin, canvasRightUpMax);
	}

	void UIListActor::SetListElementOffset(int index, std::function<Vec3f()> getElementOffset)
	{
		if (index < 0 || index > ListElements.size() || !getElementOffset)
			return;

		ListElements[index].SetGetElementOffsetFunc(getElementOffset);
	}

	void UIListActor::SetListCenterOffset(int index, std::function<Vec3f()> getCenterOffset)
	{
		if (index < 0)
			index = GetListElementCount() + index;

		if (index < 0 || index > ListElements.size() || !getCenterOffset)
			return;

		ListElements[index].SetGetCenterOffsetFunc(getCenterOffset);
	}

	void UIListActor::AddElement(const UIListElement& element)
	{
		ListElements.push_back(element);
	}

	Vec3f UIListActor::MoveElement(UIListElement& element, Vec3f nextElementBegin, float level)
	{
		const Vec3f& elementOffset = element.GetElementOffset();
		element.GetActorRef().GetTransform()->SetPosition(nextElementBegin + element.GetCenterOffset());
		std::cout << ": " << element.GetActorRef().GetTransform()->GetPos2D() << '\n';
		return Vec3f(nextElementBegin + elementOffset);
	}

	Vec3f UIListActor::MoveElements(unsigned int level)
	{
		CleanupDeadElements();
		Vec3f nextElementBegin = GetListBegin();

		for (auto& element : ListElements)
		{
			std::cout << element.GetActorRef().GetName();
			nextElementBegin = MoveElement(element, nextElementBegin, static_cast<float>(level));
		}

		return nextElementBegin;
	}

	void UIListActor::EraseListElement(std::function<bool(const UIListElement&)> removeFunc)
	{
		ListElements.erase(std::remove_if(ListElements.begin(), ListElements.end(), [&removeFunc](const UIListElement& element) { return removeFunc(element); }), ListElements.end());
	}

	void UIListActor::EraseListElement(Actor& actor)
	{
		EraseListElement([&actor](const UIListElement& listElement) { return &listElement.GetActorRef() == &actor; });
	}

	void UIListActor::CleanupDeadElements() const
	{
		const_cast<UIListActor*>(this)->EraseListElement([](const UIListElement& element) { return element.IsBeingKilled(); });
	}

	UIAutomaticListActor::UIAutomaticListActor(GameScene& scene, Actor* parentActor, const std::string& name, Vec3f elementOffset) :
		UIListActor(scene, parentActor, name),
		ElementOffset(elementOffset)
	{
	}

	UIAutomaticListActor::UIAutomaticListActor(UIAutomaticListActor&& listActor) noexcept :
		UIListActor(std::move(listActor)),
		ElementOffset(listActor.ElementOffset)
	{
	}

	Actor& UIAutomaticListActor::AddChild(UniquePtr<Actor> actor)
	{
		if (auto uiElementCast = dynamic_cast<UICanvasElement*>(actor.get()))
		{
			AddElement(UIListElement(UIListElement::ReferenceToUIActor(*actor, *uiElementCast), ElementOffset));
			if (UIListActor* listCast = dynamic_cast<UIListActor*>(actor.get()))
			{
				ListElements.back().SetGetElementOffsetFunc([listCast]() -> Vec3f { return listCast->GetListOffset(); });
				ListElements.back().SetGetCenterOffsetFunc([listCast]() -> Vec3f { return Vec3f(0.0f); });
			}
		}

		return UIListActor::AddChild(std::move(actor));
	}

	UIListElement::UIListElement(ReferenceToUIActor actorRef, std::function<Vec3f()> getElementOffset, std::function<Vec3f()> getCenterOffset) :
		UIActorRef(actorRef),
		GetElementOffsetFunc(getElementOffset),
		GetCenterOffsetFunc(getCenterOffset)
	{
		if (!GetCenterOffsetFunc)
			GetCenterOffsetFunc = [this]() -> Vec3f { return (GetElementOffsetFunc) ? (GetElementOffsetFunc() / 2.0f) : (Vec3f(0.0f)); };
	}

	UIListElement::UIListElement(ReferenceToUIActor actorRef, const Vec3f& constElementOffset) :
		UIListElement(actorRef, [constElementOffset]() { return constElementOffset; }, [constElementOffset]() { return constElementOffset / 2.0f; })
	{
	}

	bool UIListElement::IsBeingKilled() const
	{
		return GetActorRef().IsBeingKilled();
	}

	Actor& UIListElement::GetActorRef()
	{
		return UIActorRef.ActorRef.get();
	}

	const Actor& UIListElement::GetActorRef() const
	{
		return UIActorRef.ActorRef.get();
	}

	UICanvasElement& UIListElement::GetUIElementRef()
	{
		return UIActorRef.UIElementRef.get();
	}

	const UICanvasElement& UIListElement::GetUIElementRef() const
	{
		return UIActorRef.UIElementRef;
	}

	Vec3f UIListElement::GetElementOffset() const
	{
		return (GetElementOffsetFunc) ? (GetElementOffsetFunc()) : (Vec3f(0.0f));
	}

	Vec3f UIListElement::GetCenterOffset() const
	{
		return (GetCenterOffsetFunc) ? (GetCenterOffsetFunc()) : (Vec3f(0.0f));
	}

	void UIListElement::SetGetElementOffsetFunc(std::function<Vec3f()> getElementOffset)
	{
		GetElementOffsetFunc = getElementOffset;
	}

	void UIListElement::SetGetCenterOffsetFunc(std::function<Vec3f()> getCenterOffset)
	{
		GetCenterOffsetFunc = getCenterOffset;
	}

}