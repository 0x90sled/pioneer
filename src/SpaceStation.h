#ifndef _SPACESTATION_H
#define _SPACESTATION_H

#include "libs.h"
#include "ModelBody.h"
#include "ShipType.h"
#include "MarketAgent.h"
#include "ShipFlavour.h"

#define MAX_DOCKING_PORTS	4

class CollMeshSet;
class Ship;
class Mission;
class CityOnPlanet;
struct SpaceStationType;

class SBody;

class SpaceStation: public ModelBody, public MarketAgent {
public:
	OBJDEF(SpaceStation, ModelBody, SPACESTATION);
	static void Init();
	enum TYPE { JJHOOP, GROUND_FLAVOURED, TYPE_MAX };
	// Should point to SBody in Pi::currentSystem
	SpaceStation(const SBody *);
	SpaceStation() {}
	virtual ~SpaceStation();
	virtual double GetBoundingRadius() const;
	virtual bool OnCollision(Object *b, Uint32 flags, double relVel);
	virtual void Render(const vector3d &viewCoords, const matrix4x4d &viewTransform);
	void LaunchShip(Ship *ship, int port);
	void OrientDockedShip(Ship *ship, int port) const;
	bool GetDockingClearance(Ship *s, std::string &outMsg);
	virtual void TimeStepUpdate(const float timeStep);
	bool IsGroundStation() const;
	float GetDesiredAngVel() const;
	struct positionOrient_t {
		bool exists;
		vector3d pos;
		vector3d xaxis;
		vector3d yaxis;
	};
	// stage 1 position
	positionOrient_t port[MAX_DOCKING_PORTS];
	// stage 2 position of ship (inside station)
	positionOrient_t port_s2[MAX_DOCKING_PORTS];
	positionOrient_t port_s3[MAX_DOCKING_PORTS];
	void AddEquipmentStock(Equip::Type t, int num) { m_equipmentStock[t] += num; }
	/* MarketAgent stuff */
	int GetStock(Equip::Type t) const { return m_equipmentStock[t]; }
	Sint64 GetPrice(Equip::Type t) const;
	bool CanBuy(Equip::Type t) const;
	bool CanSell(Equip::Type t) const;
	bool DoesSell(Equip::Type t) const;
	const SBody *GetSBody() const { return m_sbody; }
	void ReplaceShipOnSale(int idx, const ShipFlavour *with);
	std::vector<ShipFlavour> &GetShipsOnSale() { return m_shipsOnSale; }
	const std::vector<Mission*> &GetBBMissions() { return m_bbmissions; }
	// does not dealloc
	bool BBRemoveMission(Mission *m);
	virtual void PostLoadFixup();
	virtual void NotifyDeleted(const Body* const deletedBody);
	void PositionDockedShip(Ship *ship, int port);
	sigc::signal<void> onShipsForSaleChanged;
	sigc::signal<void> onBulletinBoardChanged;
protected:
	virtual void Save();
	virtual void Load();
	/* MarketAgent stuff */
	void Bought(Equip::Type t);
	void Sold(Equip::Type t);
private:
	void DoDockingAnimation(const float timeStep);

	struct shipDocking_t {
		Ship *ship;
		int stage;
		vector3d from;
		float stagePos; // 0 -> 1.0
	};
	shipDocking_t m_shipDocking[MAX_DOCKING_PORTS];

	float m_openAnimState[MAX_DOCKING_PORTS];
	float m_dockAnimState[MAX_DOCKING_PORTS];

	void InitStation();
	void UpdateShipyard();
	void UpdateBB();
	const SpaceStationType *m_type;
	const SBody *m_sbody;
	int m_numPorts;
	int m_equipmentStock[Equip::TYPE_MAX];
	std::vector<ShipFlavour> m_shipsOnSale;
	std::vector<Mission*> m_bbmissions;
	double m_lastUpdatedShipyard;
	CityOnPlanet *m_adjacentCity;
};

#endif /* _SPACESTATION_H */
