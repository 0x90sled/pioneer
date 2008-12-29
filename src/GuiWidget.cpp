#include "Gui.h"

namespace Gui {

Widget::Widget()
{
	m_parent = 0;
	m_enabled = true;
	m_visible = false;
	m_mouseOver = false;
	m_eventMask = EVENT_MOUSEMOTION;
	m_tooltipWidget = 0;
	m_tooltipTimerSignal.connect(sigc::mem_fun(this, &Widget::OnToolTip));
	m_shortcut.sym = (SDLKey)0;
	m_shortcut.mod = (SDLMod)0;
}

bool Widget::IsVisible() const
{
	if (!m_visible) return false;
	Container *parent = m_parent;
	while ((parent) && (parent->m_parent)) {
		if (parent->m_visible == false) return false;
		parent = parent->m_parent;
	}
	if (Screen::IsBaseWidget(parent))
		return parent->m_visible;
	else
		return false;
}

void Widget::SetClipping(float width, float height)
{
	const GLdouble eqn1[4] = {1,0,0,0};
	const GLdouble eqn2[4] = {0,1,0,0};
	const GLdouble eqn3[4] = {-1,0,0,0};
	const GLdouble eqn4[4] = {0,-1,0,0};
	glClipPlane(GL_CLIP_PLANE0, eqn1);
	glClipPlane(GL_CLIP_PLANE1, eqn2);
	glPushMatrix();
	glTranslatef(width, height, 0);
	glClipPlane(GL_CLIP_PLANE2, eqn3);
	glClipPlane(GL_CLIP_PLANE3, eqn4);
	glPopMatrix();
	glEnable(GL_CLIP_PLANE0);
	glEnable(GL_CLIP_PLANE1);
	glEnable(GL_CLIP_PLANE2);
	glEnable(GL_CLIP_PLANE3);
}

void Widget::EndClipping()
{
	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_CLIP_PLANE1);
	glDisable(GL_CLIP_PLANE2);
	glDisable(GL_CLIP_PLANE3);
}

void Widget::SetShortcut(SDLKey key, SDLMod mod)
{
	assert(m_shortcut.sym == 0); // because AddShortcutWidget will add more than once. fix this otherwise on destruct we leave bad pointers in the Screen shortcut widgets list
	m_shortcut.sym = key;
	m_shortcut.mod = mod;
	Screen::AddShortcutWidget(this);
}

void Widget::OnPreShortcut(const SDL_keysym *sym)
{
	int mod = sym->mod & 0xfff; // filters out numlock, capslock, which fuck things up
	if ((sym->sym == m_shortcut.sym) && (mod == m_shortcut.mod)) {
		OnActivate();
	}
}

void Widget::GetAbsolutePosition(float pos[2])
{
	GetPosition(pos);
	const Container *parent = GetParent();
	while (parent) {
		pos[0] += parent->m_size.x;
		pos[1] += parent->m_size.y;
		parent = parent->GetParent();
	}
}
	
void Widget::OnMouseEnter()
{
	m_mouseOver = true;
	Gui::AddTimer(1000, &m_tooltipTimerSignal);
	onMouseEnter.emit();
}

void Widget::OnMouseLeave()
{
	m_mouseOver = false;
	if (m_tooltipWidget) {
		Screen::RemoveBaseWidget(m_tooltipWidget);
		m_tooltipWidget = 0;
	}
	Gui::RemoveTimer(&m_tooltipTimerSignal);
	onMouseLeave.emit();
}

void Widget::UpdateOverriddenTooltip()
{
	if (m_tooltipWidget) {
		std::string text = GetOverrideTooltip();
		m_tooltipWidget->SetText(text);
	}
}

void Widget::OnToolTip()
{
	if (!m_tooltipWidget) {
		std::string text = GetOverrideTooltip();
		if (text == "") text = m_tooltip;
		if (text == "") return;

		float pos[2];
		GetAbsolutePosition(pos);
		m_tooltipWidget = new ToolTip(text);
		if (m_tooltipWidget->m_size.w + pos[0] > Screen::GetWidth())
			pos[0] = Screen::GetWidth() - m_tooltipWidget->m_size.w;
		if (m_tooltipWidget->m_size.h + pos[1] > Screen::GetHeight())
			pos[1] = Screen::GetHeight() - m_tooltipWidget->m_size.h;

		Screen::AddBaseWidget(m_tooltipWidget, pos[0], pos[1]);
		m_tooltipWidget->Show();
	}
}

void Widget::Hide()
{
	m_visible = false;
	if (m_tooltipWidget) {
		Screen::RemoveBaseWidget(m_tooltipWidget);
		delete m_tooltipWidget;
		m_tooltipWidget = 0;
	}
}

void Widget::ResizeRequest()
{
	if (m_parent) m_parent->OnChildResizeRequest(this);
	else {
		float size[2] = { FLT_MAX, FLT_MAX };
		GetSizeRequested(size);
		SetSize(size[0], size[1]);
	}
}

Widget::~Widget()
{
	if (m_tooltipWidget) {
		Screen::RemoveBaseWidget(m_tooltipWidget);
		delete m_tooltipWidget;
	}
	Screen::RemoveShortcutWidget(this);
	Gui::RemoveTimer(&m_tooltipTimerSignal);
}

}
