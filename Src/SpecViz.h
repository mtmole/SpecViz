#pragma once

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <vector>

#ifdef _MSC_VER
#include <windows.h>
#endif

#include "FreeImage.h"

#include "gl/glew.h"

#define GLM_PRECISION_MEDIUMP_INT
#define GLM_PRECISION_HIGHP_FLOAT
#define GLM_FORCE_RADIANS
#include "../glm/glm/glm.hpp"
#include "../glm/glm/gtc/matrix_transform.hpp"

class Viewer {
public:
	virtual void MainLoop(float deltaTime) = 0;
	virtual void NotifyKeyPress(const char* name) = 0;
	virtual void NotifyMouseWheel(float amt, bool controlHeld) {}
	virtual void NotifyMouseDrag(float x, float y, uint32_t button, bool controlHeld) = 0;
	virtual void Save(const char* toFile) {}
	virtual ~Viewer() {}
};

float GetAspectRatio();
void OutputDebug(const char* buff);
Viewer* CreateModelViewer(const char* fileName);
Viewer* CreateCreateProjViewer(const char* textureFile, const char* modelFile);
Viewer* CreateNormalMapViewer(const char* normalMap, const char* colorMap);
Viewer* CreateProjViewer(const char* projFile);
Viewer* CreateMultiProjViewer(std::vector<const char*>& filenames);

#define Log(...) { char buff[512]; sprintf_s(buff, 512, __VA_ARGS__); OutputDebug(buff); }

inline int32_t mini(int32_t a, int32_t b) {
	if (a < b) return a;
	return b;
}

inline int32_t maxi(int32_t a, int32_t b) {
	if (a > b) return a;
	return b;
}

#include "Graphics.h"