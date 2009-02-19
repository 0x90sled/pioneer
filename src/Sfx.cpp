#include "libs.h"
#include "Pi.h"
#include "Sfx.h"
#include "Frame.h"
#include "StarSystem.h"
#include "Space.h"
#include "Serializer.h"

Sfx::Sfx(): Body()
{
	m_pos = vector3d(0,0,0);
	m_type = TYPE_EXPLOSION;
	m_age = 0;
}

void Sfx::Save()
{
	using namespace Serializer::Write;
	Body::Save();
	wr_vector3d(m_pos);
	wr_float(m_age);
	wr_int(m_type);
}

void Sfx::Load()
{
	using namespace Serializer::Read;
	Body::Load();
	m_pos = rd_vector3d();
	m_age = rd_float();
	m_type = static_cast<Sfx::TYPE>(rd_int());
}

void Sfx::SetPosition(vector3d p)
{
	m_pos = p;
}

void Sfx::TimeStepUpdate(const float timeStep)
{
	m_age += timeStep;
	m_pos += m_vel * timeStep;

	switch (m_type) {
		case TYPE_EXPLOSION:
			if (m_age > 0.2) Space::KillBody(this);
			break;
		case TYPE_DAMAGE:
			if (m_age > 2.0) Space::KillBody(this);
			break;
	}
}
void Sfx::Render(const Frame *camFrame)
{
	matrix4x4d ftran;
	Frame::GetFrameTransform(GetFrame(), camFrame, ftran);
	vector3d fpos = ftran * GetPosition();

	glPushMatrix();

	glTranslatef(fpos.x, fpos.y, fpos.z);
	
	switch (m_type) {
		case TYPE_EXPLOSION:
			glPushAttrib(GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT);
			glDisable(GL_LIGHTING);
			glColor3f(1,1,0.5);
			gluSphere(Pi::gluQuadric, 1000*m_age, 20,20);
			glEnable(GL_BLEND);
			glColor4f(1,0.5,0,0.66);
			gluSphere(Pi::gluQuadric, 1500*m_age, 20,20);
			glColor4f(1,0,0,0.33);
			gluSphere(Pi::gluQuadric, 2000*m_age, 20,20);
			glPopAttrib();
			break;
		case TYPE_DAMAGE:
			glPushAttrib(GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT);
			glDisable(GL_LIGHTING);
			float s = 0.5*sin(m_age*10);
			glColor3f(.5,.5,.5);
			gluSphere(Pi::gluQuadric, 5+s*s*8, 10,10);
			glPopAttrib();
			break;
	}

	glPopMatrix();
}

void Sfx::Add(const Body *b, TYPE t)
{
	Sfx *sfx = new Sfx();
	sfx->m_type = t;
	sfx->SetFrame(b->GetFrame());
	sfx->SetPosition(b->GetPosition());
	sfx->m_vel = b->GetVelocity();
	Space::AddBody(sfx);
}
