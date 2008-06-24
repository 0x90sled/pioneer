#include "Gui.h"
#include "GuiContainer.h"

namespace Gui {

void Container::HandleMouseEvent(MouseButtonEvent *e)
{
	float x = e->x;
	float y = e->y;
	for (std::list<widget_pos>::iterator i = m_children.begin(); i != m_children.end(); ++i) {
		float pos[2],size[2];
		if (!(*i).w->IsVisible()) continue;
		int evmask = (*i).w->GetEventMask();
		if (e->isdown) {
			if (!(evmask & Widget::EVENT_MOUSEDOWN)) continue;
		} else {
			if (!(evmask & Widget::EVENT_MOUSEUP)) continue;
		}
		(*i).w->GetPosition(pos);
		(*i).w->GetSize(size);

		if ((x >= pos[0]) && (x < pos[0]+size[0]) &&
		    (y >= pos[1]) && (y < pos[1]+size[1])) {
			e->x = x-pos[0];
			e->y = y-pos[1];
			if (e->isdown) {
				(*i).w->OnMouseDown(e);
			} else {
				(*i).w->OnMouseUp(e);
			}
		}
	}
}

void Container::DeleteAllChildren()
{
	for (std::list<widget_pos>::iterator i = m_children.begin(); i != m_children.end(); ++i) {
		delete (*i).w;
	}
	m_children.clear();
}

void Container::PrependChild(Widget *child, float x, float y)
{
	widget_pos wp;
	wp.w = child;
	wp.pos[0] = x; wp.pos[1] = y;
	child->SetPosition(x, y);
	child->SetParent(this);
	m_children.push_front(wp);
}
	
void Container::AppendChild(Widget *child, float x, float y)
{
	widget_pos wp;
	wp.w = child;
	wp.pos[0] = x; wp.pos[1] = y;
	child->SetPosition(x, y);
	child->SetParent(this);
	m_children.push_back(wp);
}
	
void Container::Draw()
{
	for (std::list<widget_pos>::iterator i = m_children.begin(); i != m_children.end(); ++i) {
		if (!(*i).w->IsVisible()) continue;
		glPushMatrix();
		glTranslatef((*i).pos[0], (*i).pos[1], 0);
		(*i).w->Draw();
		glPopMatrix();
	}
}

void Container::OnMouseDown(MouseButtonEvent *e)
{
	HandleMouseEvent(e);
}

void Container::OnMouseUp(MouseButtonEvent *e)
{
	HandleMouseEvent(e);
}

void Container::ShowAll()
{
	for (std::list<widget_pos>::iterator i = m_children.begin(); i != m_children.end(); ++i) {
		(*i).w->Show();
	}
	Show();
}

void Container::HideAll()
{
	for (std::list<widget_pos>::iterator i = m_children.begin(); i != m_children.end(); ++i) {
		(*i).w->Hide();
	}
	Hide();
}

}
