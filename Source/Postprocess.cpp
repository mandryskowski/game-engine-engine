#include "Postprocess.h"


Postprocess::Postprocess()
{
	float data[] = {
		-1.0f, -1.0f,
		 1.0f, -1.0f,
		 1.0f,  1.0f,
		 1.0f,  1.0f,
		-1.0f,  1.0f,
		-1.0f, -1.0f,

		 0.0f,  0.0f,
		 1.0f,  0.0f,
		 1.0f,  1.0f,
		 1.0f,  1.0f,
		 0.0f,  1.0f,
		 0.0f,  0.0f,
	};
	QuadVAO = generateVAO(QuadVBO, std::vector<unsigned int> {2, 2}, 6, data);


	PPShader.LoadShaders("postprocess.vs", "postprocess.fs");
	PPShader.Use();
	PPShader.Uniform1i("HDRbuffer", 0);
	PPShader.Uniform1i("brightnessBuffer", 1);

	GaussianBlurShader.LoadShaders("gaussianblur.vs", "gaussianblur.fs");
	GaussianBlurShader.Use();
	GaussianBlurShader.Uniform1i("tex", 0);
}

void Postprocess::LoadSettings(const GameSettings* settings)
{
	//load gamma
	PPShader.Use();
	PPShader.Uniform1f("gamma", settings->MonitorGamma);

	//load framebuffers
	for (int i = 0; i < 2; i++)
		PingPongFramebuffers[i].Load(settings->WindowSize, ColorBufferData(true, true, GL_LINEAR, GL_LINEAR));
}

void Postprocess::Render(unsigned int colorBuffer, unsigned int brightnessBuffer)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	PPShader.Use();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, colorBuffer);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, brightnessBuffer);
	PPShader.Uniform1i("HDRbuffer", 0);

	glBindVertexArray(QuadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

unsigned int Postprocess::GaussianBlur(unsigned int tex, int passes)
{
	GaussianBlurShader.Use();
	glBindVertexArray(QuadVAO);
	glActiveTexture(GL_TEXTURE0);

	bool horizontal = true;

	for (int i = 0; i < passes; i++)
	{
		PingPongFramebuffers[horizontal].Bind();
		GaussianBlurShader.Uniform1i("horizontal", horizontal);
		glBindTexture(GL_TEXTURE_2D, (i == 0) ? (tex) : (PingPongFramebuffers[!horizontal].GetColorBuffer(0).OpenGLBuffer));
		glDrawArrays(GL_TRIANGLES, 0, 6);

		horizontal = !horizontal;
	}
	return PingPongFramebuffers[!horizontal].GetColorBuffer(0).OpenGLBuffer;
}