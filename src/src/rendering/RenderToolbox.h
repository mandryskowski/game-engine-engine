#pragma once
#include <map>
#include <game/GameSettings.h>

namespace GEE
{
	class NamedTexture;

	namespace GEE_FB
	{
		class Framebuffer;
	}

	class Texture;
	enum class MaterialShaderHint;
	class Shader;
	class RenderEngineManager;

	namespace Editor
	{
		class EditorRenderer;
	}

	class ShaderFromHintGetter
	{
	public:
		ShaderFromHintGetter(RenderEngineManager& renderHandle);
		Shader* GetShader(MaterialShaderHint hint);
	private:
		void SetShader(MaterialShaderHint hint, Shader* shader);
		friend class RenderToolbox;
		std::map<MaterialShaderHint, Shader*> ShaderHintMap;
	};



	class RenderToolbox
	{
	public:
		RenderToolbox(ShaderFromHintGetter& shaders) : ShaderGetter(&shaders) {}
		virtual bool IsSetup();
		Shader* FindShader(const String& name);

		void Dispose();
	protected:
		GEE_FB::Framebuffer* AddFramebuffer();
		Shader* AddShader(const SharedPtr<Shader>& shader);
		Texture* AddTexture(const Texture&);
		void SetShaderHint(MaterialShaderHint, Shader*);

		std::vector<SharedPtr<GEE_FB::Framebuffer>> Fbs;
		std::vector<SharedPtr<Shader>> Shaders;
		std::vector<SharedPtr<Texture>> Textures;

		ShaderFromHintGetter* ShaderGetter;
		friend class RenderToolboxCollection;
	};

	class ForwardShadingToolbox : public RenderToolbox
	{
	public:
		ForwardShadingToolbox(ShaderFromHintGetter&, const GameSettings::VideoSettings& settings);
		void Setup(const GameSettings::VideoSettings& settings);

		friend class MainFramebufferToolbox;
		friend class RenderEngine;
		friend struct SceneRenderer;
	private:
		std::vector<Shader*> LightShaders;
	};

	class DeferredShadingToolbox : public RenderToolbox
	{
	public:
		DeferredShadingToolbox(ShaderFromHintGetter&, const GameSettings::VideoSettings& settings);
		void Setup(const GameSettings::VideoSettings& settings);

		const GEE_FB::Framebuffer* GetGeometryFramebuffer() const { return GFb; }

		friend class MainFramebufferToolbox;
		friend struct SceneRenderer;
		friend class RenderEngine;
		friend class LightProbeRenderer;
	private:
		GEE_FB::Framebuffer* GFb;

		std::vector<Shader*> LightShaders;
		Shader* GeometryShader;
	};

	class MainFramebufferToolbox : public RenderToolbox
	{
	public:
		MainFramebufferToolbox(ShaderFromHintGetter&, const GameSettings::VideoSettings& settings, DeferredShadingToolbox* deferredTb = nullptr);
		void Setup(const GameSettings::VideoSettings& settings, DeferredShadingToolbox* deferredTb = nullptr);	//Pass a DeferredShadingToolbox to reuse some textures and save memory.

		friend struct SceneRenderer;
	private:
		GEE_FB::Framebuffer* MainFb;
	};

	class SSAOToolbox : public RenderToolbox
	{
	public:
		SSAOToolbox(ShaderFromHintGetter&, const GameSettings::VideoSettings& settings);
		void Setup(const GameSettings::VideoSettings& settings);

		friend struct PostprocessRenderer;

	private:
		GEE_FB::Framebuffer* SSAOFb;

		Shader* SSAOShader;

		Texture* SSAONoiseTex;
	};

	class PrevFrameStorageToolbox : public RenderToolbox
	{
	public:
		PrevFrameStorageToolbox(ShaderFromHintGetter&, const GameSettings::VideoSettings& settings);
		void Setup(const GameSettings::VideoSettings& settings);

		friend struct PostprocessRenderer;
	private:
		GEE_FB::Framebuffer* StorageFb; //two color buffers; before rendering to FBO 0 we render to one of its color buffers (we switch them each frame) so as to gain access to the final color buffer in the next frame (for temporal AA)
	};

	class SMAAToolbox : public RenderToolbox
	{
	public:
		SMAAToolbox(ShaderFromHintGetter&, const GameSettings::VideoSettings& settings);
		void Setup(const GameSettings::VideoSettings& settings);

		friend struct PostprocessRenderer;
	private:
		GEE_FB::Framebuffer* SMAAFb; //three color buffers; we use the first two color buffers as edgeTex&blendTex in SMAA pass and optionally the last one to store the neighborhood blend result for reprojection

		Shader* SMAAShaders[4];

		Texture* SMAAAreaTex, * SMAASearchTex;
	};

	class ComposedImageStorageToolbox : public RenderToolbox
	{
	public:
		ComposedImageStorageToolbox(ShaderFromHintGetter&, const GameSettings::VideoSettings& settings);
		void Setup(const GameSettings::VideoSettings& settings);

				friend struct PostprocessRenderer;
	private:
		GEE_FB::Framebuffer* ComposedImageFb;	// Contains 2 color buffers on slot 0 and 1 so that you can render them ping-pong.

		Shader* TonemapGammaShader;
	};

	class GaussianBlurToolbox : public RenderToolbox
	{
	public:
		GaussianBlurToolbox(ShaderFromHintGetter&, const GameSettings::VideoSettings& settings);
		void Setup(const GameSettings::VideoSettings& settings);

		friend struct PostprocessRenderer;
	private:
		GEE_FB::Framebuffer* BlurFramebuffers[2]; //only color buffer; we use them as ping pong framebuffers in gaussian blur pass.
											//Side note: I wonder which method is quicker - switching a framebuffer or switching an attachment or switching a buffer to draw to via glDrawBuffer.
											//Side note 2: I've heard that they are similar in terms of performance. So now I wonder why do we use ping-pong framebuffers and not ping-pong color buffers. They should work the same, shouldn't they?

		Shader* GaussianBlurShader;
	};

	class ShadowMappingToolbox : public RenderToolbox
	{
	public:
		ShadowMappingToolbox(ShaderFromHintGetter&, const GameSettings::VideoSettings& settings);
		void Setup(const GameSettings::VideoSettings& settings);
		friend class RenderEngine;
		friend struct ShadowMapRenderer;
		friend class GameEngineEngineEditor;
	private:
		GEE_FB::Framebuffer* ShadowFramebuffer;
		Texture* ShadowMapArray, * ShadowCubemapArray;
	};

	class FinalRenderTargetToolbox : public RenderToolbox
	{
	public:
		FinalRenderTargetToolbox(ShaderFromHintGetter&, const GameSettings::VideoSettings& settings);
		void Setup(const GameSettings::VideoSettings&);
		GEE_FB::Framebuffer& GetFinalFramebuffer();
		friend class RenderEngine;
	private:
		UniquePtr<NamedTexture> RenderTarget;
		GEE_FB::Framebuffer* FinalFramebuffer;
	};

	class RenderToolboxCollection
	{
	public:
		/**
		 * @brief Constructor of RenderToolboxCollection.
		 * @param name: the name of this RenderToolboxCollection
		 * @param settings: settings to be used for rendering. DO NOT pass a local variable that is about to be destroyed or you will encounter UB. (should be allocated on the heap)
		*/
		RenderToolboxCollection(const std::string& name, const GameSettings::VideoSettings& settings, RenderEngineManager& renderHandle);
		RenderToolboxCollection(const RenderToolboxCollection&);

		std::string GetName() const;
		template <typename T> T* GetTb() const;

		template <typename T, typename... Args> void AddTb(Args&&... args);
		virtual void AddTbsRequiredBySettings();

		Shader* GetShaderFromHint(MaterialShaderHint hint) { return ShaderGetter->GetShader(hint); }
		const Shader* GetShaderFromHint(MaterialShaderHint hint) const { return ShaderGetter->GetShader(hint); }

		Shader* FindShader(std::string name) const
		{
			for (int i = 0; i < static_cast<int>(Tbs.size()); i++)
				if (Shader* shader = Tbs[i]->FindShader(name))
					return shader;

			return nullptr;
		}
		const GameSettings::VideoSettings& GetVideoSettings() const;
		void Dispose();

		friend class RenderEngine;

	private:
		template <typename T> void DisposeOfTb();

		static bool ShouldLoadToolbox(RenderToolbox* toolbox, bool loadIfNotPresent);

		std::vector<SharedPtr<RenderToolbox>> Tbs;
		UniquePtr<ShaderFromHintGetter> ShaderGetter;

		std::string Name;
		const GameSettings::VideoSettings& Settings;
	};

	template<typename T>
	T* RenderToolboxCollection::GetTb() const
	{
		for (int i = 0; i < Tbs.size(); i++)
			if (T* castedObj = dynamic_cast<T*>(Tbs[i].get()))
				return castedObj;

		return nullptr;
	}
	template<typename T, typename... Args>
	void RenderToolboxCollection::AddTb(Args&&... args)
	{
		if (GetTb<T>())
		{
			std::cerr << "WARNING! A toolbox collection contains two toolboxes of the same type.\n";
			DisposeOfTb<T>();
		}

		Tbs.push_back(MakeShared<T>(*ShaderGetter, Settings, std::forward<Args>(args)...));
	}
	template<typename T>
	void RenderToolboxCollection::DisposeOfTb()
	{
		auto it = Tbs.begin();
		while (it != Tbs.end())
		{
			if (dynamic_cast<T*>(it->get()))
				Tbs.erase(it);

			it++;
		}
	}
}
