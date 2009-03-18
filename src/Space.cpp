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

namespace Space {

struct laserBeam_t {
	Frame *frame;
	vector3d pos, dir;
	double length;
	Ship *firer;
	float damage;
};

std::list<Body*> bodies;
Frame *rootFrame;
static void MoveOrbitingObjectFrames(Frame *f);
static void UpdateFramesOfReference();
static void CollideFrame(Frame *f);
static void PruneCorpses();
static void ApplyGravity();
static std::list<Body*> corpses;
// laser beams fired this physics tick
static std::vector<laserBeam_t> laserBeams;

void Init()
{
	rootFrame = new Frame(NULL, "System");
	rootFrame->SetRadius(FLT_MAX);
}

void Clear()
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

void Serialize()
{
	using namespace Serializer::Write;
	Serializer::IndexFrames();
	Serializer::IndexBodies();
	Serializer::IndexSystemBodies(Pi::currentSystem);
	Frame::Serialize(rootFrame);
	wr_int(bodies.size());
	for (bodiesIter_t i = bodies.begin(); i != bodies.end(); ++i) {
		printf("Serializing %s\n", (*i)->GetLabel().c_str());
		(*i)->Serialize();
	}
}

void Unserialize()
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
	// bodies with references to others must fix these up
	Serializer::IndexBodies();
	for (bodiesIter_t i = bodies.begin(); i != bodies.end(); ++i) {
		(*i)->PostLoadFixup();
	}
	Frame::PostUnserializeFixup(rootFrame);
}

static Frame *find_frame_with_sbody(Frame *f, const SBody *b)
{
	if (f->m_sbody == b) return f;
	else {
		for (std::list<Frame*>::iterator i = f->m_children.begin();
			i != f->m_children.end(); ++i) {
			
			Frame *found = find_frame_with_sbody(*i, b);
			if (found) return found;
		}
	}
	return 0;
}

Frame *GetFrameWithSBody(const SBody *b)
{
	return find_frame_with_sbody(rootFrame, b);
}

void MoveOrbitingObjectFrames(Frame *f)
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
static Frame *MakeFrameFor(SBody *sbody, Body *b, Frame *f)
{
	Frame *orbFrame, *rotFrame;
	double frameRadius;

	if (!sbody->parent) {
		if (b) b->SetFrame(f);
		f->m_sbody = sbody;
		f->m_astroBody = b;
		return f;
	}

	if (sbody->type == SBody::TYPE_GRAVPOINT) {
		orbFrame = new Frame(f, sbody->name.c_str());
		orbFrame->m_sbody = sbody;
		orbFrame->m_astroBody = b;
		orbFrame->SetRadius(sbody->GetMaxChildOrbitalDistance()*1.1);
		return orbFrame;
	}

	SBody::BodySuperType supertype = sbody->GetSuperType();

	if ((supertype == SBody::SUPERTYPE_GAS_GIANT) ||
	    (supertype == SBody::SUPERTYPE_ROCKY_PLANET)) {
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
	else if (supertype == SBody::SUPERTYPE_STAR) {
		// stars want a single small non-rotating frame
		orbFrame = new Frame(f, sbody->name.c_str());
		orbFrame->m_sbody = sbody;
		orbFrame->m_astroBody = b;
		orbFrame->SetRadius(sbody->GetMaxChildOrbitalDistance()*1.1);
		b->SetFrame(orbFrame);
		return orbFrame;
	}
	else if (sbody->type == SBody::TYPE_STARPORT_ORBITAL) {
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
	} else if (sbody->type == SBody::TYPE_STARPORT_SURFACE) {
		// just put body into rotating frame of planet, not in its own frame
		// (because collisions only happen between objects in same frame,
		// and we want collisions on starport and on planet itself)
		Frame *frame = *f->m_children.begin();
		b->SetFrame(frame);
		assert(frame->m_astroBody->IsType(Object::PLANET));
		Planet *planet = static_cast<Planet*>(frame->m_astroBody);
		vector3d pos = sbody->orbit.rotMatrix * vector3d(0,1,0);
		pos = pos.Normalized();
		b->SetPosition(pos * planet->GetTerrainHeight(pos));
		b->SetRotMatrix(sbody->orbit.rotMatrix);
		return frame;
	} else {
		assert(0);
	}
}

void GenBody(SBody *sbody, Frame *f)
{
	Body *b = 0;

	if (sbody->type != SBody::TYPE_GRAVPOINT) {
		if (sbody->GetSuperType() == SBody::SUPERTYPE_STAR) {
			Star *star = new Star(sbody);
			b = star;
		} else if (sbody->type == SBody::TYPE_STARPORT_ORBITAL) {
			SpaceStation *ss = new SpaceStation(SpaceStation::JJHOOP);
			b = ss;
		} else if (sbody->type == SBody::TYPE_STARPORT_SURFACE) {
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

	for (std::vector<SBody*>::iterator i = sbody->children.begin(); i != sbody->children.end(); ++i) {
		GenBody(*i, f);
	}
}

void BuildSystem()
{
	GenBody(Pi::currentSystem->rootBody, rootFrame);
	MoveOrbitingObjectFrames(rootFrame);
}

void AddBody(Body *b)
{
	bodies.push_back(b);
}

void KillBody(Body* const b)
{
	b->MarkDead();
	corpses.push_back(b);
}

void UpdateFramesOfReference()
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
				
				b->SetVelocity(oldFrameVel + m.ApplyRotationOnly(b->GetVelocity() - 
					b->GetFrame()->GetStasisVelocityAtPosition(b->GetPosition())));

				b->SetFrame(new_frame);
				b->SetPosition(new_pos);
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
				b->SetVelocity(m*b->GetVelocity()
					- kid->GetVelocity()
					+ kid->GetStasisVelocityAtPosition(pos));
				break;
			}
		}
	}
}

static bool OnCollision(Object *o1, Object *o2, CollisionContact *c)
{
	Body *pb1 = static_cast<Body*>(o1);
	Body *pb2 = static_cast<Body*>(o2);
	if ((pb1 && !pb1->OnCollision(pb2, c->geomFlag)) || (pb2 && !pb2->OnCollision(pb1, c->geomFlag))) return false;
	return true;
}

static void hitCallback(CollisionContact *c)
{
	//printf("OUCH! %x (depth %f)\n", SDL_GetTicks(), c->depth);

	Object *po1 = static_cast<Object*>(c->userData1);
	Object *po2 = static_cast<Object*>(c->userData2);
	
	if (!OnCollision(po1, po2, c)) return;

	const bool po1_isDynBody = po1->IsType(Object::DYNAMICBODY);
	const bool po2_isDynBody = po2->IsType(Object::DYNAMICBODY);
	// collision response
	assert(po1_isDynBody || po2_isDynBody);

	if (po1_isDynBody && po2_isDynBody) {
		DynamicBody *b1 = static_cast<DynamicBody*>(po1);
		DynamicBody *b2 = static_cast<DynamicBody*>(po2);
		vector3d hitPos1 = c->pos - b1->GetPosition();
		vector3d hitPos2 = c->pos - b2->GetPosition();
		vector3d vel1 = b1->GetVelocity();
		vector3d vel2 = b2->GetVelocity();
		const vector3d relVel = vel2 - vel1;
		const double invMass1 = 1.0 / b1->GetMass();
		const double invMass2 = 1.0 / b2->GetMass();

		const double coeff_rest = 0.8;
		const double j = (-(1+coeff_rest) * (vector3d::Dot(relVel, c->normal))) /
			( (invMass1 + invMass2) +
			( vector3d::Dot(c->normal, vector3d::Cross(vector3d::Cross(hitPos1, c->normal) * (1.0/b1->GetAngularInertia()), hitPos1) )) +
			( vector3d::Dot(c->normal, vector3d::Cross(vector3d::Cross(hitPos2, c->normal) * (1.0/b2->GetAngularInertia()), hitPos2) ))
			);
		// step back
	//	b1->UndoTimestep();
	//	b2->UndoTimestep();
		// apply impulse
		b1->SetVelocity(vel1 - (j*c->normal)*invMass1);
		b2->SetVelocity(vel2 + (j*c->normal)*invMass2);
		b1->SetAngVelocity(b1->GetAngVelocity() - vector3d::Cross(hitPos1, (j*c->normal))*(1.0/b1->GetAngularInertia()));
		b2->SetAngVelocity(b2->GetAngVelocity() + vector3d::Cross(hitPos2, (j*c->normal))*(1.0/b2->GetAngularInertia()));
	} else {
		// one body is static
		vector3d hitNormal;
		DynamicBody *mover;
		
		if (po1_isDynBody) {
			mover = static_cast<DynamicBody*>(po1);
			hitNormal = c->normal;
		} else {
			mover = static_cast<DynamicBody*>(po2);
			hitNormal = -c->normal;
		}

		const double coeff_rest = 0.5;
		const vector3d vel = mover->GetVelocity();
		vector3d reflect = vel - (hitNormal * vector3d::Dot(vel, hitNormal) * 2.0f);

		// step back
		mover->UndoTimestep();
		// and set altered velocity
		mover->SetVelocity(reflect * coeff_rest);

		// angular effects
		const double invMass1 = 1.0 / mover->GetMass();

		const vector3d hitPos1 = c->pos - mover->GetPosition();
		const double j = (-(1+coeff_rest) * (vector3d::Dot(mover->GetVelocity(), c->normal))) /
			( invMass1 +
			( vector3d::Dot(c->normal, vector3d::Cross(vector3d::Cross(hitPos1, c->normal) * (1.0/mover->GetAngularInertia()), hitPos1) ))
			);

		mover->SetAngVelocity(mover->GetAngVelocity() - vector3d::Cross(hitPos1, (j*c->normal))*(1.0/mover->GetAngularInertia()));
	}
}

void CollideFrame(Frame *f)
{
	if (f->m_astroBody && (f->m_astroBody->IsType(Object::PLANET))) {
		// this is pretty retarded
		for (bodiesIter_t i = bodies.begin(); i!=bodies.end(); ++i) {
			if ((*i)->GetFrame() != f) continue;
			if (!(*i)->IsType(Object::DYNAMICBODY)) continue;
			vector3d pos = (*i)->GetPosition();
			vector3d s = pos.Normalized();
			double terrain_height = static_cast<Planet*>(f->m_astroBody)->GetTerrainHeight(s);
			double altitude = pos.Length();
			if (altitude < terrain_height) {
				CollisionContact c;
				c.pos = s;
				c.normal = s;
				c.depth = terrain_height - altitude;
				c.userData1 = static_cast<void*>(*i);
				c.userData2 = static_cast<void*>(f->m_astroBody);
				hitCallback(&c);
			}
		}
	}
	f->GetCollisionSpace()->Collide(&hitCallback);
	for (std::list<Frame*>::iterator i = f->m_children.begin(); i != f->m_children.end(); ++i) {
		CollideFrame(*i);
	}
}

void ApplyGravity()
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
	// just to crap in the player's frame
	if (lump) { 
		for (std::list<Body*>::iterator i = bodies.begin(); i != bodies.end(); ++i) {
			if ((*i)->GetFrame() != Pi::player->GetFrame()) continue;
			if (!(*i)->IsType(Object::DYNAMICBODY)) continue;

			vector3d b1b2 = lump->GetPosition() - (*i)->GetPosition();
			const double m1m2 = (*i)->GetMass() * lump->GetMass();
			const double r = b1b2.Length();
			const double force = G*m1m2 / (r*r);
			b1b2 = b1b2.Normalized() * force;
			static_cast<DynamicBody*>(*i)->AddForce(b1b2);
		}
	}

}

void AddLaserBeam(Frame *f, const vector3d &pos, const vector3d &dir,
	double length, Ship *firer, float damage)
{
	laserBeam_t l;
	l.frame = f;
	l.pos = pos; l.dir = dir;
	l.length = length;
	l.firer = firer;
	l.damage = damage;

	laserBeams.push_back(l);
}

void test_laser_beams()
{
	for (unsigned int i=0; i<laserBeams.size(); i++) {
		CollisionContact c;
		laserBeams[i].frame->GetCollisionSpace()->TraceRay(
			laserBeams[i].pos, laserBeams[i].dir,
			laserBeams[i].length, &c,
			laserBeams[i].firer->GetGeom());
		if (c.userData1) {
			Body *hit = static_cast<Body*>(c.userData1);
			hit->OnDamage(laserBeams[i].firer, laserBeams[i].damage);
		}
	}
}

void TimeStep(float step)
{
	laserBeams.clear();
	ApplyGravity();
	CollideFrame(rootFrame);
	// XXX does not need to be done this often
	UpdateFramesOfReference();
	MoveOrbitingObjectFrames(rootFrame);
	
	for (bodiesIter_t i = bodies.begin(); i != bodies.end(); ++i) {
		(*i)->TimeStepUpdate(step);
	}
	// see if anyone has been shot
	test_laser_beams();

	if (Pi::player->IsDead()) {
		printf("PLAYER IS DEAD :-(\n");
		Pi::TombStoneLoop();
		Pi::Quit();
	}

	PruneCorpses();
}

void PruneCorpses()
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

void Render(const Frame *cam_frame)
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

}

