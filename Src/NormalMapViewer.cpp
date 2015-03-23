
#include "SpecViz.h"
#include "PlyModel.h"

class NormalMapViewer : public Viewer {
public:
	PixelShader* pShader;
	VertexShader* vShader;
	ShaderProgram* program;
	glm::mat4 viewMatrix;
	glm::mat4 projMatrix;
	glm::vec3 lightDirection;

	float lightPitch;
	float lightYaw;

	VertexBuffer* vBuffer;
	IndexBuffer* iBuffer;
	VAO* vao;
	
	Texture* normalTexture;
	Texture* colorTexture;

	NormalMapViewer(const char* normalFile, const char* colorFile);
	void MainLoop(float deltaTime);
	void NotifyKeyPress(const char* name);
	void NotifyMouseDrag(float x, float y, uint32_t button, bool controlHeld);
	virtual ~NormalMapViewer();
};

Viewer* CreateNormalMapViewer(const char* normalMap, const char* colorMap) {
	NormalMapViewer* newViewer = new NormalMapViewer(normalMap, colorMap);
	return newViewer;
}

NormalMapViewer::NormalMapViewer(const char* normalFile, const char* colorFile) {
	GLCHECK();

	lightPitch = 1.0f;
	lightYaw = 0.0f;

	int32_t major = 0;
	int32_t minor = 0;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	printf("GL %d.%d", major, minor);

	pShader = new PixelShader("Shaders/world_normal.pix");
	vShader = new VertexShader("Shaders/passthrough.vert");
	program = new ShaderProgram(pShader, vShader);
	
	normalTexture = Texture::CreateFromFile(normalFile, GL_RGBA8);
	colorTexture = Texture::CreateFromFile(colorFile, GL_RGBA8);

	float aspect = normalTexture->GetAspect();

	struct vPos {
		float pos[3];
		float uv[2];
	};
	vPos vData[] = {
		-aspect,-1,0, 0,0,
		 aspect,-1,0, 1,0,
		-aspect, 1,0, 0,1,
		 aspect, 1,0, 1,1,
	};
	vBuffer = new VertexBuffer(vData, sizeof(vData));

	uint32_t iData[] = {
		0,1,2,2,1,3
	};
	iBuffer = new IndexBuffer(iData, sizeof(iData), GL_TRIANGLES);

	vao = new VAO(vBuffer, iBuffer);
	vao->EnableArrays(2);
}

void NormalMapViewer::MainLoop(float deltaTime) {
	GLCHECK();

	// set up z write/read
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	GLCHECK();

	// clear frame buffer
	glClearColor(0,0,0,1);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GLCHECK();

	// bind program
	program->Bind();
	GLCHECK();
	
	// set up to just fit the quad
	projMatrix = glm::infinitePerspective(60.0f * glm::pi<float>() / 180.0f, GetAspectRatio(), 0.01f);
	float cameraDistance = 2.0f;
	float rotation = glm::pi<float>() * 0.5f;
	glm::vec3 eyePosition = glm::vec3(cosf(rotation) * cameraDistance, 0.0f, sinf(rotation) * cameraDistance);
	viewMatrix = glm::lookAt(
		eyePosition,
		glm::vec3(0,0,0),
		glm::vec3(0,1,0)
	);

	float pitchVar = 1.0f - sin(lightPitch);
	lightDirection = glm::normalize(glm::vec3(cos(lightYaw) * pitchVar, sin(lightPitch), sin(lightYaw) * pitchVar));
	
	GLCHECK();
	glUniformMatrix4fv(program->GetUniform("projMatrix"), 1, GL_FALSE, &projMatrix[0][0]);
	glUniformMatrix4fv(program->GetUniform("viewMatrix"), 1, GL_FALSE, &viewMatrix[0][0]);
	glUniform3fv(program->GetUniform("lightDirection"), 1, &lightDirection.x);
	glUniform3fv(program->GetUniform("eyePosition"), 1, &eyePosition.x);
	glUniform1i(program->GetUniform("normalMap"), 0);
	glUniform1i(program->GetUniform("colorMap"), 1);
	GLCHECK();

	// bind texture
	normalTexture->Bind(0);
	colorTexture->Bind(1);

	// draw our quad
	vao->Bind();
	glDrawElements(iBuffer->GetType(), iBuffer->GetCount(), GL_UNSIGNED_INT, (void*) 0);
	GLCHECK();
}

void NormalMapViewer::NotifyKeyPress(const char* name) {
	// does nothing
}

void NormalMapViewer::NotifyMouseDrag(float x, float y, uint32_t button, bool controlHeld) {
	const float p = glm::pi<float>();

	if (button == 2) {
		lightPitch += y * 0.01f;
		lightYaw += x * 0.01f;

		if (lightPitch > p * 0.5f) {
			lightPitch = p * 0.5f;
		}
		if (lightPitch < 0.0f) {
			lightPitch = 0.0f;
		}

		if (lightYaw > p * 2.0f) lightYaw -= p * 2.0f;
		if (lightYaw < 0.0f) lightYaw += p * 2.0f;

		Log("Light angle now : yaw: %.3f pitch : %.3f", lightYaw, lightPitch);
	}
}

NormalMapViewer::~NormalMapViewer() {
	delete program;
	GLCHECK();
	delete pShader;
	GLCHECK();
	delete vShader;
	GLCHECK();
	delete vao;
	GLCHECK();
	delete vBuffer;
	GLCHECK();
	delete iBuffer;
	GLCHECK();
	delete normalTexture;
	GLCHECK();
	
	glClearColor(1,1,1,1);
	glClear(GL_COLOR_BUFFER_BIT);
}