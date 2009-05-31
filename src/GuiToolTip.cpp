#include "Gui.h"

namespace Gui {

#define TOOLTIP_PADDING	5
#define FADE_TIME_MS	500

ToolTip::ToolTip(const char *text)
{
	SetText(text);
	m_createdTime = SDL_GetTicks();
}

void ToolTip::CalcSize()
{
	float size[2];
	Screen::MeasureLayout(m_text, 400.0, size);
	size[0] += 2*TOOLTIP_PADDING;
	SetSize(size[0], size[1]);
}

ToolTip::ToolTip(std::string &text)
{
	SetText(text);
	m_createdTime = SDL_GetTicks();
}

void ToolTip::SetText(const char *text)
{
	m_text = text;
	CalcSize();
}

void ToolTip::SetText(std::string &text)
{
	m_text = text;
	CalcSize();
}

void ToolTip::Draw()
{
	float size[2];
	int age = SDL_GetTicks() - m_createdTime;
	float alpha = age/(float)FADE_TIME_MS; alpha = MIN(alpha, 0.75f);
	glEnable(GL_BLEND);
	GetSize(size);
	glColor4f(.2f,.2f,.6f,alpha);
	glBegin(GL_QUADS);
		glVertex2f(0, 0);
		glVertex2f(0, size[1]);
		glVertex2f(size[0], size[1]);
		glVertex2f(size[0], 0);
	glEnd();
	glColor4f(0,0,.8f,alpha);
	glBegin(GL_LINE_LOOP);
		glVertex2f(size[0], 0);
		glVertex2f(size[0], size[1]);
		glVertex2f(0, size[1]);
		glVertex2f(0, 0);
	glEnd();
	glPushMatrix();
	glTranslatef(TOOLTIP_PADDING,0,0);
	glColor4f(1,1,1,alpha);
	Screen::LayoutString(m_text, size[0]-2*TOOLTIP_PADDING);
	glPopMatrix();
	glDisable(GL_BLEND);
}

void ToolTip::GetSizeRequested(float size[2])
{
	Screen::MeasureLayout(m_text, size[0] - 2*TOOLTIP_PADDING, size);
	size[0] += 2*TOOLTIP_PADDING;
}

}
