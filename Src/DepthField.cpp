
#include "SpecViz.h"
#include "PlyModel.h"

#include <fstream>

void CreateDepthField(const char* projFile) {
	const uint32_t width = 1024;
	const uint32_t height = 1024;
	const uint32_t numPixels = width * height;

	glm::mat4x4 projMatrix;
	char textureFile[512];
	char modelFile[512];
	std::fstream f(projFile, std::ifstream::in);

	f >> textureFile;
	f >> modelFile;
	
	float* ref = &projMatrix[0].x;
	for (int32_t i = 0; i < 16; i++) {
		f >> *ref;
		ref++;
	}

	f.close();

	PixelShader* modelPShader = new PixelShader("Shaders/solid_color.pix");
	VertexShader* modelVShader = new VertexShader("Shaders/lit_vertex.vert");
	ShaderProgram* modelProgram = new ShaderProgram(modelPShader, modelVShader);

	// texture is used just to get the destination width and height:
	Texture* tex = Texture::CreateFromFile(textureFile, GL_RGBA8);
	uint32_t destWidth = tex->GetWidth();
	uint32_t destHeight = tex->GetHeight();
	delete tex;

	// load up the model for rendering the depths
	PlyModel* model = new PlyModel(modelFile);

	// create a frame buffer
	GLuint saveBuffer = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*) &saveBuffer);		// save the old buffer binding
	GLuint frameBuffer;
	glGenFramebuffers(1, &frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	GLCHECK();

	// create color texture and bind to frame buffer
	glActiveTexture(GL_TEXTURE7);
	GLuint colorTexture = 0;
	glGenTextures(1, &colorTexture);
	glBindTexture(GL_TEXTURE_2D, colorTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	GLCHECK();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
	GLCHECK();

	// create depth texture using pixel buffer and bind to frame buffer
	GLuint depthTexture = 0;
	glGenTextures(1, &depthTexture);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	GLCHECK();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	GLCHECK();

	GLenum val = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	assert(val == GL_FRAMEBUFFER_COMPLETE); 
	
	glViewport(0, 0, width, height);
	GLCHECK();

	// set up z write/read
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	GLCHECK();

	// clear frame buffer
	glClearColor(0,0,0,1);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GLCHECK();

	// bind program
	modelProgram->Bind();
	GLCHECK();

	glm::mat4x4 objMatrix = glm::mat4x4();
	glm::mat4x4 viewMatrix = glm::mat4x4();
	
	glDisable(GL_BLEND);

	GLCHECK();
	glUniformMatrix4fv(modelProgram->GetUniform("objMatrix"), 1, GL_FALSE, &objMatrix[0][0]);
	glUniformMatrix4fv(modelProgram->GetUniform("projMatrix"), 1, GL_FALSE, &projMatrix[0][0]);
	glUniformMatrix4fv(modelProgram->GetUniform("viewMatrix"), 1, GL_FALSE, &viewMatrix[0][0]);
	//glUniform3fv(modelProgram->GetUniform("eyePosition"), 1, &eyePosition.x);
	glUniform1f(modelProgram->GetUniform("alpha"), 1.0f);
	GLCHECK();

	// draw our model
	model->Render();
	GLCHECK();

	glFinish();

	// read depths
	float* depth = new float[numPixels];
	memset(depth, 0, sizeof(float) * numPixels);
	glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, &depth[0]);

	// clean up
 	glBindFramebuffer(GL_FRAMEBUFFER, saveBuffer);
	glDeleteTextures(1, &colorTexture);
	glDeleteTextures(1, &depthTexture);
	glDeleteFramebuffers(1, &frameBuffer);

	delete modelPShader;
	delete modelVShader;
	delete modelProgram;
	delete model;

	// transform depth values into something we can work with

	// normalize infinite perspective matrix (infinite depth becomes -1)
	float maxDepth = 0.0f, minDepth = 1.0E+30F;
	for (uint32_t i = 0; i < numPixels; i++) {
		if (depth[i] == 1.0f) {
			depth[i] = -1.0f;
		} else {
			depth[i] = 1.0f / (1.0f - depth[i]);
			if (depth[i] > maxDepth) {
				maxDepth = depth[i];
			}
			if (depth[i] < minDepth) {
				minDepth = depth[i];
			}
		}
	}

	// normalize again to min to max sampled values to 0-1, infinite depth to 2
	for (uint32_t i = 0; i < numPixels; i++) {
		if (depth[i] == -1.0f) {
			depth[i] = 2.0f;
		} else {
			depth[i] = (depth[i]-minDepth) / (maxDepth - minDepth);
		}
	}

	// TEST: create a png of the resulting depth values:
	uint8_t* colors = new uint8_t[width*height*3];
	for (uint32_t i = 0; i < numPixels; i++) {
		uint8_t d = 255 - (uint8_t) (depth[i] * 127);
		colors[i*3+0] = d;
		colors[i*3+1] = d;
		colors[i*3+2] = d;
	}

	char pngFile[512];
	strcpy(pngFile, projFile);
	strcat(pngFile, ".depth.png");
	Texture::SavePNG(pngFile, width, height, (uint8_t*) colors);

	// make array of depth differences:
	float* depthDiff = new float[width * height];
	const float depthMagnification = 3.0f;
	for (uint32_t i = 0; i < numPixels; i++) {
		float up = (i >= width) ? depth[i-width] : 2.0f;
		float down = (i <= numPixels - width) ? depth[i+width] : 2.0f;
		float left = (i != 0) ? depth[i-1] : 2.0f;
		float right = (i != numPixels-1) ? depth[i+1] : 2.0f;
		float cur = depth[i];
		float best = fabs(up-cur);
		if (fabs(down-cur) > best) best = fabs(down-cur);
		if (fabs(left-cur) > best) best = fabs(left-cur);
		if (fabs(right-cur) > best) best = fabs(right-cur);
		best *= depthMagnification;
		depthDiff[i] = best > 1.0f ? 1.0f : best;
	}
	
	// TEST: create a png of the resulting depth difference values:
	for (uint32_t i = 0; i < numPixels; i++) {
		uint8_t d = (uint8_t) (depthDiff[i] * 255);
		colors[i*3+0] = d;
		colors[i*3+1] = d;
		colors[i*3+2] = d;
	}

	strcpy(pngFile, projFile);
	strcat(pngFile, ".depthdiff.png");
	Texture::SavePNG(pngFile, width, height, (uint8_t*) colors);

	// fill in entire area outside of edge:
	for (uint32_t i = 0; i < numPixels; i++) {
		if (depth[i] == 2.0f) {
			depthDiff[i] = 1.0f;
		}
	}

	// grow out the depth diff by rough manhattan distance
	const uint32_t spread = width / 25;	// 4% spread
	const float pixDist = 1.0f / spread;
	const float pixDistSq = pixDist * 1.4142f;
	for (uint32_t pass = 0; pass < spread; pass++) {
		for (uint32_t i = 0; i < numPixels; i++) {
			float cur = depthDiff[i];
			// udlr
			if (i != 0 && depthDiff[i-1] - pixDist > cur)							cur = depthDiff[i-1] - pixDist;
			if (i != numPixels && depthDiff[i+1] - pixDist > cur)					cur = depthDiff[i+1] - pixDist;
			if (i >= width && depthDiff[i-width] - pixDist > cur)					cur = depthDiff[i-width] - pixDist;
			if (i < numPixels-width && depthDiff[i+width] - pixDist > cur)			cur = depthDiff[i+width] - pixDist;
			// diagonal
			if (i > width && depthDiff[i-width-1] - pixDistSq > cur)				cur = depthDiff[i-width-1] - pixDistSq;
			if (i >= width-1 && depthDiff[i-width+1] - pixDistSq > cur)				cur = depthDiff[i-width+1] - pixDistSq;
			if (i < numPixels-width+1 && depthDiff[i+width-1] - pixDistSq > cur)	cur = depthDiff[i+width-1] - pixDistSq;
			if (i < numPixels-width-1 && depthDiff[i+width+1] - pixDistSq > cur)	cur = depthDiff[i+width+1] - pixDistSq;
			// use depth as scratch
			depth[i] = cur;
		}

		// copy depth (now scratch) back into depthDiff now that pass is over
		memcpy(depthDiff, depth, sizeof(float) * numPixels);
	}
	
	// TEST: create a png of the resulting depth difference values:
	for (uint32_t i = 0; i < numPixels; i++) {
		uint8_t d = 255 - (uint8_t) (depthDiff[i] * 255);
		colors[i*3+0] = d;
		colors[i*3+1] = d;
		colors[i*3+2] = d;
	}

	strcpy(pngFile, projFile);
	strcat(pngFile, ".edgedist.png");
	Texture::SavePNG(pngFile, width, height, (uint8_t*) colors, destWidth, destHeight);
	
	delete[] depthDiff;
	delete[] depth;
	delete[] colors;
}