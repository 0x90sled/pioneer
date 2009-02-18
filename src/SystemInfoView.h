#ifndef _SYSTEMINFOVIEW_H
#define _SYSTEMINFOVIEW_H

#include "libs.h"
#include "Gui.h"
#include "View.h"
#include "GenericSystemView.h"

class StarSystem;
class SBody;

class SystemInfoView: public GenericSystemView {
public:
	SystemInfoView();
	virtual void Update();
	virtual void Draw3D();
	virtual void OnSwitchTo() {}
private:

	void SystemChanged(StarSystem *s);
	void OnBodySelected(SBody *b);
	void PutBodies(SBody *body, Gui::Fixed *container, int dir, float pos[2], int &majorBodies, float prevSize);
	SBody *m_bodySelected;
	Gui::Label *m_infoLabel, *m_infoData,
		*m_econLabel, *m_econData;
	Gui::Fixed *m_sbodyInfoTab, *m_econInfoTab;
};

#endif /* _SYSTEMINFOVIEW_H */
