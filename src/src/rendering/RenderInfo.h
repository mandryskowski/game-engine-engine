#pragma once
#include <math/Vec.h>
#include <rendering/Material.h>
namespace GEE
{
	class RenderToolboxCollection;
	class GameSceneRenderData;
	enum class MaterialShaderHint;

	typedef unsigned int RenderingContextID;

	/**
	 * @brief Basic MatrixInfo which serves as a rendering context. Accepted by rendering functions which can make use of camera information.
	 * Context info optional - applications with multiple windows will probably have to use it.
	*/
	class MatrixInfo
	{
	public:
		MatrixInfo(const Mat4f& view = Mat4f(1.0f), const Mat4f& projection = Mat4f(1.0f), const Vec3f& camPos = Vec3f(0.0f), unsigned int contextID = 0) : CamPosition(camPos), View(view), Projection(projection), VP(projection * view), ContextID(contextID) {}
		MatrixInfo(unsigned int contextID, const Mat4f& view = Mat4f(1.0f), const Mat4f& projection = Mat4f(1.0f), const Vec3f& camPos = Vec3f(0.0f)) : CamPosition(camPos), View(view), Projection(projection), VP(projection * view), ContextID(contextID) {}

		const Vec3f& GetCamPosition() const { return CamPosition; }
		const Mat4f& GetView() const { return View; }
		const Mat4f& GetProjection() const { return Projection; }
		const Mat4f& GetVP() const { return VP; }
		RenderingContextID GetContextID() const { return ContextID; }

		void SetCamPosition(const Vec3f& pos) { CamPosition = pos; }
		void SetView(const Mat4f& view) { View = view; }
		void SetProjection(const Mat4f& proj) { Projection = proj; }
		/**
		 * @brief Forces VP to be the given matrix. For readability reasons, it sets the view matrix to vp and projection matrix to identity.
		 * @param proj 
		*/
		void SetVPArtificially(const Mat4f& vp) { View = vp; Projection = Mat4f(1.0f); CalculateVP(); }
		void CalculateVP() { VP = Projection * View; }
	private:
		Vec3f CamPosition;
		Mat4f View;
		Mat4f Projection;
		Mat4f VP;
		RenderingContextID ContextID;
	};

	/**
	 * @brief Extended MatrixInfo, also serves as a rendering context. Accepted by rendering functions which can make use of camera information and rendering settings.
	*/
	class MatrixInfoExt : public MatrixInfo
	{
		void DefaultBools();
	public:
		MatrixInfoExt(const MatrixInfo& info)
			: MatrixInfo(info), RequiredShaderInfo(MaterialShaderHint::None) { DefaultBools(); }
		MatrixInfoExt(const MatrixInfoExt& info)
			: MatrixInfo(info), RequiredShaderInfo(info.RequiredShaderInfo), bUseMaterials(info.bUseMaterials), bOnlyShadowCasters(info.bOnlyShadowCasters), bMainPass(info.bMainPass), bAllowBlending(info.bAllowBlending) { }
		MatrixInfoExt(RenderingContextID contextID, const MatrixInfo& info) :
			MatrixInfoExt(info) {  }
			
		MatrixInfoExt(RenderingContextID contextID, const Mat4f& view = Mat4f(1.0f), const Mat4f& projection = Mat4f(1.0f), const Vec3f& camPos = Vec3f(0.0f))
			: MatrixInfo(view, projection, camPos, contextID), RequiredShaderInfo(MaterialShaderHint::None) { DefaultBools(); }
		MatrixInfoExt(const Mat4f& view = Mat4f(1.0f), const Mat4f& projection = Mat4f(1.0f), const Vec3f& camPos = Vec3f(0.0f))
			: MatrixInfoExt(0, view, projection, camPos) { }

		void SetUseMaterials(bool useMaterials) { bUseMaterials = useMaterials; }
		void SetOnlyShadowCasters(bool onlyShadowCasters) { bOnlyShadowCasters = onlyShadowCasters; }
		void SetRequiredShaderInfo(Material::ShaderInfo info) { RequiredShaderInfo = info; }
		void StopRequiringShaderInfo() { SetRequiredShaderInfo(Material::ShaderInfo(MaterialShaderHint::None)); }
		void SetMainPass(bool mainPass) { bMainPass = mainPass; }
		void SetAllowBlending(bool allowBlending) { bAllowBlending = allowBlending; }

		bool GetUseMaterials() const { return bUseMaterials; }
		bool GetOnlyShadowCasters() const { return bOnlyShadowCasters; }
		bool GetMainPass() const { return bMainPass; }
		bool GetAllowBlending() const { return bAllowBlending; }
		Material::ShaderInfo GetRequiredShaderInfo() const { return RequiredShaderInfo; }

	private:
		bool bUseMaterials, bOnlyShadowCasters, bMainPass, bAllowBlending;
		Material::ShaderInfo RequiredShaderInfo;
	};

	/**
	 * @brief A MatrixInfo which can be used to render a scene. Provides access to toolboxes, thus allowing the use of lighting, postprocessing algorithms, etc.
	*/
	class SceneMatrixInfo : public MatrixInfoExt
	{
	public:
		SceneMatrixInfo(RenderingContextID contextID, RenderToolboxCollection& tbCollection, GameSceneRenderData& sceneRenderData,
			const Mat4f& view = Mat4f(1.0f), const Mat4f& projection = Mat4f(1.0f), const Vec3f& camPos = Vec3f(0.0f))
			: MatrixInfoExt(contextID, view, projection, camPos), TbCollection(tbCollection), SceneRenderData(sceneRenderData) {}

		SceneMatrixInfo(RenderToolboxCollection& tbCollection, GameSceneRenderData& sceneRenderData,
			const Mat4f& view = Mat4f(1.0f), const Mat4f& projection = Mat4f(1.0f), const Vec3f& camPos = Vec3f(0.0f))
			: SceneMatrixInfo(0, tbCollection, sceneRenderData, view, projection, camPos) {}

		RenderToolboxCollection& GetTbCollection() { return TbCollection; }
		const RenderToolboxCollection& GetTbCollection() const { return TbCollection; }

		GameSceneRenderData& GetSceneRenderData() { return SceneRenderData; }
		const GameSceneRenderData& GetSceneRenderData() const { return SceneRenderData; }
	private:
		RenderToolboxCollection& TbCollection;
		GameSceneRenderData& SceneRenderData;
	};

	template <typename InfoType = SceneMatrixInfo>
	class TbInfo : public InfoType
	{
	public:
		TbInfo(RenderToolboxCollection& tbCollection)
			: TbCollection(tbCollection) {}
		TbInfo(RenderToolboxCollection& tbCollection, const InfoType& info)
			: TbCollection(tbCollection), InfoType(info) {}
		template <typename... Args>
		TbInfo(RenderToolboxCollection& tbCollection, Args&&... args)
			: InfoType(std::forward<Args>(args)...), TbCollection(tbCollection) {}
	private:
		RenderToolboxCollection& TbCollection;
	};
}