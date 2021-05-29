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
		virtual Boxf<Vec2f> GetBoundingBox(bool world = true);	//Returns a box at (0, 0) of size (0, 0). Canvas space
		UICanvas* GetCanvasPtr();
		unsigned int GetElementDepth() const;

		virtual ~UICanvasElement();
	protected:
		void SetParentElement(UICanvasElement&);
	protected:
		void AddChildElement(UICanvasElement&);
		void EraseChildElement(UICanvasElement&);
		virtual void AttachToCanvas(UICanvas& canvas);
		void DetachFromCanvas();
		friend class UICanvas;

		UICanvas* CanvasPtr;
		unsigned int ElementDepth;

		UICanvasElement* ParentElement;
		std::vector<UICanvasElement*> ChildElements;
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