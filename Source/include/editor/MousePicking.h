#pragma once
#include <rendering/RenderEngine.h>
#include <rendering/Texture.h>
#include <game/GameScene.h>
#include <scene/Actor.h>

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
		Component* PickComponent(RenderInfo info, GameScene& scene, const Vec2u& resolution, const Vec2u& mousePos, std::vector<Component*> components)
		{
			// Generate framebuffer
			GEE_FB::Framebuffer framebuffer;
			framebuffer.Generate();

			// Generate and attach textures
			NamedTexture compIDTexture(Texture::Loader<unsigned int>::ReserveEmpty2D(resolution, Texture::Format::Uint32::RGBA(), Texture::Format::Integer::RGBA()), "compIDTexture");
			NamedTexture depthTexture(Texture::Loader<Texture::LoaderArtificialType::Uint24_8>::ReserveEmpty2D(resolution, Texture::Format::Uint32::Depth24Stencil8(), Texture::Format::DepthStencil()), "depthTex");
			framebuffer.AttachTextures(compIDTexture, GEE_FB::FramebufferAttachment(depthTexture, GEE_FB::AttachmentSlot::DepthStencil()));

			// Get/create shader
			Shader* shader;
			if (shader = Impl.RenderHandle.FindShader("MousePicking"); !shader)
			{
				auto createdShader = ShaderLoader::LoadShadersWithInclData("MousePicking", "#define MOUSE_PICKING 1\n", "Shaders/depth.vs", "Shaders/depth.fs");
				Impl.RenderHandle.AddShader(createdShader);
				shader = createdShader.get();
				shader->SetExpectedMatrices({ MatrixType::MVP });
				shader->UniformBlockBinding("BoneMatrices", 10);
			}

			// Setup framebuffer, viewport and rasterizer state
			framebuffer.Bind(true);
			glDisable(GL_DITHER);
			glDisable(GL_BLEND);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);	
			

			// Render
			{
				unsigned int currentComponentIndex = 1;

				shader->Use();
				info.UseMaterials = false;

				std::vector<std::pair<Component*, unsigned int>> notRenderables;
				notRenderables.reserve(components.size());	// avoid some overhead

				glEnable(GL_DEPTH_TEST);
				glEnable(GL_CULL_FACE);

				// Render renderables first
				for (auto comp : components)
				{
					shader->Uniform4fv("compIDUniform", Vec4f(currentComponentIndex, 0, 0, 1));
					if (auto compRenderable = dynamic_cast<RenderableComponent*>(comp))
						compRenderable->Render(info, shader);
					else
						notRenderables.push_back(std::pair<Component*, unsigned int>(comp, currentComponentIndex));
					currentComponentIndex++;
				}

				glDisable(GL_DEPTH_TEST);
				glDisable(GL_CULL_FACE);
				
				// Render the debug icon of nonrenderables on top
				for (auto it : notRenderables)
				{
					shader->Uniform4fv("compIDUniform", Vec4f(it.second, 0, 0, 1));
					it.first->DebugRender(info, shader);
				}
			}

			// Read one pixel at mousePos
			Vec4u pickedPixelIndex(0);
			glReadPixels(mousePos.x, mousePos.y, 1, 1, Texture::Format::Integer::RGBA().GetEnumGL(), GL_UNSIGNED_INT, &pickedPixelIndex[0]);

			// Clean up after everything
			framebuffer.Dispose(true);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_DITHER);
			glEnable(GL_BLEND);

			return (pickedPixelIndex.x > 0 && pickedPixelIndex.x <= components.size()) ? (components[pickedPixelIndex.x - 1]) : (nullptr);
		}
	};
}