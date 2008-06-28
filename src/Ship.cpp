#include "Ship.h"
#include "objimport.h"
#include "Frame.h"
#include "Pi.h"
#include "WorldView.h"
#include "sbre/sbre.h"
#include "Space.h"

Ship::Ship(ShipType::Type shipType): RigidBody()
{
	m_dockedWith = 0;
	m_mesh = 0;
	m_shipType = shipType;
	m_angThrusters[0] = m_angThrusters[1] = m_angThrusters[2] = 0;
	m_laserCollisionObj.owner = this;
	m_equipment = EquipSet(shipType);
	for (int i=0; i<ShipType::GUNMOUNT_MAX; i++) {
		m_tempLaserGeom[i] = 0;
		m_gunState[i] = 0;
	}
	dGeomSetData(m_geom, static_cast<Body*>(this));
}

void Ship::UpdateMass()
{
	shipstats_t s;
	CalcStats(&s);
	dMassAdjust(&m_mass, s.total_mass*1000);
}

void Ship::SetThrusterState(enum ShipType::Thruster t, float level)
{
	m_thrusters[t] = level;
}

void Ship::ClearThrusterState()
{
	for (int i=0; i<ShipType::THRUSTER_MAX; i++) m_thrusters[i] = 0;
}

// hyperspace range is:
// (200 * hyperspace_class^2) / total mass (in tonnes)

void Ship::CalcStats(shipstats_t *stats)
{
	const ShipType &stype = GetShipType();
	stats->max_capacity = stype.capacity;
	stats->used_capacity = 0;

	for (int i=0; i<Equip::SLOT_MAX; i++) {
		for (int j=0; j<stype.equipSlotCapacity[i]; j++) {
			Equip::Type t = m_equipment.Get((Equip::Slot)i, j);
			if (t) stats->used_capacity += EquipType::types[t].mass;
		}
	}
	stats->free_capacity = stats->max_capacity - stats->used_capacity;
	stats->total_mass = stats->used_capacity + stype.hullMass;

	Equip::Type t = m_equipment.Get(Equip::SLOT_ENGINE);
	float hyperclass = EquipType::types[t].pval;
	stats->hyperspace_range = 200 * hyperclass * hyperclass / stats->total_mass;
}

void Ship::AITurn()
{
	const ShipType &stype = GetShipType();
	float timeStep = Pi::GetTimeStep();
	for (int i=0; i<ShipType::THRUSTER_MAX; i++) {
		float force = timeStep * stype.linThrust[i] * m_thrusters[i];
		switch (i) {
		case ShipType::THRUSTER_REAR: 
		case ShipType::THRUSTER_FRONT:
			dBodyAddRelForce(m_body, 0, 0, force); break;
		case ShipType::THRUSTER_TOP:
		case ShipType::THRUSTER_BOTTOM:
			dBodyAddRelForce(m_body, 0, force, 0); break;
		case ShipType::THRUSTER_LEFT:
		case ShipType::THRUSTER_RIGHT:
			dBodyAddRelForce(m_body, force, 0, 0); break;
		}
	}
	dBodyAddRelTorque(m_body, stype.angThrust*m_angThrusters[0],
				  stype.angThrust*m_angThrusters[1],
				  stype.angThrust*m_angThrusters[2]);
	// lasers
	for (int i=0; i<ShipType::GUNMOUNT_MAX; i++) {
		// free old temp laser geoms
		if (m_tempLaserGeom[i]) dGeomDestroy(m_tempLaserGeom[i]);
		m_tempLaserGeom[i] = 0;
		if (!m_gunState[i]) continue;
		dGeomID ray = dCreateRay(GetFrame()->GetSpaceID(), 10000);
		const vector3d pos = GetPosition();
		const vector3f _dir = stype.gunMount[i].dir;
		vector3d dir = vector3d(_dir.x, _dir.y, _dir.z);
		matrix4x4d m;
		GetRotMatrix(m);
		dir = m.ApplyRotationOnly(dir);
		dGeomRaySet(ray, pos.x, pos.y, pos.z, dir.x, dir.y, dir.z);
		dGeomSetData(ray, static_cast<Object*>(&m_laserCollisionObj));
		m_tempLaserGeom[i] = ray;
	}
}

const ShipType &Ship::GetShipType()
{
	return ShipType::types[m_shipType];
}

void Ship::SetDockedWith(SpaceStation *s)
{
	if (m_dockedWith && !s) {
		// launching. jesus wept this is advanced
		printf("BAIBAI!!!!!!!\n");
		m_dockedWith = 0;
		vector3d pos = GetPosition();
		pos.x += 5000;
		SetPosition(pos);
	} else {
		m_dockedWith = s;
		SetVelocity(vector3d(0,0,0));
		SetAngVelocity(vector3d(0,0,0));
	}
}

void Ship::SetMesh(ObjMesh *m)
{
	m_mesh = m;
}

void Ship::SetGunState(int idx, int state)
{
	m_gunState[idx] = state;
}

/* Assumed to be at model coords */
void Ship::RenderLaserfire()
{
	const ShipType &stype = GetShipType();
	glDisable(GL_LIGHTING);
	for (int i=0; i<ShipType::GUNMOUNT_MAX; i++) {
		if (!m_gunState[i]) continue;
		glColor3f(1,0,0);
		glBegin(GL_LINES);
		vector3f pos = stype.gunMount[i].pos;
		glVertex3f(pos.x, pos.y, pos.z);
		glVertex3fv(&((10000)*stype.gunMount[i].dir)[0]);
		glEnd();
	}
	glEnable(GL_LIGHTING);
}

static ObjParams params = {
	{ 0.5, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f },

	{	// pColor[3]
	{ { 1.0f, 0.0f, 1.0f }, { 0, 0, 0 }, { 0, 0, 0 }, 0 },
	{ { 0.8f, 0.6f, 0.5f }, { 0, 0, 0 }, { 0, 0, 0 }, 0 },
	{ { 0.5f, 0.5f, 0.5f }, { 0, 0, 0 }, { 0, 0, 0 }, 0 } },

	// pText[3][256]	
	{ "IR-L33T", "ME TOO" },
};

void Ship::Render(const Frame *camFrame)
{
	const ShipType &stype = GetShipType();
	params.angthrust[0] = m_angThrusters[0];
	params.angthrust[1] = m_angThrusters[1];
	params.angthrust[2] = m_angThrusters[2];
	params.linthrust[0] = m_thrusters[ShipType::THRUSTER_RIGHT] - m_thrusters[ShipType::THRUSTER_LEFT];
	params.linthrust[1] = m_thrusters[ShipType::THRUSTER_TOP] - m_thrusters[ShipType::THRUSTER_BOTTOM];
	params.linthrust[2] = m_thrusters[ShipType::THRUSTER_REAR] - m_thrusters[ShipType::THRUSTER_FRONT];
	strncpy(params.pText[0], GetLabel().c_str(), sizeof(params.pText));

	RenderSbreModel(camFrame, stype.sbreModel, &params);
	glPushMatrix();
	TransformToModelCoords(camFrame);
	RenderLaserfire();
	glPopMatrix();
}
