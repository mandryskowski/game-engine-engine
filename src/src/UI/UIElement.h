#pragma once
#include <math/Box.h>

namespace GEE
{
	class UICanvas;
	class Transform;

	class UICanvasElement
	{
	public:
		UICanvasElement();
		UICanvasElement(const UICanvasElement&) = delete;
		UICanvasElement(UICanvasElement&&);
		/**
		 * @param world: if true, the bounding box returned is in world (or canvas world) space. otherwise, we only use the local transform.
		 * @return a box at (0, 0) of size (0, 0). Canvas space.
		*/
		virtual Boxf<Vec2f> GetBoundingBox(bool world = true) const;
		/**
		 * @brief Get the smallest bounding box that contains the bounding box of this UICanvasElement and its child UICanvasElements (and the children of the children, etc.).
		 * @param world: if true, the bounding box returned is in world (or canvas world) space. otherwise, we only use the local transform.
		 * @return a box at (0, 0) of size (0, 0). Canvas space.
		*/
		virtual Boxf<Vec2f> GetBoundingBoxIncludingChildren(bool world = true) const;
		UICanvas* GetCanvasPtr();
		const UICanvas* GetCanvasPtr() const;
		unsigned int GetElementDepth() const;

		void DetachFromCanvas();

		virtual ~UICanvasElement();
	//protected:
		void SetParentElement(UICanvasElement&);
	//protected:
		void AddChildElement(UICanvasElement&);
		void EraseChildElement(UICanvasElement&);
		virtual void AttachToCanvas(UICanvas& canvas);
		friend class UICanvas;

		UICanvas* CanvasPtr;
		unsigned int ElementDepth;

		UICanvasElement* ParentElement;
		std::vector<UICanvasElement*> ChildUIElements;
	};
	/*
	class UICanvasComponent : public UICanvasElement
	{
	public:
		virtual Transform GetCanvasSpaceTransform() override;
	};

	class UICanvasActor : public UICanvasElement
	{
	public:
		virtual Transform GetCanvasSpaceTransform() override;
	};*/
}