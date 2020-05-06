#pragma once
#include "Postprocess.h"
#include "ModelComponent.h"
#include "LightComponent.h"

class RenderEngine
{
	Shader GShader;
	Shader DefaultShader;
	std::vector <ModelComponent*> Models;
	std::vector <Material*> Materials;
	std::vector <Mesh*> MeshCollection;

	glm::uvec2 ShadowMapSize;
	unsigned int ShadowFBO;
	unsigned int ShadowMapArray;
	unsigned int ShadowCubemapArray;
	Shader DepthShader, DepthLinearizeShader;

	Framebuffer* SSAOFramebuffer;
	Shader* SSAOShader;
	unsigned int SSAONoiseTex;

	Shader DebugShader;
	Mesh* EngineObjects[4];
	Shader LightShaders[3];
	std::vector<Shader*> ExternalShaders;
	unsigned int EmptyTexture;


	void GenerateEngineObjects();
public:
	RenderEngine();
	unsigned int GetSSAOTex();
	std::vector<Shader*> GetForwardShaders();
	void AddModel(ModelComponent* model);
	void AddModels(std::vector<ModelComponent*>& models);
	void AddMaterial(Material* material);
	void LoadModels();
	void LoadMaterials(std::string path, std::string directory);
	void LoadShaders(std::stringstream&, std::string directory);
	void SetupShadowmaps(glm::uvec2);
	void SetupAmbientOcclusion(glm::uvec2, unsigned int);
	ModelComponent* FindModel(std::string);
	Material* FindMaterial(std::string);
	Shader* FindShader(std::string);
	void RenderEngineObject(RenderInfo&, std::pair<EngineObjectType, Transform*>, Shader* = nullptr);
	void RenderEngineObjects(RenderInfo&, std::vector<std::pair<EngineObjectType, Transform*>>, Shader* = nullptr);
	void RenderShadowMaps(std::vector<LightComponent*>);
	void RenderSSAO(RenderInfo&, glm::uvec2);
	void RenderLightVolume(RenderInfo&, LightComponent*, Transform* = nullptr, bool = true);
	void RenderLightVolumes(RenderInfo&, std::vector<LightComponent*>, glm::vec2);
	void RenderScene(RenderInfo& info, Shader* shader = nullptr);
};

glm::vec3 GetCubemapFront(unsigned int);
glm::vec3 GetCubemapUp(unsigned int);