#pragma once

// Global header used for all SpecViz cpp files

// needed stdlib includes
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <vector>

// including windows.h by default.. will need to be defined out for other platforms
#ifdef _MSC_VER
#include <windows.h>
#endif

// FreeImage used for texture loading
#include "FreeImage.h"

// glew used for windows WGL interface
#include "gl/glew.h"

// GLM library includes for vertex/matrix math needs
#define GLM_PRECISION_MEDIUMP_INT
#define GLM_PRECISION_HIGHP_FLOAT
#define GLM_FORCE_RADIANS
#include "../glm/glm/glm.hpp"
#include "../glm/glm/gtc/matrix_transform.hpp"

// abstract viewer object that interacts with the main window
class Viewer {
public:
	virtual void MainLoop(float deltaTime) = 0;
	virtual void NotifyKeyPress(const char* name) = 0;
	virtual void NotifyMouseWheel(float amt, bool controlHeld) {}
	virtual void NotifyMouseDrag(float x, float y, uint32_t button, bool controlHeld) = 0;
	virtual void Save(const char* toFile) {}
	virtual ~Viewer() {}
};

// platform abstracted viewer access to viewport aspect ratio
float GetAspectRatio();

// output the given text as debug output to the platform log
void OutputDebug(const char* buff);

// various model viewer creation functions based on the type of viewer
Viewer* CreateModelViewer(const char* fileName);
Viewer* CreateCreateProjViewer(const char* textureFile, const char* modelFile);
Viewer* CreateNormalMapViewer(const char* normalMap, const char* colorMap);
Viewer* CreateProjViewer(const char* projFile);
Viewer* CreateMultiProjViewer(std::vector<const char*>& filenames);

// creates a depth field image from the given projection file
void CreateDepthField(const char* projFile);

// Log macro uses OutputDebug() function with sprint_f for dynamic log usage
#define Log(...) { char buff[512]; sprintf_s(buff, 512, __VA_ARGS__); OutputDebug(buff); }

// min function using integers
inline int32_t mini(int32_t a, int32_t b) {
	if (a < b) return a;
	return b;
}

// max function using integers
inline int32_t maxi(int32_t a, int32_t b) {
	if (a > b) return a;
	return b;
}

// include OpenGL graphics support class declarations
#include "Graphics.h"