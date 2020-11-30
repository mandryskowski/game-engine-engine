#include "RenderInfo.h"

RenderInfo::RenderInfo(RenderToolboxCollection& tbCollection, const glm::mat4& v, const glm::mat4& p, const glm::mat4& vp, const glm::vec3& camPos, bool materials, bool onlyshadow, bool careAboutShader, bool mainPass) :
	TbCollection(tbCollection),
	camPos(camPos),
	view(v),
	projection(p),
	VP(vp),
	previousFrameView(glm::mat4(1.0f)),
	UseMaterials(materials),
	OnlyShadowCasters(onlyshadow),
	CareAboutShader(careAboutShader),
	MainPass(mainPass)
{
	CalculateVP();
}
glm::mat4 RenderInfo::CalculateVP()
{
	VP = projection * view;
	return VP;
}