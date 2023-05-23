#include "OutlineRenderer.h"

#include "Framebuffer.h"
#include "Shader.h"
#include "material/Material.h"
using namespace GEE;

void OutlineRenderer::RenderOutlineStencil(SceneMatrixInfo info, Component* comp, GEE_FB::Framebuffer& targetFramebuffer)
{
	if (!comp)
		return;


	// Set up rasterizer state
	glEnable(GL_STENCIL_TEST);
	glClear(GL_STENCIL_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	// Pass 1: render component's fragments to stencil buffer
	glDrawBuffer(GL_NONE);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilFunc(GL_ALWAYS, 1, 0xFF);

	Shader* shader = Impl.RenderHandle.FindShader("Depth");
	auto compRenderableCast = dynamic_cast<RenderableComponent*>(comp);

	shader->Use();
	// We will use our own material
	info.SetUseMaterials(false);

	auto renderFunc = [&]()
	{
		if (compRenderableCast)
			compRenderableCast->Render(info, shader);
		else
			comp->DebugRender(info, *shader);
	};

	renderFunc();


	// Pass 2: render outline
	targetFramebuffer.SetDrawSlot(0);
	//glStencilMask(0x00);
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);

	// Apply scale to make the outline bigger than the mesh (works only for Renderables)
	//comp->GetTransform().ApplyScale(1.05f);
	// Apply scale for debug icons (works only for non-Renderables)
	//debugIconScale *= 1.05f;

	// Get/create shader
	if (shader = Impl.RenderHandle.FindShader("SimpleColor3D"); !shader)
	{
		auto createdShader = ShaderLoader::LoadShadersWithInclData("SimpleColor3D", "#define SIMPLE_COLOR 1\n#define EXTRUDE_VERTICES 1\n", "Shaders/depth.vs", "Shaders/depth.fs");
		Impl.RenderHandle.AddShader(createdShader);
		shader = createdShader.get();
		shader->SetExpectedMatrices({ MatrixType::MVP });
		shader->UniformBlockBinding("BoneMatrices", 10);
	}
	shader->Use();

	// Our own material
	shader->Uniform<Vec4f>("material.color", Vec4f(1.0f, 0.0f, 0.0f, 0.25f));	// Set color to red

	renderFunc();

	// De-apply scale to avoid actually scaling the component - we just needed that to render the outline
	//comp->GetTransform().ApplyScale(1.0f / 1.05f);


	// Clean up after everything
	glStencilMask(0xFF);
	glStencilFunc(GL_ALWAYS, 0, 0xFF);

	glClear(GL_STENCIL_BUFFER_BIT);
	glDisable(GL_BLEND);

	targetFramebuffer.SetDrawSlots();
}

void OutlineRenderer::RenderOutlineSilhouette(SceneMatrixInfo info, Component* comp,
	GEE_FB::Framebuffer& targetFramebuffer)
{
	// Check if comp is a renderable (doesn't have to be, we can also render the debug icon)
	auto compRenderableCast = dynamic_cast<RenderableComponent*>(comp);

	// Get/create shader
	ShaderInfo shaderInfo = MaterialShaderHint::Simple;
	if (compRenderableCast)
	{
		if (auto materials = compRenderableCast->GetMaterials(); !materials.empty() && materials.front())
			shaderInfo = materials.front()->GetShaderInfo();
	}

	// Generate silhouetteFramebuffer
	GEE_FB::Framebuffer silhouetteFramebuffer;
	silhouetteFramebuffer.Generate();
	silhouetteFramebuffer.Bind(false);

	// Generate and attach textures
	NamedTexture silhouetteTexture(Texture::Loader<>::ReserveEmpty2D(targetFramebuffer.GetSize()), "silhouetteTexture");
	silhouetteTexture.SetMagFilter(Texture::MagFilter::Nearest(), true);
	silhouetteTexture.SetMinFilter(Texture::MinFilter::Nearest(), true);
	silhouetteFramebuffer.AttachTextures(silhouetteTexture, targetFramebuffer.GetAnyDepthAttachment());

	// Pass 1: render component's silhouette:
	{
		Shader* shader = shaderInfo.RetrieveShaderForRendering(info.GetTbCollection());
		glDepthMask(0x00);
		silhouetteFramebuffer.Bind(true);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glEnable(GL_CULL_FACE);

		info.SetUseMaterials(true);
		info.SetRequiredShaderInfo(shaderInfo);
		shader->Use();

		if (compRenderableCast)
			compRenderableCast->Render(info, shader);
		else
			comp->DebugRender(info, *shader);
	}

	// Pass 2: render outline to targetFramebuffer
	{
		targetFramebuffer.Bind();
		targetFramebuffer.SetDrawSlot(0);

		info.SetUseMaterials(false);
		info.StopRequiringShaderInfo();

		Shader* shader;
		if (shader = Impl.RenderHandle.FindShader("SilhouetteOutline"); !shader)
		{
			auto createdShader = ShaderLoader::LoadShadersWithInclData("SilhouetteOutline", "#define OUTLINE_FROM_SILHOUETTE 1\n", "Shaders/quad.vs", "Shaders/quad.fs");
			Impl.RenderHandle.AddShader(createdShader);
			shader = createdShader.get();
		}

		shader->Use();
		shader->Uniform<Vec4f>("material.color", Vec4f(1.0f, 1.0f, 0.0f, 1.0f));

		silhouetteTexture.Bind(0);

		Impl.RenderHandle.GetBasicShapeMesh(EngineBasicShape::Quad).Bind(info.GetContextID());
		SceneMatrixInfo infoNoMaterials(info.GetTbCollection(), info.GetSceneRenderData());
		infoNoMaterials.SetUseMaterials(false);
		StaticMeshInstances(infoNoMaterials, { Impl.GetBasicShapeMesh(EngineBasicShape::Quad) }, Transform(), *shader);
	}

	// Clean up after everything
	glDepthMask(0xFF);
	silhouetteFramebuffer.Detach(targetFramebuffer.GetAnyDepthAttachment().GetAttachmentSlot());
	silhouetteFramebuffer.Dispose(true);
	targetFramebuffer.SetDrawSlots();
	glBindTexture(GL_TEXTURE_2D, 0);
}
