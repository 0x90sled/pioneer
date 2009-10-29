#ifndef _SHIPTYPE_H
#define _SHIPTYPE_H

#include "libs.h"
#include "vector3.h"
#include "EquipType.h"
#include <vector>

struct ShipType {
	enum Thruster { THRUSTER_FRONT, THRUSTER_REAR, THRUSTER_TOP, THRUSTER_BOTTOM, THRUSTER_LEFT, THRUSTER_RIGHT, THRUSTER_MAX };
	enum Type { SWORDFISH, SIRIUS_INTERDICTOR, LADYBIRD, TAIPAN, WALRUS, FLOWERFAIRY,
		MISSILE_GUIDED, MISSILE_SMART, MISSILE_NAVAL, END=MISSILE_GUIDED };
	enum { GUN_FRONT, GUN_REAR, GUNMOUNT_MAX = 2 };

	////////
	const char *name;
	const char *sbreModelName;
	float linThrust[THRUSTER_MAX];
	float angThrust;
	struct GunMount {
		vector3f pos;
		vector3f dir;
	} gunMount[GUNMOUNT_MAX];
	int equipSlotCapacity[Equip::SLOT_MAX];
	int capacity; // tonnes
	int hullMass;
	int baseprice;
	Equip::Type hyperdrive;
	///////

	static const ShipType types[];
	static const char *gunmountNames[GUNMOUNT_MAX];
};

class EquipSet {
public:
	EquipSet() {}
	EquipSet(const ShipType::Type t) {
		for (int i=0; i<Equip::SLOT_MAX; i++) {
			equip[i] = std::vector<Equip::Type>(ShipType::types[t].equipSlotCapacity[i]);
		}
	}
	int GetSlotSize(Equip::Slot s) const {
		return equip[s].size();
	}
	Equip::Type Get(Equip::Slot s) const {
		return equip[s][0];
	}
	Equip::Type Get(Equip::Slot s, int idx) const {
		return equip[s][idx];
	}
	void Set(Equip::Slot s, int idx, Equip::Type e) {
		equip[s][idx] = e;
	}
	bool Add(Equip::Slot s, Equip::Type e) {
		for (unsigned int i=0; i<equip[s].size(); i++) {
			if (equip[s][i] == Equip::NONE) {
				equip[s][i] = e;
				return true;
			}
		}
		return false;
	}
	void Remove(Equip::Slot s, Equip::Type e, int num) {
		for (unsigned int i=0; i<equip[s].size(); i++) {
			if (num == 0) break;
			if (equip[s][i] == e) {
				equip[s][i] = Equip::NONE;
				num--;
			}
		}
	}
	int Count(Equip::Slot s, Equip::Type e) const {
		int num = 0;
		for (unsigned int i=0; i<equip[s].size(); i++) {
			if (equip[s][i] == e) num++;
		}
		return num;
	}
	int FreeSpace(Equip::Slot s) const {
		int free = 0;
		for (unsigned int i=0; i<equip[s].size(); i++) {
			if (equip[s][i] == Equip::NONE) free++;
		}
		return free;
	}
	void Save();
	void Load();
private:
	std::vector<Equip::Type> equip[Equip::SLOT_MAX];
};


#endif /* _SHIPTYPE_H */
