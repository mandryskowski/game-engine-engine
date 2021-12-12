#pragma once
#include <game/GameManager.h>
#include <rendering/Renderer.h>

namespace GEE
{
	struct MousePickingRenderer : public Renderer
	{
		MousePickingRenderer(RenderEngineManager& renderHandle) : Renderer(renderHandle) {}
		/**
		 * @brief Render the passed components and get the component whose fragment is generated at mousePos.
		 * @param info 
		 * @param scene 
		 * @param resolution 
		 * @param mousePos: position of the mouse between (0, 0) and resolution. Warning: it is not NDC! (not between -1.0f and 1.0f).
		 * @param components 
		 * @return 
		*/
		Component* PickComponent(SceneMatrixInfo info, GameScene& scene, const Vec2u& resolution, const Vec2u& mousePos, std::vector<Component*> components);
	};
}