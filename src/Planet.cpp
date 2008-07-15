#include "libs.h"
#include "Planet.h"
#include "Frame.h"
#include "Pi.h"
#include "WorldView.h"

Planet::Planet(StarSystem::SBody::SubType subtype): Body()
{
	m_radius = 6378135.0;
	m_pos = vector3d(0,0,0);
	m_geom = dCreateSphere(0, m_radius);
	dGeomSetData(m_geom, static_cast<Body*>(this));
	m_subtype = subtype;
}
	
Planet::~Planet()
{
	dGeomDestroy(m_geom);
}

vector3d Planet::GetPosition()
{
	return m_pos;
}

void Planet::SetPosition(vector3d p)
{
	m_pos = p;
	dGeomSetPosition(m_geom, p.x, p.y, p.z);
}

void Planet::SetRadius(double radius)
{
	m_radius = radius;
	dGeomSphereSetRadius(m_geom, radius);
}

void subdivide(vector3d &v1, vector3d &v2, vector3d &v3, vector3d &v4, int depth)
{
	if (depth) {
		depth--;
		vector3d v5 = v1+v2;
		vector3d v6 = v2+v3;
		vector3d v7 = v3+v4;
		vector3d v8 = v4+v1;
		vector3d v9 = v1+v2+v3+v4;

		v5.Normalize();
		v6.Normalize();
		v7.Normalize();
		v8.Normalize();
		v9.Normalize();

		// XXX wrong wrong wrong wrong. need to do projection turd and stuff
#if 0
		// front-facing
		bool ff1, ff2, ff3, ff4, ff5, ff6, ff7, ff8, ff9;
		const matrix4x4d &r = Pi::world_view->viewingRotation;

		ff1 = (r*v1).z > 0; 
		ff2 = (r*v2).z > 0; 
		ff3 = (r*v3).z > 0; 
		ff4 = (r*v4).z > 0; 
		ff5 = (r*v5).z > 0; 
		ff6 = (r*v6).z > 0; 
		ff7 = (r*v7).z > 0; 
		ff8 = (r*v8).z > 0; 
		ff9 = (r*v9).z > 0; 
#endif
/*		if (ff1 || ff5 || ff9 || ff8)*/ subdivide(v1,v5,v9,v8,depth);
/*		if (ff5 || ff2 || ff6 || ff9)*/ subdivide(v5,v2,v6,v9,depth);
/*		if (ff9 || ff6 || ff3 || ff7)*/ subdivide(v9,v6,v3,v7,depth);
/*		if (ff8 || ff9 || ff7 || ff4)*/ subdivide(v8,v9,v7,v4,depth);
	} else {
		glBegin(GL_TRIANGLE_STRIP);
		glNormal3dv(&v1.x);
		glVertex3dv(&v1.x);
		glNormal3dv(&v2.x);
		glVertex3dv(&v2.x);
		glNormal3dv(&v4.x);
		glVertex3dv(&v4.x);
		glNormal3dv(&v3.x);
		glVertex3dv(&v3.x);
		glEnd();
	}
}

void DrawShittyRoundCube(double radius)
{
	const float mdiff[] = { 1.0, 0.8, 0.5, 1.0 };
	const float mambient[] = { 0.1, 0.08, 0.05, 1.0 };
	glMaterialfv (GL_FRONT, GL_AMBIENT, mambient);
	glMaterialfv (GL_FRONT, GL_DIFFUSE, mdiff);

	vector3d p1(1,1,1);
	vector3d p2(-1,1,1);
	vector3d p3(-1,-1,1);
	vector3d p4(1,-1,1);

	vector3d p5(1,1,-1);
	vector3d p6(-1,1,-1);
	vector3d p7(-1,-1,-1);
	vector3d p8(1,-1,-1);

	p1.Normalize();
	p2.Normalize();
	p3.Normalize();
	p4.Normalize();
	p5.Normalize();
	p6.Normalize();
	p7.Normalize();
	p8.Normalize();

//	p1 *= radius;
//	p2 *= radius;
//	p3 *= radius;
//	p4 *= radius;
//	p5 *= radius;
//	p6 *= radius;
//	p7 *= radius;
//	p8 *= radius;

//	glDisable(GL_CULL_FACE);
	glEnable(GL_NORMALIZE);
	subdivide(p1, p2, p3, p4, 4);
	subdivide(p4, p3, p7, p8, 4);
	subdivide(p1, p4, p8, p5, 4);
	subdivide(p2, p1, p5, p6, 4);
	subdivide(p3, p2, p6, p7, 4);
	subdivide(p8, p7, p6, p5, 4);
	
	glDisable(GL_NORMALIZE);
}

// both arguments in radians
void DrawHoop(float latitude, float width, const float col[4])
{
	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	float mambient[4];
	mambient[0] = col[0]*.1;
	mambient[1] = col[1]*.1;
	mambient[2] = col[2]*.1;
	mambient[3] = col[3];
	glMaterialfv (GL_FRONT, GL_AMBIENT, mambient);
	glMaterialfv (GL_FRONT, GL_DIFFUSE, col);
	glEnable(GL_NORMALIZE);
	glEnable(GL_BLEND);
	
	glBegin(GL_TRIANGLE_STRIP);
	for (double longitude=0.0f; longitude < 2*M_PI; longitude += 0.02) {
		vector3d v;
		double l;
		l = latitude+0.5*width;
		v.x = sin(longitude)*cos(l);
		v.y = sin(l);
		v.z = cos(longitude)*cos(l);
		v.Normalize();
		glNormal3dv(&v.x);
		glVertex3dv(&v.x);
		
		l = latitude-0.5*width;
		v.x = sin(longitude)*cos(l);
		v.y = sin(l);
		v.z = cos(longitude)*cos(l);
		glNormal3dv(&v.x);
		glVertex3dv(&v.x);
	}
	double l = latitude+0.5*width;
	vector3d v;
	v.x = 0;
	v.y = sin(l);
	v.z = cos(l);
	v.Normalize();
	glNormal3dv(&v.x);
	glVertex3dv(&v.x);
	
	l = latitude-0.5*width;
	v.x = 0;
	v.y = sin(l);
	v.z = cos(l);
	glNormal3dv(&v.x);
	glVertex3dv(&v.x);

	glEnd();

	glDisable(GL_BLEND);
	glDisable(GL_NORMALIZE);
	glPopAttrib();
}

static void PutPolarPoint(float latitude, float longitude)
{
	vector3d v;
	v.x = sin(longitude)*cos(latitude);
	v.y = sin(latitude);
	v.z = cos(longitude)*cos(latitude);
	v.Normalize();
	glNormal3dv(&v.x);
	glVertex3dv(&v.x);
}

void DrawBlob(float latitude, float longitude, float a, float b, const float col[4])
{
	float mambient[4];
	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	mambient[0] = col[0]*.1;
	mambient[1] = col[1]*.1;
	mambient[2] = col[2]*.1;
	mambient[3] = col[3];
	glMaterialfv (GL_FRONT, GL_AMBIENT, mambient);
	glMaterialfv (GL_FRONT, GL_DIFFUSE, col);
	glEnable(GL_NORMALIZE);
	glEnable(GL_BLEND);

	glBegin(GL_TRIANGLE_FAN);
	PutPolarPoint(latitude, longitude);
	for (double theta=2*M_PI; theta>0; theta-=0.1) {
		double _lat = latitude + a * cos(theta);
		double _long = longitude + b * sin(theta);
		PutPolarPoint(_lat, _long);
	}
	{
		double _lat = latitude + a;
		double _long = longitude;
		PutPolarPoint(_lat, _long);
	}
	glEnd();

	glDisable(GL_BLEND);
	glDisable(GL_NORMALIZE);
	glPopAttrib();
}

static void DrawRing(double inner, double outer, const float color[4])
{
	glPushAttrib(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
		GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glDisable(GL_CULL_FACE);
	
	glBegin(GL_TRIANGLE_STRIP);
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

static void SphereTriSubdivide(vector3d &v1, vector3d &v2, vector3d &v3, int depth)
{
	if (--depth > 0) {
		vector3d v4 = vector3d::Normalize(v1+v2);
		vector3d v5 = vector3d::Normalize(v2+v3);
		vector3d v6 = vector3d::Normalize(v1+v3);
		SphereTriSubdivide(v1,v4,v6,depth);
		SphereTriSubdivide(v4,v2,v5,depth);
		SphereTriSubdivide(v6,v4,v5,depth);
		SphereTriSubdivide(v6,v5,v3,depth);
	} else {
		glNormal3dv(&v1.x);
		glVertex3dv(&v1.x);
		glNormal3dv(&v2.x);
		glVertex3dv(&v2.x);
		glNormal3dv(&v3.x);
		glVertex3dv(&v3.x);
	}
}

// yPos should be 1.0 for north pole, -1.0 for south pole
// size in radians
static void DrawPole(double yPos, double size, const float col[4])
{
	float mambient[4];
	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	mambient[0] = col[0]*.1;
	mambient[1] = col[1]*.1;
	mambient[2] = col[2]*.1;
	mambient[3] = col[3];
	glMaterialfv (GL_FRONT, GL_AMBIENT, mambient);
	glMaterialfv (GL_FRONT, GL_DIFFUSE, col);
	glEnable(GL_NORMALIZE);
	glEnable(GL_BLEND);

	const bool southPole = yPos < 0;
	
	vector3d center(0, yPos, 0);
	glBegin(GL_TRIANGLES);
	for (float ang=2*M_PI; ang>0; ang-=0.1) {
		vector3d v1(sin(ang), yPos, cos(ang));
		vector3d v2(sin(ang+0.1), yPos, cos(ang+0.1));
		v1.Normalize();
		v2.Normalize();
		if (southPole)
			SphereTriSubdivide(center, v2, v1, 4);
		else
			SphereTriSubdivide(center, v1, v2, 4);
	}
	glEnd();


	glDisable(GL_BLEND);
	glDisable(GL_NORMALIZE);
	glPopAttrib();
}

void Planet::Render(const Frame *a_camFrame)
{
	glPushMatrix();
	
	double rad = m_radius;
	vector3d fpos = GetPositionRelTo(a_camFrame);

	double apparent_size = rad / fpos.Length();
	double len = fpos.Length();

	while (len > 5000.0f) {
		rad *= 0.25;
		fpos = 0.25*fpos;
		len *= 0.25;
	}

	glTranslatef(fpos.x, fpos.y, fpos.z);
	glColor3f(1,1,1);

	if (apparent_size < 0.001) {
		glDisable(GL_LIGHTING);
		glPointSize(1.0);
		glBegin(GL_POINTS);
		glVertex3f(0,0,0);
		glEnd();
		glEnable(GL_LIGHTING);
	} else {
		glScalef(rad,rad,rad);
		DrawShittyRoundCube(1.0f);
		const float col1[] = { 1,1,0,.7 };
		const float col2[] = { 1,.2,0,.7 };
		const float col3[] = { .3,1,0,.7 };
		const float col4[] = { 1,.6,0,.7 };
		const float col5[] = { 0,0,0.8,.7 };
		const float white[] = { 1,1,1,1 };
		DrawHoop(M_PI/10.0, M_PI/20.0, col1);
		DrawHoop(M_PI/12.0, M_PI/20.0, col2);
		DrawHoop(0, M_PI/20.0, col2);
		DrawHoop(-M_PI/10.0, M_PI/20.0, col3);
		DrawHoop(M_PI/2 - M_PI/10.0, M_PI/20.0, col4);
		DrawBlob(.2, -0.3, 0.05, 0.2, col5);
		DrawBlob(.3, M_PI/2, 0.05, 0.2, col5);
		DrawBlob(-.1, -M_PI/2, 0.05, 0.2, col5);
		DrawPole(1.0, 0.1, white);
		DrawPole(-1.0, 0.1, white);
		DrawRing(1.5, 1.8, col1);
		DrawRing(1.5, 1.8, col1);
		DrawRing(1.9, 2.0, col1);
		DrawRing(2.04, 2.3, col1);
//		DrawBlob(1.0, 0, 0.02, 0.5, col5);
//		DrawBlob(-1.0, 0, 0.02, 0.5, col5);
		glClear(GL_DEPTH_BUFFER_BIT);
	}
	glPopMatrix();
}

void Planet::SetFrame(Frame *f)
{
	if (GetFrame()) GetFrame()->RemoveGeom(m_geom);
	Body::SetFrame(f);
	if (f) f->AddGeom(m_geom);
}

