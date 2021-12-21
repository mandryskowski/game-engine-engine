#include <rendering/Material.h>
#include <rendering/Texture.h>
#include <assimp/material.h>
#include <assimp/pbrmaterial.h>

#include <UI/UICanvasActor.h>
#include <UI/UICanvasField.h>
#include <scene/UIWindowActor.h>
#include <scene/UIInputBoxActor.h>
#include <scene/TextComponent.h>

#include <rendering/RenderToolbox.h>

#include <game/IDSystem.h>

namespace GEE
{
	Material::Material(MaterialLoc loc, ShaderInfo shaderInfo):
		Localization(loc),
		Shininess(0.0f),
		DepthScale(0.0f),
		Color(Vec4f(0.0f)),
		RoughnessColor(0.0f),
		MetallicColor(0.0f),
		AoColor(0.0f),
		MatShaderInfo(shaderInfo)
	{
	}

	Material::Material(MaterialLoc loc, const Vec3f& color, ShaderInfo shaderInfo) :
		Material(loc, Vec4f(color, 1.0f), shaderInfo)
	{}

	Material::Material(MaterialLoc loc, const Vec4f& color, ShaderInfo shaderInfo) :
		Material(loc, shaderInfo)
	{
		SetColor(color);
	}

	const Material::MaterialLoc& Material::GetLocalization() const
	{
		return Localization;
	}

	std::string Material::GetName() const
	{
		return GetLocalization().GetFullStr();
	}

	Material::ShaderInfo Material::GetShaderInfo() const
	{
		return MatShaderInfo;
	}

	Vec4f Material::GetColor() const
	{
		return Color;
	}

	unsigned int Material::GetTextureCount() const
	{
		return Textures.size();
	}

	NamedTexture Material::GetTexture(unsigned int i) const
	{
		GEE_CORE_ASSERT(i < Textures.size());
		return *Textures[i];
	}

	void Material::SetDepthScale(float scale)
	{
		DepthScale = scale;
	}
	void Material::SetShininess(float shine)
	{
		Shininess = shine;
	}

	void Material::SetShaderInfo(ShaderInfo shaderInfo)
	{
		MatShaderInfo = shaderInfo;
	}

	void Material::SetColor(const Vec3f& color)
	{
		SetColor(Vec4f(color, 1.0f));
	}

	void Material::SetColor(const Vec4f& color)
	{
		Color = color;
	}

	void Material::SetRoughnessColor(float roughness)
	{
		RoughnessColor = roughness;
	}

	void Material::SetMetallicColor(float metallic)
	{
		MetallicColor = metallic;
	}

	void Material::SetAoColor(float ao)
	{
		AoColor = ao;
	}

	void Material::AddTexture(const NamedTexture& tex)
	{
		Textures.push_back(MakeShared<NamedTexture>(tex));
	}

	void Material::AddTexture(SharedPtr<NamedTexture> tex)
	{
		Textures.push_back(tex);
	}

	void Material::RemoveTexture(NamedTexture& tex)
	{
		Textures.erase(std::remove_if(Textures.begin(), Textures.end(), [&tex](auto& texVec) { return &tex == texVec.get(); }), Textures.end());
	}

	void Material::LoadFromAiMaterial(const aiScene* scene, aiMaterial* material, const std::string& directory, MaterialLoadingData* matLoadingData)
	{
		//Load material's name
		aiString name;
		float shininess, roughness;
		aiColor3D color(0.0f);
		aiColor3D testColor(0.0f);
		material->Get(AI_MATKEY_NAME, name);
		material->Get(AI_MATKEY_SHININESS, shininess);
		material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
		material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
		material->Get(AI_MATKEY_COLOR_SPECULAR, testColor);
		Localization.Name = name.C_Str();
		Shininess = shininess;
		RoughnessColor = roughness;
		Color = Vec4f(color.r, color.g, color.b, 1.0f);

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
			std::pair<aiTextureType, std::string>(aiTextureType_UNKNOWN, "unknown")
		};

		//Load textures by type
		for (unsigned int i = 0; i < materialTypes.size(); i++)
			LoadAiTexturesOfType(scene, material, directory, materialTypes[i].first, materialTypes[i].second, matLoadingData);
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
				// If the texture is classified as unknown, check if it is a combination of different textures.
				if (type == aiTextureType_UNKNOWN)
				{
					aiString gltfMetalicRoughness;
					material->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &gltfMetalicRoughness);
					if (pathStr == gltfMetalicRoughness.C_Str())
					{
						std::cout << "Found GLTF metallicroughness texture " << pathStr << "\n";
						shaderName = "combined";
					}
				}

				// 1. Check if the texture has been loaded.
				bool found = false;
				const std::string originalFilename = scene->GetEmbeddedTexture(pathStr.c_str())->mFilename.C_Str();
				if (matLoadingData)
					if (auto foundTex = matLoadingData->FindTexture(std::string(scene->mName.C_Str()) + ":::" + originalFilename))
					{
						AddTexture(foundTex);
						found = true;
					}

				// 2. Load the texture if we have to.
				if (!found)
				{
					NamedTexture embeddedTex = NamedTexture(Texture::Loader<unsigned char>::Assimp::FromAssimpEmbedded(*scene->GetEmbeddedTexture(pathStr.c_str()), sRGB), shaderName + std::to_string(i + 1));
					embeddedTex.SetPath(std::string(scene->mName.C_Str()) + ":::" + originalFilename);
					embeddedTex.SetWrap(GL_REPEAT, GL_REPEAT, 0, true);
					AddTexture(MakeShared<NamedTexture>(embeddedTex));

					if (matLoadingData)
						matLoadingData->AddTexture(MakeShared<NamedTexture>(embeddedTex));
				}

				continue;
			}

			pathStr = directory + pathStr;

			if (matLoadingData)
				if (SharedPtr<NamedTexture> found = matLoadingData->FindTexture(pathStr))
				{
					Textures.push_back(found);
					continue;
				}


			SharedPtr<NamedTexture> tex = MakeShared<NamedTexture>(Texture::Loader<unsigned char>::FromFile2D(pathStr, (sRGB) ? (Texture::Format::SRGBA()) : (Texture::Format::RGBA()), false, Texture::MinFilter::Trilinear(), Texture::MagFilter::Bilinear()), shaderName + std::to_string(i + 1));	//create a new Texture and pass the file path, the shader name (for example albedo1, roughness1, ...) and the sRGB info
			tex->SetWrap(GL_REPEAT, GL_REPEAT, 0, true);
			AddTexture(tex);
			if (matLoadingData)
				matLoadingData->AddTexture(tex);
		}
	}

	void Material::UpdateWholeUBOData(Shader* shader, Texture& emptyTexture) const
	{
		shader->Uniform<float>("material.shininess", Shininess);
		shader->Uniform<float>("material.depthScale", DepthScale);
		shader->Uniform<Vec4f>("material.color", Color);
		shader->Uniform<Vec3f>("material.roughnessMetallicAoColor", Vec3f(RoughnessColor, MetallicColor, AoColor));


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

		shader->CallOnMaterialWholeDataUpdateFunc(*this);
	}

	void Material::GetEditorDescription(EditorDescriptionBuilder descBuilder)
	{
		descBuilder.AddField("Material name").CreateChild<UIInputBoxActor>("MaterialNameInputBox", [=](const std::string& materialName) { Localization.Name = materialName; }, [=]() { return GetName(); });
		//descBuilder.AddField("Shader name").CreateChild<UIInputBoxActor>("ShaderNameInputBox", [=](const std::string& shaderName) { SetRenderShaderName(shaderName); }, [=]() { return GetRenderShaderName(); });// .SetDisableInput(true);
		descBuilder.AddField("Color").GetTemplates().VecInput<4, float>(Color);
		for (int i = 0; i < 4; i++)
			descBuilder.GetDescriptionParent().GetActor<UIInputBoxActor>("VecBox" + std::to_string(i))->SetRetrieveContentEachFrame(true);
		
		{
			std::string colorNames[] = { "Red", "Green", "Blue" };
			for (int colorChannel = 0; colorChannel < 3; colorChannel++)
			{
				auto& redColorField = descBuilder.AddField(colorNames[colorChannel]);
				Vec3f color(0.0f);
				color[colorChannel] = 1.0f;
				redColorField.GetTitleComp()->SetMaterialInst(MaterialInstance(*new Material("", color)));
				redColorField.GetTemplates().SliderUnitInterval([this, colorChannel](float val) { Color[colorChannel] = val; }, Color[colorChannel]);
			}
		}

		UICanvasFieldCategory& texturesCat = descBuilder.AddCategory("Textures");
		UIAutomaticListActor& list = texturesCat.CreateChild<UIAutomaticListActor>("TexturesList");

		std::function<void(UIAutomaticListActor&, NamedTexture&)> addTextureButtonFunc = [this, &list, descBuilder](UIAutomaticListActor& list, NamedTexture& tex) mutable {
			std::string texName = tex.GetPath();
			Material* dummyMaterial = new Material("dummy");
			dummyMaterial->AddTexture(MakeShared<NamedTexture>(NamedTexture((Texture)tex, "albedo1")));
			UIButtonActor& texButton = list.CreateChild<UIButtonActor>(texName + "Button", texName);
			texButton.SetMatDisabled(*dummyMaterial);
			texButton.SetDisableInput(true);
			texButton.CreateChild<UIButtonActor>("DeleteTexture", "Delete", [this, &list, &tex, &texButton, descBuilder]() mutable { texButton.MarkAsKilled(); RemoveTexture(tex); dynamic_cast<UICanvasActor*>(&descBuilder.GetCanvas())->RefreshFieldsList(); }).GetTransform()->Move(Vec2f(2.0f, 0.0f));
			texButton.CreateComponent<TextComponent>("TexShaderNameButton", Transform(Vec2f(3.0f, 0.0f)), tex.GetShaderName(), "");
		};

		UIButtonActor& addTexToMaterialButton = list.CreateChild<UIButtonActor>("AddTextureToMaterialButton", [this, addTextureButtonFunc, &list, descBuilder]() mutable {
			UIWindowActor& addTexWindow = dynamic_cast<UICanvasActor*>(&descBuilder.GetCanvas())->CreateChildCanvas<UIWindowActor>("AddTextureWindow");
			EditorDescriptionBuilder texWindowDescBuilder(descBuilder.GetEditorHandle(), addTexWindow, addTexWindow);
			SharedPtr<std::string> path = MakeShared<std::string>();
			SharedPtr<std::string> shaderName = MakeShared<std::string>("albedo1");
			SharedPtr<bool> sRGB = MakeShared<bool>(false);

			texWindowDescBuilder.AddField("Tex path").GetTemplates().PathInput([this, path](const std::string& str) {
				*path = str;
				}, [path]() { return *path; }, { "*.png", "*.jpg" });
			texWindowDescBuilder.AddField("Shader name").CreateChild<UIInputBoxActor>("ShaderNameInputBox").SetOnInputFunc([shaderName](const std::string& input) { *shaderName = input; }, [shaderName]() { return *shaderName; });
			texWindowDescBuilder.AddField("sRGB").GetTemplates().TickBox([sRGB]() { return *sRGB = !(*sRGB); });
			texWindowDescBuilder.AddField("Add texture").CreateChild<UIButtonActor>("OKButton", "OK", [this, path, shaderName, sRGB, &addTextureButtonFunc, &list, &addTexWindow, descBuilder]() mutable {
				auto tex = MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D(*path, (*sRGB) ? (Texture::Format::SRGBA()) : (Texture::Format::RGBA())), *shaderName);
				AddTexture(tex);
				addTextureButtonFunc(list, *tex);
				addTexWindow.MarkAsKilled();
				dynamic_cast<UICanvasActor*>(&descBuilder.GetCanvas())->RefreshFieldsList();
				});

			addTexWindow.AutoClampView();
			addTexWindow.RefreshFieldsList();
			});

		for (auto& it : Textures)
			addTextureButtonFunc(list, *it);

		AtlasMaterial* addIconMat = dynamic_cast<AtlasMaterial*>(descBuilder.GetEditorHandle().GetGameHandle()->GetRenderEngineHandle()->FindMaterial("GEE_E_Add_Icon_Mat").get());
		if (addIconMat)
		{
			addTexToMaterialButton.SetMatIdle(MaterialInstance(*addIconMat, addIconMat->GetTextureIDInterpolatorTemplate(0.0f)));
			addTexToMaterialButton.SetMatHover(MaterialInstance(*addIconMat, addIconMat->GetTextureIDInterpolatorTemplate(1.0f)));
			addTexToMaterialButton.SetMatClick(MaterialInstance(*addIconMat, addIconMat->GetTextureIDInterpolatorTemplate(2.0f)));
		}

		list.Refresh();

		descBuilder.AddField("Roughness color").CreateChild<UIInputBoxActor>("RoughnessColorInputBoxActor", [this](float val) { SetRoughnessColor(val); }, [this]() { return RoughnessColor; });
		descBuilder.AddField("Metallic color").CreateChild<UIInputBoxActor>("MetallicColorInputBoxActor", [this](float val) { SetMetallicColor(val); }, [this]() { return MetallicColor; });
		descBuilder.AddField("Ao color").CreateChild<UIInputBoxActor>("AoColorInputBoxActor", [this](float val) { SetAoColor(val); }, [this]() { return AoColor; });

		descBuilder.AddField("Depth scale").CreateChild<UIInputBoxActor>("DepthScaleInputBoxActor", [this](float val) { SetDepthScale(val); }, [this]() { return DepthScale; });
	}

	/*
	========================================================================================================================
	========================================================================================================================
	========================================================================================================================
	*/

	AtlasMaterial::AtlasMaterial(Material&& material, Vec2i atlasSize, ShaderInfo shaderInfo) :
		Material(material),
		AtlasSize(static_cast<Vec2f>(atlasSize)),
		TextureID(0.0f)
	{
		SetShaderInfo(shaderInfo);
	}

	AtlasMaterial::AtlasMaterial(Material::MaterialLoc loc, Vec2i atlasSize, ShaderInfo shaderInfo) :
		AtlasMaterial(Material(loc), atlasSize, shaderInfo)
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
		shader->Uniform2fv("atlasData", Vec2f(TextureID, AtlasSize.x));
	}

	void AtlasMaterial::UpdateWholeUBOData(Shader* shader, Texture& emptyTexture) const
	{
		UpdateInstanceUBOData(shader);
		Material::UpdateWholeUBOData(shader, emptyTexture);

		shader->Uniform2fv("atlasTexOffset", Vec2f(1.0f) / AtlasSize);
	}

	Interpolator<float>& AtlasMaterial::GetTextureIDInterpolatorTemplate(float constantTextureID)
	{
		return GetTextureIDInterpolatorTemplate(Interpolation(0.0f, 0.0f, InterpolationType::CONSTANT), constantTextureID, constantTextureID);
	}

	Interpolator<float>& AtlasMaterial::GetTextureIDInterpolatorTemplate(const Interpolation& interp, float min, float max)
	{
		return *new Interpolator<float>(MakeShared<Interpolation>(interp), min, max, false, &TextureID);
	}

	void AtlasMaterial::GetEditorDescription(EditorDescriptionBuilder descBuilder)
	{
		Material::GetEditorDescription(descBuilder);

		descBuilder.AddField("Atlas size").GetTemplates().VecInput(AtlasSize);
	}

	/*
	========================================================================================================================
	========================================================================================================================
	========================================================================================================================
	*/

	MaterialInstance::MaterialInstance(Material& materialRef) :
		MaterialRef(materialRef),
		AnimationInterp(nullptr),
		DrawBeforeAnim(true),
		DrawAfterAnim(true)
	{
	}

	MaterialInstance::MaterialInstance(Material& materialRef, InterpolatorBase& interp, bool drawBefore, bool drawAfter) :
		MaterialRef(materialRef),
		AnimationInterp(&interp),
		DrawBeforeAnim(drawBefore),
		DrawAfterAnim(drawAfter)
	{
	}

	MaterialInstance::MaterialInstance(MaterialInstance&& matInst) :
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

	bool MaterialInstance::IsAnimated() const
	{
		return AnimationInterp != nullptr;
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
		if (AnimationInterp)
			AnimationInterp->GetInterp()->Reset();
		else
			std::cout << "ERROR: Cannot reset animation of MaterialInstance " + this->GetMaterialRef().GetName() << ". No AnimationInterp pointer detected.\n";
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

	SharedPtr<NamedTexture> MaterialLoadingData::FindTexture(const std::string& path) const
	{
		auto found = std::find_if(LoadedTextures.begin(), LoadedTextures.end(), [path](const SharedPtr<NamedTexture>& tex) { return tex->GetPath() == path; });
		if (found != LoadedTextures.end())
			return *found;

		return nullptr;
	}

	void MaterialLoadingData::AddTexture(SharedPtr<NamedTexture> tex)
	{
		LoadedTextures.push_back(tex);
	}
	Shader* Material::ShaderInfo::RetrieveShaderForRendering(RenderToolboxCollection& tbCol)
	{
		if (CustomShader)
			return CustomShader;

		return tbCol.GetShaderFromHint(GetShaderHint());
	}
}

void GEE::MaterialUtil::DisableColorIfAlbedoTextureDetected(Shader& shader, const Material& mat)
{
	bool foundAlbedo = false;
	for (int i = 0; i < static_cast<int>(mat.GetTextureCount()); i++)
		if (mat.GetTexture(i).GetShaderName() == "albedo1")
		{
			shader.Uniform<bool>("material.disableColor", true);
			foundAlbedo = true;
		}

	if (!foundAlbedo)
		shader.Uniform<bool>("material.disableColor", false);
}
