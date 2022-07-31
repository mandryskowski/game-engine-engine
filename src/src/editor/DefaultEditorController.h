#pragma once
#include <scene/Controller.h>

namespace GEE
{
	namespace Editor
	{
		class DefaultEditorController : public Controller
		{
		public:
			enum class ControllerState
			{
				Default,
				Grab,
				Rotate,
				Resize
			};
			enum AxisBits
			{
				X = 1,
				Y = 2,
				Z = 4,
				W = 8,
				All = 15
			};
		
			DefaultEditorController(GameScene& scene, Actor* parentActor, const std::string& name);

			unsigned int GetAxisBits() const { return AxisModeBits; }

			virtual void HandleEvent(const Event& ev) override;
			virtual void OnMouseMovement(const Vec2f& previousPosPx, const Vec2f& currentPosPx, const Vec2u& windowSize) override;
			
			// Resets ControllerState and axis.
			void Reset(const Transform& t);

		private:
			ControllerState State;
			unsigned int AxisModeBits;
			Transform TransformBeforeAction;
		};

	}
}
