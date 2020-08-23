#pragma once
#include "Postprocess.h"
#include "GameManager.h"
#include "MeshSystem.h"
#include "LightsManager.h"

class LightProbe;
class LocalLightProbe;
class RenderableVolume;

enum ShadingModel
{
	SHADING_PHONG,
	SHADING_PBR_COOK_TORRANCE
};


class RenderEngine : public RenderEngineManager
{
	GameManager* GameHandle;
	LightsManager LightData;
	const ShadingModel Shading;
	glm::uvec2 Resolution;

	Postprocess Postprocessing;

	GEE_FB::Framebuffer GFramebuffer;
	GEE_FB::Framebuffer MainFramebuffer;

	std::vector <std::shared_ptr <ModelComponent>> Models;
	std::vector <Material*> Materials;
	std::vector <std::shared_ptr <LightProbe>> LightProbes;
	std::vector <std::unique_ptr <MeshSystem::MeshTree>> MeshTrees;

	GEE_FB::Framebuffer ShadowFramebuffer;
	std::shared_ptr <Texture> ShadowMapArray, ShadowCubemapArray;

	std::vector <std::shared_ptr <Shader>> Shaders;
	std::vector <std::shared_ptr <Shader>> ForwardShaders;	//We store forward shaders in a different vector to attempt rendering the scene at forward render stage. Putting non-forward-rendering shaders here will reduce performance.
	std::vector <Shader*> LightShaders;		//Same with light shaders
	std::shared_ptr <Texture> EmptyTexture;
	glm::mat4 PreviousFrameView;

	struct CubemapRenderData
	{
		GEE_FB::Framebuffer DefaultFramebuffer;
		glm::mat4 DefaultV[6];
		glm::mat4 DefaultVP[6];
	} CubemapData;


	void GenerateEngineObjects();
	void LoadInternalShaders();
	void Resize(glm::uvec2 resolution);
	void SetupShadowmaps();
public:
	RenderEngine(GameManager*, const ShadingModel& shading);
	void Init(glm::uvec2 resolution);
 
	virtual const std::shared_ptr<Mesh> GetBasicShapeMesh(EngineBasicShape) override;
	virtual int GetAvailableLightIndex() override;
	virtual Shader* GetLightShader(LightType type) override;

	virtual std::shared_ptr<ModelComponent> AddModel(std::shared_ptr<ModelComponent> model) override;
	virtual std::shared_ptr<LightProbe> AddLightProbe(std::shared_ptr<LightProbe> probe) override;
	virtual std::shared_ptr<LightComponent> AddLight(std::shared_ptr<LightComponent> light) override;
	virtual std::shared_ptr<ModelComponent> CreateModel(const ModelComponent&) override;
	virtual std::shared_ptr<ModelComponent> CreateModel(ModelComponent&&) override;
	virtual MeshSystem::MeshTree* CreateMeshTree(std::string path) override;
	virtual MeshSystem::MeshTree* FindMeshTree(std::string path, MeshSystem::MeshTree* ignore = nullptr) override;
	virtual Material* AddMaterial(Material* material) override;
	virtual std::shared_ptr<Shader> AddShader(std::shared_ptr<Shader> shader, bool bForwardShader = false) override;

	ModelComponent* FindModel(std::string);
	Material* FindMaterial(std::string);
	virtual Shader* FindShader(std::string) override;

	void RenderBoundInDebug(RenderInfo&, GLenum mode, GLint first, GLint count, glm::vec3 color = glm::vec3(1.0f));
	void PreRenderPass();

	void RenderShadowMaps(std::vector<std::shared_ptr<LightComponent>>);
	void RenderVolume(RenderInfo&, EngineBasicShape, Shader&, const Transform* = nullptr);
	void RenderVolume(RenderInfo&, RenderableVolume*, Shader* boundShader, bool shadedRender);
	void RenderVolumes(RenderInfo&, const GEE_FB::Framebuffer& framebuffer, const std::vector<std::unique_ptr<RenderableVolume>>&, bool bIBLPass);
	void RenderLightProbes();
	void RenderScene(RenderInfo& info, Shader* shader = nullptr);
	void FullRender(RenderInfo& info, GEE_FB::Framebuffer* framebuffer = nullptr);
	void TestRenderCubemap(RenderInfo& info);
	virtual void RenderCubemapFromTexture(Texture targetTex, Texture tex, glm::uvec2 size, Shader& shader, int* layer = nullptr, int mipLevel = 0) override;
	virtual void RenderCubemapFromScene(RenderInfo& info, GEE_FB::Framebuffer target, GEE_FB::FramebufferAttachment targetTex, GLenum attachmentType, Shader* shader = nullptr, int* layer = nullptr, bool fullRender = false) override;
	virtual void Render(RenderInfo& info, const ModelComponent& model, Shader* shader, const Mesh* boundMesh = nullptr, Material* materialBound = nullptr) override;	//Note: this function does not call the Use method of passed Shader. Do it manually
	virtual void Render(RenderInfo& info, const std::shared_ptr<Mesh>&, const Transform&, Shader* shader, const Mesh* boundMesh = nullptr, Material* materialBound = nullptr) override; //Note: this function does not call the Use method of passed Shader. Do it manually

	void Dispose();
};

glm::vec3 GetCubemapFront(unsigned int);
glm::vec3 GetCubemapUp(unsigned int);