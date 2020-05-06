#include "ModelComponent.h"

ModelComponent::ModelComponent(std::string name, Material* overrideMat):
	Component(name, Transform())
{
	OverrideMaterial = overrideMat;
}

void ModelComponent::SetFilePath(std::string path)
{
	FilePath = path;
}

void ModelComponent::SetOverrideMaterial(Material* overrideMat)
{
	OverrideMaterial = overrideMat;
}

std::string ModelComponent::GetFilePath()
{
	return FilePath;
}

std::vector<Mesh*> ModelComponent::GetReferencedMeshes()
{
	std::vector<Mesh*> meshes;

	for (unsigned int i = 0; i < MeshInstances.size(); i++)
	{
		Mesh* mesh = MeshInstances[i]->GetMesh();

		for (unsigned int j = 0; j < meshes.size(); j++)
		{
			if (meshes[j] == mesh)
			{
				mesh = nullptr;
				break;
			}
		}

		if (mesh)
			meshes.push_back(mesh);
	}

	return meshes;
}

MeshInstance* ModelComponent::FindMeshInstance(std::string name)
{
	for (unsigned int i = 0; i < MeshInstances.size(); i++)
		if (MeshInstances[i]->GetMesh()->GetName().find(name) != std::string::npos)
			return MeshInstances[i];

	for (unsigned int i = 0; i < Children.size(); i++)
	{
		MeshInstance* meshInst = FindMeshInstance(name);
		if (meshInst)
			return meshInst;
	}

	return nullptr;
}

void ModelComponent::AddMeshInst(Mesh* mesh)
{
	MeshInstances.push_back(new MeshInstance(mesh, OverrideMaterial));	//memory leak
}

void ModelComponent::ProcessAiNode(const aiScene* scene, std::string directory, aiNode* node, std::vector<ModelComponent*>& modelsPtr, MaterialLoadingData* matLoadingData)
{
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* assimpMesh = scene->mMeshes[node->mMeshes[i]];
		Mesh* mesh = new Mesh(node->mName.C_Str());
		mesh->LoadFromAiMesh(scene, assimpMesh, directory, true, matLoadingData);

		MeshInstances.push_back(new MeshInstance(mesh, OverrideMaterial));
	}
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		ModelComponent* child = new ModelComponent(Name + " Child" + std::to_string(i), OverrideMaterial);
		AddComponent(child);
		modelsPtr.push_back(child);
		child->ProcessAiNode(scene, directory, node->mChildren[i], modelsPtr, matLoadingData);
		child->GetTransform()->bConstrain = ComponentTransform.bConstrain;	//force the rotation setting from file transform to all of the children nodes
	}
}

void ModelComponent::Render(Shader* shader, RenderInfo& info, unsigned int& VAOBound, Material* materialBound, unsigned int emptyTexture)
{
	glm::mat4 modelMat = ComponentTransform.GetWorldTransformMatrix();	//don't worry, the ComponentTransform's world transform is cached

	if (shader->ExpectsMatrix(MatrixType::MODEL))
		shader->UniformMatrix4fv("model", modelMat);
	if (shader->ExpectsMatrix(MatrixType::VIEW))
		shader->UniformMatrix4fv("view", *info.view);
	if (shader->ExpectsMatrix(MatrixType::MV))
		shader->UniformMatrix4fv("MV", (*info.view) * modelMat);
	if (shader->ExpectsMatrix(MatrixType::MVP))
		shader->UniformMatrix4fv("MVP", (*info.VP) * modelMat);
	if (shader->ExpectsMatrix(MatrixType::NORMAL))
		shader->UniformMatrix3fv("normalMat", ModelToNormal(modelMat));

	for (unsigned int i = 0; i < MeshInstances.size(); i++)
	{
		MeshInstance* meshInst = MeshInstances[i];
		Mesh* mesh = meshInst->GetMesh();
		Material* material = meshInst->GetMaterialPtr();
		MaterialInstance* materialInst = meshInst->GetMaterialInst();

		if ((info.CareAboutShader && material && shader != material->GetRenderShader()) || (info.OnlyShadowCasters && !mesh->CanCastShadow()) || !materialInst->ShouldBeDrawn())
			continue;

		if (VAOBound != mesh->VAO || i == 0)
		{
			glBindVertexArray(mesh->VAO);
			VAOBound = mesh->VAO;
		}

		if (info.UseMaterials && materialBound != material && material)	//jesli ostatni zbindowany material jest taki sam, to nie musimy zmieniac danych w shaderze; oszczedzmy sobie roboty
		{
			materialInst->UpdateWholeUBOData(shader, emptyTexture);
			materialBound = material;
		}
		else if (materialBound)
			materialInst->UpdateInstanceUBOData(shader);

		mesh->Render();
	}
}