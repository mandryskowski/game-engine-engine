#pragma once
#include "Mesh.h"

class ModelComponent: public Component
{
	std::vector<MeshInstance*> MeshInstances;
	std::string FilePath;
	Material* OverrideMaterial;

public:
	ModelComponent(std::string name = "undefined", Material* overrideMat = nullptr);
	void SetFilePath(std::string);
	void SetOverrideMaterial(Material*);
	std::string GetFilePath();
	std::vector<Mesh*> GetReferencedMeshes();
	MeshInstance* FindMeshInstance(std::string);
	void AddMeshInst(Mesh*);
	void ProcessAiNode(const aiScene*, std::string, aiNode*, std::vector<ModelComponent*>&, MaterialLoadingData*);	//the vector contains the pointers of newly created ModelComponents for the render engine
	virtual void Render(Shader*, RenderInfo&, unsigned int&, Material*, unsigned int);
};