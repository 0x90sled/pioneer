#include "libs.h"
#include "Gui.h"

#define BUTTON_SIZE	16

namespace Gui {
ToggleButton::ToggleButton()
{
	m_pressed = false;
	SetSize(BUTTON_SIZE, BUTTON_SIZE);
}
void ToggleButton::OnMouseDown(MouseButtonEvent *e)
{
	if (e->button == 1) {
		onPress.emit();
		m_pressed = !m_pressed;
		if (m_pressed) {
			onSelect.emit(this);
		} else {
			onDeselect.emit(this);
		}
	}
}
void ToggleButton::GetSizeRequested(float &w, float &h)
{
	w = BUTTON_SIZE;
	h = BUTTON_SIZE;
}

void ToggleButton::Draw()
{
	if (m_pressed) {
		glBegin(GL_QUADS);
			glColor3fv(Color::bgShadow);
			glVertex2f(0,0);
			glVertex2f(15,0);
			glVertex2f(15,15);
			glVertex2f(0,15);
			
			glColor3f(.6,.6,.6);
			glVertex2f(2,0);
			glVertex2f(15,0);
			glVertex2f(15,13);
			glVertex2f(2,13);
			
			glColor3fv(Color::bg);
			glVertex2f(2,2);
			glVertex2f(13,2);
			glVertex2f(13,13);
			glVertex2f(2,13);
		glEnd();
	} else {
		glBegin(GL_QUADS);
			glColor3f(.6,.6,.6);
			glVertex2f(0,0);
			glVertex2f(15,0);
			glVertex2f(15,15);
			glVertex2f(0,15);
			
			glColor3fv(Color::bgShadow);
			glVertex2f(2,0);
			glVertex2f(15,0);
			glVertex2f(15,13);
			glVertex2f(2,13);
			
			glColor3fv(Color::bg);
			glVertex2f(2,2);
			glVertex2f(13,2);
			glVertex2f(13,13);
			glVertex2f(2,13);
		glEnd();
	}

}
}
