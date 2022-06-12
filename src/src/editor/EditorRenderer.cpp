#include <editor/EditorRenderer.h>
#include <editor/EditorActions.h>
#include <rendering/RenderToolbox.h>
#include <rendering/RenderInfo.h>
#include <rendering/Material.h>
#include <rendering/OutlineRenderer.h>
#include <game/GameScene.h>
#include <scene/CameraComponent.h>
#include <input/InputDevicesStateRetriever.h>
#include <game/Game.h>


namespace GEE
{
	namespace Editor
	{
		EditorRenderer::EditorRenderer(Editor::EditorManager& editorHandle, RenderToolboxCollection& mainSceneCol, RenderToolboxCollection& uiCol, SystemWindow& mainWindow) :
			GameRenderer(*editorHandle.GetGameHandle()->GetRenderEngineHandle(), nullptr),
			MainWindow(&mainWindow),
			EditorHandle(editorHandle),
			MainSceneCollection(&mainSceneCol),
			UICollection(&uiCol)
		{
		}
		void EditorRenderer::RenderPass(GameScene* mainScene, GameScene* editorScene, GameScene* menuScene, GameScene* meshPreviewScene)
		{
			// Bind cameras
			if ((mainScene && !mainScene->GetActiveCamera()) || (editorScene && !editorScene->GetActiveCamera()))
			{
				if (auto camActor = mainScene->FindActor("CameraActor"))
					if (auto comp = camActor->GetRoot()->GetComponent<CameraComponent>("Camera"))
						mainScene->BindActiveCamera(comp);
				if (auto camActor = editorScene->FindActor("OrthoCameraActor"))
					if (auto comp = camActor->GetRoot()->GetComponent<CameraComponent>("OrthoCameraComp"))
						editorScene->BindActiveCamera(comp);
			}

			PrepareFrame(mainScene);

			if (mainScene) RenderMainScene(mainScene);
			if (editorScene && editorScene->GetActiveCamera()) RenderEditorScene(editorScene);
			else if (menuScene && menuScene->GetActiveCamera()) RenderMainMenu(menuScene);

			if (RenderToolboxCollection* tbCol = Impl.RenderHandle.FindTbCollection("GEE_E_Mesh_Preview_Toolbox_Collection"); meshPreviewScene && meshPreviewScene->GetActiveCamera() && tbCol) RenderMeshPreview(meshPreviewScene, *tbCol);

			glfwSwapBuffers(MainWindow);

			RenderPopups();
		}
		void EditorRenderer::RenderMainScene(GameScene* mainScene)
		{
			if (mainScene->GetActiveCamera())
			{
				//Render 3D scene
				EnsureShadowMapCorectness(RenderingContextID(0), *MainSceneCollection, mainScene->GetRenderData());
				SceneMatrixInfo mainSceneRenderInfo = mainScene->GetActiveCamera()->GetRenderInfo(0, *MainSceneCollection);


				// Main render
				if (!EditorHandle.GetGameHandle()->GetInputRetriever().IsKeyPressed(Key::F3))
				{
					SceneRenderer(Impl.RenderHandle, &MainSceneCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer()).FullRender(mainSceneRenderInfo, Viewport(Vec2f(0.0f)), true, false,
						[&](GEE_FB::Framebuffer& mainFramebuffer) {
							if (EditorHandle.GetDebugRenderComponents())	// Render debug icons
							{
								mainFramebuffer.SetDrawSlot(0);
								Impl.RenderHandle.GetSimpleShader()->Use();
								mainScene->GetRootActor()->DebugRenderAll(mainScene->GetActiveCamera()->GetRenderInfo(0, *MainSceneCollection), Impl.RenderHandle.GetSimpleShader());
								mainFramebuffer.SetDrawSlots();
							}
							if (Component* selectedComponent = EditorHandle.GetActions().GetSelectedComponent(); selectedComponent && selectedComponent->GetScene().GetRenderData() == mainScene->GetRenderData())	// Render selected component outline and gizmos
							{
								OutlineRenderer(*EditorHandle.GetGameHandle()->GetRenderEngineHandle()).RenderOutlineSilhouette(mainSceneRenderInfo, EditorHandle.GetActions().GetSelectedComponent(), mainFramebuffer);

								// Gizmos
								Impl.RenderHandle.GetBasicShapeMesh(EngineBasicShape::Cone);
							}

							if (EditorHandle.GetGameHandle()->GetInputRetriever().IsKeyPressed(Key::F2))
								PhysicsDebugRenderer(Impl).DebugRender(*mainScene->GetPhysicsData(), mainSceneRenderInfo);



						}, EditorHandle.GetGameHandle()->CheckForceForwardShading());

				}
				else // Only debug render physics engine
				{
					auto fb = &MainSceneCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer();
					fb->Bind(true);
					fb->GetColorTexture(0).Bind(0);
					glClear(GL_COLOR_BUFFER_BIT);


					PhysicsDebugRenderer(Impl).DebugRender(*mainScene->GetPhysicsData(), mainSceneRenderInfo);
					Viewport(Vec2f(0.0f), Vec2f(UICollection->GetSettings().Resolution)).SetOpenGLState();
				}
				if (EditorHandle.GetDebugRenderPhysicsMeshes() && EditorHandle.GetActions().GetSelectedActor() && !EditorHandle.GetGameHandle()->GetInputRetriever().IsKeyPressed(Key::F2))
				{
					std::function<void(Component&)> renderAllColObjs = [this, &renderAllColObjs, mainScene](Component& comp) {
						if (comp.GetCollisionObj())
							CollisionObjRendering(mainScene->GetActiveCamera()->GetRenderInfo(0, *MainSceneCollection), *EditorHandle.GetGameHandle(), *comp.GetCollisionObj(), comp.GetTransform().GetWorldTransform(), (EditorHandle.GetActions().GetSelectedComponent() && EditorHandle.GetActions().GetSelectedComponent() == &comp) ? (Vec3f(0.1f, 0.6f, 0.3f)) : (Vec3f(0.063f, 0.325f, 0.071f)));

						for (auto& child : comp.GetChildren())
							renderAllColObjs(*child);
					};

					renderAllColObjs(*EditorHandle.GetActions().GetSelectedActor()->GetRoot());
				}
			}
			else
			{
				MainSceneCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer().Bind(true);
				glClear(GL_COLOR_BUFFER_BIT);

				Impl.RenderHandle.GetSimpleShader()->Use();
				Renderer(Impl.RenderHandle).StaticMeshInstances(SceneMatrixInfo(*MainSceneCollection, *mainScene->GetRenderData()), { MeshInstance(Impl.RenderHandle.GetBasicShapeMesh(EngineBasicShape::Quad), Impl.RenderHandle.FindMaterial("GEE_No_Camera_Icon")) }, Transform(), *Impl.RenderHandle.GetSimpleShader());

				TextRenderer(Impl.RenderHandle).RenderText(SceneMatrixInfo(*MainSceneCollection, *mainScene->GetRenderData()), *EditorHandle.GetGameHandle()->GetDefaultFont(), "No camera", Transform(Vec2f(0.0f, -0.8f), Vec2f(0.1f)), Vec3f(1.0f, 0.0f, 0.0f), nullptr, false, Alignment2D::Center());

			}
		}
		void EditorRenderer::RenderEditorScene(GameScene* editorScene)
		{

			if (EditorHandle.GetViewportMaximized())
			{
				glBindFramebuffer(GL_FRAMEBUFFER, 0);

				Impl.RenderHandle.GetSimpleShader()->Use();
				Impl.RenderHandle.GetSimpleShader()->Uniform2fv("atlasData", Vec2f(0.0f));
				Renderer(Impl.RenderHandle).StaticMeshInstances(MatrixInfoExt(), { MeshInstance(Impl.RenderHandle.GetBasicShapeMesh(EngineBasicShape::Quad), Impl.RenderHandle.FindMaterial("GEE_3D_SCENE_PREVIEW_MATERIAL")) }, editorScene->FindActor("SceneViewportActor")->GetTransform()->GetWorldTransform(), *Impl.RenderHandle.GetSimpleShader());
			}
			else
			{
				SceneMatrixInfo info = editorScene->GetActiveCamera()->GetRenderInfo(0, *UICollection);
				SceneRenderer(Impl.RenderHandle).FullRender(info, Viewport(Vec2f(0.0f), Vec2f(UICollection->GetSettings().Resolution)), true, true, nullptr);
			}
			
		}
		void EditorRenderer::RenderMainMenu(GameScene* menuScene)
		{
			SceneMatrixInfo info = menuScene->GetActiveCamera()->GetRenderInfo(0, *UICollection);
			SceneRenderer(Impl.RenderHandle).FullRender(info, Viewport(Vec2f(0.0f), Vec2f(UICollection->GetSettings().Resolution)), true, true);
		}
		void EditorRenderer::RenderMeshPreview(GameScene* meshPreviewScene, RenderToolboxCollection& renderTbCollection)
		{
			SceneMatrixInfo info = meshPreviewScene->GetActiveCamera()->GetRenderInfo(0, renderTbCollection);
			EnsureShadowMapCorectness(0, renderTbCollection, meshPreviewScene->GetRenderData());
			SceneRenderer(Impl.RenderHandle, &renderTbCollection.GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer()).FullRender(info);
		}
		void EditorRenderer::RenderPopups()
		{
			for (auto& popup : EditorHandle.GetPopupsForRendering())
			{
				glfwMakeContextCurrent(&popup.Window.get());
				glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
				glDebugMessageCallbackARB(DebugCallbacks::OpenGLDebug, nullptr);
				glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

				glEnable(GL_DEPTH_TEST);
				glDisable(GL_CULL_FACE);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glViewport(0, 0, 400, 400);
				glDrawBuffer(GL_BACK);
				glClearColor(glm::abs(glm::cos((EditorHandle.GetGameHandle()->GetProgramRuntime()))), 0.3f, 0.8f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				SceneMatrixInfo info(popup.ID, *MainSceneCollection, *popup.Scene.get().GetRenderData(), Mat4f(1.0f), glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -2550.0f), Vec3f(0.0f));
				info.SetRequiredShaderInfo(MaterialShaderHint::Simple);
				Material mat("");
				mat.SetColor(Vec4f(1.0f, 0.0f, 0.2f, 1.0f));

				Impl.RenderHandle.GetSimpleShader()->Use();
				mat.UpdateWholeUBOData(Impl.RenderHandle.GetSimpleShader(), Impl.RenderHandle.GetEmptyTexture());

				Impl.RenderHandle.GetBasicShapeMesh(EngineBasicShape::Quad).Bind(popup.ID);
				SceneRenderer(Impl.RenderHandle).RawUIRender(info);

				glfwSwapBuffers(&popup.Window.get());
			}

			glfwMakeContextCurrent(MainWindow);
		}
	}
}