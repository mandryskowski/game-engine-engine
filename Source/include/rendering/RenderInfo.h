#pragma once
#include <math/Vec.h>
namespace GEE
{
	class RenderToolboxCollection;

	class MatrixInfo
	{
	public:
		MatrixInfo() : CamPosition(Vec3f(0.0f)), View(1.0f), Projection(1.0f), VP(1.0f) {}

		const Vec3f& GetCamPosition() const { return CamPosition; }
		const Mat4f& GetView() const { return View; }
		const Mat4f& GetProjection() const { return Projection; }
		const Mat4f& GetVP() const { return VP; }

		void SetCamPosition(const Vec3f& pos) { CamPosition = pos; }
		void SetView(const Mat4f& view) { View = view; }
		void SetProjection(const Mat4f& proj) { Projection = proj; }
		void CalculateVP() { VP = Projection * View; }
	private:
		Vec3f CamPosition;
		Mat4f View;
		Mat4f Projection;
		Mat4f VP;
	};

	class MatrixInfoExt : public MatrixInfo
	{
	public:
		void SetUseMaterials(bool useMaterials) { bUseMaterials = useMaterials; }
		void SetOnlyShadowCasters(bool onlyShadowCasters) { bOnlyShadowCasters = onlyShadowCasters; }
		void SetCareAboutShader(bool careAboutShader) { bCareAboutShader = careAboutShader; }
		void SetMainPass(bool mainPass) { bMainPass = mainPass; }

		bool GetUseMaterials() const { return bUseMaterials; }
		bool GetOnlyShadowCasters() const { return bOnlyShadowCasters; }
		bool GetCareAboutShader() const { return bCareAboutShader; }
		bool GetMainPass() const { return bMainPass; }

	private:
		bool bUseMaterials;
		bool bOnlyShadowCasters;
		bool bCareAboutShader;
		bool bMainPass;
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

		RenderInfo(RenderToolboxCollection& tbCollection, const Mat4f& v = Mat4f(1.0f), const Mat4f& p = Mat4f(1.0f), const Mat4f& vp = Mat4f(1.0f), const Vec3f& camPos = Vec3f(0.0f), bool materials = true, bool onlyshadow = false, bool careAboutShader = false, bool mainPass = false, bool allowBlending = true);
		Mat4f CalculateVP();
	};
}