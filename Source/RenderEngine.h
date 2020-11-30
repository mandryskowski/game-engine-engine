#pragma once
#include "Postprocess.h"
#include "GameManager.h"
#include "MeshSystem.h"
#include "RenderToolbox.h"

class LightProbe;
struct LightProbeTextureArrays;
class LocalLightProbe;
class RenderableVolume;
class BoneComponent;
class TextComponent;

class RenderEngine : public RenderEngineManager
{
public:
	RenderEngine(GameManager*);
	void Init(glm::uvec2 resolution);

	virtual const ShadingModel& GetShadingModel() override;
	virtual const std::shared_ptr<Mesh> GetBasicShapeMesh(EngineBasicShape) override;
	virtual Shader* GetLightShader(const RenderToolboxCollection& renderCol, LightType type) override;
	virtual RenderToolboxCollection* GetCurrentTbCollection() override;

	RenderToolboxCollection& AddRenderTbCollection(const RenderToolboxCollection&, bool setupToolboxesAccordingToSettings = true); //Pass false as the second argument to disable loading any toolboxes. By default, we load every toolbox that will be needed according to RenderToolboxCollection::Settings
	virtual void AddSceneRenderDataPtr(GameSceneRenderData*) override;
	virtual MeshSystem::MeshTree* CreateMeshTree(std::string path) override;
	virtual MeshSystem::MeshTree* FindMeshTree(std::string path, MeshSystem::MeshTree* ignore = nullptr) override;
	virtual Material* AddMaterial(Material* material) override;
	virtual std::shared_ptr<Shader> AddShader(std::shared_ptr<Shader> shader, bool bForwardShader = false) override;

	void BindSkeletonBatch(SkeletonBatch* batch);
	void BindSkeletonBatch(GameSceneRenderData* sceneRenderData, unsigned int index);

	virtual Material* FindMaterial(std::string) override;
	virtual Shader* FindShader(std::string) override;


	void RenderShadowMaps(RenderToolboxCollection& tbCollection, GameSceneRenderData* sceneRenderData, std::vector<std::shared_ptr<LightComponent>>);
	void RenderVolume(RenderInfo&, EngineBasicShape, Shader&, const Transform* = nullptr);
	void RenderVolume(RenderInfo&, RenderableVolume*, Shader* boundShader, bool shadedRender);
	void RenderVolumes(RenderInfo&, const GEE_FB::Framebuffer& framebuffer, const std::vector<std::unique_ptr<RenderableVolume>>&, bool bIBLPass);
	void RenderLightProbes(GameSceneRenderData* sceneRenderData);
	void RenderRawScene(RenderInfo& info, GameSceneRenderData* sceneRenderData, Shader* shader = nullptr);

	void RenderBoundInDebug(RenderInfo&, GLenum mode, GLint first, GLint count, glm::vec3 color = glm::vec3(1.0f));
	void PreLoopPass();

	void PrepareScene(RenderToolboxCollection& tbCollection, GameSceneRenderData* sceneRenderData);	//Call this method once per frame for each scene that will be rendered in order to prepare stuff like shadow maps
	void FullSceneRender(RenderInfo& info, GameSceneRenderData* sceneRenderData, GEE_FB::Framebuffer* framebuffer = nullptr, Viewport = Viewport(glm::vec2(0.0f), glm::vec2(0.0f)));	//This method renders a scene with lighting and some postprocessing that improve the visual quality (e.g. SSAO, if enabled).

	void PrepareFrame();
	void PostFrame();	//This method completes the postprocessing after everything has been rendered. Call it at the end of your frame rendering function to minimize overhead. Algorithms like anti-aliasing don't need to run multiple times.
	virtual void RenderFrame(RenderInfo& info);

	void TestRenderCubemap(RenderInfo& info, GameSceneRenderData* sceneRenderData);
	virtual void RenderCubemapFromTexture(Texture targetTex, Texture tex, glm::uvec2 size, Shader& shader, int* layer = nullptr, int mipLevel = 0) override;
	virtual void RenderCubemapFromScene(RenderInfo info, GameSceneRenderData* sceneRenderData, GEE_FB::Framebuffer target, GEE_FB::FramebufferAttachment targetTex, GLenum attachmentType, Shader* shader = nullptr, int* layer = nullptr, bool fullRender = false) override;
	virtual void RenderText(RenderInfo info, const Font& font, std::string content, Transform t = Transform(), glm::vec3 color = glm::vec3(1.0f), Shader* shader = nullptr, bool convertFromPx = false) override; //Pass a shader if you do not want the default shader to be used.
	virtual void RenderStaticMesh(RenderInfo info, const MeshInstance& mesh, const Transform& transform, Shader* shader, glm::mat4* lastFrameMVP = nullptr, Material* overrideMaterial = nullptr, bool billboard = false) override; //Note: this function does not call the Use method of passed Shader. Do it manually.
	virtual void RenderStaticMeshes(RenderInfo info, const std::vector<std::unique_ptr<MeshInstance>>& meshes, const Transform& transform, Shader* shader, glm::mat4* lastFrameMVP = nullptr, Material* overrideMaterial = nullptr, bool billboard = false) override; //Note: this function does not call the Use method of passed Shader. Do it manually.
	virtual void RenderSkeletalMeshes(RenderInfo info, const std::vector<std::unique_ptr<MeshInstance>>& meshes, const Transform& transform, Shader* shader, SkeletonInfo& skelInfo, Material* overrideMaterial = nullptr) override; //Note: this function does not call the Use method of passed Shader. Do it manually

	void Dispose();

private:
	void GenerateEngineObjects();
	void LoadInternalShaders();
	void Resize(glm::uvec2 resolution);
	void SetupShadowmaps();

	GameManager* GameHandle;
	glm::uvec2 Resolution;

	Postprocess Postprocessing;

	std::vector <GameSceneRenderData*> ScenesRenderData;
	std::vector <Material*> Materials;
	std::vector <std::unique_ptr <MeshSystem::MeshTree>> MeshTrees;

	std::vector <std::shared_ptr <Shader>> Shaders;
	std::vector <std::shared_ptr <Shader>> ForwardShaders;		//We store forward shaders in a different vector to attempt rendering the scene at forward render stage. Putting non-forward-rendering shaders here will reduce performance.
	//std::vector <std::shared_ptr <Shader>> SettingIndependentShaders;		//TODO: Put setting independent shaders here. When the user asks for a shader, search in CurrentTbCollection first. Then check here.
	std::vector <Shader*> LightShaders;		//Same with light shaders
	std::shared_ptr <Texture> EmptyTexture;

	const Mesh* BoundMesh;
	Material* BoundMaterial;
	SkeletonBatch* BoundSkeletonBatch;

	std::vector <std::unique_ptr <RenderToolboxCollection>> RenderTbCollections;
	RenderToolboxCollection* CurrentTbCollection;

	glm::mat4 PreviousFrameView;

	struct CubemapRenderData
	{
		GEE_FB::Framebuffer DefaultFramebuffer;
		glm::mat4 DefaultV[6];
		glm::mat4 DefaultVP[6];
	} CubemapData;
};

glm::vec3 GetCubemapFront(unsigned int);
glm::vec3 GetCubemapUp(unsigned int);