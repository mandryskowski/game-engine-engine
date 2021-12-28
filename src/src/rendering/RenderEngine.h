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
		virtual Shader* GetSimpleShader() override;

		virtual std::vector<Shader*> GetCustomShaders() override;
		virtual std::vector<SharedPtr<Material>> GetMaterials() override;

		/**
		 * @brief Add a new RenderToolboxCollection to the render engine to enable updating shadow maps automatically. By default, we load every toolbox that will be needed according to RenderToolboxCollection::Settings
		 * @param tbCollection: a constructed RenderToolboxCollection object containing the name and settings of it
		 * @param setupToolboxesAccordingToSettings: pass false as the second argument to disable loading any toolboxes
		 * @return a reference to the added RenderToolboxCollection
		*/
		virtual RenderToolboxCollection& AddRenderTbCollection(const RenderToolboxCollection& tbCollection, bool setupToolboxesAccordingToSettings = true) override;
		virtual SharedPtr<Material> AddMaterial(SharedPtr<Material> material) override;
		virtual SharedPtr<Shader> AddShader(SharedPtr<Shader> shader) override;

		virtual void BindSkeletonBatch(SkeletonBatch* batch) override;
		virtual void BindSkeletonBatch(GameSceneRenderData* sceneRenderData, unsigned int index) override;

		virtual void EraseRenderTbCollection(RenderToolboxCollection& tbCollection) override;
		virtual void EraseMaterial(Material&) override;


	private:
		virtual SharedPtr<Material> FindMaterialImpl(const std::string&) override;
	public:
		virtual Shader* FindShader(const std::string& name) override;

		virtual RenderToolboxCollection* FindTbCollection(const std::string& name) override;

		void Dispose();

	private:
		friend class Game;
		virtual void AddSceneRenderDataPtr(GameSceneRenderData&) override;
		virtual void RemoveSceneRenderDataPtr(GameSceneRenderData&) override;
		virtual SharedPtr<Shader> AddNonCustomShader(SharedPtr<Shader>);
		void GenerateEngineObjects();
		void LoadInternalShaders();
		void Resize(Vec2u resolution);

		GameManager* GameHandle;
		Vec2u Resolution;

		Postprocess Postprocessing;

		std::vector <GameSceneRenderData*> ScenesRenderData;
	public:
		std::vector <SharedPtr <Material>> Materials;

		std::unordered_map<EngineBasicShape, Mesh*> BasicShapes;
		std::vector <SharedPtr <Shader>> Shaders;
		std::vector <SharedPtr <Shader>> CustomShaders;	//Shaders created by the user used at the forward rendering stage.
		//std::vector <SharedPtr <Shader>> SettingIndependentShaders;		//TODO: Put setting independent shaders here. When the user asks for a shader, search in CurrentTbCollection first. Then check here.
		std::vector <Shader*> LightShaders;		//Same with light shaders
		Shader* SimpleShader;
		Texture EmptyTexture;

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