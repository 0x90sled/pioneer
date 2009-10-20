#ifndef _SECTORVIEW_H
#define _SECTORVIEW_H

#include "libs.h"
#include "Gui.h"
#include "View.h"
#include <vector>
#include <string>
#include "GenericSystemView.h"
#include "SysLoc.h"

class SectorView: public GenericSystemView {
public:
	SectorView();
	virtual ~SectorView();
	virtual void Update();
	virtual void Draw3D();
	bool GetSelectedSystem(int *sector_x, int *sector_y, int *system_idx);
	void GotoSystem(int sector_x, int sector_y, int system_idx);
	void GetSector(int *outSecX, int *outSecY) const { *outSecX = m_secx; *outSecY = m_secy; }
	virtual void Save();
	virtual void Load();
	virtual void OnSwitchTo() {}
private:
	void DrawSector(int x, int y);
	void PutClickableLabel(std::string &text, int sx, int sy, int sys_idx);
	void OnClickSystem(const Gui::MouseButtonEvent *e, int sx, int sy, int sys_idx);

	float m_zoom;
	int m_secx, m_secy;
	int m_selected;
	float m_px, m_py;
	float m_rot_x, m_rot_z;
	float m_pxMovingTo, m_pyMovingTo;
	Gui::Label *m_infoLabel;
	Gui::ImageButton *m_zoomInButton;
	Gui::ImageButton *m_zoomOutButton;
	Gui::ImageButton *m_galaxyButton;
	GLuint m_gluDiskDlist;
	
	SysLoc m_lastShownLoc;
	Gui::Label *m_systemName;
	Gui::Label *m_distance;
	Gui::Label *m_starType;
	Gui::Label *m_shortDesc;
};

#endif /* _SECTORVIEW_H */
