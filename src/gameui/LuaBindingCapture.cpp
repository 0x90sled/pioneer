// Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "BindingCapture.h"
#include "LuaObject.h"
#include "ui/LuaSignal.h"
#include "LuaPushPull.h"

inline void pi_lua_generic_push(lua_State * l, const KeyBindings::KeyBinding &value) {
	const std::string token = KeyBindings::KeyBindingToString(value);
	pi_lua_generic_push(l, token);
}

namespace GameUI {

class LuaKeyBindingCapture {
public:

	static int l_new(lua_State *l) {
		UI::Context *c = LuaObject<UI::Context>::CheckFromLua(1);
		LuaObject<GameUI::KeyBindingCapture>::PushToLua(new KeyBindingCapture(c));
		return 1;
	}

	static int l_capture(lua_State *l)
	{
		KeyBindingCapture *kbc = LuaObject<GameUI::KeyBindingCapture>::CheckFromLua(1);
		kbc->Capture();
		return 0;
	}

	static int l_attr_binding(lua_State *l)
	{
		KeyBindingCapture *kbc = LuaObject<GameUI::KeyBindingCapture>::CheckFromLua(1);
		const std::string &binding = KeyBindings::KeyBindingToString(kbc->GetBinding());
		lua_pushlstring(l, binding.c_str(), binding.size());
		return 1;
	}

	static int l_attr_binding_description(lua_State *l)
	{
		KeyBindingCapture *kbc = LuaObject<GameUI::KeyBindingCapture>::CheckFromLua(1);
		const std::string &desc = kbc->GetBinding().Description();
		lua_pushlstring(l, desc.c_str(), desc.size());
		return 1;
	}

	static int l_attr_on_capture(lua_State *l) {
		KeyBindingCapture *kbc = LuaObject<GameUI::KeyBindingCapture>::CheckFromLua(1);
		UI::LuaSignal<const KeyBindings::KeyBinding &>().Wrap(l, kbc->onCapture);
		return 1;
	}
};

}

using namespace GameUI;

template <> const char *LuaObject<GameUI::KeyBindingCapture>::s_type = "UI.Game.KeyBindingCapture";

template <> void LuaObject<GameUI::KeyBindingCapture>::RegisterClass()
{
	static const char *l_parent = "UI.Single";

	static const luaL_Reg l_methods[] = {
		{ "New",                LuaKeyBindingCapture::l_new },
		{ "Capture",            LuaKeyBindingCapture::l_capture },
		{ 0, 0 }
	};

	static const luaL_Reg l_attrs[] = {
		{ "binding",            LuaKeyBindingCapture::l_attr_binding },
		{ "bindingDescription", LuaKeyBindingCapture::l_attr_binding_description },
		{ "onCapture",          LuaKeyBindingCapture::l_attr_on_capture },
		{ 0, 0 }
	};

	LuaObjectBase::CreateClass(s_type, l_parent, l_methods, l_attrs, 0);
	LuaObjectBase::RegisterPromotion(l_parent, s_type, LuaObject<GameUI::KeyBindingCapture>::DynamicCastPromotionTest);
}
