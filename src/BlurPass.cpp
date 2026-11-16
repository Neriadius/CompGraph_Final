#include "BlurPass.h"

#include <algorithm>
#include <iostream>

#include "shaderClass.h"

namespace
{
const int DOWNSCALE = 4;          // blur at a quarter of the window size
const int BLUR_ITERATIONS = 3;
const float BLUR_STEP = 1.2f;

GLuint MakeTexture(int w, int h, GLint internalFormat, GLenum type)
{
	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, GL_RGBA, type, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
	return tex;
}

bool Linked(const Shader* shader)
{
	GLint ok = GL_FALSE;
	glGetProgramiv(shader->ID, GL_LINK_STATUS, &ok);
	return ok == GL_TRUE;
}

bool Complete(const char* name)
{
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status == GL_FRAMEBUFFER_COMPLETE) return true;
	std::cerr << "BlurPass: framebuffer " << name << " incomplete (0x" << std::hex << status
		<< std::dec << "), menu blur disabled" << std::endl;
	return false;
}
} // namespace

bool BlurPass::Init(int w, int h)
{
	downShader = new Shader("shaders/post.vert", "shaders/post_down.frag");
	blurShader = new Shader("shaders/post.vert", "shaders/post_blur.frag");
	compositeShader = new Shader("shaders/post.vert", "shaders/post_composite.frag");
	compositeShader->Activate();
	glUniform1i(glGetUniformLocation(compositeShader->ID, "sharp"), 0);
	glUniform1i(glGetUniformLocation(compositeShader->ID, "blurred"), 1);

	// Match the window's MSAA so the picture does not change when the blur ends
	GLint windowSamples = 0, maxSamples = 0;
	glGetIntegerv(GL_SAMPLES, &windowSamples);
	glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
	samples = std::min(windowSamples, maxSamples);

	glGenVertexArrays(1, &emptyVao);   // full-screen triangle comes from gl_VertexID
	width = std::max(w, 1);
	height = std::max(h, 1);
	linked = Linked(downShader) && Linked(blurShader) && Linked(compositeShader);
	if (!linked) std::cerr << "BlurPass: blur shaders failed, menu blur disabled" << std::endl;
	ready = linked && Create();
	return ready;
}

void BlurPass::Resize(int w, int h)
{
	if (!downShader || (w == width && h == height)) return;
	Destroy();
	width = std::max(w, 1);
	height = std::max(h, 1);
	ready = linked && Create();
}

bool BlurPass::Create()
{
	bool ok = true;
	smallW = std::max(width / DOWNSCALE, 1);
	smallH = std::max(height / DOWNSCALE, 1);

	glGenFramebuffers(1, &sceneFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, sceneFbo);
	sceneTex = MakeTexture(width, height, GL_RGBA8, GL_UNSIGNED_BYTE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTex, 0);
	if (samples == 0)
	{
		glGenRenderbuffers(1, &sceneDepth);
		glBindRenderbuffer(GL_RENDERBUFFER, sceneDepth);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, sceneDepth);
	}
	ok = ok && Complete("scene");

	if (samples > 0)
	{
		glGenFramebuffers(1, &msFbo);
		glBindFramebuffer(GL_FRAMEBUFFER, msFbo);
		glGenRenderbuffers(1, &msColor);
		glBindRenderbuffer(GL_RENDERBUFFER, msColor);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msColor);
		glGenRenderbuffers(1, &msDepth);
		glBindRenderbuffer(GL_RENDERBUFFER, msDepth);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, msDepth);
		ok = ok && Complete("multisample");
	}

	for (int i = 0; i < 2; i++)
	{
		glGenFramebuffers(1, &smallFbo[i]);
		glBindFramebuffer(GL_FRAMEBUFFER, smallFbo[i]);
		smallTex[i] = MakeTexture(smallW, smallH, GL_RGBA16F, GL_FLOAT);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, smallTex[i], 0);
		ok = ok && Complete("blur");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	return ok;
}

void BlurPass::Destroy()
{
	glDeleteFramebuffers(1, &msFbo);
	glDeleteRenderbuffers(1, &msColor);
	glDeleteRenderbuffers(1, &msDepth);
	glDeleteFramebuffers(1, &sceneFbo);
	glDeleteTextures(1, &sceneTex);
	glDeleteRenderbuffers(1, &sceneDepth);
	glDeleteFramebuffers(2, smallFbo);
	glDeleteTextures(2, smallTex);
	msFbo = msColor = msDepth = sceneFbo = sceneTex = sceneDepth = 0;
	smallFbo[0] = smallFbo[1] = smallTex[0] = smallTex[1] = 0;
}

void BlurPass::BeginScene()
{
	glBindFramebuffer(GL_FRAMEBUFFER, samples > 0 ? msFbo : sceneFbo);
	glViewport(0, 0, width, height);
}

void BlurPass::Blur(int from, int to, float dx, float dy)
{
	glBindFramebuffer(GL_FRAMEBUFFER, smallFbo[to]);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, smallTex[from]);
	glUniform2f(glGetUniformLocation(blurShader->ID, "blurStep"), dx / smallW, dy / smallH);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void BlurPass::EndScene(float amount, float time)
{
	// Resolve MSAA into the scene texture.
	if (samples > 0)
	{
		glBindFramebuffer(GL_READ_FRAMEBUFFER, msFbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, sceneFbo);
		glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	}

	const GLboolean hadDepth = glIsEnabled(GL_DEPTH_TEST);
	const GLboolean hadBlend = glIsEnabled(GL_BLEND);
	const GLboolean hadCull = glIsEnabled(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glBindVertexArray(emptyVao);

	glViewport(0, 0, smallW, smallH);
	glBindFramebuffer(GL_FRAMEBUFFER, smallFbo[0]);
	downShader->Activate();
	glUniform2f(glGetUniformLocation(downShader->ID, "texel"), 1.0f / width, 1.0f / height);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sceneTex);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	blurShader->Activate();
	const float spacing = BLUR_STEP * amount;
	for (int i = 0; i < BLUR_ITERATIONS; i++)
	{
		Blur(0, 1, spacing, 0.0f);
		Blur(1, 0, 0.0f, spacing);
	}

	// Back to the window: sharp and blurred mixed by the amount, darker towards the edges.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, width, height);
	compositeShader->Activate();
	glUniform1f(glGetUniformLocation(compositeShader->ID, "amount"), amount);
	glUniform1f(glGetUniformLocation(compositeShader->ID, "time"), time);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, smallTex[0]);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sceneTex);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	if (hadDepth) glEnable(GL_DEPTH_TEST);
	if (hadBlend) glEnable(GL_BLEND);
	if (hadCull) glEnable(GL_CULL_FACE);
}
