#include <editor/EditorRenderer.h>
#include <editor/EditorActions.h>
#include <rendering/RenderToolbox.h>
#include <rendering/RenderInfo.h>
#include <rendering/Material.h>
#include <rendering/OutlineRenderer.h>
#include <game/GameScene.h>
#include <scene/CameraComponent.h>

/*
namespace GEE
{
	EditorRenderer::EditorRenderer(Editor::EditorManager& editorHandle, RenderToolboxCollection& mainSceneCol, RenderToolboxCollection& uiCol, SystemWindow& mainWindow):
		Renderer(*editorHandle.GetGameHandle()->GetRenderEngineHandle(), nullptr),
		MainWindow(&mainWindow),
		EditorHandle(editorHandle),
		MainSceneCollection(&mainSceneCol),
		UICollection(&uiCol)
	{
	}
	void EditorRenderer::FullRender(GameScene* mainScene, GameScene* editorScene, GameScene* menuScene)
	{
		//////////////////////POPUPS
		OpenPopups.erase(std::remove_if(OpenPopups.begin(), OpenPopups.end(), [](Popup& popup) { if (glfwWindowShouldClose(&popup.Window.get())) { glfwDestroyWindow(&popup.Window.get()); return true; } return false; }), OpenPopups.end());

		////////////////////////MAIN WINDOW
		if ((mainScene && !mainScene->GetActiveCamera()) || (editorScene && !editorScene->GetActiveCamera()))
		{
			if (auto camActor = mainScene->FindActor("CameraActor"))
				if (auto comp = camActor->GetRoot()->GetComponent<CameraComponent>("Camera"))
					mainScene->BindActiveCamera(comp);
			if (auto camActor = editorScene->FindActor("OrthoCameraActor"))
				if (auto comp = camActor->GetRoot()->GetComponent<CameraComponent>("OrthoCameraComp"))
					editorScene->BindActiveCamera(comp);
		}

		RenderEng.PrepareFrame();

		if (mainScene && mainScene->GetActiveCamera())
		{
			//Render 3D scene
			RenderEng.PrepareScene(0, *MainSceneCollection, mainScene->GetRenderData());
			SceneMatrixInfo mainSceneRenderInfo = mainScene->GetActiveCamera()->GetRenderInfo(0, *MainSceneCollection);


			// Main render
			if (!GetInputRetriever().IsKeyPressed(Key::F3))
			{
				SceneRenderer(RenderEng, &MainSceneCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer()).FullRender(mainSceneRenderInfo, Viewport(Vec2f(0.0f)), true, false,
					[&](GEE_FB::Framebuffer& mainFramebuffer) {
						if (bDebugRenderComponents)
						{
							mainFramebuffer.SetDrawSlot(0);
							RenderEng.GetSimpleShader()->Use();
							mainScene->GetRootActor()->DebugRenderAll(mainScene->GetActiveCamera()->GetRenderInfo(0, *MainSceneCollection), RenderEng.GetSimpleShader());
							mainFramebuffer.SetDrawSlots();
						}
						if (Component* selectedComponent = EditorHandle.GetActions().GetSelectedComponent(); selectedComponent && selectedComponent->GetScene().GetRenderData() == mainScene->GetRenderData())
							OutlineRenderer(*EditorHandle.GetGameHandle()->GetRenderEngineHandle()).RenderOutlineSilhouette(mainSceneRenderInfo, EditorHandle.GetActions().GetSelectedComponent(), mainFramebuffer);

						if (GetInputRetriever().IsKeyPressed(Key::F2))
							PhysicsEng.DebugRender(*mainScene->GetPhysicsData(), RenderEng, mainSceneRenderInfo);

					}, CheckEEForceForwardShading());

			}
			else // Only debug render physics engine
			{
				auto fb = &MainSceneCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer();
				fb->Bind(true);
				fb->GetColorTexture(0).Bind(0);
				glClear(GL_COLOR_BUFFER_BIT);

				PhysicsEng.DebugRender(*mainScene->GetPhysicsData(), RenderEng, mainSceneRenderInfo);
				Viewport(Vec2f(0.0f), Vec2f(Settings->WindowSize)).SetOpenGLState();
			}
			if (EditorHandle.GetActions().GetSelectedActor() && !GetInputRetriever().IsKeyPressed(Key::F2))
			{
				std::function<void(Component&)> renderAllColObjs = [this, &renderAllColObjs, mainScene](Component& comp) {
					if (comp.GetCollisionObj() && comp.GetName() != "Counter")
						CollisionObjRendering(mainScene->GetActiveCamera()->GetRenderInfo(0, *MainSceneCollection), *EditorHandle.GetGameHandle(), *comp.GetCollisionObj(), comp.GetTransform().GetWorldTransform(), (EditorHandle.GetActions().GetSelectedComponent() && EditorHandle.GetActions().GetSelectedComponent() == &comp) ? (Vec3f(0.1f, 0.6f, 0.3f)) : (Vec3f(0.063f, 0.325f, 0.071f)));

					for (auto& child : comp.GetChildren())
						renderAllColObjs(*child);
				};

				renderAllColObjs(*EditorHandle.GetActions().GetSelectedActor()->GetRoot());
			}

			//RenderEng.RenderText(RenderInfo(*MainSceneCollection), *GetDefaultFont(), "(C) twoja babka studios", Transform(Vec3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f), Vec3f(32.0f)), glm::pow(Vec3f(0.0f, 0.73f, 0.84f), Vec3f(1.0f / 2.2f)), nullptr, true);
		}
		else if (mainScene)
		{
			MainSceneCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer().Bind(true);
			glClear(GL_COLOR_BUFFER_BIT);

			RenderEng.GetSimpleShader()->Use();
			Renderer(RenderEng).StaticMeshInstances(SceneMatrixInfo(*MainSceneCollection, *mainScene->GetRenderData()), { MeshInstance(RenderEng.GetBasicShapeMesh(EngineBasicShape::QUAD), RenderEng.FindMaterial("GEE_No_Camera_Icon").get()) }, Transform(), *RenderEng.GetSimpleShader());

			RenderEng.RenderText(SceneMatrixInfo(*MainSceneCollection, *mainScene->GetRenderData()), *EditorHandle.GetGameHandle()->GetDefaultFont(), "No camera", Transform(Vec2f(0.0f, -0.8f), Vec2f(0.1f)), Vec3f(1.0f, 0.0f, 0.0f), nullptr, false, std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER));

		}

		bool renderEditorScene = editorScene && editorScene->GetActiveCamera();
		if (renderEditorScene)
		{
			///Render editor scene

			if (bViewportMaximzized)
			{
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				Vec2i windowSize;
				glfwGetWindowSize(MainWindow, &windowSize.x, &windowSize.y);
				glViewport(0, 0, windowSize.x, windowSize.y);
				RenderEng.GetSimpleShader()->Use();
				RenderEng.GetSimpleShader()->Uniform2fv("atlasData", Vec2f(0.0f));
				Renderer(RenderEng).StaticMeshInstances(MatrixInfoExt(), { MeshInstance(RenderEng.GetBasicShapeMesh(EngineBasicShape::QUAD), RenderEng.FindMaterial("GEE_3D_SCENE_PREVIEW_MATERIAL").get()) }, EditorScene->FindActor("SceneViewportActor")->GetTransform()->GetWorldTransform(), *RenderEng.GetSimpleShader());
			}
			else
			{
				SceneMatrixInfo info = editorScene->GetActiveCamera()->GetRenderInfo(0, *UICollection);

				RenderEng.PrepareScene(0, *UICollection, editorScene->GetRenderData());
				SceneRenderer(RenderEng).FullRender(info, Viewport(Vec2f(0.0f), Vec2f(Settings->WindowSize)), true, true, nullptr);
			}
		}

		if (menuScene)
			if (menuScene->GetActiveCamera())
			{
				SceneMatrixInfo info = menuScene->GetActiveCamera()->GetRenderInfo(0, *UICollection);
				RenderEng.PrepareScene(0, *UICollection, menuScene->GetRenderData());
				SceneRenderer(RenderEng).FullRender(info, Viewport(Vec2f(0.0f), Vec2f(Settings->WindowSize)), !renderEditorScene, true);
			}

		/*if (GameScene* meshPreviewScene = GetScene("GEE_Mesh_Preview_Scene"))
			if (meshPreviewScene->GetActiveCamera())
			{
				RenderToolboxCollection& renderTbCollection = **std::find_if(RenderEng.RenderTbCollections.begin(), RenderEng.RenderTbCollections.end(), [](const UniquePtr<RenderToolboxCollection>& tbCol) { return tbCol->GetName() == "GEE_E_Mesh_Preview_Toolbox_Collection"; });
				SceneMatrixInfo info = meshPreviewScene->ActiveCamera->GetRenderInfo(0, renderTbCollection);
				RenderEng.PrepareScene(0, renderTbCollection, meshPreviewScene->GetRenderData());
				SceneRenderer(RenderEng, &renderTbCollection.GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer()).FullRender(info);
			}* /

		glfwSwapBuffers(MainWindow);

		for (auto& popup : OpenPopups)
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
			info.SetRequiredShaderHint(MaterialShaderHint::Simple);
			Material mat("");
			mat.SetColor(Vec4f(1.0f, 0.0f, 0.2f, 1.0f));

			RenderEng.GetSimpleShader()->Use();
			mat.UpdateWholeUBOData(RenderEng.GetSimpleShader(), RenderEng.GetEmptyTexture());

			RenderEng.GetBasicShapeMesh(EngineBasicShape::QUAD).Bind(popup.ID);
			SceneRenderer(RenderEng).RawUIScene(info);

			glfwSwapBuffers(&popup.Window.get());
		}
		glfwMakeContextCurrent(MainWindow);
	}
}*/