#ifndef BLUR_PASS_H
#define BLUR_PASS_H

#include <glad/glad.h>

class Shader;

// Menu background: the scene is drawn into an off-screen framebuffer (with the same MSAA
// as the window), shrunk to a quarter, blurred with a separable Gaussian, and drawn back
// mixed with the sharp image.
class BlurPass
{
public:
	bool Init(int width, int height);
	void Resize(int width, int height);
	bool Ready() const { return ready; }

	void BeginScene();                       // draw the 3D scene after this
	void EndScene(float amount, float time); // amount 0 = sharp, 1 = full menu blur

private:
	bool ready = false;
	bool linked = false;
	int width = 0, height = 0, samples = 0;
	int smallW = 1, smallH = 1;

	GLuint msFbo = 0, msColor = 0, msDepth = 0;          // multisampled scene (if MSAA)
	GLuint sceneFbo = 0, sceneTex = 0, sceneDepth = 0;   // resolved scene
	GLuint smallFbo[2] = { 0, 0 }, smallTex[2] = { 0, 0 };
	GLuint emptyVao = 0;

	Shader* downShader = nullptr;
	Shader* blurShader = nullptr;
	Shader* compositeShader = nullptr;

	bool Create();
	void Destroy();
	void Blur(int from, int to, float dx, float dy);
};

#endif
