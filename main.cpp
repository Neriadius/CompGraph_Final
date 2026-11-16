// Snow Globe - project skeleton: window + orbit camera + one placeholder sphere.
//
// TODO (core):    glass sphere with specular highlights, snow particles, wooden base
// TODO (stretch): cubemap reflections, warm window lights
// TODO (wow):     hard shake -> blizzard that slowly settles

#include <iostream>
#include <cstdio>
#include <cmath>
#include <filesystem>

#include <glad/glad.h>
#include <GL/freeglut.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "src/Camera.h"
#include "src/Shake.h"
#include "src/FrameContext.h"
#include "src/Scene.h"
#include "src/Snow.h"
#include "src/Globe.h"
#include "src/MusicBox.h"

const int WINDOW_W = 1280;
const int WINDOW_H = 720;

// Globe size; the camera orbits around its centre.
const glm::vec3 GLOBE_CENTER(0.0f, 0.0f, 0.0f);
const float GLOBE_RADIUS = 1.0f;
const float CAMERA_SOFT_RADIUS = 5.0f * GLOBE_RADIUS;   // easing starts here
const float CAMERA_EASE_SPEED = 3.0f;                  // 1/seconds

Camera* camera = nullptr;
Shake shake;
Scene scene;
Snow snow;
Globe globe;
MusicBox musicBox;
float frameDt = 0.0f;
int fpsFrames = 0;
int fpsLastTime = 0;

// ---------------------------------------------------------------
// GLUT callbacks
// ---------------------------------------------------------------
FrameContext MakeFrame(float timeSeconds)
{
	FrameContext frame;
	frame.dt = frameDt;
	frame.time = timeSeconds;
	frame.shakeIntensity = shake.intensity;
	frame.camPos = camera->Position;
	frame.cameraInside = glm::length(camera->Position - GLOBE_CENTER) < GLOBE_RADIUS;
	frame.globeCenter = GLOBE_CENTER;
	frame.globeRadius = GLOBE_RADIUS;

	glm::vec3 jitter(
		std::sin(timeSeconds * 47.0f),
		std::sin(timeSeconds * 61.0f),
		std::sin(timeSeconds * 53.0f));
	frame.globeModel = glm::translate(glm::mat4(1.0f), jitter * (0.03f * shake.intensity));
	frame.daylight = scene.Lights().daylight;          // I key: day / night (Person 3)
	frame.keyLightDir = scene.Lights().moonDir;

	return frame;
}

void display()
{
	float time = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
	FrameContext frame = MakeFrame(time);

	glm::vec3 background = BackgroundColor(frame.daylight);
	glClearColor(background.r, background.g, background.b, 1.0f);
	// Restore depth writes so glClear clears the depth buffer.
	glDepthMask(GL_TRUE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Glass is drawn back faces -> snow -> front faces so it works outside and inside the globe.
	scene.DrawOpaque(frame, *camera);
	globe.DrawGlass(GlassPass::Back, frame, *camera);
	snow.Draw(frame, *camera);
	globe.DrawGlass(GlassPass::Front, frame, *camera);

	glutSwapBuffers();

	// Count rendered frames; refresh the title twice per second.
	++fpsFrames;
	int now = glutGet(GLUT_ELAPSED_TIME);
	if (fpsLastTime == 0) fpsLastTime = now;
	int elapsed = now - fpsLastTime;
	if (elapsed >= 500)
	{
		float fps = 1000.0f * fpsFrames / elapsed;
		char title[96];
		std::snprintf(title, sizeof(title), "Snow Globe | FPS: %.1f", fps);
		glutSetWindowTitle(title);
		fpsFrames = 0;
		fpsLastTime = now;
	}
}

void reshape(int w, int h)
{
	if (h == 0) h = 1;
	glViewport(0, 0, w, h);
	camera->Resize(w, h);
	glutPostRedisplay();
}

void idle()
{
	static int lastTime = glutGet(GLUT_ELAPSED_TIME);
	int currentTime = glutGet(GLUT_ELAPSED_TIME);
	float dt = (currentTime - lastTime) / 1000.0f;
	if (dt > 0.1f) dt = 0.1f;
	lastTime = currentTime;
	frameDt = dt;

	shake.Update(dt);
	musicBox.Update(shake.count, dt);       // key sound on each shake, music after the 3rd
	camera->Update(dt);                  // frame-rate-independent camera easing
	camera->ClampToSphere(GLOBE_CENTER, CAMERA_SOFT_RADIUS, CAMERA_EASE_SPEED, dt);
	FrameContext frame = MakeFrame(currentTime / 1000.0f);
	scene.Update(frame);                    // interior: fire flicker, light fades
	snow.Update(frame);
	glutPostRedisplay();
}

// Arrow keys: orbit angle and height
void specialKeys(int key, int, int)
{
	if (camera->SpecialInputs(key))
		glutPostRedisplay();
}

void specialKeysUp(int key, int, int)
{
	camera->SpecialUp(key);
}

// C / P / WASD / +- are handled by the camera, ESC quits
void keyboard(unsigned char key, int, int)
{
	if (key == 27) { glutLeaveMainLoop(); return; }

	if (key == ' ')
		shake.Impulse(0.6f);

	if (scene.Inputs(key))                  // L = hut lights, F = campfire
		return;

	if (camera->Inputs(key))
		glutPostRedisplay();
}

void keyboardUp(unsigned char key, int, int)
{
	camera->KeyUp(key);
}

// Mouse: RIGHT button drag = look around (free camera).
// LEFT button is kept free for shaking the globe.
bool lookDragging = false;
bool leftDragging = false;
int lastX = 0, lastY = 0;
int lastLeftX = 0, lastLeftY = 0;

void mouse(int button, int state, int x, int y)
{
	if (button == GLUT_RIGHT_BUTTON)
	{
		lookDragging = (state == GLUT_DOWN);
		lastX = x; lastY = y;
	}
	if (button == GLUT_LEFT_BUTTON)
	{
		leftDragging = (state == GLUT_DOWN);
		lastLeftX = x; lastLeftY = y;
		if (state == GLUT_DOWN)
			shake.Impulse(0.5f);
	}
}

void motion(int x, int y)
{
	if (lookDragging)
	{
		camera->MouseLook(x - lastX, y - lastY);
		glutPostRedisplay();
	}
	if (leftDragging)
	{
		float dx = (float)(x - lastLeftX);
		float dy = (float)(y - lastLeftY);
		shake.Impulse(0.002f * std::sqrt(dx * dx + dy * dy));
		lastLeftX = x; lastLeftY = y;
	}
	lastX = x; lastY = y;
}

// ---------------------------------------------------------------
int main(int argc, char** argv)
{
	glutInit(&argc, argv);

	// Run from the exe's folder so "shaders/..." is always found.
	{
		std::error_code ec;
		std::filesystem::path exeDir = std::filesystem::absolute(argv[0], ec).parent_path();
		if (!exeDir.empty())
			std::filesystem::current_path(exeDir, ec);
	}

	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);
	glutSetOption(GLUT_MULTISAMPLE, 4); // Request four samples; actual support depends on driver.
	glutInitWindowSize(WINDOW_W, WINDOW_H);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Snow Globe");

	if (!gladLoadGL())
	{
		std::cerr << "Failed to initialize glad" << std::endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);
	GLint actualSamples = 0;
	glGetIntegerv(GL_SAMPLES, &actualSamples);
	std::cout << "MSAA samples: " << actualSamples << std::endl;

	camera = new Camera(WINDOW_W, WINDOW_H, GLOBE_CENTER, GLOBE_RADIUS);
	scene.Init();
	musicBox.Init("audio/music_box.mp3", "audio/winding_key.mp3");
	snow.Init();
	globe.Init(GLOBE_RADIUS);

	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutKeyboardUpFunc(keyboardUp);
	glutSpecialFunc(specialKeys);
	glutSpecialUpFunc(specialKeysUp);
	glutIgnoreKeyRepeat(1); // Let Camera::Update() handle held keys.
	glutMouseFunc(mouse);
	glutMotionFunc(motion);

	std::cout << "SPACE / left mouse: shake the globe (3 shakes start the music box)\n"
		<< "C: orbit / free camera   P: projection   ESC: quit\n"
		<< "L: hut lights on/off   F: campfire on/off   I: day / night\n"
		<< "ORBIT: arrows = angle/height, +/- = zoom\n"
		<< "FREE : hold WASD to move, Q/E down/up, right-mouse drag or arrows = look, +/- = speed" << std::endl;

	glutMainLoop();
	musicBox.Shutdown();
	return 0;
}
