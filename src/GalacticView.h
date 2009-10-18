#ifndef _GALACTICVIEW_H
#define _GALACTICVIEW_H

#include "libs.h"
#include "Gui.h"
#include "View.h"
#include <vector>
#include <string>
#include "GenericSystemView.h"

class GalacticView: public GenericSystemView {
public:
	GalacticView();
	virtual ~GalacticView();
	virtual void Update();
	virtual void Draw3D();
	virtual void Save();
	virtual void Load();
	virtual void OnSwitchTo() {}
private:
	void OnClickGalacticView();
	void PutLabels(vector3d offset);
	Gui::ImageButton *m_zoomInButton;
	Gui::ImageButton *m_zoomOutButton;
	float m_zoom;
	GLuint m_texture;
};

#endif /* _GALACTICVIEW_H */
