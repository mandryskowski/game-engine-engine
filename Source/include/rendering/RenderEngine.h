#pragma once
#include "Postprocess.h"
#include <rendering/Renderer.h>
#include "RenderToolbox.h"
#include <input/Event.h>

namespace GEE
{
	class LightProbe;
	struct LightProbeTextureArrays;
	class LocalLightProbe;
	class RenderableVolume;
	class BoneComponent;
	class TextComponent;
	class SkeletonBatch;

	class RenderEngine : public RenderEngineManager
	{
	public:
		RenderEngine(GameManager*);
		void Init(Vec2u resolution);

		virtual const ShadingAlgorithm& GetShadingModel() override;
		virtual Mesh& GetBasicShapeMesh(EngineBasicShape) override;
		virtual Shader* GetLightShader(const RenderToolboxCollection& renderCol, LightType type) override;
		virtual RenderToolboxCollection* GetCurrentTbCollection() override;
		virtual Texture GetEmptyTexture() override;
		virtual SkeletonBatch* GetBoundSkeletonBatch() override;

		virtual std::vector<Shader*> GetForwardShaders() override;
		virtual std::vector<Material*> GetMaterials() override;

		virtual void SetBoundMaterial(Material*) override;

		/**
		 * @brief Add a new RenderToolboxCollection to the render engine to enable updating shadow maps automatically. By default, we load every toolbox that will be needed according to RenderToolboxCollection::Settings
		 * @param tbCollection: a constructed RenderToolboxCollection object containing the name and settings of it
		 * @param setupToolboxesAccordingToSettings: pass false as the second argument to disable loading any toolboxes
		 * @return a reference to the added RenderToolboxCollection
		*/
		virtual RenderToolboxCollection& AddRenderTbCollection(const RenderToolboxCollection& tbCollection, bool setupToolboxesAccordingToSettings = true) override;
		virtual Material* AddMaterial(SharedPtr<Material> material) override;
		virtual SharedPtr<Shader> AddShader(SharedPtr<Shader> shader, bool bForwardShader = false) override;

		virtual void BindSkeletonBatch(SkeletonBatch* batch) override;
		virtual void BindSkeletonBatch(GameSceneRenderData* sceneRenderData, unsigned int index) override;

		virtual void EraseRenderTbCollection(RenderToolboxCollection& tbCollection) override;
		virtual void EraseMaterial(Material&) override;

		virtual SharedPtr<Material> FindMaterial(std::string) override;
		virtual Shader* FindShader(std::string) override;

		void RenderBoundInDebug(RenderInfo&, GLenum mode, GLint first, GLint count, Vec3f color = Vec3f(1.0f));

		void PrepareScene(RenderingContextID contextID, RenderToolboxCollection& tbCollection, GameSceneRenderData* sceneRenderData);	//Call this method once per frame for each scene that will be rendered in order to prepare stuff like shadow maps

		void PrepareFrame();

		void TestRenderCubemap(RenderInfo& info, GameSceneRenderData* sceneRenderData);
		
		virtual void RenderText(const SceneMatrixInfo& info, const Font& font, std::string content, Transform t = Transform(), Vec3f color = Vec3f(1.0f), Shader* shader = nullptr, bool convertFromPx = false, const std::pair<TextAlignment, TextAlignment> & = std::pair<TextAlignment, TextAlignment>(TextAlignment::LEFT, TextAlignment::BOTTOM)) override; //Pass a shader if you do not want the default shader to be used.
		//virtual void RenderStaticMesh(const RenderInfo& info, const MeshInstance& mesh, const Transform& transform, Shader* shader, Mat4f* lastFrameMVP = nullptr, Material* overrideMaterial = nullptr, bool billboard = false) override; //Note: this function does not call the Use method of passed Shader. Do it manually.
		//virtual void RenderStaticMeshes(const RenderInfo& info, const std::vector<std::reference_wrapper<const MeshInstance>>& meshes, const Transform& transform, Shader* shader, Mat4f* lastFrameMVP = nullptr, Material* overrideMaterial = nullptr, bool billboard = false) override; //Note: this function does not call the Use method of passed Shader. Do it manually.
		//virtual void RenderSkeletalMeshes(const RenderInfo& info, const std::vector<std::reference_wrapper<const MeshInstance>>& meshes, const Transform& transform, Shader* shader, SkeletonInfo& skelInfo, Mat4f* lastFrameMVP, Material* overrideMaterial = nullptr) override; //Note: this function does not call the Use method of passed Shader. Do it manually

		void Dispose();

	private:
		virtual void AddSceneRenderDataPtr(GameSceneRenderData&) override;
		friend class Game;
		virtual void RemoveSceneRenderDataPtr(GameSceneRenderData&) override;
		void GenerateEngineObjects();
		void LoadInternalShaders();
		void Resize(Vec2u resolution);

		GameManager* GameHandle;
		Vec2u Resolution;

		Postprocess Postprocessing;

		std::vector <GameSceneRenderData*> ScenesRenderData;
	public:
		std::vector <SharedPtr <Material>> Materials;

		std::vector <SharedPtr <Shader>> Shaders;
		std::vector <SharedPtr <Shader>> UserShaders;
		std::vector <SharedPtr <Shader>> ForwardShaders;		//We store forward shaders in a different vector to attempt rendering the scene at forward render stage. Putting non-forward-rendering shaders here will reduce performance.
		//std::vector <SharedPtr <Shader>> SettingIndependentShaders;		//TODO: Put setting independent shaders here. When the user asks for a shader, search in CurrentTbCollection first. Then check here.
		std::vector <Shader*> LightShaders;		//Same with light shaders
		Texture EmptyTexture;

		const Mesh* BoundMesh;
		const Material* BoundMaterial;
		SkeletonBatch* BoundSkeletonBatch;

		std::vector <UniquePtr <RenderToolboxCollection>> RenderTbCollections;
		RenderToolboxCollection* CurrentTbCollection;

		Mat4f PreviousFrameView;

		struct CubemapRenderData
		{
			GEE_FB::Framebuffer DefaultFramebuffer;
			Mat4f DefaultV[6];
			Mat4f DefaultVP[6];
		} CubemapData;
	};

	Vec3f GetCubemapFront(unsigned int);
	Vec3f GetCubemapUp(unsigned int);
}