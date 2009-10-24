#include "libs.h"
#include "Gui.h"

namespace Gui {

TextEntry::TextEntry()
{
	m_eventMask = EVENT_MOUSEDOWN;
	m_cursPos = 0;
	m_scroll = 0;
	m_rawKbDownCon = Gui::RawEvents::onKeyDown.connect(sigc::mem_fun(this,
				&TextEntry::OnRawKeyDown));
}

TextEntry::~TextEntry()
{
	m_rawKbDownCon.disconnect();
}

void TextEntry::OnRawKeyDown(SDL_KeyboardEvent *event)
{
	Uint16 unicode = event->keysym.unicode;
	if (event->keysym.sym == SDLK_LEFT) SetCursorPos(m_cursPos-1);
	if (event->keysym.sym == SDLK_RIGHT) SetCursorPos(m_cursPos+1);
	if (event->keysym.sym == SDLK_BACKSPACE) {
		if (m_cursPos > 0) {
			m_text = m_text.substr(0, m_cursPos-1) + m_text.substr(m_cursPos);
			SetCursorPos(m_cursPos-1);
		}
	}
	if (event->keysym.sym == SDLK_DELETE) {
		if (m_cursPos < (signed)m_text.size()) {
			m_text = m_text.substr(0, m_cursPos) + m_text.substr(m_cursPos+1);
		}
	}
	if (isalnum(unicode) || (unicode == ' ') || (unicode == '_')) {
		char buf[2] = { (char)unicode, 0 };
		m_text.insert(m_cursPos, std::string(buf));
		SetCursorPos(m_cursPos+1);
	}
}

void TextEntry::GetSizeRequested(float size[2])
{
	size[1] = 1.5*Gui::Screen::GetFontHeight() + 2.0;
}

bool TextEntry::OnMouseDown(MouseButtonEvent *e)
{
	unsigned int len = m_text.size();
	unsigned int i = 0;

	for (; i<len; i++) {
		float x,y;
		Gui::Screen::MeasureString(m_text.substr(0, i), x, y);
		if (x-m_scroll > e->x) {
			SetCursorPos(i-1);
			break;
		}
	}
	if (i == len) SetCursorPos(len);

	return false;
}

void TextEntry::Draw()
{
	float size[2];
	GetSize(size);

	// find cursor position
	float curs_x, curs_y;
	glColor3f(1,0,0);
	Gui::Screen::MeasureString(m_text.substr(0, m_cursPos), curs_x, curs_y);
	if (curs_x - m_scroll > size[0]*0.75f) {
		m_scroll += size[0]*0.25f;
	} else if (curs_x - m_scroll < size[0]*0.25f) {
		m_scroll -= size[0]*0.25f;
		if (m_scroll < 0) m_scroll = 0;
	}

	glColor3f(0,0,0);
	glBegin(GL_TRIANGLE_FAN);
		glVertex2f(0,size[1]);
		glVertex2f(size[0],size[1]);
		glVertex2f(size[0],0);
		glVertex2f(0,0);
	glEnd();
	glColor3f(1,1,1);
	glBegin(GL_LINE_LOOP);
		glVertex2f(0,0);
		glVertex2f(size[0],0);
		glVertex2f(size[0],size[1]);
		glVertex2f(0,size[1]);
	glEnd();


	SetClipping(size[0], size[1]);
	glPushMatrix();
	glTranslatef(1.0f - m_scroll, 1.0f, 0.0f);
	Gui::Screen::RenderString(m_text);

	/* Cursor */
	glColor3f(0.5,0.5,0.5);
	glBegin(GL_LINES);
		glVertex2f(curs_x, 0);
		glVertex2f(curs_x, size[1]);
	glEnd();
	
	glPopMatrix();
	
	EndClipping();
}

} /* namespace Gui */
