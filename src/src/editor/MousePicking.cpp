#include "MousePicking.h"
#include <rendering/RenderEngine.h>
#include <rendering/Texture.h>
#include <game/GameScene.h>
#include <scene/Actor.h>
#include <scene/RenderableComponent.h>

namespace GEE
{
	/**
	* @brief Render the passed components and get the component whose fragment is generated at mousePos.
	* @param info
	* @param scene
	* @param resolution
	* @param mousePos: position of the mouse between (0, 0) and resolution. Warning: it is not NDC! (not between -1.0f and 1.0f).
	* @param components
	* @return
	*/

	Component* GEE::MousePickingRenderer::PickComponent(SceneMatrixInfo info, GameScene& scene, const Vec2u& resolution, const Vec2u& mousePos, std::vector<Component*> components)
	{
		// Generate framebuffer
		GEE_FB::Framebuffer framebuffer;
		framebuffer.Generate();

		// Generate and attach textures
		NamedTexture compIDTexture(Texture::Loader<float>::ReserveEmpty2D(resolution, Texture::Format::Float32::RGB()), "compIDTexture");
		NamedTexture depthTexture(Texture::Loader<Texture::LoaderArtificialType::Uint24_8>::ReserveEmpty2D(resolution, Texture::Format::Uint32::Depth24Stencil8(), Texture::Format::DepthStencil()), "depthTex");
		framebuffer.Attach(GEE_FB::FramebufferAttachment(compIDTexture, GEE_FB::AttachmentSlot::Color(0)));
		framebuffer.Attach(GEE_FB::FramebufferAttachment(depthTexture, GEE_FB::AttachmentSlot::DepthStencil()));

		// Get/create shader
		Shader* debugShader = Impl.RenderHandle.GetSimpleShader();

		// Setup framebuffer, viewport and rasterizer state
		framebuffer.Bind(true);
		glDisable(GL_DITHER);
		glDisable(GL_BLEND);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);


		// Render
		{
			unsigned int currentComponentIndex = 1;

			info.SetUseMaterials(false);
			info.SetAllowBlending(false);

			std::vector<std::pair<Component*, unsigned int>> notRenderables;
			notRenderables.reserve(components.size());	// avoid some overhead

			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);

			// Unbind current texture
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, 0);

			// Render renderables first
			for (auto comp : components)
			{
				auto compRenderableCast = dynamic_cast<RenderableComponent*>(comp);
				if (!compRenderableCast)
				{
					notRenderables.push_back(std::pair<Component*, unsigned int>(comp, currentComponentIndex++));
					continue;
				}

				Shader* shader = nullptr;
				if (compRenderableCast)
					if (auto materials = compRenderableCast->GetMaterials(); !materials.empty() && materials.front())
						shader = materials.front()->GetShaderInfo().RetrieveShaderForRendering(info.GetTbCollection());

				if (!shader)
				{
					currentComponentIndex++;
					continue;
				}

				shader->Use();
				shader->Uniform<Vec4f>("material.color", Vec4f(currentComponentIndex++, 0, 0, 1));
				shader->Uniform<bool>("material.disableColor", false);

				compRenderableCast->Render(info, shader);
			}

			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE);

			debugShader->Use();
			debugShader->Uniform<bool>("material.useAlbedoAsMask", true);
			debugShader->Uniform<bool>("material.disableColor", false);
			//info.UseMaterials = true;

			// Render the debug icon of nonrenderables on top
			for (auto it : notRenderables)
			{
				if (it.first->GetDebugMatInst())
					it.first->GetDebugMatInst()->UpdateWholeUBOData(debugShader, Impl.RenderHandle.GetEmptyTexture());

				debugShader->Uniform4fv("material.color", Vec4f(it.second, 0, 0, 1));
				it.first->DebugRender(info, *debugShader);
			}

			debugShader->Uniform<bool>("material.useAlbedoAsMask", false);
		}

		// Read one pixel at mousePos
		Vec4f pickedPixelIndex(0);
		glReadPixels(mousePos.x, mousePos.y, 1, 1, Texture::Format::RGBA().GetEnumGL(), GL_FLOAT, &pickedPixelIndex[0]);
		std::cout << "#$# Picked pixel " << pickedPixelIndex << '\n';

		// Clean up after everything
		framebuffer.Dispose(true);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_DITHER);

		return (pickedPixelIndex.x > 0 && pickedPixelIndex.x <= components.size()) ? (components[pickedPixelIndex.x - 1]) : (nullptr);
	}
}