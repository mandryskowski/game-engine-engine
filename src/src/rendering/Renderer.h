#pragma once
#include <game/GameManager.h>
#include <rendering/Viewport.h>
#include <rendering/Texture.h>
#include <rendering/RenderToolbox.h>
#include <UI/Font.h>

namespace GEE
{
	class LightProbe;
	struct LightProbeTextureArrays;
	class LocalLightProbe;
	class RenderableVolume;
	class BoneComponent;
	class TextComponent;
	class SkeletonBatch;

	class MatrixInfo;
	class MatrixInfoExt;
	class SceneMatrixInfo;

	template<typename> class TbInfo;

	class SMAAToolbox;
	class GaussianBlurToolbox;
	class ComposedImageStorageToolbox;

	typedef unsigned int RenderingContextID;

	namespace GEE_FB { enum class Axis; }

	namespace BindingsGL
	{
		extern const Mesh* BoundMesh;
		extern Shader* UsedShader;
		extern const Material* BoundMaterial;
	}

	enum class RendererShaderHint
	{
		DefaultForward,
		Quad,
		DepthOnly,
		DepthOnlyLinearized,
		IBL,

		// Postprocessing enums
		TonemapDefault,
		SMAAEdge,
		SSAO,
		GaussianBlur
	};

	

	struct Renderer
	{
	protected:
		struct ImplUtil;
	public:
		Renderer(RenderEngineManager&, const GEE_FB::Framebuffer* optionalFramebuffer = nullptr);
		Renderer(const ImplUtil&);
		Renderer(const Renderer&);
		Renderer(Renderer&&);

		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) = delete;

		// Utility rendering function
		void StaticMeshInstances(const MatrixInfoExt& info, const std::vector<MeshInstance>& meshes, const Transform& transform, Shader& shader, bool billboard = false); //Note: this function does not call the Use method of passed Shader. Do it manually.

	protected:
		struct ImplUtil
		{
			ImplUtil(RenderEngineManager& engineHandle, const GEE_FB::Framebuffer* optionalFramebuffer = nullptr)
				:	RenderHandle(engineHandle), OptionalFramebuffer(optionalFramebuffer) {}
			Mat4f GetCubemapView(GEE_FB::Axis cubemapSide);
			Mesh& GetBasicShapeMesh(EngineBasicShape);
			Shader& GetShader(MaterialShaderHint);
			Shader& GetShader(RendererShaderHint);

			struct MeshBinding
			{
				static void Set(const Mesh*);
				static const Mesh* Get();
			private:
				static const Mesh* BoundMesh;
			};

			RenderEngineManager& RenderHandle;
			const GEE_FB::Framebuffer* OptionalFramebuffer;
		} Impl;
	};

	struct CubemapRenderer : public Renderer
	{
		using Renderer::Renderer;

		void TextureToCubemap(RenderingContextID, const Texture& targetCubemap, const Texture& texture, Shader& shader, unsigned int layer = std::numeric_limits<unsigned int>::max(), unsigned int mipLevel = 0);
		void FromTexture(RenderingContextID, Texture targetTex, Texture tex, Vec2u size, Shader& shader, int* layer = nullptr, int mipLevel = 0);
		void FromTexture(Texture targetTex, Texture tex, Vec2u size, Shader& shader, int* layer = nullptr, int mipLevel = 0)
		{
			FromTexture(0, targetTex, tex, size, shader, layer, mipLevel);
		}
		void FromScene(SceneMatrixInfo info, GEE_FB::Framebuffer target, GEE_FB::FramebufferAttachment targetTex, Shader* shader = nullptr, int* layer = nullptr, bool fullRender = false);
	};

	struct VolumeRenderer : public Renderer
	{
		using Renderer::Renderer;

		void Volume(const MatrixInfoExt&, EngineBasicShape, Shader&, const Transform & = Transform());
		void Volumes(const SceneMatrixInfo&, const std::vector<UniquePtr<RenderableVolume>>&, bool bIBLPass);

	};

	struct ShadowMapRenderer : public Renderer
	{
		using Renderer::Renderer;

		void ShadowMaps(RenderingContextID, RenderToolboxCollection& tbCollection, GameSceneRenderData& sceneRenderData, std::vector<std::reference_wrapper<LightComponent>>);
	};

	struct SceneRenderer : public Renderer
	{
		using Renderer::Renderer;
		/**
		 * @brief Called once before the render loop runs. Might be really expensive; this runs only once anyways.
		*/
		void PreRenderLoopPassStatic(RenderingContextID, std::vector<GameSceneRenderData*>& renderDatas);
		/**
		 * @brief Called per frame. Does the run-time operations required to render a single frame of a scene (e.g. render missing shadow maps).
		 * @param renderingContext The OpenGL context to be used.
		 * @param tbCollection: Toolbox collection of the scene.
		 * @param sceneRenderData: Render data of the scene.
		*/
		void PreFramePass(RenderingContextID renderingContext, RenderToolboxCollection& tbCollection, GameSceneRenderData& sceneRenderData);

		/**
		 * @brief Called per frame. Does the run-time operations required to render a single frame of a scene (e.g. render missing shadow maps).
		 * @param tbCollection: Toolbox collection of the scene.
		 * @param sceneRenderData: Render data of the scene.
		*/
		void PreFramePass(RenderToolboxCollection& tbCollection, GameSceneRenderData& sceneRenderData)
		{
			PreFramePass(0, tbCollection, sceneRenderData);
		}

		void FullRender(SceneMatrixInfo& info, Viewport = Viewport(Vec2f(0.0f), Vec2f(0.0f)), bool clearMainFB = true, bool modifyForwardsDepthForUI = false, std::function<void(GEE_FB::Framebuffer&)>&& renderIconsFunc = nullptr, bool forceForwardShading = false);	//This method renders a scene with lighting and some postprocessing that improve the visual quality (e.g. SSAO, if enabled).
		void RawRender(const SceneMatrixInfo& info, Shader& shader);
		void RawUIScene(const SceneMatrixInfo& info);
	};


	struct PostprocessRenderer : public Renderer
	{
		template<typename ToolboxType> class PPToolbox
		{
		public:
			PPToolbox(RenderingContextID contextID, PostprocessRenderer& postprocessRef, RenderToolboxCollection& collection) : ContextID(contextID), PostprocessRef(postprocessRef), TbCollection(collection), Tb(*collection.GetTb<ToolboxType>()) {}
			ToolboxType& GetTb() { return Tb; }
			void RenderFullscreenQuad(Shader* shader, bool useShader = true) { PostprocessRef.RenderFullscreenQuad(ContextID, TbCollection, shader); }	//Pass nullptr to render with default Quad shader (which just renders the texture at slot 0)

		private:
			RenderingContextID ContextID;
			PostprocessRenderer& PostprocessRef;
			RenderToolboxCollection& TbCollection;	//for rendering quad
			ToolboxType& Tb;
		};

		PostprocessRenderer(RenderEngineManager& renderHandle, const GEE_FB::Framebuffer& requiredFramebuffer)
			: Renderer(renderHandle, &requiredFramebuffer) {}
		void Init(GameManager* gameHandle, Vec2u resolution);

		unsigned int GetFrameIndex();

		Mat4f GetJitterMat(const GameSettings::VideoSettings& usedSettings, int optionalIndex = -1);	//dont pass anything to receive the current jitter matrix

		template <typename ToolboxType> PPToolbox<ToolboxType> GetPPToolbox(RenderingContextID contextID, RenderToolboxCollection& toolboxCol)
		{
			return PPToolbox<ToolboxType>(contextID, *this, toolboxCol);
		}

		Texture GaussianBlur(PPToolbox<GaussianBlurToolbox> tb,
			const Viewport* viewport, const Texture& tex, int passes,
			unsigned int writeColorBuffer = 0);

		Texture SSAO(SceneMatrixInfo&, const Texture& gPosition, const Texture& gNormal);

		Texture SMAA(PPToolbox<SMAAToolbox> tb,
			const Viewport* viewport, const Texture& colorTex, const Texture& depthTex,
			const Texture& previousColorTex = Texture(), const Texture& velocityTex = Texture(),
			unsigned int writeColorBuffer = 0, bool bT2x = false);

		Texture TonemapGamma(PPToolbox<ComposedImageStorageToolbox> tbCollection,
			const Viewport* viewport, const Texture& colorTex, const Texture& blurTex);	//converts from linear to gamma and from HDR data to LDR

		void Render(RenderingContextID contextID, RenderToolboxCollection& tbCollection, const Viewport* viewport, const Texture& colorTex, Texture blurTex = Texture(), const Texture& depthTex = Texture(), const Texture& velocityTex = Texture());

		void RenderFullscreenQuad(RenderingContextID contextID, RenderToolboxCollection& tbCollection, Shader* shader = nullptr, bool useShader = true);
		void RenderFullscreenQuad(TbInfo<MatrixInfoExt>& info, Shader* shader = nullptr, bool useShader = true);


	private:
		PostprocessRenderer ToFramebuffer(const GEE_FB::Framebuffer& framebuffer) const { return PostprocessRenderer(Impl.RenderHandle, framebuffer); }
	};

	class LightProbeRenderer : public Renderer
	{
	public:
		using Renderer::Renderer;

		void AllSceneProbes(GameSceneRenderData& renderData)
		{
			AllSceneProbes(0, renderData);
		}
		void AllSceneProbes(RenderingContextID, GameSceneRenderData&);
	};

	class SkeletalMeshRenderer : public Renderer
	{
	public:
		using Renderer::Renderer;

		void SkeletalMeshInstances(const MatrixInfoExt& info, const std::vector<MeshInstance>& meshes, SkeletonInfo& skelInfo, const Transform& transform, Shader& shader);
	};

	class TextRenderer : public Renderer
	{
	public:
		using Renderer::Renderer;

		void RenderText(const SceneMatrixInfo& info, const Font::Variation&, std::string content, Transform t = Transform(), Vec3f color = Vec3f(1.0f), Shader* shader = nullptr, bool convertFromPx = false, Alignment2D = Alignment2D::LeftCenter()); //Pass a shader if you do not want the default shader to be used.
	};

	class GameRenderer : public Renderer
	{
	public:
		using Renderer::Renderer;

		virtual void RenderPass(GameScene* mainScene, GameScene* editorScene, GameScene* menuScene, GameScene* meshPreviewScene) = 0;

		void PrepareFrame(GameScene* mainScene);
		void EnsureShadowMapCorectness(RenderingContextID contextID, RenderToolboxCollection& tbCollection, GameSceneRenderData* sceneRenderData);
	};


	class PhysicsDebugRenderer : public Renderer
	{
	public:
		PhysicsDebugRenderer(RenderEngineManager&, const GEE_FB::Framebuffer* optionalFramebuffer = nullptr);
		PhysicsDebugRenderer(const ImplUtil&);

		void DebugRender(Physics::GameScenePhysicsData& scenePhysicsData, SceneMatrixInfo& info);
		virtual ~PhysicsDebugRenderer();
	private:
		void BoundSceneDebug(SceneMatrixInfo&, GLenum mode, GLint first, GLint count, Vec3f color = Vec3f(1.0f));

		unsigned int VAO, VBO;
	};
}