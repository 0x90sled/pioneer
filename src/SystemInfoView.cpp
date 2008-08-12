#include "Pi.h"
#include "Sector.h"
#include "SectorView.h"
#include "SystemInfoView.h"
#include "ShipCpanel.h"

SystemInfoView::SystemInfoView(): GenericSystemView()
{
	SetTransparency(true);
	m_bodySelected = 0;
	onSelectedSystemChanged.connect(sigc::mem_fun(this, &SystemInfoView::SystemChanged));
}

void SystemInfoView::OnBodySelected(StarSystem::SBody *b)
{
	m_bodySelected = b;

	std::string desc;

	char buf[1024];

	snprintf(buf, sizeof(buf), "%s: %s\n"
		"Mass                       %.2f %s masses\n",
		b->name.c_str(), b->GetAstroDescription(), b->mass.ToDouble(),
		(b->GetSuperType() == StarSystem::SUPERTYPE_STAR ? "Solar" : "Earth"));
	desc += buf;

	snprintf(buf, sizeof(buf), "Surface temperature        %d C\n", b->averageTemp-273);
	desc += buf;

	// surface temperature
	// major starports
	// orbital period
	// orbital radius
	// ecc, incl

	if (b->parent) {
		float days = b->orbit.period / (60*60*24);
		if (days > 1000) {
			snprintf(buf, sizeof(buf), "Orbital period             %.1f years\n", days / 365);
		} else {
			snprintf(buf, sizeof(buf), "Orbital period             %.1f days\n", b->orbit.period / (60*60*24));
		}
		desc += buf;
		snprintf(buf, sizeof(buf), "Perihelion distance        %.2f AU\n", b->radMin.ToDouble());
		desc += buf;
		snprintf(buf, sizeof(buf), "Aphelion distance          %.2f AU\n", b->radMax.ToDouble());
		desc += buf;
		snprintf(buf, sizeof(buf), "Eccentricity               %.2f\n", b->orbit.eccentricity);
		desc += buf;
	}

	m_infoText->SetText(desc);
}

void SystemInfoView::SystemChanged(StarSystem *s)
{
	DeleteAllChildren();
	float csize[2];
	int majorBodies = 0;
	GetSize(csize);

	float xpos = 0;
	float size[2];
	float ycent;
	std::vector<StarSystem::SBody*>::iterator i = s->rootBody->children.begin();

	if (s->rootBody->type == StarSystem::TYPE_GRAVPOINT) {
		// binary system
		Gui::ImageButton *ib = new Gui::ImageButton((*i)->GetIcon());
		ib->GetSize(size);
		ib->onClick.connect(sigc::bind(sigc::mem_fun(this, &SystemInfoView::OnBodySelected), *i));
		Add(ib, 0, csize[1] - size[1]);
		float yoffset = size[1];
		float xoffset = size[0];
		++i; majorBodies++;
		
		ib = new Gui::ImageButton((*i)->GetIcon());
		ib->GetSize(size);
		ib->onClick.connect(sigc::bind(sigc::mem_fun(this, &SystemInfoView::OnBodySelected), *i));
		Add(ib, 0, csize[1] - size[1] - yoffset);
		++i; majorBodies++;
		
		xpos += xoffset;
		ycent = csize[1] - yoffset*0.5;
	} else {
		Gui::ImageButton *ib = new Gui::ImageButton(s->rootBody->GetIcon());
		ib->GetSize(size);
		ib->onClick.connect(sigc::bind(sigc::mem_fun(this, &SystemInfoView::OnBodySelected), s->rootBody));
		Add(ib, 0, csize[1] - size[1]);
		xpos += size[0];
		ycent = csize[1] - size[1]*0.5;
		majorBodies++;
	}

	for (; i != s->rootBody->children.end(); ++i) {
		Gui::ImageButton *ib = new Gui::ImageButton((*i)->GetIcon());
		ib->GetSize(size);
		ib->onClick.connect(sigc::bind(sigc::mem_fun(this, &SystemInfoView::OnBodySelected), *i));
		Add(ib, xpos, ycent - 0.5*size[1]);
		majorBodies++;

		float moon_ypos = ycent - size[1] - 5;
		if ((*i)->children.size()) for(std::vector<StarSystem::SBody*>::iterator moon = (*i)->children.begin(); moon != (*i)->children.end(); ++moon) {
			float msize[2];
			Gui::ImageButton *ib = new Gui::ImageButton((*moon)->GetIcon());
			ib->GetSize(msize);
			ib->onClick.connect(sigc::bind(sigc::mem_fun(this, &SystemInfoView::OnBodySelected), *moon));
			Add(ib, xpos+0.5*size[0] - 0.5*msize[0], moon_ypos);
			moon_ypos -= msize[1];
		}
		xpos += size[0];
	}
	
	char buf[512];
	snprintf(buf, sizeof(buf), "Stable system with %d major bodies.", majorBodies);
	m_infoText = new Gui::Label(buf);
	m_infoText->SetColor(1,1,0);
	Add(m_infoText, 50, 200);
	
	ShowAll();
}

void SystemInfoView::Draw3D()
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GenericSystemView::Draw3D();
}

void SystemInfoView::Update()
{

}

