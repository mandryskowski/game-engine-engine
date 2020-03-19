#pragma once
#include "MeshComponent.h"

class ModelComponent: public Component
{
	std::vector<MeshComponent*> Meshes;
	std::string FilePath;

public:
	ModelComponent(std::string="undefined");
	void SetFilePath(std::string);
	std::string GetFilePath();
	void ProcessAiNode(const aiScene*, aiNode*, std::vector<ModelComponent*>&, MaterialLoadingData*);	//the vector contains the pointers of newly created ModelComponents for the render engine
	virtual void Render(Shader*, RenderInfo&, unsigned int&, Material*, bool&, unsigned int&);
};

