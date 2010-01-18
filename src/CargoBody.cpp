#include "CargoBody.h"
#include "Pi.h"
#include "Serializer.h"
#include "collider/collider.h"
#include "Sfx.h"
#include "Space.h"
#include "LmrModel.h"

void CargoBody::Save()
{
	using namespace Serializer::Write;
	DynamicBody::Save();
	wr_int(static_cast<int>(m_type));
	wr_float(m_hitpoints);
}

void CargoBody::Load()
{
	using namespace Serializer::Read;
	DynamicBody::Load();
	m_type = static_cast<Equip::Type>(rd_int());
	Init();
	m_hitpoints = rd_float();
}

void CargoBody::Init()
{
	m_hitpoints = 1.0f;
	SetLabel(EquipType::types[m_type].name);
	SetModel("cargo");
	SetMassDistributionFromModel();
}

CargoBody::CargoBody(Equip::Type t)
{
	m_type = t;
	Init();	
	SetMass(1.0);
}

bool CargoBody::OnDamage(Object *attacker, float kgDamage)
{
	m_hitpoints -= kgDamage*0.001f;
	if (m_hitpoints < 0) {
		Space::KillBody(this);
		Sfx::Add(this, Sfx::TYPE_EXPLOSION);
	}
	return true;
}

void CargoBody::Render(const vector3d &viewCoords, const matrix4x4d &viewTransform)
{
	if (!IsEnabled()) return;
	GetLmrObjParams().argStrings[0] = EquipType::types[m_type].name;
	RenderLmrModel(viewCoords, viewTransform);
}

