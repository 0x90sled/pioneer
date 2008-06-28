#include "InfoView.h"
#include "Pi.h"
#include "Player.h"
#include "WorldView.h"
#include "Ship.h"

InfoView::InfoView(): View()
{
	SetTransparency(true);

	float size[2];
	GetSize(size);
	
	info1 = new Gui::Label("some crap starshit");
	Add(info1, 40, size[1]-40);
}

void InfoView::UpdateInfo()
{
	char buf[512];
	std::string nfo;
	const ShipType &stype = Pi::player->GetShipType();
	nfo = "SHIP INFORMATION:  "+std::string(stype.name);
	
	Equip::Type e = Pi::player->m_equipment.Get(Equip::SLOT_ENGINE);
	nfo += std::string("\n\nDrive system:      ")+EquipType::types[e].name;

	shipstats_t stats;
	Pi::player->CalcStats(&stats);
	snprintf(buf, sizeof(buf), "\n\nCapacity:          %dt\n"
				       "Free:              %dt\n"
			               "Used:              %dt\n"
				       "All-up weight:     %dt", stats.max_capacity,
			stats.free_capacity, stats.used_capacity, stats.total_mass);
	nfo += std::string(buf);

	snprintf(buf, sizeof(buf), "\n\nHyperspace range:  %.2f light years", stats.hyperspace_range);
	nfo += std::string(buf);

	info1->SetText(nfo);
}

static ObjParams params = {
	{ 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f },

	{	// pColor[3]
	{ { 1.0f, 0.0f, 1.0f }, { 0, 0, 0 }, { 0, 0, 0 }, 0 },
	{ { 0.8f, 0.6f, 0.5f }, { 0, 0, 0 }, { 0, 0, 0 }, 0 },
	{ { 0.5f, 0.5f, 0.5f }, { 0, 0, 0 }, { 0, 0, 0 }, 0 } },

	// pText[3][256]	
	{ "IR-L33T", "ME TOO" },
};

void InfoView::Draw3D()
{
	static float rot1, rot2;
	rot1 += .5*Pi::GetFrameTime();
	rot2 += Pi::GetFrameTime();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// why the hell do i give these functions such big names..
	glFrustum(-Pi::GetScrWidth()*.5, Pi::GetScrWidth()*.5,
		  -Pi::GetScrHeight()*.5, Pi::GetScrHeight()*.5,
		   Pi::GetScrWidth()*.5, 100000);
	glDepthRange (-10, -100000);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glClearColor(0,.2,.4,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float lightCol[] = { 1,1,1 };
	float lightDir[] = { 1,0,0 };

	glPushAttrib(GL_ALL_ATTRIB_BITS);
	sbreSetDirLight (lightCol, lightDir);
	sbreSetViewport(Pi::GetScrWidth(), Pi::GetScrHeight(), Pi::GetScrWidth()*0.5, 5.0f, 100000.0f, 0.0f, 1.0f);
	// sod you sbre i want my own viewport!
	glViewport(Pi::GetScrWidth()/4, 0, Pi::GetScrWidth(), Pi::GetScrHeight());
	
	Vector p; p.x = 0; p.y = 0; p.z = 100;
	matrix4x4d rot = matrix4x4d::RotateXMatrix(rot1);
	rot.RotateY(rot2);
	Matrix m;
	m.x1 = rot[0]; m.x2 = rot[4]; m.x3 = -rot[8];
	m.y1 = rot[1]; m.y2 = rot[5]; m.y3 = -rot[9];
	m.z1 = -rot[2]; m.z2 = -rot[6]; m.z3 = rot[10];
	const ShipType &stype = Pi::player->GetShipType();
	
	sbreRenderModel(&p, &m, stype.sbreModel, &params);
	glPopAttrib();
}

void InfoView::Update()
{
}
