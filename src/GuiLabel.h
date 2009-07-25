#ifndef _GUILABEL_H
#define _GUILABEL_H

#include "GuiWidget.h"
#include <string>

class TextLayout;

namespace Gui {
	class Label: public Widget {
	public:
		Label(const char *text);
		Label(const std::string &text);
		virtual void Draw();
		virtual ~Label();
		virtual void GetSizeRequested(float size[2]);
		void SetText(const char *text);
		void SetText(const std::string text);
		void SetColor(float r, float g, float b);
	private:
		void UpdateLayout();
		void RecalcSize();
		std::string m_text;
		float m_color[3];
		GLuint m_dlist;
		TextLayout *m_layout;
	};
}

#endif /* _GUILABEL_H */
