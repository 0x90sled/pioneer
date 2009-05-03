#include "libs.h"
#include "Planet.h"
#include "Frame.h"
#include "Pi.h"
#include "WorldView.h"
#include "Serializer.h"
#include "StarSystem.h"
#include "GeoSphere.h"

struct ColRangeObj_t {
	float baseCol[4]; float modCol[4]; float modAll;

	void GenCol(float col[4], MTRand &rng) const {
		float ma = 1 + (rng.Double(modAll*2)-modAll);
		for (int i=0; i<4; i++) col[i] = baseCol[i] + rng.Double(-modCol[i], modCol[i]);
		for (int i=0; i<3; i++) col[i] = CLAMP(ma*col[i], 0, 1);
	}
};

ColRangeObj_t barrenBodyCol = { { .3,.3,.3,1 },{0,0,0,0},.3 };
ColRangeObj_t barrenContCol = { { .2,.2,.2,1 },{0,0,0,0},.3 };
ColRangeObj_t barrenEjectaCraterCol = { { .5,.5,.5,1 },{0,0,0,0},.2 };
float darkblue[4] = { .05, .05, .2, 1 };
float blue[4] = { .2, .2, 1, 1 };
float green[4] = { .2, .8, .2, 1 };
float white[4] = { 1, 1, 1, 1 };

Planet::Planet(SBody *sbody): Body()
{
	pos = vector3d(0,0,0);
	this->sbody = sbody;
	this->m_geosphere = 0;
	Init();
}

void Planet::Init()
{
	m_mass = sbody->GetMass();
	if (!m_geosphere) {
		float col[4];
		MTRand rand;	
		rand.seed(sbody->seed);
		m_geosphere = new GeoSphere(sbody);
	//	m_geosphere->AddCraters(rand, 20, M_PI*0.005, M_PI*0.05);
	}
	
	crudDList = 0;
}
	
void Planet::Save()
{
	using namespace Serializer::Write;
	Body::Save();
	wr_vector3d(pos);
	wr_int(Serializer::LookupSystemBody(sbody));
}

void Planet::Load()
{
	using namespace Serializer::Read;
	Body::Load();
	pos = rd_vector3d();
	sbody = Serializer::LookupSystemBody(rd_int());
	Init();
}

Planet::~Planet()
{
	if (m_geosphere) delete m_geosphere;
}

double Planet::GetRadius() const
{
	return sbody->GetRadius();
}

vector3d Planet::GetPosition() const
{
	return pos;
}

void Planet::SetPosition(vector3d p)
{
	pos = p;
}

void Planet::SetRadius(double radius)
{
	assert(0);
}

double Planet::GetTerrainHeight(const vector3d pos)
{
	double radius = GetRadius();
	if (m_geosphere) {
		return radius * (1.0 + m_geosphere->GetHeight(pos));
	} else {
		return radius;
	}
}

static void DrawRing(double inner, double outer, const float color[4])
{
	glPushAttrib(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
		GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT);
	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glDisable(GL_CULL_FACE);

	glColor4fv(color);
	
	glBegin(GL_TRIANGLE_STRIP);
	glNormal3f(0,1,0);
	for (float ang=0; ang<2*M_PI; ang+=0.1) {
		glVertex3f(inner*sin(ang), 0, inner*cos(ang));
		glVertex3f(outer*sin(ang), 0, outer*cos(ang));
	}
	glVertex3f(0, 0, inner);
	glVertex3f(0, 0, outer);
	glEnd();

	//gluDisk(Pi::gluQuadric, inner, outer, 40, 20);
	glEnable(GL_CULL_FACE);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
	glDisable(GL_BLEND);
	glDisable(GL_NORMALIZE);
	glPopAttrib();
}

struct GasGiantDef_t {
	int hoopMin, hoopMax; float hoopWobble;
	int blobMin, blobMax;
	float poleMin, poleMax; // size range in radians. zero for no poles.
	float ringProbability;
	ColRangeObj_t ringCol;
	ColRangeObj_t bodyCol;
	ColRangeObj_t hoopCol;
	ColRangeObj_t blobCol;
	ColRangeObj_t poleCol;
};

static GasGiantDef_t ggdefs[] = {
{
	/* jupiter */
	30, 40, 0.05,
	20, 30,
	0, 0,
	0.5,
	{ { .61,.48,.384,.1 }, {0,0,0,.9}, 0.3 },
	{ { .99,.76,.62,1 }, { 0,.1,.1,0 }, 0.3 },
	{ { .99,.76,.62,.5 }, { 0,.1,.1,0 }, 0.3 },
	{ { .99,.76,.62,1 }, { 0,.1,.1,0 }, 0.7 },
}, {
	/* saturnish */
	10, 15, 0.0,
	8, 20, // blob range
	0.2, 0.2, // pole size
	0.5,
	{ { .61,.48,.384,.1 }, {0,0,0,.9}, 0.3 },
	{ { .87, .68, .39, 1 }, { 0,0,0,0 }, 0.1 },
	{ { .87, .68, .39, 1 }, { 0,0,0,0 }, 0.1 },
	{ { .87, .68, .39, 1 }, { 0,0,0,0 }, 0.1 },
	{ { .77, .58, .29, 1 }, { 0,0,0,0 }, 0.1 },
}, {
	/* neptunish */
	3, 6, 0.0,
	2, 6,
	0, 0,
	0.5,
	{ { .61,.48,.384,.1 }, {0,0,0,.9}, 0.3 },
	{ { .31,.44,.73,1 }, {0,0,0,0}, .05}, // body col
	{ { .31,.44,.73,0.5 }, {0,0,0,0}, .1},// hoop col
	{ { .21,.34,.54,1 }, {0,0,0,0}, .05},// blob col
}, {
	/* uranus-like *wink* */
	0, 0, 0.0,
	0, 0,
	0, 0,
	0.5,
	{ { .61,.48,.384,.1 }, {0,0,0,.9}, 0.3 },
	{ { .70,.85,.86,1 }, {.1,.1,.1,0}, 0 },
	{ { .70,.85,.86,1 }, {.1,.1,.1,0}, 0 },
	{ { .70,.85,.86,1 }, {.1,.1,.1,0}, 0 },
	{ { .70,.85,.86,1 }, {.1,.1,.1,0}, 0 }
}, {
	/* brown dwarf-like */
	0, 0, 0.05,
	10, 20,
	0, 0,
	0.5,
	{ { .81,.48,.384,.1 }, {0,0,0,.9}, 0.3 },
	{ { .4,.1,0,1 }, {0,0,0,0}, 0.1 },
	{ { .4,.1,0,1 }, {0,0,0,0}, 0.1 },
	{ { .4,.1,0,1 }, {0,0,0,0}, 0.1 },
},
};

#define PLANET_AMBIENT	0.1

static void SetMaterialColor(const float col[4])
{
	float mambient[4];
	mambient[0] = col[0]*PLANET_AMBIENT;
	mambient[1] = col[1]*PLANET_AMBIENT;
	mambient[2] = col[2]*PLANET_AMBIENT;
	mambient[3] = col[3];
	glMaterialfv (GL_FRONT, GL_AMBIENT, mambient);
	glMaterialfv (GL_FRONT, GL_DIFFUSE, col);
}

void Planet::DrawGasGiantRings()
{
//	MTRand rng((int)Pi::GetGameTime());
	MTRand rng(sbody->seed+9);
	float col[4];
	
	// just use a random gas giant flavour for the moment
	GasGiantDef_t &ggdef = ggdefs[rng.Int32(0,3)];

	if (rng.Double(1.0) < ggdef.ringProbability) {
		float pos = rng.Double(1.2,1.7);
		float end = pos + rng.Double(0.1, 1.0);
		end = MIN(end, 2.5);
		while (pos < end) {
			float size = rng.Double(0.1);
			ggdef.ringCol.GenCol(col, rng);
			DrawRing(pos, pos+size, col);
			pos += size;
		}
	}
}

static void _DrawAtmosphere(double rad1, double rad2, vector3d &pos, const float col[4])
{
	glPushMatrix();
	// face the camera dammit
	vector3d zaxis = (-pos).Normalized();
	vector3d xaxis = (vector3d::Cross(zaxis, vector3d(0,1,0))).Normalized();
	vector3d yaxis = vector3d::Cross(zaxis,xaxis);
	matrix4x4d rot = matrix4x4d::MakeRotMatrix(xaxis, yaxis, zaxis).InverseOf();
	glMultMatrixd(&rot[0]);

	const double angStep = M_PI/32;
	// find angle player -> centre -> tangent point
	// tangent is from player to surface of sphere
	float tanAng = acos(rad1 / pos.Length());

	// then we can put the fucking atmosphere on the horizon
	vector3d r1(0.0, 0.0, rad1);
	vector3d r2(0.0, 0.0, rad2);
	rot = matrix4x4d::RotateYMatrix(tanAng);
	r1 = rot * r1;
	r2 = rot * r2;

	rot = matrix4x4d::RotateZMatrix(angStep);

	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glBegin(GL_TRIANGLE_STRIP);
	for (float ang=0; ang<2*M_PI; ang+=angStep) {
		glColor4fv(col);
		glVertex3dv(&r1.x);
		glColor4f(0,0,0,0);
		glVertex3dv(&r2.x);
		r1 = rot * r1;
		r2 = rot * r2;
	}
	
	glEnd();
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glEnable(GL_LIGHTING);
	glPopMatrix();
}

void Planet::DrawAtmosphere(double rad, vector3d &pos)
{
	if (sbody->type == SBody::TYPE_PLANET_SMALL) {
		const float c[4] = { .2, .2, .3, .8 };
		_DrawAtmosphere(rad*0.99, rad*1.05, pos, c);
	}
	else if (sbody->type == SBody::TYPE_PLANET_CO2_THICK_ATMOS) {
		const float c[4] = { .8, .8, .8, .8 };
		_DrawAtmosphere(rad*0.99, rad*1.1, pos, c);
	}
	else if (sbody->type == SBody::TYPE_PLANET_CO2) {
		const float c[4] = { .5, .5, .5, .8 };
		_DrawAtmosphere(rad*0.99, rad*1.05, pos, c);
	}
	else if (sbody->type == SBody::TYPE_PLANET_METHANE_THICK_ATMOS) {
		const float c[4] = { .2, .6, .3, .8 };
		_DrawAtmosphere(rad*0.99, rad*1.1, pos, c);
	}
	else if (sbody->type == SBody::TYPE_PLANET_METHANE) {
		const float c[4] = { .2, .6, .3, .8 };
		_DrawAtmosphere(rad*0.99, rad*1.05, pos, c);
	}
	else if (sbody->type == SBody::TYPE_PLANET_HIGHLY_VOLCANIC) {
		const float c[4] = { .5, .2, .2, .8 };
		_DrawAtmosphere(rad*0.99, rad*1.05, pos, c);
	}
	else if (sbody->type == SBody::TYPE_PLANET_WATER_THICK_ATMOS) {
		const float c[4] = { .8, .8, .8, .8 };
		_DrawAtmosphere(rad*0.99, rad*1.1, pos, c);
	}
	else if (sbody->type == SBody::TYPE_PLANET_WATER) {
		const float c[4] = { .2, .2, .4, .8 };
		_DrawAtmosphere(rad*0.99, rad*1.05, pos, c);
	}
	else if (sbody->type == SBody::TYPE_PLANET_INDIGENOUS_LIFE) {
		const float c[4] = { .2, .2, .5, .8 };
		_DrawAtmosphere(rad*0.99, rad*1.05, pos, c);
	}
}

void Planet::Render(const Frame *a_camFrame)
{
	glPushMatrix();

	matrix4x4d ftran;
	Frame::GetFrameTransform(GetFrame(), a_camFrame, ftran);
	vector3d fpos = ftran * GetPosition();
	double rad = GetRadius();

	double apparent_size = rad / fpos.Length();
	double len = fpos.Length();
	double origLen = len;

	while ((len-rad)*0.25 > 32*WORLDVIEW_ZNEAR) {
		rad *= 0.25;
		fpos = 0.25*fpos;
		len *= 0.25;
	}

	glTranslatef(fpos.x, fpos.y, fpos.z);
	glColor3f(1,1,1);

	if (apparent_size < 0.001) {
		if (crudDList) {
			glDeleteLists(crudDList, 1);
			crudDList = 0;
		}
		/* XXX WRONG. need to pick light from appropriate turd. */
		GLfloat col[4];
		glGetLightfv(GL_LIGHT0, GL_DIFFUSE, col);
		// face the camera dammit
		vector3d zaxis = fpos.Normalized();
		vector3d xaxis = vector3d::Cross(vector3d(0,1,0), zaxis).Normalized();
		vector3d yaxis = vector3d::Cross(zaxis,xaxis);
		matrix4x4d rot = matrix4x4d::MakeRotMatrix(xaxis, yaxis, zaxis).InverseOf();
		glMultMatrixd(&rot[0]);

		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
		
		glEnable(GL_BLEND);
		glColor4f(col[0], col[1], col[2], 1);
		glBegin(GL_TRIANGLE_FAN);
		glVertex3f(0,0,0);
		glColor4f(col[0],col[1],col[2],0);
		
		const float spikerad = 0.005*len +  1e1*(1.0*sbody->GetRadius()*len)/origLen;
		{
			/* bezier with (0,0,0) control points */
			vector3f p0(0,spikerad,0), p1(spikerad,0,0);
			float t=0.1; for (int i=1; i<10; i++, t+= 0.1f) {
				vector3f p = (1-t)*(1-t)*p0 + t*t*p1;
				glVertex3fv(&p[0]);
			}
		}
		{
			vector3f p0(spikerad,0,0), p1(0,-spikerad,0);
			float t=0.1; for (int i=1; i<10; i++, t+= 0.1f) {
				vector3f p = (1-t)*(1-t)*p0 + t*t*p1;
				glVertex3fv(&p[0]);
			}
		}
		{
			vector3f p0(0,-spikerad,0), p1(-spikerad,0,0);
			float t=0.1; for (int i=1; i<10; i++, t+= 0.1f) {
				vector3f p = (1-t)*(1-t)*p0 + t*t*p1;
				glVertex3fv(&p[0]);
			}
		}
		{
			vector3f p0(-spikerad,0,0), p1(0,spikerad,0);
			float t=0.1; for (int i=1; i<10; i++, t+= 0.1f) {
				vector3f p = (1-t)*(1-t)*p0 + t*t*p1;
				glVertex3fv(&p[0]);
			}
		}
		glEnd();
		glDisable(GL_BLEND);

		glEnable(GL_LIGHTING);
		glEnable(GL_DEPTH_TEST);
	} else {
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		float fracH = WORLDVIEW_ZNEAR / Pi::GetScrAspect();
		// very conservative zfar...
		glFrustum(-WORLDVIEW_ZNEAR, WORLDVIEW_ZNEAR, -fracH, fracH, WORLDVIEW_ZNEAR, MAX(rad, WORLDVIEW_ZFAR));
		glMatrixMode(GL_MODELVIEW);

		vector3d campos = -fpos;
		ftran.ClearToRotOnly();
		campos = ftran.InverseOf() * campos;
		glMultMatrixd(&ftran[0]);
		glEnable(GL_NORMALIZE);
		glPushMatrix();
		glScalef(rad,rad,rad);
		campos = campos * (1.0/rad);
		m_geosphere->Render(campos);
		
		if (sbody->GetSuperType() == SBody::SUPERTYPE_GAS_GIANT) DrawGasGiantRings();
		
		glPopMatrix();
		glDisable(GL_NORMALIZE);
		

		fpos = ftran.InverseOf() * fpos;

		DrawAtmosphere(rad, fpos);
		glClear(GL_DEPTH_BUFFER_BIT);

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
	}
	glPopMatrix();
}

void Planet::SetFrame(Frame *f)
{
	if (GetFrame()) {
		GetFrame()->SetPlanetGeom(0, 0);
	}
	Body::SetFrame(f);
	if (f) {
		GetFrame()->SetPlanetGeom(0, 0);
	}
}

