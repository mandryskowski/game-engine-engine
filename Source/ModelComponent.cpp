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

void ModelComponent::Render(Shader* shader, RenderInfo& info, unsigned int& VAOBound, Material* materialBound, bool& materials, MeshType& renderMeshType, unsigned int& emptyTexture)
{
	shader->UniformMatrix4fv("model", ComponentTransform.GetWorldTransformMatrix());
	for (unsigned int i = 0; i < Meshes.size(); i++)
	{
		MeshComponent* mesh = Meshes[i];
		MeshType thisMeshType = mesh->GetMeshType();
		if (!(renderMeshType == thisMeshType || renderMeshType == MeshType::MESH_ALL || thisMeshType == MeshType::MESH_ALL))
			continue;

		if (VAOBound != mesh->VAO || i == 0)
		{
			glBindVertexArray(mesh->VAO);
			VAOBound = mesh->VAO;
		}

		if (materials && materialBound != mesh->MeshMaterial && mesh->MeshMaterial)	//jesli ostatni zbindowany material jest taki sam, to nie musimy zmieniac danych w shaderze; oszczedzmy sobie roboty
		{
			mesh->MeshMaterial->UpdateUBOData(shader, emptyTexture);
			materialBound = mesh->MeshMaterial;
		}

		mesh->Render(shader, info);
	}
}