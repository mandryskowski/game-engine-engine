#include <rendering/Material.h>
#include <rendering/Texture.h>
#include <assimp/material.h>
#include <assimp/pbrmaterial.h>

Material::Material(MaterialLoc loc, float depthScale, Shader* shader):
	Localization(loc),
	Shininess(0.0f),
	DepthScale(depthScale),
	Color(Vec4f(0.0f))
{
	if (shader)
		RenderShaderName = shader->GetName();
}

Material::Material(const std::string& name, float depthScale, Shader* shader):
	Material(MaterialLoc(name, ""), depthScale, shader)
{
}

const Material::MaterialLoc& Material::GetLocalization() const
{
	return Localization;
}

const std::string& Material::GetRenderShaderName() const
{
	return RenderShaderName;
}

void Material::SetDepthScale(float scale)
{
	DepthScale = scale;
}
void Material::SetShininess(float shine)
{
	Shininess = shine;
}

void Material::SetRenderShaderName(const std::string& name)
{
	RenderShaderName = name;
}

void Material::SetColor(const glm::vec3& color)
{
	SetColor(glm::vec4(color, 1.0f));
}

void Material::SetColor(const glm::vec4& color)
{
	Color = color;
}

void Material::AddTexture(std::shared_ptr<NamedTexture> tex)
{
	Textures.push_back(tex);
}

void Material::LoadFromAiMaterial(const aiScene* scene, aiMaterial* material, const std::string& directory, MaterialLoadingData* matLoadingData)
{
	//Load material's name
	aiString name;
	float shininess;
	aiColor3D color(0.0f);
	aiColor3D testColor(0.0f);
	material->Get(AI_MATKEY_NAME, name);
	material->Get(AI_MATKEY_SHININESS, shininess);
	material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
	material->Get(AI_MATKEY_COLOR_SPECULAR, testColor);
	Localization.Name = name.C_Str();
	Shininess = shininess;
	Color = glm::vec4(color.r, color.g, color.b, 1.0f);

	material->Get(AI_MATKEY_COLOR_SPECULAR, testColor);
	printVector(glm::vec3(testColor.r, testColor.g, testColor.b), Localization.Name + " ROUGHNESS??");
	material->Get(AI_MATKEY_COLOR_AMBIENT, testColor);
	printVector(glm::vec3(testColor.r, testColor.g, testColor.b), Localization.Name + " ROUGHNESS??");
	material->Get(AI_MATKEY_COLOR_REFLECTIVE, testColor);
	printVector(glm::vec3(testColor.r, testColor.g, testColor.b), Localization.Name + " ROUGHNESS??");
	material->Get(AI_MATKEY_COLOR_TRANSPARENT, testColor);
	printVector(glm::vec3(testColor.r, testColor.g, testColor.b), Localization.Name + " ROUGHNESS??");
	std::cout << "SHININESS: " << shininess << '\n';
	float shininessStrength = 0.0f;
	material->Get(AI_MATKEY_SHININESS_STRENGTH, shininessStrength);
	std::cout << "SHININESS STRENGTH: " << shininessStrength << '\n';
		
	//This vector should contain all of the texture types that the engine can process (or they will not be loaded)
	std::vector<std::pair<aiTextureType, std::string>> materialTypes =
	{
		std::pair<aiTextureType, std::string>(aiTextureType_DIFFUSE, "albedo"),
		std::pair<aiTextureType, std::string>(aiTextureType_SPECULAR, "specular"),
		std::pair<aiTextureType, std::string>(aiTextureType_HEIGHT, "normal"),
		std::pair<aiTextureType, std::string>(aiTextureType_NORMALS, "normal"),
		std::pair<aiTextureType, std::string>(aiTextureType_DISPLACEMENT, "depth"),
		std::pair<aiTextureType, std::string>(aiTextureType_SHININESS, "roughness"),
		std::pair<aiTextureType, std::string>(aiTextureType_METALNESS, "metallic"),
		std::pair<aiTextureType, std::string>(aiTextureType_NORMAL_CAMERA, "normal"),
		std::pair<aiTextureType, std::string>(aiTextureType_UNKNOWN, "aaa"),
		std::pair<aiTextureType, std::string>(aiTextureType_LIGHTMAP, "aaa"),
	};

	if (directory.find("Container") != std::string::npos)
		for (int i = 0; i <= aiTextureType_UNKNOWN; i++)
			std::cout << i << ": " << material->GetTextureCount(aiTextureType(i)) << "\n";

	aiString fileBaseColor, fileMetallicRoughness;
	material->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_TEXTURE, &fileBaseColor);
	material->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &fileMetallicRoughness);
	std::cout << "Test " << fileBaseColor.C_Str() << ", " << fileMetallicRoughness.C_Str() << "\n";
	
	//Load textures by type
	for (unsigned int i = 0; i < materialTypes.size(); i++)
		LoadAiTexturesOfType(scene, material, directory, materialTypes[i].first, materialTypes[i].second, matLoadingData);

	if (fileBaseColor.length > 0 && material->GetTextureCount(aiTextureType_DIFFUSE) == 0 && fileBaseColor.data[0] == '*')
	{
		//AddTexture(std::make_shared<NamedTexture>(textureFromAiEmbedded(*scene->mTextures[std::stoi(std::string(fileBaseColor.C_Str()).substr(1))], true), "albedo1"));
		NamedTexture embeddedTex = NamedTexture(textureFromAiEmbedded(*scene->mTextures[std::stoi(std::string(fileBaseColor.C_Str()).substr(1))], true), "albedo1");
		embeddedTex.SetPath(std::string("*") + scene->mTextures[std::stoi(std::string(fileBaseColor.C_Str()).substr(1))]->mFilename.C_Str());
		AddTexture(std::make_shared<NamedTexture>(embeddedTex));
	}
	if (fileMetallicRoughness.length > 0 && fileMetallicRoughness.data[0] == '*')
	{
		//AddTexture(std::make_shared<NamedTexture>(textureFromAiEmbedded(*scene->mTextures[std::stoi(std::string(fileMetallicRoughness.C_Str()).substr(1))], true), "combined1"));
		NamedTexture embeddedTex = NamedTexture(textureFromAiEmbedded(*scene->mTextures[std::stoi(std::string(fileMetallicRoughness.C_Str()).substr(1))], true), "combined1");
		embeddedTex.SetPath(std::string("*") + scene->mTextures[std::stoi(std::string(fileMetallicRoughness.C_Str()).substr(1))]->mFilename.C_Str());
		AddTexture(std::make_shared<NamedTexture>(embeddedTex));
	}
	//if (fileMetallicRoughness.length > 0 && material->GetTextureCount(aiTextureType_SHININESS) == 0 && fileMetallicRoughness.data[0] == '*')
		//AddTexture(new NamedTexture(textureFromAiEmbedded(*scene->mTextures[std::stoi(std::string(fileMetallicRoughness.C_Str()).substr(1))], true), "roughness1"));
}

void Material::LoadAiTexturesOfType(const aiScene* scene, aiMaterial* material, const std::string& directory, aiTextureType type, std::string shaderName, MaterialLoadingData* matLoadingData)
{
	std::string name;
	switch (type)
	{
	case aiTextureType_DIFFUSE:
		name = "aiTextureType_DIFFUSE"; break;
	case aiTextureType_SPECULAR:
		name = "aiTextureType_SPECULAR"; break;
	case aiTextureType_HEIGHT:
		name = "aiTextureType_HEIGHT"; break;
	case aiTextureType_DISPLACEMENT:
		name = "aiTextureType_DISPLACEMENT"; break;
	case aiTextureType_SHININESS:
		name = "aiTextureType_SHININESS"; break;
	case aiTextureType_NORMALS:
		name = "aiTextureType_NORMALS"; break;
	case aiTextureType_UNKNOWN:
		name = "aiTextureType_UNKNOWN"; break;
	}

	bool sRGB = false;
	if (type == aiTextureType_DIFFUSE)
		sRGB = true;
	

	for (unsigned int i = 0; i < material->GetTextureCount(type); i++)	//for each texture of this type (there can be more than one)
	{
		aiString path;
		std::string pathStr;
		aiTextureMapping texMapping;	//TODO: maybe use this later?

		material->GetTexture(type, i, &path, &texMapping);
		pathStr = path.C_Str();

		if (!pathStr.empty() && *pathStr.begin() == '*')
		{
			std::cout << "Znaleziono " + pathStr << " na aiTextureType " << type << "\n";
			NamedTexture embeddedTex = NamedTexture(textureFromAiEmbedded(*scene->mTextures[std::stoi(pathStr.substr(1))], sRGB), shaderName + std::to_string(i + 1));
			embeddedTex.SetPath(std::string("*") + scene->mTextures[std::stoi(pathStr.substr(1))]->mFilename.C_Str());
			AddTexture(std::make_shared<NamedTexture>(embeddedTex));
			continue;
		}

		pathStr = directory + pathStr;

		if (matLoadingData)
			if (std::shared_ptr<NamedTexture> found = matLoadingData->FindTexture(pathStr))
			{
				Textures.push_back(found);
				continue;
			}
	

		std::shared_ptr<NamedTexture> tex = std::make_shared<NamedTexture>(textureFromFile(pathStr, (sRGB) ? (GL_SRGB) : (GL_RGB)), shaderName + std::to_string(i + 1));	//create a new Texture and pass the file path, the shader name (for example albedo1, roughness1, ...) and the sRGB info
		AddTexture(tex);
		if (matLoadingData)
			matLoadingData->AddTexture(tex);
	}
}

void Material::UpdateWholeUBOData(Shader* shader, Texture& emptyTexture) const
{
	shader->Uniform1f("material.shininess", Shininess);
	shader->Uniform1f("material.depthScale", DepthScale);
	shader->Uniform4fv("material.color", Color);

	std::vector<std::pair<unsigned int, std::string>>* textureUnits = shader->GetMaterialTextureUnits();

	for (unsigned int i = 0; i < textureUnits->size(); i++)
	{
		std::pair<unsigned int, std::string>* unitPair = &(*textureUnits)[i];
		glActiveTexture(GL_TEXTURE0 + unitPair->first);

		bool found = false;
		for (unsigned int j = 0; j < Textures.size(); j++)
		{
			if (Textures[j]->GetShaderName() == unitPair->second)
			{
				glBindTexture(GL_TEXTURE_2D, Textures[j]->GetID());
				found = true;
				break;
			}
		}
		if (!found)
			emptyTexture.Bind();
	}
}

/*
========================================================================================================================
========================================================================================================================
========================================================================================================================
*/

AtlasMaterial::AtlasMaterial(Material&& material, glm::ivec2 atlasSize) :
	Material(material),
	AtlasSize(static_cast<glm::vec2>(atlasSize)),
	TextureID(0.0f)
{
}

AtlasMaterial::AtlasMaterial(Material::MaterialLoc loc, glm::ivec2 atlasSize) :
	AtlasMaterial(Material(loc), atlasSize)
{
}

AtlasMaterial::AtlasMaterial(const std::string& name, glm::ivec2 atlasSize):
	AtlasMaterial(Material(name), atlasSize)
{
}

float AtlasMaterial::GetMaxTextureID() const
{
	return AtlasSize.x * AtlasSize.y - 1.0f;
}

void AtlasMaterial::UpdateInstanceUBOData(Shader* shader, bool setValuesToDefault) const
{
	if (setValuesToDefault)
		TextureID = 0.0f;
	shader->Uniform2fv("atlasData", glm::vec2(TextureID, AtlasSize.x));
}

void AtlasMaterial::UpdateWholeUBOData(Shader* shader, Texture& emptyTexture) const
{
	UpdateInstanceUBOData(shader);
	Material::UpdateWholeUBOData(shader, emptyTexture);

	shader->Uniform2fv("atlasTexOffset", glm::vec2(1.0f) / AtlasSize);
}

Interpolator<float>& AtlasMaterial::GetTextureIDInterpolatorTemplate(float constantTextureID)
{
	return GetTextureIDInterpolatorTemplate(Interpolation(0.0f, 0.0f, InterpolationType::CONSTANT), constantTextureID, constantTextureID);
}

Interpolator<float>& AtlasMaterial::GetTextureIDInterpolatorTemplate(const Interpolation& interp, float min, float max)
{
	return *new Interpolator<float>(std::make_shared<Interpolation>(interp), min, max, false, &TextureID);
}

/*
========================================================================================================================
========================================================================================================================
========================================================================================================================
*/

MaterialInstance::MaterialInstance(Material& materialRef):
	MaterialRef(materialRef),
	AnimationInterp(nullptr),
	DrawBeforeAnim(true),
	DrawAfterAnim(true)
{
}

MaterialInstance::MaterialInstance(Material& materialRef, InterpolatorBase& interp, bool drawBefore, bool drawAfter):
	MaterialRef(materialRef),
	AnimationInterp(&interp),
	DrawBeforeAnim(drawBefore),
	DrawAfterAnim(drawAfter)
{
}

MaterialInstance::MaterialInstance(MaterialInstance&& matInst):
	MaterialRef(matInst.MaterialRef),
	AnimationInterp(matInst.AnimationInterp),
	DrawBeforeAnim(matInst.DrawBeforeAnim),
	DrawAfterAnim(matInst.DrawAfterAnim)
{
}

Material& MaterialInstance::GetMaterialRef()
{
	return MaterialRef;
}

const Material& MaterialInstance::GetMaterialRef() const
{
	return MaterialRef;
}

bool MaterialInstance::ShouldBeDrawn() const
{
	if ((!AnimationInterp) ||
		((DrawBeforeAnim || AnimationInterp->GetInterp()->GetT() > 0.0f) && (DrawAfterAnim || AnimationInterp->GetInterp()->GetT() < 1.0f)))
		return true;

	return false;
}

void MaterialInstance::SetInterp(InterpolatorBase* interp)
{
	AnimationInterp = interp;
}

void MaterialInstance::SetDrawBeforeAnim(bool drawBefore)
{
	DrawBeforeAnim = drawBefore;
}

void MaterialInstance::SetDrawAfterAnim(bool drawAfter)
{
	DrawAfterAnim = drawAfter;
}

void MaterialInstance::ResetAnimation()
{
	AnimationInterp->GetInterp()->Reset();
}

void MaterialInstance::Update(float deltaTime)
{
	if (!AnimationInterp)	//if not animated, return
		return;

	AnimationInterp->Update(deltaTime);
}

void MaterialInstance::UpdateInstanceUBOData(Shader* shader) const
{
	if (AnimationInterp)
		AnimationInterp->UpdateInterpolatedValPtr();	//Update animated values for each instance
	MaterialRef.UpdateInstanceUBOData(shader, AnimationInterp == nullptr);	//If no animation is present, just ask for setting the default material values.
}

void MaterialInstance::UpdateWholeUBOData(Shader* shader, Texture& emptyTexture) const
{
	UpdateInstanceUBOData(shader);
	MaterialRef.UpdateWholeUBOData(shader, emptyTexture);
}

MaterialInstance& MaterialInstance::operator=(MaterialInstance&& matInst)
{
	MaterialRef = matInst.MaterialRef;
	AnimationInterp = matInst.AnimationInterp;
	DrawBeforeAnim = matInst.DrawBeforeAnim;
	DrawAfterAnim = matInst.DrawAfterAnim;

	return *this;
}

std::shared_ptr<NamedTexture> MaterialLoadingData::FindTexture(const std::string& path) const
{
	auto found = std::find_if(LoadedTextures.begin(), LoadedTextures.end(), [path](const std::shared_ptr<NamedTexture>& tex) { return tex->GetPath() == path; });
	if (found != LoadedTextures.end())
		return *found;

	return nullptr;
}

void MaterialLoadingData::AddTexture(std::shared_ptr<NamedTexture> tex)
{
	LoadedTextures.push_back(tex);
}
