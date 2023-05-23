#pragma once
#include <rendering/RenderEngine.h>
#include <scene/ModelComponent.h>

#include "Renderer.h"

namespace GEE
{
	struct OutlineRenderer : public Renderer
	{
		using Renderer::Renderer;
		/**
		 * @brief 
		 * @param info 
		 * @param comp 
		 * @param targetFramebuffer: A stencil buffer must be attached
		*/
		void RenderOutlineStencil(SceneMatrixInfo info, Component* comp, GEE_FB::Framebuffer& targetFramebuffer);
		void RenderOutlineSilhouette(SceneMatrixInfo info, Component* comp, GEE_FB::Framebuffer& targetFramebuffer);
	};
}
