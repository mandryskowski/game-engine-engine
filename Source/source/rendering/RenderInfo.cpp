#include <rendering/RenderInfo.h>

namespace GEE
{
	RenderInfo::RenderInfo(RenderToolboxCollection& tbCollection, const Mat4f& v, const Mat4f& p, const Mat4f& vp, const Vec3f& camPos, bool materials, bool onlyshadow, bool careAboutShader, bool mainPass, bool allowBlending) :
		TbCollection(tbCollection),
		camPos(camPos),
		view(v),
		projection(p),
		VP(vp),
		previousFrameView(Mat4f(1.0f)),
		UseMaterials(materials),
		OnlyShadowCasters(onlyshadow),
		CareAboutShader(careAboutShader),
		MainPass(mainPass),
		AllowBlending(allowBlending)
	{
		CalculateVP();
	}
	Mat4f RenderInfo::CalculateVP()
	{
		VP = projection * view;
		return VP;
	}
}