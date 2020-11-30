#pragma once
#include "Utility.h"

enum ShadingModel
{
	SHADING_FULL_LIT,
	SHADING_PHONG,
	SHADING_PBR_COOK_TORRANCE
};

enum AntiAliasingType
{
	AA_NONE,
	AA_SMAA1X,
	AA_SMAAT2X
};

enum SettingLevel
{
	SETTING_NONE,
	SETTING_LOW,
	SETTING_MEDIUM,
	SETTING_HIGH,
	SETTING_ULTRA
};

struct GameSettings
{
	glm::uvec2 WindowSize;

	bool bWindowFullscreen;
	std::string WindowTitle;

	struct VideoSettings
	{
		glm::vec2 Resolution;
		ShadingModel Shading;

		unsigned int AmbientOcclusionSamples;
		bool bVSync;
		bool bBloom;
		AntiAliasingType AAType;
		SettingLevel AALevel;
		SettingLevel POMLevel;
		SettingLevel ShadowLevel;

		float MonitorGamma;

		VideoSettings();
		bool IsVelocityBufferNeeded() const;
		bool IsTemporalReprojectionEnabled() const;
		std::string GetShaderDefines(glm::uvec2 resolution = glm::uvec2(0)) const;
		virtual bool LoadSetting(std::stringstream& filestr, std::string settingName);
	} Video;

	/////////////////////////////

	GameSettings();
	GameSettings(std::string path);

	void LoadFromFile(std::string path);

	virtual bool LoadSetting(std::stringstream& filestr, std::string settingName);
};


struct Dupa : public GameSettings
{
	struct LOL : public GameSettings::VideoSettings
	{

	} Video;
};

template <class T> void LoadEnum(std::stringstream& filestr, T& var);