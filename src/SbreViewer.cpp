#include "libs.h"
#include "sbre/sbre.h"
#include "glfreetype.h"
#include "Gui.h"
#include "collider/GeomTree.h"

static SDL_Surface *g_screen;
static int g_width, g_height;
static int g_mouseMotion[2];
static char g_keyState[SDLK_LAST];
static int g_mouseButton[5];
static int g_model = 0; // sbre model number. set with argc
static float g_zbias;

static GLuint mytexture;

static void PollEvents()
{
	SDL_Event event;

	g_mouseMotion[0] = g_mouseMotion[1] = 0;
	while (SDL_PollEvent(&event)) {
		Gui::HandleSDLEvent(&event);
		switch (event.type) {
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_q) { SDL_Quit(); exit(0); }
				if (event.key.keysym.sym == SDLK_F11) SDL_WM_ToggleFullScreen(g_screen);
				g_keyState[event.key.keysym.sym] = 1;
				break;
			case SDL_KEYUP:
				g_keyState[event.key.keysym.sym] = 0;
				break;
			case SDL_MOUSEBUTTONDOWN:
				g_mouseButton[event.button.button] = 1;
	//			Pi::onMouseButtonDown.emit(event.button.button,
	//					event.button.x, event.button.y);
				break;
			case SDL_MOUSEBUTTONUP:
				g_mouseButton[event.button.button] = 0;
	//			Pi::onMouseButtonUp.emit(event.button.button,
	//					event.button.x, event.button.y);
				break;
			case SDL_MOUSEMOTION:
				g_mouseMotion[0] += event.motion.xrel;
				g_mouseMotion[1] += event.motion.yrel;
				break;
			case SDL_QUIT:
				SDL_Quit();
				exit(0);
				break;
		}
	}
}

static int g_wheelMoveDir = -1;
static float g_wheelPos = 0;
static bool g_renderCollMesh = false;
static float lightCol[4] = { 1,1,1,0 };
static float lightDir[4] = { 0,1,0,0 };
static float g_frameTime;
static ObjParams params = {
	{ 0.5, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f },

	{	// pColor[3]
	{ { 1.0f, 0.0f, 1.0f }, { 0, 0, 0 }, { 0, 0, 0 }, 0 },
	{ { 0.8f, 0.6f, 0.5f }, { 0, 0, 0 }, { 0, 0, 0 }, 0 },
	{ { 0.5f, 0.5f, 0.5f }, { 0, 0, 0 }, { 0, 0, 0 }, 0 } },

	// pText[3][256]	
	{ "IR-L33T", "ME TOO" },
};

static void SetSbreParams()
{
	float gameTime = SDL_GetTicks() * 0.001;
	params.pAnim[ASRC_SECFRAC] = gameTime;
	params.pAnim[ASRC_MINFRAC] = gameTime / 60;
	params.pAnim[ASRC_HOURFRAC] = gameTime / 3600.0f;
	params.pAnim[ASRC_DAYFRAC] = gameTime / (24*3600.0f);
	if (g_wheelPos <= 0) {
		params.pAnim[ASRC_GEAR] = 0;
		params.pFlag[AFLAG_GEAR] = 0;
	} else {
		params.pAnim[ASRC_GEAR] = g_wheelPos;
		params.pFlag[AFLAG_GEAR] = 1;
	}

	// 2 seconds to move wheels
	g_wheelPos += 0.5*g_frameTime*g_wheelMoveDir;
	if (g_wheelPos < 0) g_wheelPos = 0;
	if (g_wheelPos > 1) g_wheelPos = 1;
}

class Viewer: public Gui::Fixed {
public:
	Viewer(): Gui::Fixed(g_width, g_height) {
		Gui::Screen::AddBaseWidget(this, 0, 0);
		SetTransparency(true);
		{
			Gui::ToggleButton *b = new Gui::ToggleButton();
			b->SetShortcut(SDLK_c, KMOD_NONE);
			b->onChange.connect(sigc::mem_fun(*this, &Viewer::OnToggleCollMesh));
			Add(b, 10, 10);
			Add(new Gui::Label("[c] Show collision mesh"), 30, 10);
		} {
			Gui::Button *b = new Gui::SolidButton();
			b->SetShortcut(SDLK_g, KMOD_NONE);
			b->onClick.connect(sigc::mem_fun(*this, &Viewer::OnToggleGearState));
			Add(b, 10, 30);
			Add(new Gui::Label("[g] Toggle gear state"), 30, 30);
		}

		ShowAll();
		Show();
	}

	void OnToggleGearState() {
		if (g_wheelMoveDir == -1) g_wheelMoveDir = +1;
		else g_wheelMoveDir = -1;
	}

	void OnToggleCollMesh(Gui::ToggleButton *b, bool state) {
		g_renderCollMesh = state;
	}

	void MainLoop();
};

static void render_coll_mesh(const CollMesh *m)
{
	glDisable(GL_LIGHTING);
	glColor3f(1,0,1);
	glBegin(GL_TRIANGLES);
	glDepthRange(0.0+g_zbias,1.0);
	for (int i=0; i<m->ni; i+=3) {
		glVertex3fv(&m->pVertex[3*m->pIndex[i+1]]);
		glVertex3fv(&m->pVertex[3*m->pIndex[i]]);
		glVertex3fv(&m->pVertex[3*m->pIndex[i+2]]);
	}
	glEnd();
	glColor3f(1,1,1);
	glDepthRange(0,1.0f-g_zbias);
	for (int i=0; i<m->ni; i+=3) {
		glBegin(GL_LINE_LOOP);
		glVertex3fv(&m->pVertex[3*m->pIndex[i]]);
		glVertex3fv(&m->pVertex[3*m->pIndex[i+1]]);
		glVertex3fv(&m->pVertex[3*m->pIndex[i+2]]);
		glEnd();
	}
	glDepthRange(0,1);
	glEnable(GL_LIGHTING);
}

float wank[512][512];
float aspectRatio = 1.0;
float camera_zoom = 1.0;
static void raytraceCollMesh(vector3f camPos, vector3f camera_up, vector3f camera_forward, GeomTree *geomtree)
{
	memset(wank, 0, sizeof(float)*512*512);

	vector3f toPoint, xMov, yMov;

	vector3f topLeft, topRight, botRight, cross;
	topLeft = topRight = botRight = camera_forward * camera_zoom;
	cross = vector3f::Cross (camera_forward, camera_up) * aspectRatio;
	topLeft = topLeft + camera_up - cross;
	topRight = topRight + camera_up + cross;
	botRight = botRight - camera_up + cross;

	xMov = topRight - topLeft;
	yMov = botRight - topRight;
	float xstep = 1.0f / 512;
	float ystep = 1.0f / 512;
	float xpos, ypos;
	ypos = 0.0f;

	Uint32 t = SDL_GetTicks();
	for (int y=0; y<512; y++, ypos += ystep) {
		xpos = 0.0f;
		for (int x=0; x<512; x++, xpos += xstep) {
			toPoint = topLeft + (xMov * xpos) + (yMov * ypos);
			toPoint.Normalize ();
			toPoint *= 10000;
			
			isect_t isect;
			geomtree->TraceRay(camPos, toPoint, &isect);

			if (isect.triIdx != -1) {
				wank[x][y] = 10.0/isect.dist;
			} else {
				wank[x][y] = 0;
			}
		}
	}
	printf("%.3f million rays/sec\n", (512*512)/(1000.0*(SDL_GetTicks()-t)));
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 512, 512, 0, GL_LUMINANCE, GL_FLOAT, wank);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glBindTexture(GL_TEXTURE_2D, mytexture);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glBegin(GL_TRIANGLE_FAN);
		glTexCoord2i(1,1);
		glVertex3f(1,1,0);
		glTexCoord2i(0,1);
		glVertex3f(0,1,0);
		glTexCoord2i(0,0);
		glVertex3f(0,0,0);
		glTexCoord2i(1,0);
		glVertex3f(1,0,0);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	printf("done..\n");
}

void Viewer::MainLoop()
{
	matrix4x4d rot = matrix4x4d::Identity();
	float distance = 100;
	Uint32 lastTurd = SDL_GetTicks();

	CollMesh *cmesh = (CollMesh*)calloc(1, sizeof(CollMesh));
	sbreGenCollMesh (cmesh, g_model, &params, 1.0f);

	Uint32 t = SDL_GetTicks();
	GeomTree *geomtree = new GeomTree(cmesh->ni/3, cmesh->pVertex, cmesh->pIndex);
	printf("Geom tree build in %dms\n", SDL_GetTicks() - t);

	for (;;) {
		PollEvents();

		if (g_keyState[SDLK_UP]) rot = matrix4x4d::RotateXMatrix(g_frameTime) * rot;
		if (g_keyState[SDLK_DOWN]) rot = matrix4x4d::RotateXMatrix(-g_frameTime) * rot;
		if (g_keyState[SDLK_LEFT]) rot = matrix4x4d::RotateYMatrix(g_frameTime) * rot;
		if (g_keyState[SDLK_RIGHT]) rot = matrix4x4d::RotateYMatrix(-g_frameTime) * rot;
		if (g_keyState[SDLK_EQUALS]) distance *= pow(0.5,g_frameTime);
		if (g_keyState[SDLK_MINUS]) distance *= pow(2.0,g_frameTime);
		if (g_mouseButton[1] || g_mouseButton[3]) {
			float rx = 0.01*g_mouseMotion[1];
			float ry = 0.01*g_mouseMotion[0];
			rot = matrix4x4d::RotateXMatrix(rx) * rot;
			rot = matrix4x4d::RotateYMatrix(ry) * rot;
		}

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		// why the hell do i give these functions such big names..
		float fracH = g_height / (float)g_width;
		glFrustum(-1, 1, -fracH, fracH, 1.0f, 10000.0f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
		glPushAttrib(GL_ALL_ATTRIB_BITS);
		if (!g_renderCollMesh) {
		SetSbreParams();
		sbreSetViewport(g_width, g_height, g_width*0.5, 1.0f, 10000.0f, 0.0f, 1.0f);
		sbreSetDirLight (lightCol, lightDir);
	
		Matrix m;
		Vector p;
		m.x1 = rot[0]; m.x2 = rot[4]; m.x3 = rot[8];
		m.y1 = rot[1]; m.y2 = rot[5]; m.y3 = rot[9];
		m.z1 = rot[2]; m.z2 = rot[6]; m.z3 = rot[10];
		p.x = 0; p.y = 0; p.z = distance;
		if (g_renderCollMesh) {
			glPushMatrix();
			glTranslatef(p.x, p.y, p.z);
			glMultMatrixd(&rot[0]);
			render_coll_mesh(cmesh);
			glPopMatrix();
			//sbreRenderCollMesh(cmesh, &p, &m);
		}
		else sbreRenderModel(&p, &m, g_model, &params);
		
		} else {
			vector3d _p = rot * vector3d(0,0,-distance);
			vector3f camPos(_p.x, _p.y, _p.z);
			vector3f forward = -vector3f::Normalize(camPos);
			vector3f up = vector3f::Cross(vector3f(0,1,0), forward);
			up.Normalize();
			raytraceCollMesh(camPos, up, forward, geomtree);
		}

		glPopAttrib();
		
		Gui::Draw();
		
		SDL_GL_SwapBuffers();
		g_frameTime = (SDL_GetTicks() - lastTurd) * 0.001;
		lastTurd = SDL_GetTicks();
	}
	delete geomtree;
}

int main(int argc, char **argv)
{
	if ((argc>1) && (0==strcmp(argv[1],"--help"))) {
		printf("Usage:\n\nSbreViewer <model number> <width> <height>\n");
		exit(0);
	}
	if (argc > 1) {
		g_model = atoi(argv[1]);
	}
	if (argc == 4) {
		g_width = atoi(argv[2]);
		g_height = atoi(argv[3]);
	} else {
		g_width = 800;
		g_height = 600;
	}

	const SDL_VideoInfo *info = NULL;
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Video initialization failed: %s\n", SDL_GetError());
		exit(-1);
	}

	info = SDL_GetVideoInfo();

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	g_zbias = 2.0/(1<<24);
	sbreSetZBias(g_zbias);

	Uint32 flags = SDL_OPENGL;

	if ((g_screen = SDL_SetVideoMode(g_width, g_height, info->vfmt->BitsPerPixel, flags)) == 0) {
		// fall back on 16-bit depth buffer...
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
		g_zbias = 2.0/(1<<16);
		sbreSetZBias(g_zbias);
		fprintf(stderr, "Failed to set video mode. (%s). Re-trying with 16-bit depth buffer.\n", SDL_GetError());
		if ((g_screen = SDL_SetVideoMode(g_width, g_height, info->vfmt->BitsPerPixel, flags)) == 0) {
			fprintf(stderr, "Video mode set failed: %s\n", SDL_GetError());
		}
	}

	glShadeModel(GL_SMOOTH);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	

	glGenTextures(1, &mytexture);
	glBindTexture(GL_TEXTURE_2D, mytexture);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glClearColor(0,0,0,0);
	glViewport(0, 0, g_width, g_height);
	GLFTInit();
	Gui::Init(g_width, g_height, g_width, g_height);

	Viewer v;
	v.MainLoop();
}
