#include "libs.h"
#include "Shader.h"
#include "Pi.h"
#include "sbre/sbre_int.h"
#include "WorldView.h"

namespace Shader {

// 4 vertex programs per enum VertexProgram, for 1,2,3,4 lights
GLuint vtxprog[VPROG_MAX*4];
bool isEnabled = false;
GLint activeProgram = 0;

bool IsEnabled() { return isEnabled; }
bool IsVtxProgActive() { return activeProgram != 0; }

void ToggleState()
{
	if (GLEW_VERSION_2_0 && !isEnabled) isEnabled = true;
	else isEnabled = false;
	printf("Vertex shaders %s.\n", isEnabled ? "on" : "off");
}

void DisableVertexProgram()
{
	if (!isEnabled) return;
	activeProgram = 0;
	glUseProgram(0);
}

void EnableVertexProgram(VertexProgram p)
{
	if (!isEnabled) return;
	activeProgram = vtxprog[4*(int)p + Pi::worldView->GetNumLights() - 1];
	glUseProgram(activeProgram);
}
	
GLint GetActiveProgram()
{
	return activeProgram;
}

char *load_file(const char *filename)
{
	FILE *f = fopen(filename, "r");
	if (!f) {
		printf("Could not open %s.\n", filename);
		Pi::Quit();
	}
	fseek(f, 0, SEEK_END);
	size_t len = ftell(f);
	fseek(f, 0, SEEK_SET);
	char *buf = (char*)malloc(sizeof(char) * (len+1));
	fread(buf, 1, len, f);
	fclose(f);
	buf[len] = 0;
	return buf;
}


static void printLog(const char *filename, GLuint obj)
{
	int infologLength = 0;
	char infoLog[1024];

	if (glIsShader(obj))
		glGetShaderInfoLog(obj, 1024, &infologLength, infoLog);
	else
		glGetProgramInfoLog(obj, 1024, &infologLength, infoLog);

	if (infologLength > 0)
		printf("%s: %s", filename, infoLog);
}

static void CompileProgram(VertexProgram p, const char *filename)
{
	static char *universal_code = 0;
	if (!universal_code) universal_code = load_file("data/shaders/_library.vert.glsl");

	char *code = load_file(filename);
	for (int i=0; i<4; i++) {
		GLuint vs = glCreateShader(GL_VERTEX_SHADER);
		const char *shader_src[3];
		char lightDef[128];
		snprintf(lightDef, sizeof(lightDef), "#define NUM_LIGHTS %d\n", i+1);
		shader_src[0] = lightDef;
		shader_src[1] = universal_code;
		shader_src[2] = code;
		glShaderSource(vs, 3, shader_src, 0);
		glCompileShader(vs);
		GLint status;
		glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
		if (!status) {
			printLog(filename, vs);
			Pi::Quit();
		}

		GLuint prog = glCreateProgram();
		glAttachShader(prog, vs);
		glLinkProgram(prog);
		glGetProgramiv(prog, GL_LINK_STATUS, &status);
		if (!status) {
			printLog(filename, prog);
			Pi::Quit();
		}
		{
			/* zbuffer values are (where z = z*modelview*projection):
			 * depthvalue = log(C*z + 1) / log(C*zfar + 1)
			 * using C = 1.0 */
			glUseProgram(prog);
			GLint invLogZfarPlus1Loc = glGetUniformLocation(prog, "invLogZfarPlus1");
			assert(invLogZfarPlus1Loc);
			float znear, zfar;
			Pi::worldView->GetNearFarClipPlane(&znear, &zfar);
			const float invDenominator = 1.0/log2(zfar + 1.0);

			glUniform1f(invLogZfarPlus1Loc, invDenominator);
			glUseProgram(0);
		}

		vtxprog[4*(int)p + i] = prog;
	}
	free(code);
}


void Init()
{
	isEnabled = GLEW_VERSION_2_0;
	fprintf(stderr, "OpenGL 2.0+: %s\n", isEnabled ? "Yes" : "No");
	if (!isEnabled) return;

	CompileProgram(VPROG_GEOSPHERE, "data/shaders/geosphere.vert.glsl");
	CompileProgram(VPROG_SBRE, "data/shaders/sbre.vert.glsl");
	CompileProgram(VPROG_SIMPLE, "data/shaders/simple.vert.glsl");
	CompileProgram(VPROG_POINTSPRITE, "data/shaders/pointsprite.vert.glsl");
	CompileProgram(VPROG_PLANETHORIZON, "data/shaders/planethorizon.vert.glsl");
#if 0	
	{
		/* zbuffer values are (where z = z*modelview*projection):
		 * depthvalue = log(C*z + 1) / log(C*zfar + 1)
		 * using C = 1.0 */
		float znear, zfar;
		Pi::worldView->GetNearFarClipPlane(&znear, &zfar);
		const float invDenominator = 1.0/log2(zfar + 1.0);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 0, invDenominator, 1.0, 0.0, 0.0);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 1, SBRE_AMB, SBRE_AMB, SBRE_AMB, 1.0);
	}
	CompileProgram(VPROG_GEOSPHERE, geosphere_prog);
	CompileProgram(VPROG_SBRE, sbre_prog);
	CompileProgram(VPROG_SIMPLE, simple_prog);
	CompileProgram(VPROG_POINTSPRITE, pointsprite_prog);
#endif	
	DisableVertexProgram();
}

} /* namespace Shader */ 

