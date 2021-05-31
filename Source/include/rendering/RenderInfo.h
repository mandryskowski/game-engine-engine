#pragma once
#include <glm/glm.hpp>
namespace GEE
{
	class RenderToolboxCollection;

	class RenderInfo
	{
	public:
		RenderToolboxCollection& TbCollection;
		glm::vec3 camPos;
		glm::mat4 view;
		glm::mat4 projection;
		glm::mat4 VP;
		glm::mat4 previousFrameView;
		bool UseMaterials;
		bool OnlyShadowCasters;
		bool CareAboutShader;
		bool MainPass;

		RenderInfo(RenderToolboxCollection& tbCollection, const glm::mat4& v = glm::mat4(1.0f), const glm::mat4& p = glm::mat4(1.0f), const glm::mat4& vp = glm::mat4(1.0f), const glm::vec3& camPos = glm::vec3(0.0f), bool materials = true, bool onlyshadow = false, bool careAboutShader = false, bool mainPass = false);
		glm::mat4 CalculateVP();
	};
}