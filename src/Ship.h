#ifndef _SHIP_H
#define _SHIP_H

#include "libs.h"
#include "RigidBody.h"
#include "ShipType.h"

class SpaceStation;

struct shipstats_t {
	int max_capacity;
	int used_capacity;
	int free_capacity;
	int total_mass; // cargo, equipment + hull
	float hyperspace_range;
};

class Ship: public RigidBody {
public:
	Ship(ShipType::Type shipType);
	virtual void AITurn();
	virtual Object::Type GetType() { return Object::SHIP; }
	virtual void SetDockedWith(SpaceStation *);
	SpaceStation *GetDockedWith() { return m_dockedWith; }
	virtual void Render(const Frame *camFrame);
	void SetMesh(ObjMesh *m);
	void SetThrusterState(enum ShipType::Thruster t, float level);
	void SetAngThrusterState(int axis, float level) { m_angThrusters[axis] = CLAMP(level, -1, 1); }
	void ClearThrusterState();
	void SetGunState(int idx, int state);
	const ShipType &GetShipType();
	void CalcStats(shipstats_t *stats);
	void UpdateMass();
	
	class LaserObj: public Object {
	public:
		virtual Object::Type GetType() { return Object::LASER; }
		Ship *owner;
	};

	EquipSet m_equipment;

protected:
	void RenderLaserfire();

	SpaceStation *m_dockedWith;
	enum ShipType::Type m_shipType;
	ObjMesh *m_mesh;
	Uint32 m_gunState[ShipType::GUNMOUNT_MAX];
private:
	float m_thrusters[ShipType::THRUSTER_MAX];
	float m_angThrusters[3];
	dGeomID m_tempLaserGeom[ShipType::GUNMOUNT_MAX];

	LaserObj m_laserCollisionObj;
};

#endif /* _SHIP_H */
