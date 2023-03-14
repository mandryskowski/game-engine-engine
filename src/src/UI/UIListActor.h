#pragma once
#include <UI/UIActor.h>
#include <functional>

namespace GEE
{
	class UIListElement
	{
	public:
		/**
		 * @brief Contains references to a class that is both an Actor and a UICanvasElement. DO NOT pass nullptr to the constructor.
		*/
		class ReferenceToUIActor
		{
		public:
			ReferenceToUIActor(Actor& actor, UICanvasElement& uiElement) : ActorRef(actor), UIElementRef(uiElement) {}
			template <typename ActorWhichIsAlsoAnUICanvasElement>
			ReferenceToUIActor(ActorWhichIsAlsoAnUICanvasElement* actor) :
				ActorRef(static_cast<Actor&>(*actor)), UIElementRef(static_cast<UICanvasElement&>(*actor)) { GEE_CORE_ASSERT(actor != nullptr); }
		private:
			std::reference_wrapper<Actor> ActorRef;
			std::reference_wrapper<UICanvasElement> UIElementRef;

			friend class UIListElement;
		};
		UIListElement(ReferenceToUIActor actorRef, std::function<Vec3f()> getElementOffset, std::function<Vec3f()> getCenterOffset = nullptr);
		UIListElement(ReferenceToUIActor actorRef, const Vec3f& constElementOffset);
		bool IsBeingKilled() const;
		Actor& GetActorRef();
		const Actor& GetActorRef()const;
		UICanvasElement& GetUIElementRef();
		const UICanvasElement& GetUIElementRef() const;
		Vec3f GetElementOffset() const;
		Vec3f GetCenterOffset() const;

		void SetGetElementOffsetFunc(std::function<Vec3f()>);
		void SetGetCenterOffsetFunc(std::function<Vec3f()>);

	private:
		ReferenceToUIActor UIActorRef;
		std::function<Vec3f()> GetElementOffsetFunc, GetCenterOffsetFunc;
	};

	class UIListActor : public UIActorDefault
	{
	public:
		UIListActor(GameScene& scene, Actor* parentActor, const std::string& name);
		UIListActor(UIListActor&&);

		virtual void Refresh();

		virtual Vec3f GetListOffset();
		Vec3f GetListBegin();

		const UIListElement GetListElement(unsigned int index) const;
		int GetListElementCount() const;
		virtual Boxf<Vec2f> GetBoundingBoxIncludingChildren(bool world = true) const override;

		void SetListElementOffset(int index, std::function<Vec3f()> getElementOffset);
		void SetListCenterOffset(int index, std::function<Vec3f()> getCenterOffset);

		void AddElement(const UIListElement&);

		static Vec3f MoveElement(UIListElement& element, Vec3f nextElementBegin, float level);
		Vec3f MoveElements(unsigned int level = 0);

		void EraseListElement(std::function<bool(const UIListElement&)>);
		void EraseListElement(Actor&);


	protected:
		void CleanupDeadElements() const;

		std::vector<UIListElement> ListElements;
		bool bExpanded;
	};

	class UIAutomaticListActor : public UIListActor
	{
	public:
		UIAutomaticListActor(GameScene& scene, Actor* parentActor, const std::string& name, Vec3f elementOffset = Vec3f(0.0, -2.0f, 0.0f));
		UIAutomaticListActor(UIAutomaticListActor&&) noexcept;

		virtual Actor& AddChild(UniquePtr<Actor>) override;

	private:
		Vec3f ElementOffset;
	};
}