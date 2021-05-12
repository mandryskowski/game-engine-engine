#pragma once
#include <game/GameManager.h>
#include <game/GameSettings.h>
#include "Framebuffer.h"
#include <typeinfo>

struct ToolboxCollectionInfo
{
	std::string Name;
	glm::vec2 Resolution;
	GameSettings Settings;
	ShadingModel Shading;
};

class RenderToolbox
{
public:
	virtual bool IsSetup();
	Shader* FindShader(std::string name);
	
	void Dispose();
protected:
	GEE_FB::Framebuffer* AddFramebuffer();
	Shader* AddShader(const std::shared_ptr<Shader>& shader);
	Texture* AddTexture(const Texture& = Texture());

	std::vector<std::shared_ptr<GEE_FB::Framebuffer>> Fbs;
	std::vector<std::shared_ptr<Shader>> Shaders;	//add type ShaderVariant?
	std::vector<std::shared_ptr<Texture>> Textures;
};

class DeferredShadingToolbox : public RenderToolbox
{
public:
	DeferredShadingToolbox();
	DeferredShadingToolbox(const GameSettings::VideoSettings& settings);
	void Setup(const GameSettings::VideoSettings& settings);

	friend class MainFramebufferToolbox;
	friend class RenderEngine;
private:
	GEE_FB::Framebuffer* GFb;

	std::vector<Shader*> LightShaders;
	Shader* GeometryShader;
};

class MainFramebufferToolbox : public RenderToolbox
{
public:
	MainFramebufferToolbox();
	MainFramebufferToolbox(const GameSettings::VideoSettings& settings, DeferredShadingToolbox* deferredTb = nullptr);
	void Setup(const GameSettings::VideoSettings& settings, DeferredShadingToolbox* deferredTb = nullptr);	//Pass a DeferredShadingToolbox to reuse some textures and save memory.

	friend class RenderEngine;
private:
	GEE_FB::Framebuffer* MainFb;
};

class SSAOToolbox : public RenderToolbox
{
public:
	SSAOToolbox();
	SSAOToolbox(const GameSettings::VideoSettings& settings);
	void Setup(const GameSettings::VideoSettings& settings);

	friend class Postprocess;

private:
	GEE_FB::Framebuffer* SSAOFb;

	Shader* SSAOShader;

	Texture* SSAONoiseTex;
};

class PrevFrameStorageToolbox : public RenderToolbox
{
public:
	PrevFrameStorageToolbox();
	PrevFrameStorageToolbox(const GameSettings::VideoSettings& settings);
	void Setup(const GameSettings::VideoSettings& settings);

	friend class Postprocess;
private:
	GEE_FB::Framebuffer* StorageFb; //two color buffers; before rendering to FBO 0 we render to one of its color buffers (we switch them each frame) so as to gain access to the final color buffer in the next frame (for temporal AA)
};

class SMAAToolbox : public RenderToolbox
{
public:
	SMAAToolbox();
	SMAAToolbox(const GameSettings::VideoSettings& settings);
	void Setup(const GameSettings::VideoSettings& settings);

	friend class Postprocess;
private:
	GEE_FB::Framebuffer* SMAAFb; //three color buffers; we use the first two color buffers as edgeTex&blendTex in SMAA pass and optionally the last one to store the neighborhood blend result for reprojection

	Shader* SMAAShaders[4];

	Texture* SMAAAreaTex, *SMAASearchTex;
};

class ComposedImageStorageToolbox : public RenderToolbox
{
public:
	ComposedImageStorageToolbox();
	ComposedImageStorageToolbox(const GameSettings::VideoSettings& settings);
	void Setup(const GameSettings::VideoSettings& settings);

	friend class Postprocess;
private:
	GEE_FB::Framebuffer* ComposedImageFb;

	Shader* TonemapGammaShader;
};

class GaussianBlurToolbox : public RenderToolbox
{
public:
	GaussianBlurToolbox();
	GaussianBlurToolbox(const GameSettings::VideoSettings& settings);
	void Setup(const GameSettings::VideoSettings& settings);

	friend class Postprocess;
private:
	GEE_FB::Framebuffer* BlurFramebuffers[2]; //only color buffer; we use them as ping pong framebuffers in gaussian blur pass.
										//Side note: I wonder which method is quicker - switching a framebuffer or switching an attachment or switching a buffer to draw to via glDrawBuffer.
										//Side note 2: I've heard that they are similar in terms of performance. So now I wonder why do we use ping-pong framebuffers and not ping-pong color buffers. They should work the same, shouldn't they?

	Shader* GaussianBlurShader;
};

class ShadowMappingToolbox : public RenderToolbox
{
public:
	ShadowMappingToolbox();
	ShadowMappingToolbox(const GameSettings::VideoSettings& settings);
	void Setup(const GameSettings::VideoSettings& settings);
	friend class RenderEngine;
private:
	GEE_FB::Framebuffer* ShadowFramebuffer;
	Texture* ShadowMapArray, *ShadowCubemapArray;
};

class FinalRenderTargetToolbox : public RenderToolbox
{
public:
	FinalRenderTargetToolbox();
	FinalRenderTargetToolbox(const GameSettings::VideoSettings& settings);
	void Setup(const GameSettings::VideoSettings&);
	GEE_FB::Framebuffer& GetFinalFramebuffer();
	friend class RenderEngine;
private:
	std::shared_ptr<GEE_FB::FramebufferAttachment> RenderTarget;
	GEE_FB::Framebuffer* FinalFramebuffer;
};

class RenderToolboxCollection
{
public:
	RenderToolboxCollection(std::string name, const GameSettings::VideoSettings& settings):
		Name(name),
		Settings(settings)
	{

	}
	template <class T> T* GetTb() const
	{
		for (int i = 0; i < Tbs.size(); i++)
			if (T* castedObj = dynamic_cast<T*>(Tbs[i].get()))
				return castedObj;

		return nullptr;
	}
	template <class T> void AddTb(const T& obj)
	{
		if (GetTb<T>())
		{
			std::cerr << "WARNING! A toolbox collection contains two toolboxes of the same type.\n";
			DisposeOfTb<T>();
		}

		Tbs.push_back(std::make_shared<T>(T(obj)));
	}
	virtual void AddTbsRequiredBySettings()
	{	
		Dispose();
		
		std::cout << "Tobox 1.\n";
		if (Settings.AAType == AA_SMAA1X || Settings.AAType == AA_SMAAT2X)
			AddTb<SMAAToolbox>(SMAAToolbox(Settings));
		std::cout << "Tobox 2.\n";
		if (Settings.bBloom || Settings.AmbientOcclusionSamples > 0)
			AddTb<GaussianBlurToolbox>(GaussianBlurToolbox(Settings));
		std::cout << "Tobox 3.\n";
		if (Settings.AmbientOcclusionSamples > 0)
			AddTb<SSAOToolbox>(SSAOToolbox(Settings));
		std::cout << "Tobox 4.\n";
		if (Settings.IsVelocityBufferNeeded())
			AddTb<PrevFrameStorageToolbox>(PrevFrameStorageToolbox(Settings));
		std::cout << "Tobox 5.\n";
		if (Settings.Shading != ShadingModel::SHADING_FULL_LIT)
			AddTb<DeferredShadingToolbox>(DeferredShadingToolbox(Settings));
		std::cout << "Tobox 6.\n";
		if (Settings.ShadowLevel > SettingLevel::SETTING_NONE)
			AddTb<ShadowMappingToolbox>(ShadowMappingToolbox(Settings));
		std::cout << "Tobox 7.\n";
		if (!Settings.DrawToWindowFBO)
			AddTb<FinalRenderTargetToolbox>(FinalRenderTargetToolbox(Settings));

		AddTb<MainFramebufferToolbox>(MainFramebufferToolbox(Settings, GetTb<DeferredShadingToolbox>()));
		std::cout << "Tobox 8.\n";
		AddTb<ComposedImageStorageToolbox>(ComposedImageStorageToolbox(Settings));
		std::cout << "Tobox 9.\n";
	}
	Shader* FindShader(std::string name) const;
	const GameSettings::VideoSettings& GetSettings()
	{
		return Settings;
	}
	void Dispose()
	{
		for (auto& tb : Tbs)
			tb->Dispose();

		Tbs.clear();
	}

	friend class RenderEngine;

private:
	template <class T> void DisposeOfTb()
	{
		auto it = Tbs.begin();
		while (it != Tbs.end())
		{
			if (dynamic_cast<T*>(it->get()))
				Tbs.erase(it);

			it++;
		}
	}
	bool ShouldLoadToolbox(RenderToolbox* toolbox, bool loadIfNotPresent)
	{
		if ((toolbox != nullptr && toolbox->IsSetup()) || (!loadIfNotPresent))
			return false;

		return true;
	}

	std::vector<std::shared_ptr<RenderToolbox>> Tbs;

	std::string Name;
	const GameSettings::VideoSettings& Settings;
};