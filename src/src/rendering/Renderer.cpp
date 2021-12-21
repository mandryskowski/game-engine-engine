#include <rendering/Renderer.h>
#include <rendering/Mesh.h>
#include <rendering/Framebuffer.h>
#include <rendering/RenderInfo.h>
#include <rendering/RenderToolbox.h>
#include <game/GameScene.h>
#include <rendering/LightProbe.h>
#include <scene/LightProbeComponent.h>
#include <rendering/OutlineRenderer.h>
#include <scene/LightComponent.h>
#include <editor/EditorManager.h>

namespace GEE
{
	const Mesh* BindingsGL::BoundMesh = nullptr;
	Shader* BindingsGL::UsedShader = nullptr;
	const Material* BindingsGL::BoundMaterial = nullptr;

	Renderer::Renderer(RenderEngineManager& engineHandle, const GEE_FB::Framebuffer* optionalFramebuffer) :
		Renderer(ImplUtil(engineHandle, optionalFramebuffer))
	{
	}
	
	Renderer::Renderer(const ImplUtil& impl) :
		Impl(impl)
	{
	}

	Renderer::Renderer(Renderer&& renderer):
		Impl(renderer.Impl)
	{
	}

	Renderer::Renderer(const Renderer& renderer):
		Impl(renderer.Impl)
	{
	}

	void Renderer::StaticMeshInstances(const MatrixInfoExt& info, const std::vector<MeshInstance>& meshes, const Transform& transform, Shader& shader, bool billboard)
	{
		if (meshes.empty())
			return;

		bool handledShader = false;
		for (const MeshInstance& meshInst : meshes)
		{
			const Mesh& mesh = meshInst.GetMesh();
			MaterialInstance* materialInst = meshInst.GetMaterialInst();
			const Material* material = meshInst.GetMaterialPtr().get();

			if ((material && !material->GetShaderInfo().MatchesRequiredInfo(info.GetRequiredShaderInfo())) ||
				(info.GetOnlyShadowCasters() && !mesh.CanCastShadow()) || 
				(materialInst && !materialInst->ShouldBeDrawn()))
				continue;

			if (!handledShader)
			{
				handledShader = true;

				Mat4f modelMat = transform.GetWorldTransformMatrix();	//the ComponentTransform's world transform is cached
				if (billboard)
					modelMat = modelMat * Mat4f(glm::inverse(transform.GetWorldTransform().GetRotationMatrix()) * glm::inverse(Mat3f(info.GetView())));


				shader.BindMatrices(modelMat, &info.GetView(), &info.GetProjection(), &info.GetVP());
				shader.CallPreRenderFunc();	// Call user-defined pre render function
			}

			////////////////////////////////////////Impl.BindMesh(&mesh);
			////////////////////////////////////////Impl::MeshBind->Set(&mesh);

			if (info.GetUseMaterials() && materialInst)
			{
				materialInst->UpdateWholeUBOData(&shader, Texture());//////////////////////////////////////// Impl::BindMaterialInstance(materialInst);
			}

			mesh.Bind(info.GetContextID());
			mesh.Render();
		}
	}

	Mat4f Renderer::ImplUtil::GetCubemapView(GEE_FB::Axis cubemapSide)
	{
		static Mat4f defaultVPs[6] = { glm::lookAt(Vec3f(0.0f), Vec3f(1.0f, 0.0f, 0.0f), Vec3f(0.0f, -1.0f, 0.0f)),
								glm::lookAt(Vec3f(0.0f), Vec3f(-1.0f, 0.0f, 0.0f), Vec3f(0.0f, -1.0f, 0.0f)),
								glm::lookAt(Vec3f(0.0f), Vec3f(0.0f, 1.0f, 0.0f), Vec3f(0.0f, 0.0f, 1.0f)),
								glm::lookAt(Vec3f(0.0f), Vec3f(0.0f, -1.0f, 0.0f), Vec3f(0.0f, 0.0f, -1.0f)),
								glm::lookAt(Vec3f(0.0f), Vec3f(0.0f, 0.0f, 1.0f), Vec3f(0.0f, -1.0f, 0.0f)),
								glm::lookAt(Vec3f(0.0f), Vec3f(0.0f, 0.0f, -1.0f), Vec3f(0.0f, -1.0f, 0.0f)), };

		Mat4f cubemapProj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 2.0f);

		return defaultVPs[static_cast<unsigned int>(cubemapSide)];
	}

	Mesh Renderer::ImplUtil::GetBasicShapeMesh(EngineBasicShape shapeType)
	{
		return RenderHandle.GetBasicShapeMesh(shapeType);
	}

	Shader& Renderer::ImplUtil::GetShader(RendererShaderHint hint)
	{
		switch (hint)
		{
		case RendererShaderHint::DefaultForward:
			return *RenderHandle.GetSimpleShader();
		case RendererShaderHint::DepthOnly:
			return *RenderHandle.FindShader("Depth");
		case RendererShaderHint::DepthOnlyLinearized:
			return *RenderHandle.FindShader("DepthLinearize");
		case RendererShaderHint::IBL:
			return Shader();// *RenderHandle.FindShader("CookTorranceIBL");
		}
		return Shader();
	}

	void CubemapRenderer::FromTexture(RenderingContextID contextID, Texture targetTex, Texture tex, Vec2u size, Shader& shader, int* layer, int mipLevel)
	{
		GEE_FB::Framebuffer framebuffer;
		framebuffer.Generate();
		framebuffer.Bind(Viewport(size));

		glActiveTexture(GL_TEXTURE0);
		glDepthFunc(GL_LEQUAL);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);

		shader.Use();
		Mesh& cubeMesh = Impl.GetBasicShapeMesh(EngineBasicShape::CUBE);
		cubeMesh.Bind(contextID);
		BindingsGL::BoundMesh = &cubeMesh;

		if (PrimitiveDebugger::bDebugCubemapFromTex)
		{
			std::cout << "0: ";
			debugFramebuffer();
		}

		framebuffer.SetDrawSlot(0);

		for (int i = 0; i < 6; i++)
		{
			targetTex.Bind();
			switch (targetTex.GetType())
			{
			case GL_TEXTURE_CUBE_MAP:		framebuffer.Attach(GEE_FB::FramebufferAttachment(targetTex, static_cast<GEE_FB::Axis>(i), GEE_FB::AttachmentSlot::Color(0), mipLevel), false, false); break;
			case GL_TEXTURE_CUBE_MAP_ARRAY: framebuffer.Attach(GEE_FB::FramebufferAttachment(targetTex, *layer, GEE_FB::AttachmentSlot::Color(0), mipLevel), false, false); *layer = *layer + 1; break;
			default:						std::cerr << "ERROR: Can't render a scene to cubemap texture, when the texture's type is " << targetTex.GetType() << ".\n"; return;
			}
			if (PrimitiveDebugger::bDebugCubemapFromTex)
			{
				std::cout << "1: ";
				debugFramebuffer();
			}

			tex.Bind();

			MatrixInfoExt info;
			info.SetView(Impl.GetCubemapView(static_cast<GEE_FB::Axis>(i)));
			info.SetProjection(glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 2.0f));
			info.CalculateVP();
			info.StopRequiringShaderInfo();

			StaticMeshInstances(info, { cubeMesh }, Transform(), shader);
			framebuffer.Detach(GEE_FB::AttachmentSlot::Color(0));
		}

		framebuffer.Dispose(false);

		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
	}

	void CubemapRenderer::FromScene(SceneMatrixInfo info, GEE_FB::Framebuffer target, GEE_FB::FramebufferAttachment targetTex, Shader* shader, int* layer, bool fullRender)
	{
		if (fullRender) std::cout << "!*! Przed bind\n";

		{
			target.Bind(targetTex.GetSize2D());
		}
		glActiveTexture(GL_TEXTURE0);

		Mat4f viewtranslation = info.GetView();

		for (int i = 0; i < 6; i++)
		{
			if (fullRender) std::cout << "!*! Przed framebuffer bind\n";
			targetTex.Bind();
			switch (targetTex.GetType())
			{
			case GL_TEXTURE_CUBE_MAP:		 target.Attach(GEE_FB::FramebufferAttachment(targetTex, static_cast<GEE_FB::Axis>(i), targetTex.GetAttachmentSlot()), false); break;
			case GL_TEXTURE_CUBE_MAP_ARRAY: target.Attach(GEE_FB::FramebufferAttachment(targetTex, *layer, targetTex.GetAttachmentSlot()), false); *layer = *layer + 1; break;
			default:						std::cerr << "ERROR: Can't render a scene to cubemap texture, when the texture's type is " << targetTex.GetType() << ".\n"; return;
			}
			if (fullRender) std::cout << "!*! Po framebuffer bind\n";
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			info.SetView(Impl.GetCubemapView(static_cast<GEE_FB::Axis>(i)) * viewtranslation);
			info.CalculateVP();

			SceneRenderer sceneRenderer(Impl.RenderHandle, &target);
			if (fullRender) std::cout << "!*! Przed actual render\n";
			(fullRender) ? (sceneRenderer.FullRender(info, Viewport(targetTex.GetSize2D()))) : (sceneRenderer.RawRender(info, *shader));
			if (fullRender) std::cout << "!*! Po actual render\n";
			target.Detach(targetTex.GetAttachmentSlot());
		}
	}

	void VolumeRenderer::Volume(const MatrixInfoExt& info, EngineBasicShape shape, Shader& shader, const Transform& transform)
	{
		if (shape == EngineBasicShape::QUAD)
			glDisable(GL_CULL_FACE);

		shader.Use();
		BindingsGL::BoundMaterial = nullptr;
		BindingsGL::BoundMesh = nullptr;
		StaticMeshInstances(info, { Impl.GetBasicShapeMesh(shape) }, transform, shader);

		if (shape == EngineBasicShape::QUAD)
			glEnable(GL_CULL_FACE);
	}

	void VolumeRenderer::Volumes(const SceneMatrixInfo& info, const std::vector<UniquePtr<RenderableVolume>>& volumes, bool bIBLPass)
	{
		glEnable(GL_DEPTH_TEST);
		glDepthMask(0x00);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ONE, GL_ONE);
		glEnable(GL_STENCIL_TEST);
		glStencilMask(0xFF);
		glClear(GL_STENCIL_BUFFER_BIT);
		//glClearStencil(0);	//my AMD gpu forces me to call it...

		for (const UniquePtr<RenderableVolume>& volume : volumes)
		{
			//1st pass: stencil
			if (volume->GetShape() != EngineBasicShape::QUAD)		//if this is a quad everything will be in range; don't waste time
			{
				glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_INVERT, GL_KEEP);
				glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_INVERT);
				glStencilMask(0xFF);
				glDisable(GL_CULL_FACE);
				glStencilFunc(GL_GREATER, 128, 0xFF);
				glDepthFunc(GL_LEQUAL);
				glDrawBuffer(GL_NONE);
				Volume(info, volume->GetShape(), Impl.GetShader(RendererShaderHint::DepthOnly), volume->GetRenderTransform());
			}

			//2nd pass: lighting
			glStencilOp(GL_INVERT, GL_INCR, GL_INCR);
			if (volume->GetShape() == EngineBasicShape::QUAD)
				glStencilMask(0x00);
			if (bIBLPass)
				glStencilFunc(GL_EQUAL, 0, 0xFF);
			else
				glStencilFunc(((volume->GetShape() == EngineBasicShape::QUAD) ? (GL_ALWAYS) : (GL_GREATER)), 128, 0xFF);

			glEnable(GL_CULL_FACE);
			glDepthFunc(GL_ALWAYS);

			if (Impl.OptionalFramebuffer)
				Impl.OptionalFramebuffer->SetDrawSlots();

			Shader* shader = volume->GetRenderShader(info.GetTbCollection());
			shader->Use();
			volume->SetupRenderUniforms(*shader);
			Volume((volume->GetShape() == EngineBasicShape::QUAD) ? (MatrixInfoExt()) : (info), volume->GetShape(), *shader, volume->GetRenderTransform());
		}

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_BLEND);
		glCullFace(GL_BACK);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(0xFF);
		glStencilMask(0xFF);
		glDisable(GL_CULL_FACE);
		glDisable(GL_STENCIL_TEST);
	}

	void ShadowMapRenderer::ShadowMaps(RenderingContextID contextID, RenderToolboxCollection& tbCollection, GameSceneRenderData& sceneRenderData, std::vector<std::reference_wrapper<LightComponent>> lights)
	{
		ShadowMappingToolbox* shadowsTb = tbCollection.GetTb<ShadowMappingToolbox>();
		shadowsTb->ShadowFramebuffer->Bind();
		bool dynamicShadowRender = tbCollection.GetSettings().ShadowLevel > SettingLevel::SETTING_MEDIUM;
		{
			if (shadowsTb->ShadowCubemapArray->GetSize2D() != shadowsTb->ShadowMapArray->GetSize2D())
				std::cout << "INFO: 2D size of shadow cubemap array does not match the 2D size of shadow map array. This is not currently supported - shadow map array size will be picked.\n";
			Viewport(shadowsTb->ShadowMapArray->GetSize2D()).SetOpenGLState();
		}
		glEnable(GL_DEPTH_TEST);
		glDrawBuffer(GL_NONE);

		bool bCubemapBound = false;

		shadowsTb->ShadowMapArray->Bind(10);
		shadowsTb->ShadowCubemapArray->Bind(11);

		Impl.RenderHandle.BindSkeletonBatch(&sceneRenderData, static_cast<unsigned int>(0));

		for (int i = 0; i < static_cast<int>(lights.size()); i++)
		{
			LightComponent& light = lights[i].get();
			if (!dynamicShadowRender && light.HasValidShadowMap())
				continue;

			if (light.ShouldCullFrontsForShadowMap())
			{
				glEnable(GL_CULL_FACE);
				glCullFace(GL_FRONT);
			}
			else
			{
				glDisable(GL_CULL_FACE);
			}

			if (light.GetType() == LightType::POINT)
			{
				if (!bCubemapBound)
				{
					Impl.GetShader(RendererShaderHint::DepthOnlyLinearized).Use();
					bCubemapBound = true;
				}

				Transform lightWorld = light.GetTransform().GetWorldTransform();
				Vec3f lightPos = lightWorld.GetPos();
				Mat4f viewTranslation = glm::translate(Mat4f(1.0f), -lightPos);
				Mat4f projection = light.GetProjection();

				Impl.GetShader(RendererShaderHint::DepthOnlyLinearized).Uniform1f("far", light.GetFar());
				Impl.GetShader(RendererShaderHint::DepthOnlyLinearized).Uniform<float>("lightBias", light.GetShadowBias());
				Impl.GetShader(RendererShaderHint::DepthOnlyLinearized).Uniform3fv("lightPos", lightPos);

				int cubemapFirst = light.GetShadowMapNr() * 6;


				SceneMatrixInfo info(contextID, tbCollection, sceneRenderData, viewTranslation, projection, lightPos);
				info.SetUseMaterials(false);
				info.SetOnlyShadowCasters(true);
				info.StopRequiringShaderInfo();
				shadowsTb->ShadowFramebuffer->Attachments.push_back(GEE_FB::FramebufferAttachment(*shadowsTb->ShadowCubemapArray, GEE_FB::AttachmentSlot::Depth()));

				CubemapRenderer(Impl.RenderHandle, shadowsTb->ShadowFramebuffer).FromScene(info, *shadowsTb->ShadowFramebuffer, shadowsTb->ShadowFramebuffer->GetAnyDepthAttachment(), &Impl.GetShader(RendererShaderHint::DepthOnlyLinearized), &cubemapFirst);
			}
			else
			{
				if (bCubemapBound || i == 0)
				{
					Impl.GetShader(RendererShaderHint::DepthOnly).Use();
					bCubemapBound = false;
				}

				Mat4f view = light.GetTransform().GetWorldTransform().GetViewMatrix();
				Mat4f projection = light.GetProjection();
				Mat4f VP = projection * view;

				Impl.GetShader(RendererShaderHint::DepthOnly).Uniform<float>("lightBias", light.GetShadowBias());

				shadowsTb->ShadowFramebuffer->Attach(GEE_FB::FramebufferAttachment(*shadowsTb->ShadowMapArray, light.GetShadowMapNr(), GEE_FB::AttachmentSlot::Depth()), false, false);
				glClear(GL_DEPTH_BUFFER_BIT);

				SceneMatrixInfo info(contextID, tbCollection, sceneRenderData, view, projection, Vec3f(0.0f));
				info.CalculateVP();
				info.SetUseMaterials(false);
				info.SetOnlyShadowCasters(true);
				SceneRenderer(Impl.RenderHandle, shadowsTb->ShadowFramebuffer).RawRender(info, Impl.GetShader(RendererShaderHint::DepthOnly));

				shadowsTb->ShadowFramebuffer->Detach(GEE_FB::AttachmentSlot::Depth());
			}

			if (!dynamicShadowRender)
				light.MarkValidShadowMap();
		}

		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//std::cout << "Wyczyscilem sobie " << timeSum * 1000.0f << "ms.\n";
	}

	void SceneRenderer::PreRenderLoopPassStatic(RenderingContextID contextID, std::vector<GameSceneRenderData*>& renderDatas)
	{
		std::cout << "(((((( scene render datas: " << renderDatas.size() << "\n";
		for (int sceneIndex = 0; sceneIndex < static_cast<int>(renderDatas.size()); sceneIndex++)
		{
			GameSceneRenderData& sceneRenderData = *renderDatas[sceneIndex];
			sceneRenderData.SetupLights(sceneIndex);
			for (int i = 0; i < 2; i++)	// Render light probes twice
				LightProbeRenderer(Impl).AllSceneProbes(contextID, sceneRenderData);

			sceneRenderData.ProbesLoaded = true;
		}
	}

	void SceneRenderer::PreFramePass(RenderingContextID contextID, RenderToolboxCollection& tbCollection, GameSceneRenderData& sceneRenderData)
	{
		if (sceneRenderData.ContainsLights() && (tbCollection.GetSettings().ShadowLevel > SettingLevel::SETTING_MEDIUM || sceneRenderData.HasLightWithoutShadowMap()))
			ShadowMapRenderer(Impl).ShadowMaps(contextID, tbCollection, sceneRenderData, sceneRenderData.Lights);
	}

	void SceneRenderer::FullRender(SceneMatrixInfo& info, Viewport viewport, bool clearMainFB, bool modifyForwardsDepthForUI, std::function<void(GEE_FB::Framebuffer&)>&& renderIconsFunc, bool forceForwardShading)
	{
		auto& tbCollection = info.GetTbCollection();
		auto& sceneRenderData = info.GetSceneRenderData();
		const GameSettings::VideoSettings& settings = tbCollection.GetSettings();
		bool debugPhysics = false;//GameHandle->GetInputRetriever().IsKeyPressed(Key::F2);
		bool debugComponents = true;
		bool useLightingAlgorithms = sceneRenderData.ContainsLights() || sceneRenderData.ContainsLightProbes();
		Mat4f currentFrameView = info.GetView();
		//info.previousFrameView = PreviousFrameView;			AAAAAAAAAAAAAAAAAAAAAAAA SMAA NAPRAW

		const GEE_FB::Framebuffer& target = ((Impl.OptionalFramebuffer) ? (*Impl.OptionalFramebuffer) : (GEE_FB::getDefaultFramebuffer(settings.Resolution)));
		if (viewport.GetSize() == Vec2u(0))
			viewport = Viewport(target.GetSize());

		//sceneRenderData.LightsBuffer.SubData4fv(info.camPos, 16); TO BYLO TU...

		MainFramebufferToolbox* mainTb = tbCollection.GetTb<MainFramebufferToolbox>();
		GEE_FB::Framebuffer& MainFramebuffer = *mainTb->MainFb;

		glDepthFunc(GL_LEQUAL);

		if (!forceForwardShading)
		{
			////////////////////2. Deferred Shading
			if (useLightingAlgorithms)
			{
				if (sceneRenderData.LightsBuffer.HasBeenGenerated())	// Update camera position
					sceneRenderData.LightsBuffer.SubData4fv(info.GetCamPosition(), 16);
				DeferredShadingToolbox* deferredTb = tbCollection.GetTb<DeferredShadingToolbox>();	//Get the required textures and shaders for Deferred Shading.
				GEE_FB::Framebuffer& GFramebuffer = *deferredTb->GFb;

				GFramebuffer.Bind(Viewport(viewport.GetSize()));	// Make sure the viewport for the g framebuffer stays at (0, 0) and covers the entire buffer

				// Clear buffers to erase what has been left over from the previous frame.
				glEnable(GL_DEPTH_TEST);
				glEnable(GL_CULL_FACE);
				glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				info.SetMainPass(true);
				info.SetRequiredShaderInfo(MaterialShaderHint::Shaded);


				////////////////////2.1 Geometry pass of Deferred Shading
				if (debugPhysics)
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

				Shader* gShader = deferredTb->GeometryShader;
				gShader->Use();
				gShader->Uniform3fv("camPos", info.GetCamPosition());

				RawRender(info, *gShader);

				info.SetMainPass(false);
				info.StopRequiringShaderInfo();


				if (debugPhysics)
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

				////////////////////2.2 SSAO pass
				Texture SSAOtex;
				
				if (settings.AmbientOcclusionSamples > 0)
					SSAOtex = PostprocessRenderer(Impl.RenderHandle, GFramebuffer).SSAO(info, GFramebuffer.GetColorTexture(1), GFramebuffer.GetColorTexture(2));	//pass gPosition and gNormal

				////////////////////3. Lighting pass
				for (int i = 0; i < static_cast<int>(deferredTb->LightShaders.size()); i++)
					deferredTb->LightShaders[i]->UniformBlockBinding("Lights", sceneRenderData.LightsBuffer.BlockBindingSlot);
				sceneRenderData.UpdateLightUniforms();

				MainFramebuffer.Bind(); // Bind the "Main" framebuffer, where we output light information. 
				glClearBufferfv(GL_COLOR, 0, Math::GetDataPtr(Vec3f(0.0f, 0.0f, 0.0f))); // Clear ONLY the color buffers. The depth information stays for the light pass (we need to know which light volumes actually contain any objects that are affected by the light)
				if (tbCollection.GetSettings().bBloom)
					glClearBufferfv(GL_COLOR, 1, Math::GetDataPtr(Vec3f(0.0f)));
				else
					Texture().Bind(1);
				
				for (int i = 0; i < 4; i++)	// Bind all GBuffer textures (Albedo+Specular, Position, Normal, PBR Cook-Torrance Alpha+Metalness+AmbientOcclusion)
					GFramebuffer.GetColorTexture(i).Bind(i);

				if (SSAOtex.HasBeenGenerated())
					SSAOtex.Bind(4);

				VolumeRenderer(Impl.RenderHandle, &MainFramebuffer).Volumes(info, sceneRenderData.GetSceneLightsVolumes(), false);
				
				////////////////////3.1 IBL pass
				
				deferredTb->FindShader("CookTorranceIBL")->Use();
				//Impl.GetShader(RendererShaderHint::IBL).Use();
				for (int i = 0; i < static_cast<int>(sceneRenderData.LightProbes.size()); i++)
				{
					LightProbeComponent* probe = sceneRenderData.LightProbes[i];
					Shader* shader = probe->GetRenderShader(tbCollection);
					shader->Uniform1f("lightProbes[" + std::to_string(i) + "].intensity", probe->GetProbeIntensity());

					if (probe->GetShape() == EngineBasicShape::QUAD)
						continue;

					shader->Uniform3fv("lightProbes[" + std::to_string(i) + "].position", probe->GetTransform().GetWorldTransform().GetPos());
				}
				
				auto probeVolumes = sceneRenderData.GetLightProbeVolumes();

				if (!probeVolumes.empty())
				{
					sceneRenderData.ProbeTexArrays->IrradianceMapArr.Bind(12);
					sceneRenderData.ProbeTexArrays->PrefilterMapArr.Bind(13);
					sceneRenderData.ProbeTexArrays->BRDFLut.Bind(14);
					VolumeRenderer(Impl.RenderHandle, &MainFramebuffer).Volumes(info, probeVolumes, sceneRenderData.ProbesLoaded == true);
				}
				
			}
			else
			{
				MainFramebuffer.Bind(viewport.GetSize());	// make sure the viewport for the main framebuffer stays at (0, 0)

				if (clearMainFB)
				{
					glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				}
			}
		}
		else
		{
			MainFramebuffer.Bind(viewport.GetSize());

			if (clearMainFB)
			{
				glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			}
			
			////////////////// Forward Shading
			info.SetMainPass(true);
			info.SetRequiredShaderInfo(MaterialShaderHint::Shaded);
			auto forwardTb = tbCollection.GetTb<ForwardShadingToolbox>();
			if (!forwardTb->LightShaders.empty())
			{
				Shader* lightShader = forwardTb->LightShaders[0];
				lightShader->Use();
				lightShader->UniformBlockBinding("Lights", sceneRenderData.LightsBuffer.BlockBindingSlot);
				sceneRenderData.UpdateLightUniforms();

				RawRender(info, *lightShader);
			}
		}
		
		////////////////////3.25 Render global cubemap
		info.SetMainPass(false);
		info.StopRequiringShaderInfo();
	
		if (sceneRenderData.ContainsLightProbes())
		{
			sceneRenderData.LightProbes.back()->GetEnvironmentMap().Bind(0);
			//sceneRenderData->ProbeTexArrays->PrefilterMapArr.Bind(0);
			//LightProbes[1]->EnvironmentMap.Bind(0);
			Impl.RenderHandle.FindShader("Cubemap")->Use();
			Impl.RenderHandle.FindShader("Cubemap")->Uniform1f("mipLevel", (float)std::fmod(glfwGetTime(), 4.0));


			Renderer(Impl.RenderHandle, &MainFramebuffer).StaticMeshInstances(info, { Impl.GetBasicShapeMesh(EngineBasicShape::CUBE) }, Transform(), *Impl.RenderHandle.FindShader("Cubemap"));
		}


		////////////////////3.5 Forward rendering pass
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		info.SetMainPass(true);
		
		if (modifyForwardsDepthForUI)
			RawUIScene(info);
		else
		{
			info.SetRequiredShaderInfo(MaterialShaderHint::Simple);
			RawRender(info, *Impl.RenderHandle.GetSimpleShader());
			for (auto shader : this->Impl.RenderHandle.GetCustomShaders())
			{
				info.SetRequiredShaderInfo(*shader);
				RawRender(info, *shader);
			}
		}
		
		Shader& defaultForwardShader = Impl.GetShader(RendererShaderHint::DefaultForward);
		defaultForwardShader.Use();
		defaultForwardShader.Uniform2fv("atlasData", Vec2f(0.0f));
		defaultForwardShader.Uniform2fv("atlasTexOffset", Vec2f(0.0f));


		info.SetMainPass(false);
		info.StopRequiringShaderInfo();
		
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

		if (renderIconsFunc)
			renderIconsFunc(MainFramebuffer);

		/*if (Component* selectedComponent = dynamic_cast<Editor::EditorManager*>()->GetSelectedComponent(); selectedComponent && selectedComponent->GetScene().GetRenderData() == sceneRenderData)
			OutlineRenderer(Impl.RenderHandle, info.GetContextID()).RenderOutlineSilhouette(info, dynamic_cast<Editor::EditorManager*>(GameHandle)->GetSelectedComponent(), MainFramebuffer);

		if (debugPhysics && GameHandle->GetMainScene()->GetRenderData() == sceneRenderData)
			GameHandle->GetPhysicsHandle()->DebugRender(*GameHandle->GetMainScene()->GetPhysicsData(), *this, info);*/

		////////////////////4. Postprocessing pass (Blur + Tonemapping & Gamma Correction)
		PostprocessRenderer(Impl.RenderHandle, target).Render(info.GetContextID(), tbCollection, &viewport, MainFramebuffer.GetColorTexture(0), (settings.bBloom) ? (MainFramebuffer.GetColorTexture(1)) : (Texture()), MainFramebuffer.GetAnyDepthAttachment(), (settings.IsVelocityBufferNeeded()) ? (MainFramebuffer.GetAttachment("velocityTex")) : (Texture()));

	//	PreviousFrameView = currentFrameView;										AAAAAAAAAAA SMAA NAPRAW
	}

	void SceneRenderer::RawRender(const SceneMatrixInfo& info, Shader& shader)
	{
		shader.Use();
		BindingsGL::BoundMesh = nullptr;
		BindingsGL::BoundMaterial = nullptr;

		for (auto renderable : info.GetSceneRenderData().Renderables)
		{
			if (info.GetOnlyShadowCasters() && !renderable->CastsShadow())
				continue;
			renderable->Render(info, &shader);
		}
	}

	void SceneRenderer::RawUIScene(const SceneMatrixInfo& infoTemplate)
	{
		SceneMatrixInfo info = infoTemplate;
		GameSceneRenderData& renderData = info.GetSceneRenderData();
		const float maxDepth = 2550.0f;

		BindingsGL::BoundMesh = nullptr;
		BindingsGL::BoundMaterial = nullptr;

		//std::stable_sort(sceneRenderData->Renderables.begin(), sceneRenderData->Renderables.end(), [](Renderable* lhs, Renderable* rhs) { return lhs->GetUIDepth() < rhs->GetUIDepth(); });
		renderData.AssertThatUIRenderablesAreSorted();

		glDepthFunc(GL_LEQUAL);
		auto forwardShaders = Impl.RenderHandle.GetCustomShaders();
		forwardShaders.insert(forwardShaders.begin(), Impl.RenderHandle.GetSimpleShader());
		for (auto& shader : forwardShaders)
		{
			if (shader == Impl.RenderHandle.GetSimpleShader())
				info.SetRequiredShaderInfo(MaterialShaderHint::Simple);
			else
				info.SetRequiredShaderInfo(*shader);

			shader->Use();
			info.SetView(infoTemplate.GetView());
			//Render from back to front (elements inserted last in the container probably have the biggest depth; early depth testing should work better using this method
			for (auto it = renderData.Renderables.rbegin(); it != renderData.Renderables.rend(); it++)
			{
				//float uiDepth = static_cast<float>(it->get().GetUIDepth());
				//info.view = (uiDepth !=	0.0f) ? (glm::translate(maxDepthMat, Vec3f(0.0f, 0.0f, -uiDepth))) : (maxDepthMat);
				info.SetView(glm::translate(info.GetView(), Vec3f(0.0f, 0.0f, 1.0f)));
				info.CalculateVP();
				(*it)->Render(info, shader);
			}
		}
	}

	//Implemented using ping-pong framebuffers (one gaussian blur pass is separable into two lower-cost passes) and linear filtering for performance
	Texture PostprocessRenderer::GaussianBlur(PPToolbox<GaussianBlurToolbox> ppTb,
		const Viewport* viewport, const Texture& tex, int passes,
		unsigned int writeColorBuffer)
	{
		GEE_CORE_ASSERT(Impl.OptionalFramebuffer != nullptr);
		const GEE_FB::Framebuffer& framebuffer = *Impl.OptionalFramebuffer;
		if (passes == 0)
			return tex;

		GaussianBlurToolbox& tb = ppTb.GetTb();

		tb.GaussianBlurShader->Use();
		glActiveTexture(GL_TEXTURE0);

		bool horizontal = true;

		for (int i = 0; i < passes; i++)
		{
			if (i == passes - 1)	//If the calling function specified the framebuffer to write the final result to, bind the given framebuffer for the last pass.
			{
				framebuffer.Bind();
				framebuffer.SetDrawSlot(writeColorBuffer);
			}
			else	//For any pass that doesn't match given criteria, just switch the ping pong fbos
			{
				tb.BlurFramebuffers[horizontal]->Bind();
				const Texture& bindTex = (i == 0) ? (tex) : static_cast<Texture>(tb.BlurFramebuffers[!horizontal]->GetColorTexture(0));
				bindTex.Bind();
			}
			tb.GaussianBlurShader->Uniform1i("horizontal", horizontal);
			ppTb.RenderFullscreenQuad(tb.GaussianBlurShader, false);

			horizontal = !horizontal;
		}

		glDrawBuffer(GL_COLOR_ATTACHMENT0);

		tb.BlurFramebuffers[!horizontal]->GetColorTexture(0);
		return static_cast<Texture>(framebuffer.GetColorTexture(writeColorBuffer));
	}

	Texture PostprocessRenderer::SSAO(SceneMatrixInfo& info, const Texture& gPosition, const Texture& gNormal)
	{
		SSAOToolbox* tb;
		if ((tb = info.GetTbCollection().GetTb<SSAOToolbox>()) == nullptr)
		{
			std::cerr << "ERROR! Attempted to render SSAO image without calling the setup\n";
			return Texture();
		}

		tb->SSAOFb->Bind();
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);


		gPosition.Bind(0);
		gNormal.Bind(1);
		tb->SSAONoiseTex->Bind(2);

		tb->SSAOShader->Use();
		tb->SSAOShader->UniformMatrix4fv("view", info.GetView());
		tb->SSAOShader->UniformMatrix4fv("projection", info.GetProjection());


		RenderFullscreenQuad(TbInfo<MatrixInfoExt>(info.GetTbCollection(), info), tb->SSAOShader, false);

		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);

		ToFramebuffer(*tb->SSAOFb).GaussianBlur(GetPPToolbox<GaussianBlurToolbox>(info.GetContextID(), info.GetTbCollection()), nullptr, tb->SSAOFb->GetColorTexture(0), 4);
		return tb->SSAOFb->GetColorTexture(0);
	}

	Texture PostprocessRenderer::SMAA(PPToolbox<SMAAToolbox> ppTb,
		const Viewport* viewport, const Texture& colorTex, const Texture& depthTex,
		const Texture& previousColorTex, const Texture& velocityTex,
		unsigned int writeColorBuffer, bool bT2x)
	{
		GEE_CORE_ASSERT(Impl.OptionalFramebuffer != nullptr);
		const GEE_FB::Framebuffer& framebuffer = *Impl.OptionalFramebuffer;
		SMAAToolbox& tb = ppTb.GetTb();
		tb.SMAAFb->Bind();

		/////////////////1. Edge detection
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		tb.SMAAShaders[0]->Use();
		colorTex.Bind(0);
		depthTex.Bind(1);

		ppTb.RenderFullscreenQuad(tb.SMAAShaders[0]);

		///////////////2. Weight blending
		glDrawBuffer(GL_COLOR_ATTACHMENT1);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		tb.SMAAShaders[1]->Use();
		if (bT2x)
			;////////////////////////////// tb.SMAAShaders[1]->Uniform4fv("ssIndices", (FrameIndex == 0) ? (Vec4f(1, 1, 1, 0)) : (Vec4f(2, 2, 2, 0))); QAAAAAAAAAAAAAAA NAPRAW TO

		tb.SMAAFb->GetColorTexture(0).Bind(0);	//bind edges texture to slot 0
		tb.SMAAAreaTex->Bind(1);
		tb.SMAASearchTex->Bind(2);

		ppTb.RenderFullscreenQuad(tb.SMAAShaders[1]);

		///////////////3. Neighborhood blending
		if (bT2x)
		{
			glDrawBuffer(GL_COLOR_ATTACHMENT2);
		}
		else
		{
			if (viewport) framebuffer.Bind(*viewport);
			else framebuffer.Bind(true);
			framebuffer.SetDrawSlot(writeColorBuffer);
		}

		//glEnable(GL_FRAMEBUFFER_SRGB);

		tb.SMAAShaders[2]->Use();

		colorTex.Bind(0);
		tb.SMAAFb->GetColorTexture(1).Bind(1); //bind weights texture to slot 0

		velocityTex.Bind(2);

		ppTb.RenderFullscreenQuad(tb.SMAAShaders[2]);
		//glDisable(GL_FRAMEBUFFER_SRGB);

		if (!bT2x)
			return Texture();
		if (!velocityTex.HasBeenGenerated())
			return Texture();

		///////////////4. Temporal resolve (optional)
		if (viewport) framebuffer.Bind(*viewport);
		else framebuffer.Bind(true);
		framebuffer.SetDrawSlot(writeColorBuffer);

		tb.SMAAShaders[3]->Use();

		tb.SMAAFb->GetColorTexture(2).Bind(0);
		previousColorTex.Bind(1);
		velocityTex.Bind(2);

		ppTb.RenderFullscreenQuad(tb.SMAAShaders[3]);

		glDrawBuffer(GL_COLOR_ATTACHMENT0);

		return framebuffer.GetColorTexture(writeColorBuffer);
	}

	Texture PostprocessRenderer::TonemapGamma(PPToolbox<ComposedImageStorageToolbox> tb, const Viewport* viewport, const Texture& colorTex, const Texture& blurTex)
	{
		GEE_CORE_ASSERT(Impl.OptionalFramebuffer != nullptr);
		const GEE_FB::Framebuffer& framebuffer = *Impl.OptionalFramebuffer;
		if (viewport) framebuffer.Bind(*viewport);
		else framebuffer.Bind(true);
		framebuffer.SetDrawSlot(0);

		glDisable(GL_DEPTH_TEST);
		//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		//glClear(GL_COLOR_BUFFER_BIT);

		tb.GetTb().TonemapGammaShader->Use();

		colorTex.Bind(0);
		if (blurTex.HasBeenGenerated())
			blurTex.Bind(1);
		else
			Texture().Bind(1);
		tb.GetTb().TonemapGammaShader->Uniform1i("HDRbuffer", 0);

		tb.RenderFullscreenQuad(tb.GetTb().TonemapGammaShader);

		return framebuffer.GetColorTexture(0);
	}

	void PostprocessRenderer::Render(RenderingContextID contextID, RenderToolboxCollection& tbCollection, const Viewport* viewport, const Texture& colorTex, Texture blurTex, const Texture& depthTex, const Texture& velocityTex)
	{
		GEE_CORE_ASSERT(Impl.OptionalFramebuffer != nullptr);
		const GEE_FB::Framebuffer& finalFramebuffer = *Impl.OptionalFramebuffer;
		const GameSettings::VideoSettings& settings = tbCollection.GetSettings();


		if (settings.bBloom && blurTex.HasBeenGenerated())
			blurTex = ToFramebuffer(*tbCollection.GetTb<GaussianBlurToolbox>()->BlurFramebuffers[0]).GaussianBlur(GetPPToolbox<GaussianBlurToolbox>(contextID, tbCollection), nullptr, blurTex, 10);


		PostprocessRenderer composedImageRenderer = ToFramebuffer(*tbCollection.GetTb<ComposedImageStorageToolbox>()->ComposedImageFb);

		if (settings.AAType == AntiAliasingType::AA_SMAA1X)
		{
			const Texture& compositedTex = composedImageRenderer.TonemapGamma(GetPPToolbox<ComposedImageStorageToolbox>(contextID, tbCollection), nullptr, colorTex, blurTex);
			SMAA(GetPPToolbox<SMAAToolbox>(contextID, tbCollection), viewport, compositedTex, depthTex);
		}
		else if (settings.AAType == AntiAliasingType::AA_SMAAT2X && velocityTex.HasBeenGenerated())
		{
			const auto storageTb = tbCollection.GetTb<PrevFrameStorageToolbox>();
			const Texture& compositedTex = composedImageRenderer.TonemapGamma(GetPPToolbox<ComposedImageStorageToolbox>(contextID, tbCollection), nullptr, colorTex, blurTex);
			const unsigned int previousFrameIndex = 0;// (static_cast<int>(FrameIndex) - 1) % storageTb->StorageFb->GetColorTextureCount();
			const Texture& SMAAresult = ToFramebuffer(*storageTb->StorageFb).SMAA(GetPPToolbox<SMAAToolbox>(contextID, tbCollection), nullptr, compositedTex, depthTex, storageTb->StorageFb->GetColorTexture(previousFrameIndex), velocityTex, /*FrameIndex*/0, true);

			if (viewport) finalFramebuffer.Bind(*viewport);
			else finalFramebuffer.Bind(true);

			finalFramebuffer.SetDrawSlot(0);
			glEnable(GL_FRAMEBUFFER_SRGB);
			Impl.GetShader(RendererShaderHint::Quad).Use();

			SMAAresult.Bind(0);

			RenderFullscreenQuad(contextID, tbCollection);
			glDisable(GL_FRAMEBUFFER_SRGB);

			//FrameIndex = (FrameIndex + 1) % storageTb->StorageFb->GetColorTextureCount();
		}
		else
			TonemapGamma(GetPPToolbox<ComposedImageStorageToolbox>(contextID, tbCollection), viewport, colorTex, blurTex);
	}
	void PostprocessRenderer::RenderFullscreenQuad(RenderingContextID contextID, RenderToolboxCollection& tbCollection, Shader* shader, bool useShader)
	{
		RenderFullscreenQuad(TbInfo<MatrixInfoExt>(tbCollection, MatrixInfoExt(contextID)), shader, useShader);
	}
	void PostprocessRenderer::RenderFullscreenQuad(TbInfo<MatrixInfoExt>& info, Shader* shader, bool useShader)
	{
		if (!shader) shader = Impl.RenderHandle.FindShader("Quad");
		if (useShader)	shader->Use();
		StaticMeshInstances(info, { MeshInstance(Impl.GetBasicShapeMesh(EngineBasicShape::QUAD)) }, Transform(), *shader);
	}
	void LightProbeRenderer::AllSceneProbes(RenderingContextID contextID, GameSceneRenderData& sceneRenderData)
	{
		if (sceneRenderData.LightProbes.empty())
		{
			std::cout << "INFO: No light probes in scene " << sceneRenderData.GetSceneName() << ". Nothing will be rendered.\n";
			return;
		}

		GameSettings::VideoSettings settings;
		settings.AAType = AA_NONE;	//Disable SMAA; it doesn't work on HDR colorbuffers
		settings.AALevel = SettingLevel::SETTING_ULTRA;
		settings.bBloom = false;
		settings.AmbientOcclusionSamples = 64;
		settings.POMLevel = SettingLevel::SETTING_ULTRA;
		settings.ShadowLevel = SettingLevel::SETTING_ULTRA;
		settings.Resolution = Vec2u(1024);
		settings.Shading = ShadingAlgorithm::SHADING_PBR_COOK_TORRANCE;
		settings.MonitorGamma = 1.0f;	//Keep radiance data in linear space
		settings.TMType = ToneMappingType::TM_NONE;	//Do not tonemap when rendering probes; we want to keep HDR data

		std::cout << "Probe rendering for " << sceneRenderData.LightProbes.size() << " probes.\n";

		RenderToolboxCollection probeRenderingCollection("LightProbeRendering", settings, *sceneRenderData.GetRenderHandle());
		probeRenderingCollection.AddTbsRequiredBySettings();

		GEE_FB::Framebuffer framebuffer;
		framebuffer.Generate();
		framebuffer.Bind();
		Viewport(Vec2u(1024)).SetOpenGLState();

		Shader* gShader = probeRenderingCollection.GetTb<DeferredShadingToolbox>()->GeometryShader;

		std::cout << "*!* Przed cieniami\n";
		SceneRenderer(Impl.RenderHandle, &framebuffer).PreFramePass(probeRenderingCollection, sceneRenderData);
		std::cout << "*!* Po cieniach\n";
		for (LightProbeComponent* probe : sceneRenderData.LightProbes)
		{
			if (probe->GetShape() == EngineBasicShape::QUAD)
				continue;
			Vec3f camPos = probe->GetTransform().GetWorldTransform().GetPos();
			Mat4f viewTranslation = glm::translate(Mat4f(1.0f), -camPos);
			Mat4f p = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);

			SceneMatrixInfo info(contextID, probeRenderingCollection, sceneRenderData, Mat4f(1.0f), Mat4f(1.0f), camPos);
			info.SetView(viewTranslation);
			info.SetProjection(p);
			gShader->Use();
			std::cout << "*!* Przed env map\n";
			CubemapRenderer(Impl.RenderHandle, &framebuffer).FromScene(info, framebuffer, GEE_FB::FramebufferAttachment(probe->GetEnvironmentMap(), GEE_FB::AttachmentSlot::Color(0)), gShader, 0, true);
			std::cout << "*!* Po env map\n";
			probe->GetEnvironmentMap().Bind();
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
			std::cout << "*!* Przed convolute\n";
			LightProbeLoader::ConvoluteLightProbe(*probe, probe->GetEnvironmentMap());
			std::cout << "*!* Po convolute\n";
			if (PrimitiveDebugger::bDebugProbeLoading)
				printVector(camPos, "Rendering probe");
		}

		//Dispose();

		//Init(Vec2u(GameHandle->GetGameSettings()->ViewportData.z, GameHandle->GetGameSettings()->ViewportData.w));
		probeRenderingCollection.Dispose();
		framebuffer.Dispose();

		std::cout << "Initting done.\n";
	}
	void SkeletalMeshRenderer::SkeletalMeshInstances(const MatrixInfoExt& info, const std::vector<MeshInstance>& meshes, SkeletonInfo& skelInfo, const Transform& transform, Shader& shader)
	{
		if (meshes.empty())
			return;

		bool handledShader = false;
		for (const MeshInstance& meshInst : meshes)
		{
			const Mesh& mesh = meshInst.GetMesh();
			MaterialInstance* materialInst = meshInst.GetMaterialInst();
			const Material* material = meshInst.GetMaterialPtr().get();

			if ((material && material->GetShaderInfo().MatchesRequiredInfo(info.GetRequiredShaderInfo())) ||
				(info.GetOnlyShadowCasters() && !mesh.CanCastShadow()) ||
				!materialInst->ShouldBeDrawn())
				continue;

			if (!handledShader)
			{
				handledShader = true;

				shader.Uniform1i("boneIDOffset", skelInfo.GetBoneIDOffset());

				Mat4f modelMat = transform.GetWorldTransformMatrix();	//the ComponentTransform's world transform is cached

				shader.BindMatrices(modelMat, &info.GetView(), &info.GetProjection(), &info.GetVP());
				shader.CallPreRenderFunc();
			}
			if (skelInfo.GetBatchPtr() !=  Impl.RenderHandle.GetBoundSkeletonBatch())
				Impl.RenderHandle.BindSkeletonBatch(skelInfo.GetBatchPtr());

			shader.Uniform1i("boneIDOffset", skelInfo.GetBoneIDOffset());

			//if (BindingsGL::BoundMesh != &mesh || i == 0)
			{
				mesh.Bind(info.GetContextID());
				BindingsGL::BoundMesh = &mesh;
			}

			if (info.GetUseMaterials() && material)
			{
			//	if (BindingsGL::BoundMaterial != material) //jesli zbindowany jest inny material niz potrzebny obecnie, musimy zmienic go w shaderze
				{
					materialInst->UpdateWholeUBOData(&shader, Texture());
					BindingsGL::BoundMaterial = material;
				}
			//	else if (BoundMaterial) //jesli ostatni zbindowany material jest taki sam, to nie musimy zmieniac wszystkich danych w shaderze; oszczedzmy sobie roboty
			//		materialInst->UpdateInstanceUBOData(shader);
			}

			mesh.Render();
		}
	}
	void PhysicsDebugRenderer::BoundSceneDebug(SceneMatrixInfo& info, GLenum mode, GLint first, GLint count, Vec3f color)
	{
		Shader* debugShader = Impl.RenderHandle.FindShader("Debug");
		debugShader->Use();
		debugShader->UniformMatrix4fv("MVP", info.GetVP());
		debugShader->Uniform3fv("color", color);
		glDrawArrays(mode, first, count);
	}
}