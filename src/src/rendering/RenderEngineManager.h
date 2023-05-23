#pragma once
#include "game/GameSettings.h"

namespace GEE
{
	class Material;
	class GameSceneRenderData;
	class SkeletonBatch;
	enum class LightType;
	class RenderToolboxCollection;
	class Shader;
	class Mesh;
	class RenderEngineManager
	{
	public:
		virtual const ShadingAlgorithm& GetShadingModel() = 0;
		virtual Mesh& GetBasicShapeMesh(EngineBasicShape) = 0;
		virtual Shader* GetSimpleShader() = 0;
		virtual Shader* GetLightShader(const RenderToolboxCollection& renderCol, LightType) = 0;
		virtual RenderToolboxCollection* GetCurrentTbCollection() = 0;
		virtual Texture GetEmptyTexture() = 0;
		virtual SkeletonBatch* GetBoundSkeletonBatch() = 0;


		//TODO: THIS SHOULD NOT BE HERE
		virtual std::vector<Shader*> GetCustomShaders() = 0;
		virtual std::vector<SharedPtr<Material>> GetMaterials() = 0;

		virtual void BindSkeletonBatch(SkeletonBatch* batch) = 0;
		virtual void BindSkeletonBatch(GameSceneRenderData* sceneRenderData, unsigned int index) = 0;

		/**
		 * @brief Add a new RenderToolboxCollection to the render engine to enable updating shadow maps automatically. By default, we load every toolbox that will be needed according to RenderToolboxCollection::Settings
		 * @param tbCollection: a constructed RenderToolboxCollection object containing the name and settings of it
		 * @param setupToolboxesAccordingToSettings: pass false as the second argument to disable loading any toolboxes
		 * @return a reference to the added RenderToolboxCollection
		*/
		virtual RenderToolboxCollection& AddRenderTbCollection(UniquePtr<RenderToolboxCollection> tbCollection,
			bool setupToolboxesAccordingToSettings = true) = 0;
		virtual SharedPtr<Material> AddMaterial(SharedPtr<Material>) = 0;
		virtual SharedPtr<Shader> AddShader(SharedPtr<Shader>) = 0;

		virtual void EraseRenderTbCollection(RenderToolboxCollection& tbCollection) = 0;
		virtual void EraseMaterial(Material&) = 0;

		template <typename T = Material> SharedPtr<T> FindMaterial(const String& name);
	protected:
		virtual SharedPtr<Material> FindMaterialImpl(const String&) = 0;
	public:
		virtual Shader* FindShader(const String& name) = 0;
		virtual RenderToolboxCollection* FindTbCollection(const String& name) = 0;

		virtual ~RenderEngineManager() = default;
	protected:
		virtual void AddSceneRenderDataPtr(GameSceneRenderData&) = 0;
		virtual void RemoveSceneRenderDataPtr(GameSceneRenderData&) = 0;

		virtual SharedPtr<Shader> AddNonCustomShader(SharedPtr<Shader>) = 0;
		friend class GameScene;
	};

	template<typename T>
	SharedPtr<T> RenderEngineManager::FindMaterial(const String& name)
	{
		return std::dynamic_pointer_cast<T, Material>(FindMaterialImpl(name));
	}
}
