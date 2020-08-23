#pragma once
#include "Texture.h"
#include "GameManager.h"
#include "Transform.h"
#include "RenderableVolume.h"

struct LightProbeTextureArrays
{
	static Texture IrradianceMapArr;
	static Texture PrefilterMapArr;
	static Texture BRDFLut;

	static unsigned int NextLightProbeNr;
};

class LightProbe
{
protected:
	RenderEngineManager* RenderHandle;
public:
	unsigned int ProbeNr;
	Texture EnvironmentMap;

	LightProbe();
	virtual EngineBasicShape GetShape() const;
	virtual Transform GetTransform() const;
	virtual Shader* GetRenderShader() const;
};

class LocalLightProbe : public LightProbe
{
public:
	EngineBasicShape Shape;
	Transform ProbeTransform;

	LocalLightProbe(Transform = Transform(), EngineBasicShape = EngineBasicShape::CUBE);
	virtual EngineBasicShape GetShape() const override;
	virtual Transform GetTransform() const override;
};

struct LightProbeLoader
{
	static RenderEngineManager* RenderHandle;
	static std::shared_ptr<LightProbe> LightProbeFromFile(std::string path);
	static void ConvoluteLightProbe(const LightProbe&, Texture* envMap = nullptr);	//Note: this light probes should have an environment map located at LightProbe::ProbeNr of texture array LightProbeTextureArrays::EnvironmentMapArr
	static void LoadLightProbeTextureArrays();	//Important: Call this function before loading any light probes!
};

class LightProbeVolume: public RenderableVolume
{
	const LightProbe* ProbePtr;
public:
	LightProbeVolume(const LightProbe& probe);
	virtual EngineBasicShape GetShape() const override;
	virtual Transform GetRenderTransform() const override;
	virtual Shader* GetRenderShader() const override;
	virtual void SetupRenderUniforms(const Shader& shader) const override;
};