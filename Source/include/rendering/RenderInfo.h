#pragma once
#include <math/Vec.h>
namespace GEE
{
	class RenderToolboxCollection;
	class GameSceneRenderData;

	/**
	 * @brief Basic MatrixInfo which serves as a rendering context. Accepted by rendering functions which can make use of camera information.
	*/
	class MatrixInfo
	{
	public:
		MatrixInfo(const Mat4f& view = Mat4f(1.0f), const Mat4f& projection = Mat4f(1.0f), const Vec3f& camPos = Vec3f(0.0f)) : CamPosition(camPos), View(view), Projection(projection), VP(projection * view) {}

		const Vec3f& GetCamPosition() const { return CamPosition; }
		const Mat4f& GetView() const { return View; }
		const Mat4f& GetProjection() const { return Projection; }
		const Mat4f& GetVP() const { return VP; }

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
	};

	/**
	 * @brief Extended MatrixInfo, also serves as a rendering context. Accepted by rendering functions which can make use of camera information and rendering settings.
	*/
	class MatrixInfoExt : public MatrixInfo
	{
		void DefaultBools()
		{
			bUseMaterials = true;
			bOnlyShadowCasters = false;
			bCareAboutShader = false;
			bMainPass = false;
			bAllowBlending = true;
		}
	public:
		MatrixInfoExt(const MatrixInfo& info)
			: MatrixInfo(info) { DefaultBools(); }
		MatrixInfoExt(const Mat4f& view = Mat4f(1.0f), const Mat4f& projection = Mat4f(1.0f), const Vec3f& camPos = Vec3f(0.0f))
			: MatrixInfo(view, projection, camPos) { DefaultBools(); }
		void SetUseMaterials(bool useMaterials) { bUseMaterials = useMaterials; }
		void SetOnlyShadowCasters(bool onlyShadowCasters) { bOnlyShadowCasters = onlyShadowCasters; }
		void SetCareAboutShader(bool careAboutShader) { bCareAboutShader = careAboutShader; }
		void SetMainPass(bool mainPass) { bMainPass = mainPass; }
		void SetAllowBlending(bool allowBlending) { bAllowBlending = allowBlending; }

		bool GetUseMaterials() const { return bUseMaterials; }
		bool GetOnlyShadowCasters() const { return bOnlyShadowCasters; }
		bool GetCareAboutShader() const { return bCareAboutShader; }
		bool GetMainPass() const { return bMainPass; }
		bool GetAllowBlending() const { return bAllowBlending; }

	private:
		bool bUseMaterials, bOnlyShadowCasters, bCareAboutShader, bMainPass, bAllowBlending;
	};

	/**
	 * @brief A MatrixInfo which can be used to render a scene. Provides access to toolboxes, thus allowing the use of lighting, postprocessing algorithms, etc.
	*/
	class SceneMatrixInfo : public MatrixInfoExt
	{
	public:
		SceneMatrixInfo(RenderToolboxCollection& tbCollection, GameSceneRenderData& sceneRenderData,
						const Mat4f& view = Mat4f(1.0f), const Mat4f& projection = Mat4f(1.0f), const Vec3f& camPos = Vec3f(0.0f))
			: MatrixInfoExt(view, projection, camPos), TbCollection(tbCollection), SceneRenderData(sceneRenderData) {}

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

	class RenderInfo
	{
	public:
		RenderToolboxCollection& TbCollection;
		Vec3f camPos;
		Mat4f view;
		Mat4f projection;
		Mat4f VP;
		Mat4f previousFrameView;
		bool UseMaterials;
		bool OnlyShadowCasters;
		bool CareAboutShader;
		bool MainPass;

		bool AllowBlending;

		RenderInfo(RenderToolboxCollection& tbCollection, const Mat4f& v = Mat4f(1.0f), const Mat4f& p = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f), const Mat4f& vp = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f), const Vec3f& camPos = Vec3f(0.0f), bool materials = true, bool onlyshadow = false, bool careAboutShader = false, bool mainPass = false, bool allowBlending = true);
		Mat4f CalculateVP();
	};
}