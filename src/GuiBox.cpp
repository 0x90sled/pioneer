
#include "libs.h"
#include "Gui.h"

namespace Gui {

Box::Box(BoxOrientation orient): Container()
{
	m_orient = orient;
	_Init();
}

void Box::_Init()
{
	m_wantedSize[0] = m_wantedSize[1] = 0;
	m_eventMask = EVENT_ALL;
}

void Box::SetSizeRequest(float x, float y)
{
	m_wantedSize[0] = x;
	m_wantedSize[1] = y;
}

void Box::SetSizeRequest(float size[2])
{
	SetSizeRequest(size[0], size[1]);
}

void Box::GetSizeRequested(float size[2])
{
	if (m_wantedSize[0] && m_wantedSize[1]) {
		size[0] = m_wantedSize[0];
		size[1] = m_wantedSize[1];
	} else {
		float want[2];
		want[0] = want[1] = 0;
		// see how big we need to be
		for (std::list<widget_pos>::iterator i = m_children.begin(); i != m_children.end(); ++i) {
			float rsize[2];
			rsize[0] = size[0];
			rsize[1] = size[1];
			(*i).w->GetSizeRequested(rsize);
			if (m_orient == BOX_VERTICAL) {
				want[0] = MAX(want[0], rsize[0]);
				want[1] += rsize[1];
			} else {
				want[0] += rsize[0];
				want[1] = MAX(want[1], rsize[1]);
			}
		}
		size[0] = want[0];
		size[1] = want[1];
	}
}

Box::~Box()
{
	Screen::RemoveBaseWidget(this);
}

void Box::OnChildResizeRequest(Widget *child)
{
	UpdateChildrenSizes();
}

void Box::PackStart(Widget *child, bool expand)
{
	PrependChild(child, 0, 0);
	m_children.front().flags = expand;
	UpdateChildrenSizes();
	ResizeRequest();
}

void Box::PackEnd(Widget *child, bool expand)
{
	AppendChild(child, 0, 0);
	m_children.back().flags = expand;
	UpdateChildrenSizes();
	ResizeRequest();
}

void Box::UpdateChildrenSizes()
{
	float size[2];
	GetSize(size);
	float pos = 0;
	float space = (m_orient == BOX_VERTICAL ? size[1] : size[0]);
	int num_expand_children = 0;
	// look at all children...
	for (std::list<widget_pos>::iterator i = m_children.begin(); i != m_children.end(); ++i) {
		float rsize[2];
		if ((*i).flags) num_expand_children++;
		if (m_orient == BOX_VERTICAL) {
			rsize[0] = size[0];
			rsize[1] = space;
			(*i).w->GetSizeRequested(rsize);
			if (rsize[0] > size[0]) rsize[0] = size[0];
			if (rsize[1] > space) rsize[1] = space;
			(*i).w->SetSize(size[0], rsize[1]);
			(*i).pos[0] = 0;
			(*i).pos[1] = pos;
			pos += rsize[1];
			space -= rsize[1];
		} else {
			rsize[0] = space;
			rsize[1] = size[1];
			(*i).w->GetSizeRequested(rsize);
			if (rsize[0] > space) rsize[0] = space;
			if (rsize[1] > size[1]) rsize[1] = size[1];
			(*i).w->SetSize(rsize[0], size[1]);
			(*i).pos[0] = pos;
			(*i).pos[1] = 0;
			pos += rsize[0];
			space -= rsize[0];
		}
	}
	pos = 0;
	if ((space > 0) && num_expand_children) {
		/* give expand children the space space */
		for (std::list<widget_pos>::iterator i = m_children.begin(); i != m_children.end(); ++i) {
			bool expand = (*i).flags;
			float s[2];
			(*i).w->GetSize(s);

			if (m_orient == BOX_VERTICAL) {
				(*i).pos[0] = 0;
				(*i).pos[1] = pos;
				if (expand) {
					s[1] += space / num_expand_children;
					(*i).w->SetSize(s[0], s[1]);
				}
				pos += s[1];
			} else {
				(*i).pos[0] = pos;
				(*i).pos[1] = 0;
				if (expand) {
					s[0] += space / num_expand_children;
					(*i).w->SetSize(s[0], s[1]);
				}
				pos += s[0];
			}
		}
	}
	for (std::list<widget_pos>::iterator i = m_children.begin(); i != m_children.end(); ++i) {
		(*i).w->SetPosition((*i).pos[0], (*i).pos[1]);
	}
}

void Box::Remove(Widget *child)
{
	Container::RemoveChild(child);
	UpdateChildrenSizes();
}

}
