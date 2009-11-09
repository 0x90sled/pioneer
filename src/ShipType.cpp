#include "ShipType.h"
#include "Serializer.h"

const char *ShipType::gunmountNames[GUNMOUNT_MAX] = {
	"Front", "Rear" };

const ShipType ShipType::types[] = {
	{
		"Swordfish Starfighter",
		"66",
		{ 2e6,-2e6,1e6,-1e6,-1e6,1e6 },
		1e7,
		{
			{ vector3f(0,-0.5,0), vector3f(0,0,-1) },
			{ vector3f(0,0,0), vector3f(0,0,1) }
		},
		{ 20, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
		20, 20, 4000000,
		Equip::DRIVE_CLASS1,
	}, {
		// besides running a wicked corporatist regime in the
		// sirius system, Sirius corporation make a range of
		// lovely starships
		"Sirius Interdictor", "61",
		{ 2e7,-2e7,1e7,-1e7,-1e7,1e7 },
		4e7,
		{
			{ vector3f(0,-0.5,0), vector3f(0,0,-1) },
			{ vector3f(0,-0.5,0), vector3f(0,0,1) }
		},
		{ 90, 1, 2, 8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
		90, 100, 16000000,
		Equip::DRIVE_CLASS3,
	}, {
		// john - you should pick names yourself or this happens
		"Ladybird Starfighter",
		"62",
		{ 2e6,-2e6,1e6,-1e6,-1e6,1e6 },
		1e7,
		{
			{ vector3f(0,-0.5,0), vector3f(0,0,-1) },
			{ vector3f(0,0,0), vector3f(0,0,1) }
		},
		{ 60, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
		60, 60, 8700000,
		Equip::DRIVE_CLASS2,
	}, {
		"Taipan",
		"taipan",
		{ 4e6,-4e6,1e6,-1e6,-1e6,1e6 },
		1e7,
		{
			{ vector3f(0,-0.5,0), vector3f(0,0,-1) },
			{ vector3f(0,0,0), vector3f(0,0,1) }
		},
		{ 240, 1, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
		240, 200, 56000000,
		Equip::DRIVE_CLASS4,
	}, {
		"Walrus",
		"60",
		{ 12e6,-12e6,4e6,-4e6,-4e6,4e6 },
		1e7,
		{
			{ vector3f(0,-0.5,0), vector3f(0,0,-1) },
			{ vector3f(0,0,0), vector3f(0,0,1) }
		},
		{ 320, 1, 1, 6, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
		320, 300, 35000000,
		Equip::DRIVE_CLASS5,
	}, {
		"Flowerfairy Heavy Trader",
		"63",
		{ 1e5,-1e5,1e5,-1e5,-1e5,1e5 },
		1e7,
		{
			{ vector3f(0,-0.5,0), vector3f(0,0,-1) },
			{ vector3f(0,0,0), vector3f(0,0,1) }
		},
		{ 500, 1, 2, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
		500, 500, 55000000,
		Equip::DRIVE_CLASS6,
	}, {
		0,
		"missile",
		{ 0, -4e5, 0, 0, 0, 0 },
		0, {},
		{ 0, 0, 1, 0 },
		10, 1, 100
	}, {
		0,
		"missile",
		{ 1e5, -2e5, 0, 0, 0, 0 },
		2e4, {},
		{ 0, 0, 1, 0 },
		10, 1, 100
	}, {
		0,
		"missile",
		{ 1.5e5, -3e5, 0, 0, 0, 0 },
		2e4, {},
		{ 0, 0, 1, 0 },
		10, 1, 100
	}, {
		0,
		"missile",
		{ 2.0e5, -4e5, 0, 0, 0, 0 },
		2e4, {},
		{ 0, 0, 1, 0 },
		10, 1, 100
	}
};

void EquipSet::Save()
{
	using namespace Serializer::Write;
	wr_int(Equip::SLOT_MAX);
	for (int i=0; i<Equip::SLOT_MAX; i++) {
		wr_int(equip[i].size());
		for (unsigned int j=0; j<equip[i].size(); j++) {
			wr_int(static_cast<int>(equip[i][j]));
		}
	}
}

/*
 * Should have initialised with EquipSet(ShipType::Type) first
 */
void EquipSet::Load()
{
	using namespace Serializer::Read;
	const int numSlots = rd_int();
	assert(numSlots <= Equip::SLOT_MAX);
	for (int i=0; i<numSlots; i++) {
		const int numItems = rd_int();
		assert(numItems <= (signed)equip[i].size());
		for (int j=0; j<numItems; j++) {
			equip[i][j] = static_cast<Equip::Type>(rd_int());
		}
	}
	onChange.emit();
}

