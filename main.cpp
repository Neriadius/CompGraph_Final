// Snow Globe - project skeleton: window + orbit camera + one placeholder sphere.
//
// TODO (core):    glass sphere with specular highlights, snow particles, wooden base
// TODO (stretch): cubemap reflections, warm window lights, music box sound
// TODO (wow):     hard shake -> blizzard that slowly settles

#include <iostream>
#include <vector>
#include <cmath>
#include <filesystem>

#include <glad/glad.h>
#include <GL/freeglut.h>

#include "src/shaderClass.h"
#include "src/VAO.h"
#include "src/VBO.h"
#include "src/EBO.h"
#include "src/Camera.h"

const int WINDOW_W = 800;
const int WINDOW_H = 600;

// Globe size; the camera orbits around its centre.
const glm::vec3 GLOBE_CENTER(0.0f, 0.0f, 0.0f);
const float GLOBE_RADIUS = 1.0f;

Camera* camera = nullptr;
Shader* shader = nullptr;
VAO* sphereVAO = nullptr;
GLsizei sphereIndexCount = 0;

// ---------------------------------------------------------------
// Simple UV sphere (position + normal, interleaved). Used as the globe for now.
// ---------------------------------------------------------------
void buildSphere(float radius, int stacks, int slices,
                 std::vector<GLfloat>& vertices, std::vector<GLuint>& indices)
{
	const float PI = 3.14159265358979f;

	for (int i = 0; i <= stacks; i++)
	{
		float phi = PI * i / stacks;                 // 0 (top) .. PI (bottom)
		for (int j = 0; j <= slices; j++)
		{
			float theta = 2.0f * PI * j / slices;
			float nx = std::sin(phi) * std::cos(theta);
			float ny = std::cos(phi);
			float nz = std::sin(phi) * std::sin(theta);

			vertices.insert(vertices.end(), { nx * radius, ny * radius, nz * radius, nx, ny, nz });
		}
	}

	for (int i = 0; i < stacks; i++)
		for (int j = 0; j < slices; j++)
		{
			GLuint a = i * (slices + 1) + j;
			GLuint b = a + slices + 1;
			indices.insert(indices.end(), { a, b, a + 1, a + 1, b, b + 1 });
		}
}

// ---------------------------------------------------------------
// GLUT callbacks
// ---------------------------------------------------------------
void display()
{
	glClearColor(0.05f, 0.06f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shader->Activate();
	camera->Matrix(45.0f, GLOBE_RADIUS * 0.01f, GLOBE_RADIUS * 50.0f, *shader, "camMatrix");

	glm::mat4 model(1.0f);
	glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE, glm::value_ptr(model));

	sphereVAO->Bind();
	glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);

	glutSwapBuffers();
}

void reshape(int w, int h)
{
	if (h == 0) h = 1;
	glViewport(0, 0, w, h);
	camera->Resize(w, h);
	glutPostRedisplay();
}

// Arrow keys: orbit angle and height
void specialKeys(int key, int, int)
{
	if (camera->SpecialInputs(key))
		glutPostRedisplay();
}

// C / P / WASD / +- are handled by the camera, ESC quits
void keyboard(unsigned char key, int, int)
{
	if (key == 27) { glutLeaveMainLoop(); return; }

	// TODO: SPACE = shake the globe

	if (camera->Inputs(key))
		glutPostRedisplay();
}

// Mouse: RIGHT button drag = look around (free camera).
// LEFT button is kept free for shaking the globe.
bool lookDragging = false;
int lastX = 0, lastY = 0;

void mouse(int button, int state, int x, int y)
{
	if (button == GLUT_RIGHT_BUTTON)
	{
		lookDragging = (state == GLUT_DOWN);
		lastX = x; lastY = y;
	}
	// TODO: GLUT_LEFT_BUTTON = shake the globe
}

void motion(int x, int y)
{
	if (lookDragging)
	{
		camera->MouseLook(x - lastX, y - lastY);
		glutPostRedisplay();
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
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(WINDOW_W, WINDOW_H);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Snow Globe");

	if (!gladLoadGL())
	{
		std::cerr << "Failed to initialize glad" << std::endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);

	camera = new Camera(WINDOW_W, WINDOW_H, GLOBE_CENTER, GLOBE_RADIUS);
	shader = new Shader("shaders/default.vert", "shaders/default.frag");

	// Placeholder globe
	std::vector<GLfloat> vertices;
	std::vector<GLuint> indices;
	buildSphere(GLOBE_RADIUS, 32, 48, vertices, indices);
	sphereIndexCount = (GLsizei)indices.size();

	sphereVAO = new VAO();
	sphereVAO->Bind();
	VBO vbo(vertices.data(), (GLsizeiptr)(vertices.size() * sizeof(GLfloat)));
	EBO ebo(indices.data(), (GLsizeiptr)(indices.size() * sizeof(GLuint)));
	sphereVAO->LinkAttrib(vbo, 0, 3, GL_FLOAT, 6 * sizeof(GLfloat), (void*)0);
	sphereVAO->LinkAttrib(vbo, 1, 3, GL_FLOAT, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(specialKeys);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);

	std::cout << "C: orbit / free camera   P: projection   ESC: quit\n"
		<< "ORBIT: arrows = angle/height, +/- = zoom\n"
		<< "FREE : WASD move, Q/E down/up, right-mouse drag or arrows = look, +/- = speed" << std::endl;

	glutMainLoop();
	return 0;
}
