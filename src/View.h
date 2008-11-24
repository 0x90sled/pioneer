#ifndef _VIEW_H
#define _VIEW_H

#include "libs.h"
#include "Gui.h"

/*
 * For whatever draws crap into the main area of the screen.
 * Eg:
 *  game 3d view
 *  system map
 *  sector map
 */
class View: public Gui::Fixed {
public:
	View(): Gui::Fixed(Gui::Screen::GetWidth(), Gui::Screen::GetHeight()-64) {
		Gui::Screen::AddBaseWidget(this, 0, 64);
		
		m_rightButtonBar = new Gui::Fixed(128, 26);
		m_rightButtonBar->SetBgColor(.65, .65, .65);
		Gui::Screen::AddBaseWidget(m_rightButtonBar, Gui::Screen::GetWidth()-128, 0);

		m_rightRegion2 = new Gui::Fixed(122, 17);
		m_rightRegion2->SetTransparency(true);
		Gui::Screen::AddBaseWidget(m_rightRegion2, Gui::Screen::GetWidth()-123, 26);
	}
	virtual ~View() { delete m_rightButtonBar; delete m_rightRegion2; }
	virtual void ShowAll() {
		m_rightButtonBar->ShowAll();
		m_rightRegion2->ShowAll();
		Gui::Fixed::ShowAll();
	}
	virtual void HideAll() {
		m_rightButtonBar->HideAll();
		m_rightRegion2->HideAll();
		Gui::Fixed::HideAll();
	}
	// called before Gui::Draw will call widget ::Draw methods.
	virtual void Draw3D() = 0;
	// for checking key states, mouse crud
	virtual void Update() = 0;
	virtual void Save() {}
	virtual void Load() {}
	virtual void OnSwitchTo() = 0;
protected:
	// each view can put some buttons in the bottom right of the cpanel
	Gui::Fixed *m_rightButtonBar;
//	Gui::Fixed *m_rightRegion1;
	Gui::Fixed *m_rightRegion2;
};

#endif /* _VIEW_H */
