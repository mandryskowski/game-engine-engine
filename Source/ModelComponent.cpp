#include "ModelComponent.h"

ModelComponent::ModelComponent(std::string name):
	Component(name, Transform())
{

}

void ModelComponent::SetFilePath(std::string path)
{
	FilePath = path;
}

std::string ModelComponent::GetFilePath()
{
	return FilePath;
}


void ModelComponent::ProcessAiNode(const aiScene* scene, aiNode* node, std::vector<ModelComponent*>& modelsPtr, MaterialLoadingData* matLoadingData)
{
	Name = node->mName.C_Str();
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		Meshes.push_back(new MeshComponent);
		AddComponent(Meshes.back());	//TODO: usunac potem
		Meshes.back()->LoadFromAiMesh(scene, mesh, matLoadingData);
	}
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		ModelComponent* child = new ModelComponent;
		AddComponent(child);
		modelsPtr.push_back(child);
		child->ProcessAiNode(scene, node->mChildren[i], modelsPtr, matLoadingData);
	}
}

void ModelComponent::Render(Shader* shader, glm::mat4& view, unsigned int& VAOBound, Material* materialBound, bool& bUseMaterials, unsigned int& emptyTexture)
{
	shader->UniformMatrix4fv("model", ComponentTransform.GetWorldTransform().GetMatrix());
	for (unsigned int i = 0; i < Meshes.size(); i++)
	{
		if (VAOBound != Meshes[i]->VAO || i == 0)
		{
			glBindVertexArray(Meshes[i]->VAO);
			VAOBound = Meshes[i]->VAO;
		}
		if (bUseMaterials && materialBound != Meshes[i]->MeshMaterial && Meshes[i]->MeshMaterial)	//jesli ostatni zbindowany material jest taki sam, to nie musimy zmieniac danych w shaderze; oszczedzmy sobie roboty
		{
			Meshes[i]->MeshMaterial->UpdateUBOData(shader, emptyTexture);
			materialBound = Meshes[i]->MeshMaterial;
		}
		Meshes[i]->Render(shader, view);
	}
}