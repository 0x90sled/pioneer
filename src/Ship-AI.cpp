#include "libs.h"
#include "Ship.h"
#include "Pi.h"

void Ship::AITimeStep(const float timeStep)
{
	bool done = false;

	if (m_todo.size() != 0) {
		AIInstruction &inst = m_todo.front();
		switch (inst.cmd) {
			case DO_KILL:
				done = AICmdKill(static_cast<const Ship*>(inst.arg));
				break;
			case DO_NOTHING: done = true; break;
		}
	}
	if (done) { 
		printf("AI '%s' successfully executed %d:'%s'\n", GetLabel().c_str(), m_todo.front().cmd,
				static_cast<Ship*>(m_todo.front().arg)->GetLabel().c_str());
		m_todo.pop_front();
	}
}

bool Ship::AICmdKill(const Ship *enemy)
{
	SetGunState(0,0);
	/* needs to deal with frames, large distances, and success */
	if (GetFrame() == enemy->GetFrame()) {
		const float dist = (enemy->GetPosition() - GetPosition()).Length();
		vector3d dir = (enemy->GetPosition() - GetPosition()).Normalized();
		if (dist > 500.0) {
			AIFaceDirection(dir);
			// thunder at player at 400m/sec
			AIModelCoordsMatchSpeedRelTo(vector3d(0,0,-400), enemy);
			// fire guns if aiming well enough	
			matrix4x4d rot;
			GetRotMatrix(rot);
			const vector3d zaxis = vector3d(-rot[8], -rot[9], -rot[10]);
			const float dot = vector3d::Dot(dir, vector3d(-rot[8], -rot[9], -rot[10]));
			if (dot > 0.95f) SetGunState(0,1);
		} else {
			// if too close turn away!
			AIFaceDirection(-dir);
			AIModelCoordsMatchSpeedRelTo(vector3d(0,0,-1000), enemy);
		}
	}
	return false;
}

void Ship::AIInstruct(enum AICommand cmd, void *arg)
{
	m_todo.push_back(AIInstruction(cmd, arg));
}

/* Orient so our -ve z axis == dir. ie so that dir points forwards */
void Ship::AIFaceDirection(const vector3d &dir)
{
	matrix4x4d rot;
	GetRotMatrix(rot);
	rot = rot.InverseOf();
	const vector3d zaxis = vector3d(-rot[2], -rot[6], -rot[10]);
	vector3d rotaxis = vector3d::Cross(zaxis, dir);
	vector3d angVel = rot * GetAngVelocity();
	const float dot = vector3d::Dot(dir, zaxis);
	// if facing > 90 degrees away then max turn rate
	if (dot < 0) rotaxis = rotaxis.Normalized();
	rotaxis = rot*rotaxis;
	vector3d desiredAngVelChange = 4*(rotaxis - angVel);
	ClearThrusterState();
	// still must apply rotation damping
	rotaxis -= CalcRotDamping();
	SetAngThrusterState(0, desiredAngVelChange.x);
	SetAngThrusterState(1, desiredAngVelChange.y);
	SetAngThrusterState(2, desiredAngVelChange.z);
}

void Ship::AIModelCoordsMatchSpeedRelTo(const vector3d v, const Ship *other)
{
	matrix4x4d m; GetRotMatrix(m);
	vector3d relToVel = m.InverseOf() * other->GetVelocity() + v;
	AIAccelToModelRelativeVelocity(relToVel);
}

#include "Frame.h"
/* Try to reach this model-relative velocity.
 * (0,0,-100) would mean going 100m/s forward.
 */
void Ship::AIAccelToModelRelativeVelocity(const vector3d v)
{
	const ShipType &stype = GetShipType();
	
	// OK. For rotating frames linked to space stations we want to set
	// speed relative to non-rotating frame (so we apply Frame::GetStasisVelocityAtPosition.
	// For rotating frames linked to planets we want to set velocity relative to
	// surface, so we do not apply Frame::GetStasisVelocityAtPosition
	vector3d relVel = GetVelocity();
	if (!GetFrame()->m_astroBody) {
		relVel -= GetFrame()->GetStasisVelocityAtPosition(GetPosition());
	}
	matrix4x4d m; GetRotMatrix(m);
	relVel = m.InverseOf() * relVel;

	vector3d difVel = v - relVel;
	// want to change velocity by difVel...
//	SetVelocity(m * (relVel + difVel));
	const float invMass = 1.0 / GetMass();

	if (difVel.x > 0) {
		// figure out biggest accel can get, and then what we need this timestep.
		float velChange = Pi::GetTimeStep() * stype.linThrust[ShipType::THRUSTER_RIGHT] * invMass;
		float thrust;
		if (velChange < difVel.x) thrust = 1.0;
		else thrust = difVel.x / velChange;
		thrust *= thrust; // this is just to hide control jiggle
		SetThrusterState(ShipType::THRUSTER_RIGHT, thrust);
	} else {
		float velChange = Pi::GetTimeStep() * stype.linThrust[ShipType::THRUSTER_LEFT] * invMass;
		float thrust;
		if (velChange > difVel.x) thrust = 1.0;
		else thrust = difVel.x / velChange;
		thrust *= thrust;
		SetThrusterState(ShipType::THRUSTER_LEFT, thrust);
	}

	if (difVel.y > 0) {
		float velChange = Pi::GetTimeStep() * stype.linThrust[ShipType::THRUSTER_TOP] * invMass;
		float thrust;
		if (velChange < difVel.y) thrust = 1.0;
		else thrust = difVel.y / velChange;
		thrust *= thrust;
		SetThrusterState(ShipType::THRUSTER_TOP, thrust);
	} else {
		float velChange = Pi::GetTimeStep() * stype.linThrust[ShipType::THRUSTER_BOTTOM] * invMass;
		float thrust;
		if (velChange > difVel.y) thrust = 1.0;
		else thrust = difVel.y / velChange;
		thrust *= thrust;
		SetThrusterState(ShipType::THRUSTER_BOTTOM, thrust);
	}

	if (difVel.z > 0) {
		float velChange = Pi::GetTimeStep() * stype.linThrust[ShipType::THRUSTER_FRONT] * invMass;
		float thrust;
		if (velChange < difVel.z) thrust = 1.0;
		else thrust = difVel.z / velChange;
		thrust *= thrust;
		SetThrusterState(ShipType::THRUSTER_FRONT, thrust);
	} else {
		float velChange = Pi::GetTimeStep() * stype.linThrust[ShipType::THRUSTER_REAR] * invMass;
		float thrust;
		if (velChange > difVel.z) thrust = 1.0;
		else thrust = difVel.z / velChange;
		thrust *= thrust;
		SetThrusterState(ShipType::THRUSTER_REAR, thrust);
	}
}

