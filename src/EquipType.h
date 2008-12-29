#ifndef _EQUIPTYPE_H
#define _EQUIPTYPE_H

namespace Equip {
	enum Slot { SLOT_CARGO, SLOT_ENGINE, SLOT_LASER, SLOT_MISSILE, SLOT_MAX };
	enum Type { NONE, HYDROGEN, LIQUID_OXYGEN, METAL_ORE, CARBON_ORE, METAL_ALLOYS, PLASTICS, FRUIT_AND_VEG, ANIMAL_MEAT, LIQUOR, GRAIN, TEXTILES, FERTILIZER, WATER, MEDICINES, CONSUMER_GOODS, COMPUTERS, ROBOTS, PRECIOUS_METALS, INDUSTRIAL_MACHINERY, FARM_MACHINERY, MINING_MACHINERY, AIR_PROCESSORS, HAND_WEAPONS, BATTLE_WEAPONS, NARCOTICS, DRIVE_INTERPLANETARY, DRIVE_CLASS1, DRIVE_CLASS2,
	DRIVE_CLASS3, DRIVE_CLASS4, DRIVE_CLASS5, DRIVE_CLASS6,
	LASER_1MW_BEAM, LASER_2MW_BEAM, LASER_4MW_BEAM, TYPE_MAX,
	FIRST_COMMODITY=HYDROGEN, LAST_COMMODITY=NARCOTICS };
};

#define EQUIP_INPUTS	2

struct EquipType {
	const char *name;
	const char *description;
	Equip::Slot slot;
	Equip::Type inputs[EQUIP_INPUTS]; // production requirement. eg metal alloys input would be metal ore
	int basePrice;
	int mass;
	int pval; // hello angband. used for general 'power' attribute...
	int econType;
	int techLevel;
	
	static const EquipType types[];
};

#endif /* _EQUIPTYPE_H */
