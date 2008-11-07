#include "libs.h"
#include "Space.h"
#include "Body.h"
#include "Frame.h"
#include "Star.h"
#include "Planet.h"
#include <algorithm>
#include <functional>
#include "Pi.h"
#include "Player.h"
#include "StarSystem.h"
#include "SpaceStation.h"
#include "sbre/sbre.h"
#include "Serializer.h"
#include "collider/collider.h"

dWorldID Space::world;
std::list<Body*> Space::bodies;
Frame *Space::rootFrame;
static dJointGroupID _contactgroup;
std::list<Body*> Space::corpses;

void Space::Init()
{
	world = dWorldCreate();
	rootFrame = new Frame(NULL, "System");
	rootFrame->SetRadius(FLT_MAX);
	_contactgroup = dJointGroupCreate(0);
	//dWorldSetGravity(world, 0,-9.81,0);
}

void Space::Clear()
{
	for (std::list<Body*>::iterator i = bodies.begin(); i != bodies.end(); ++i) {
		(*i)->SetFrame(NULL);
		if ((*i) != (Body*)Pi::player) {
			KillBody(*i);
		}
	}
	PruneCorpses();

	Pi::player->SetFrame(rootFrame);
	for (std::list<Frame*>::iterator i = rootFrame->m_children.begin(); i != rootFrame->m_children.end(); ++i) delete *i;
	rootFrame->m_children.clear();
}

void Space::Serialize()
{
	using namespace Serializer::Write;
	Serializer::IndexFrames();
	Serializer::IndexBodies();
	Serializer::IndexSystemBodies(Pi::currentSystem);
	Frame::Serialize(rootFrame);
	printf("%d bodies to write\n", bodies.size());
	wr_int(bodies.size());
	for (bodiesIter_t i = bodies.begin(); i != bodies.end(); ++i) {
		printf("Serializing %s\n", (*i)->GetLabel().c_str());
		(*i)->Serialize();
	}
}

void Space::Unserialize()
{
	using namespace Serializer::Read;
	Serializer::IndexSystemBodies(Pi::currentSystem);
	rootFrame = Frame::Unserialize(0);
	Serializer::IndexFrames();
	int num_bodies = rd_int();
	printf("%d bodies to read\n", num_bodies);
	for (int i=0; i<num_bodies; i++) {
		Body *b = Body::Unserialize();
		if (b) bodies.push_back(b);
		if (b->IsType(Object::PLAYER)) Pi::player = (Player*)b;
	}
	printf("%d bodies read\n", bodies.size());
	// bodies with references to others must fix these up
	Serializer::IndexBodies();
	for (bodiesIter_t i = bodies.begin(); i != bodies.end(); ++i) {
		(*i)->PostLoadFixup();
	}
	Frame::PostUnserializeFixup(rootFrame);
}

void Space::MoveOrbitingObjectFrames(Frame *f)
{
	if (f->m_sbody) {
		// this isn't very smegging efficient
		vector3d pos = f->m_sbody->orbit.CartesianPosAtTime(Pi::GetGameTime());
		vector3d pos2 = f->m_sbody->orbit.CartesianPosAtTime(Pi::GetGameTime()+1.0);
		vector3d vel = pos2 - pos;
		f->SetPosition(pos);
		f->SetVelocity(vel);
	}
	f->RotateInTimestep(Pi::GetTimeStep());

	for (std::list<Frame*>::iterator i = f->m_children.begin(); i != f->m_children.end(); ++i) {
		MoveOrbitingObjectFrames(*i);
	}
}
static Frame *MakeFrameFor(StarSystem::SBody *sbody, Body *b, Frame *f)
{
	Frame *orbFrame, *rotFrame;
	double frameRadius;

	if (!sbody->parent) {
		if (b) b->SetFrame(f);
		f->m_sbody = sbody;
		f->m_astroBody = b;
		return f;
	}

	if (sbody->type == StarSystem::TYPE_GRAVPOINT) {
		orbFrame = new Frame(f, sbody->name.c_str());
		orbFrame->m_sbody = sbody;
		orbFrame->m_astroBody = b;
		orbFrame->SetRadius(sbody->GetMaxChildOrbitalDistance()*1.1);
		return orbFrame;
	}

	StarSystem::BodySuperType supertype = sbody->GetSuperType();

	if ((supertype == StarSystem::SUPERTYPE_GAS_GIANT) ||
	    (supertype == StarSystem::SUPERTYPE_ROCKY_PLANET)) {
		// for planets we want an non-rotating frame for a few radii
		// and a rotating frame in the same position but with maybe 1.1*radius,
		// which actually contains the object.
		frameRadius = sbody->GetMaxChildOrbitalDistance()*1.1;
		orbFrame = new Frame(f, sbody->name.c_str());
		orbFrame->m_sbody = sbody;
		orbFrame->SetRadius(frameRadius ? frameRadius : 10*sbody->GetRadius());
	
		assert(sbody->GetRotationPeriod() != 0);
		rotFrame = new Frame(orbFrame, sbody->name.c_str());
		rotFrame->SetRadius(1.1*sbody->GetRadius());
		rotFrame->SetAngVelocity(vector3d(0,2*M_PI/sbody->GetRotationPeriod(),0));
		rotFrame->m_astroBody = b;
		b->SetFrame(rotFrame);
		return orbFrame;
	}
	else if (supertype == StarSystem::SUPERTYPE_STAR) {
		// stars want a single small non-rotating frame
		orbFrame = new Frame(f, sbody->name.c_str());
		orbFrame->m_sbody = sbody;
		orbFrame->m_astroBody = b;
		orbFrame->SetRadius(sbody->GetMaxChildOrbitalDistance()*1.1);
		b->SetFrame(orbFrame);
		return orbFrame;
	}
	else if (sbody->type == StarSystem::TYPE_STARPORT_ORBITAL) {
		// space stations want non-rotating frame to some distance
		// and a much closer rotating frame
		frameRadius = 1000000.0; // XXX NFI!
		orbFrame = new Frame(f, sbody->name.c_str());
		orbFrame->m_sbody = sbody;
		orbFrame->SetRadius(frameRadius ? frameRadius : 10*sbody->GetRadius());
	
		assert(sbody->GetRotationPeriod() != 0);
		rotFrame = new Frame(orbFrame, sbody->name.c_str());
		rotFrame->SetRadius(5000.0);//(1.1*sbody->GetRadius());
		rotFrame->SetAngVelocity(vector3d(0,2*M_PI/sbody->GetRotationPeriod(),0));
		b->SetFrame(rotFrame);
		return orbFrame;
	} else if (sbody->type == StarSystem::TYPE_STARPORT_SURFACE) {
		// just put body into rotating frame of planet, not in its own frame
		// (because collisions only happen between objects in same frame,
		// and we want collisions on starport and on planet itself)
		Frame *frame = *f->m_children.begin();
		b->SetFrame(frame);
		b->SetPosition(sbody->orbit.rotMatrix * (frame->m_astroBody->GetRadius()*vector3d(0,1,0)));
		b->SetRotMatrix(sbody->orbit.rotMatrix);
		return frame;
	} else {
		assert(0);
	}
}

void Space::GenBody(StarSystem::SBody *sbody, Frame *f)
{
	Body *b = 0;

	if (sbody->type != StarSystem::TYPE_GRAVPOINT) {
		if (sbody->GetSuperType() == StarSystem::SUPERTYPE_STAR) {
			Star *star = new Star(sbody);
			b = star;
		} else if (sbody->type == StarSystem::TYPE_STARPORT_ORBITAL) {
			SpaceStation *ss = new SpaceStation(SpaceStation::JJHOOP);
			b = ss;
		} else if (sbody->type == StarSystem::TYPE_STARPORT_SURFACE) {
			SpaceStation *ss = new SpaceStation(SpaceStation::GROUND_FLAVOURED);
			b = ss;
		} else {
			Planet *planet = new Planet(sbody);
			b = planet;
		}
		b->SetLabel(sbody->name.c_str());
		b->SetPosition(vector3d(0,0,0));
		AddBody(b);
	}
	f = MakeFrameFor(sbody, b, f);

	for (std::vector<StarSystem::SBody*>::iterator i = sbody->children.begin(); i != sbody->children.end(); ++i) {
		GenBody(*i, f);
	}
}

void Space::BuildSystem()
{
	GenBody(Pi::currentSystem->rootBody, rootFrame);
	MoveOrbitingObjectFrames(rootFrame);
}

void Space::AddBody(Body *b)
{
	bodies.push_back(b);
}

void Space::KillBody(Body* const b)
{
	b->MarkDead();
	corpses.push_back(b);
}

void Space::UpdateFramesOfReference()
{
	for (std::list<Body*>::iterator i = bodies.begin(); i != bodies.end(); ++i) {
		Body *b = *i;

		if (!b->GetFlags() & Body::FLAG_CAN_MOVE_FRAME) continue;

		// falling out of frames
		if (!b->GetFrame()->IsLocalPosInFrame(b->GetPosition())) {
			printf("%s leaves frame %s\n", b->GetLabel().c_str(), b->GetFrame()->GetLabel());
			
			vector3d oldFrameVel = b->GetFrame()->GetVelocity();
			
			Frame *new_frame = b->GetFrame()->m_parent;
			if (new_frame) { // don't let fall out of root frame
				matrix4x4d m = matrix4x4d::Identity();
				b->GetFrame()->ApplyLeavingTransform(m);

				vector3d new_pos = m * b->GetPosition();//b->GetPositionRelTo(new_frame);

				matrix4x4d rot;
				b->GetRotMatrix(rot);
				b->SetRotMatrix(m * rot);
				

				b->SetFrame(new_frame);
				b->SetPosition(new_pos);

				// get rid of transforms
				m.ClearToRotOnly();
				b->SetVelocity(m*b->GetVelocity() + oldFrameVel);
			} else {
				b->SetVelocity(b->GetVelocity() + oldFrameVel);
			}
		}

		// entering into frames
		for (std::list<Frame*>::iterator j = b->GetFrame()->m_children.begin(); j != b->GetFrame()->m_children.end(); ++j) {
			Frame *kid = *j;
			matrix4x4d m;
			Frame::GetFrameTransform(b->GetFrame(), kid, m);
			vector3d pos = m * b->GetPosition();
			if (kid->IsLocalPosInFrame(pos)) {
				printf("%s enters frame %s\n", b->GetLabel().c_str(), kid->GetLabel());
				b->SetPosition(pos);
				b->SetFrame(kid);

				matrix4x4d rot;
				b->GetRotMatrix(rot);
				b->SetRotMatrix(m * rot);
				
				// get rid of transforms
				m.ClearToRotOnly();
				b->SetVelocity(m*b->GetVelocity() - kid->GetVelocity());
				break;
			}
		}
	}
}

/*
 * return false if ode is not to apply collision
 */
static bool _OnCollision(dGeomID g1, dGeomID g2, Object *o1, Object *o2, int numContacts, dContact contacts[])
{
	if ((o1->IsType(Object::LASER)) || (o2->IsType(Object::LASER))) {
		if (o1->IsType(Object::LASER)) {
			std::swap<Object*>(o1, o2);
			std::swap<dGeomID>(g1, g2);
		}
		Ship::LaserObj *lobj = static_cast<Ship::LaserObj*>(o2);
		if (o1 == lobj->owner) return false;

		if (o1->IsType(Object::SHIP)) {
			DynamicBody *rb = (DynamicBody*)o1;
			dVector3 start,dir;
			dGeomRayGet(g2, start, dir);
			dBodyAddForceAtPos(rb->m_body,
				100*dir[0],
				100*dir[1],
				100*dir[2],
				contacts[0].geom.pos[0],
				contacts[0].geom.pos[1],
				contacts[0].geom.pos[2]);
		}

		return false;
	} else {
		Body *pb1, *pb2;
		int flags = 0;
		// geom bodies point to their parents
		if (o1->IsType(Object::GEOM)) {
			pb1 = static_cast<ModelBody::GeomBit*>(o1)->parent;
			flags |= static_cast<ModelBody::GeomBit*>(o1)->flags;
		} else pb1 = static_cast<Body*>(o1);
		if (o2->IsType(Object::GEOM)) {
			pb2 = static_cast<ModelBody::GeomBit*>(o2)->parent;
			flags |= static_cast<ModelBody::GeomBit*>(o2)->flags;
		} else pb2 = static_cast<Body*>(o2);

		if ((pb1 && !pb1->OnCollision(pb2, flags)) || (pb2 && !pb2->OnCollision(pb1, flags))) return false;
	}
	return true;
}

static void dump_contact(const dContact *c)
{
	printf("pos %f,%f,%f\n", c->geom.pos[0], c->geom.pos[1], c->geom.pos[2]);
	printf("normal %f,%f,%f\n", c->geom.normal[0], c->geom.normal[1], c->geom.normal[2]);
	printf("depth %f\n", c->geom.depth);
	printf("side1:side2 %d:%d\n", c->geom.side1, c->geom.side2);
	printf("fdir1 %f,%f,%f\n", c->fdir1[0], c->fdir1[1], c->fdir1[2]);
}

static bool _OnCollision2(Object *o1, Object *o2, CollisionContact *c)
{
	Body *pb1, *pb2;
	int flags = c->geomFlag;
//	printf("Collision flags %x (triIdx %d)\n", flags, c->triIdx);
	// geom bodies point to their parents
	if (o1->IsType(Object::GEOM)) {
		pb1 = static_cast<ModelBody::GeomBit*>(o1)->parent;
		flags |= static_cast<ModelBody::GeomBit*>(o1)->flags;
	} else pb1 = static_cast<Body*>(o1);
	if (o2->IsType(Object::GEOM)) {
		pb2 = static_cast<ModelBody::GeomBit*>(o2)->parent;
		flags |= static_cast<ModelBody::GeomBit*>(o2)->flags;
	} else pb2 = static_cast<Body*>(o2);

	if ((pb1 && !pb1->OnCollision(pb2, flags)) || (pb2 && !pb2->OnCollision(pb1, flags))) return false;
	return true;
}

#define MAX_CONTACTS	1
static int contact_num;
static void hitCallback(CollisionContact *c)
{
	if (contact_num++ >= MAX_CONTACTS) return;
	printf("OUCH! %x\n", SDL_GetTicks());
	dContact contact;

	contact.surface.mode = dContactBounce;
	contact.surface.mu = 0.8;
	contact.surface.mu2 = 0;
	contact.surface.bounce = 0.1;
	contact.surface.bounce_vel = 0.1;

	contact.geom.pos[0] = c->pos.x;
	contact.geom.pos[1] = c->pos.y;
	contact.geom.pos[2] = c->pos.z;
	contact.geom.pos[3] = 1;
	contact.geom.normal[0] = c->normal.x;
	contact.geom.normal[1] = c->normal.y;
	contact.geom.normal[2] = c->normal.z;
	contact.geom.normal[3] = 1;
	contact.geom.depth = c->depth;
	contact.geom.g1 = 0;
	contact.geom.g2 = 0;
	contact.fdir1[0] = 0;
	contact.fdir1[1] = 0;
	contact.fdir1[2] = 0;

	Object *po1 = static_cast<Object*>(c->userData1);
	Object *po2 = static_cast<Object*>(c->userData2);

	if (!_OnCollision2(po1, po2, c)) return;
	
	dBodyID b1 = 0;
	dBodyID b2 = 0;
	// Get the dynamics body for each geom
	if (po1->IsType(Object::DYNAMICBODY)) b1 = static_cast<DynamicBody*>(po1)->m_body;
	if (po2->IsType(Object::DYNAMICBODY)) b2 = static_cast<DynamicBody*>(po2)->m_body;

	dJointID j = dJointCreateContact(Space::world, _contactgroup, &contact);
	dJointAttach(j, b1, b2);
}
#if 0
static void nearCallback(void *data, dGeomID o0, dGeomID o1)
{
	// Create an array of dContact objects to hold the contact joints
	static const int MAX_CONTACTS = 100;
	dContact contact[MAX_CONTACTS];

	for (int i = 0; i < MAX_CONTACTS; i++)
	{
		contact[i].surface.mode = dContactBounce;
		contact[i].surface.mu = 0.8;
		contact[i].surface.mu2 = 0;
		contact[i].surface.bounce = 0.1;
		contact[i].surface.bounce_vel = 0.1;
	}
	if (int numc = dCollide(o0, o1, MAX_CONTACTS, &contact[0].geom, sizeof(dContact)))
	{
		// don't ye get confused between Pi Body class and libODE bodies
		Object *po1 = static_cast<Object*>(dGeomGetData(o0));
		Object *po2 = static_cast<Object*>(dGeomGetData(o1));
		if (!_OnCollision(o0, o1, po1, po2, numc, contact)) return;
		// Get the dynamics body for each geom
		dBodyID b1 = dGeomGetBody(o0);
		dBodyID b2 = dGeomGetBody(o1);
		// To add each contact point found to our joint group we call dJointCreateContact which is just one of the many
		// different joint types available.  
		for (int i = 0; i < numc; i++)
		{
			printf("\nODE collision:\n");
			dump_contact(contact+i);
			// dJointCreateContact needs to know which world and joint group to work with as well as the dContact
			// object itself. It returns a new dJointID which we then use with dJointAttach to finally create the
			// temporary contact joint between the two geom bodies.
			//dJointID c = dJointCreateContact(Space::world, _contactgroup, contact + i);
/*			struct dContactGeom {
  dVector3 pos;       // contact position
  dVector3 normal;    // normal vector
  dReal depth;        // penetration depth
  dGeomID g1,g2;      // the colliding geoms
};*/
			//dJointAttach(c, b1, b2);
		}
	}	
}
#endif
void Space::CollideFrame(Frame *f)
{
	f->GetCollisionSpace()->Collide(&hitCallback);
	//dSpaceCollide(f->GetSpaceID(), NULL, &nearCallback);
	for (std::list<Frame*>::iterator i = f->m_children.begin(); i != f->m_children.end(); ++i) {
		CollideFrame(*i);
	}
}

void Space::ApplyGravity()
{
	Body *lump = 0;
	// gravity is applied when our frame contains an 'astroBody', ie a star or planet,
	// or when our frame contains a rotating frame which contains this body.
	if (Pi::player->GetFrame()->m_astroBody) {
		lump = Pi::player->GetFrame()->m_astroBody;
	} else if (Pi::player->GetFrame()->m_sbody &&
		(Pi::player->GetFrame()->m_children.begin() !=
	           Pi::player->GetFrame()->m_children.end())) {

		lump = (*Pi::player->GetFrame()->m_children.begin())->m_astroBody;
	}
	// just to the player and only in the most stupid way for the moment
	if (lump) {
		vector3d b1b2 = lump->GetPosition() - Pi::player->GetPosition();
		const double m1m2 = Pi::player->GetMass() * lump->GetMass();
		const double r = b1b2.Length();
		const double force = G*m1m2 / (r*r);
		b1b2.Normalize();
		b1b2 = b1b2 * force;
		dBodyAddForce(Pi::player->m_body, b1b2.x, b1b2.y, b1b2.z);
	}

}

void Space::TimeStep(float step)
{
	ApplyGravity();

	contact_num = 0;
	CollideFrame(rootFrame);
	dWorldQuickStep(world, step);
	dJointGroupEmpty(_contactgroup);
	// XXX does not need to be done this often
	UpdateFramesOfReference();
	MoveOrbitingObjectFrames(rootFrame);
	
	for (bodiesIter_t i = bodies.begin(); i != bodies.end(); ++i) {
		(*i)->TimeStepUpdate(step);
	}

	PruneCorpses();
}

void Space::PruneCorpses()
{
	for (bodiesIter_t corpse = corpses.begin(); corpse != corpses.end(); ++corpse) {
		for (bodiesIter_t i = bodies.begin(); i != bodies.end(); ++i)
			(*i)->NotifyDeath(*corpse);
		bodies.remove(*corpse);
		delete *corpse;
	}
	corpses.clear();
}

struct body_zsort_t {
	double dist;
	Body *b;
};

struct body_zsort_compare : public std::binary_function<body_zsort_t, body_zsort_t, bool> {
	bool operator()(body_zsort_t a, body_zsort_t b) { return a.dist > b.dist; }
};

void Space::Render(const Frame *cam_frame)
{
	// simple z-sort!!!!!!!!!!!!!11
	body_zsort_t *bz = new body_zsort_t[bodies.size()];
	int idx = 0;
	for (std::list<Body*>::iterator i = bodies.begin(); i != bodies.end(); ++i) {
		vector3d toBody = (*i)->GetPositionRelTo(cam_frame);
		bz[idx].dist = toBody.Length();
		bz[idx].b = *i;
		idx++;
	}
	sort(bz, bz+bodies.size(), body_zsort_compare());

	// Probably the right place for this when partitioning is done
	sbreSetDepthRange (Pi::GetScrWidth()*0.5f, 0.0f, 1.0f);

	for (unsigned int i=0; i<bodies.size(); i++) {
		bz[i].b->Render(cam_frame);
	}
	delete [] bz;
}
