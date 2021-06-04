#pragma once
#include <math/Vec.h>
namespace GEE
{
	class RenderToolboxCollection;

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

		RenderInfo(RenderToolboxCollection& tbCollection, const Mat4f& v = Mat4f(1.0f), const Mat4f& p = Mat4f(1.0f), const Mat4f& vp = Mat4f(1.0f), const Vec3f& camPos = Vec3f(0.0f), bool materials = true, bool onlyshadow = false, bool careAboutShader = false, bool mainPass = false);
		Mat4f CalculateVP();
	};
}