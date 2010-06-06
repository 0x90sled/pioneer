#include "libs.h"
#include "PiLuaAPI.h"
#include "Pi.h"
#include "Space.h"
#include "ShipCpanel.h"
#include "Ship.h"
#include "Player.h"
#include "SpaceStation.h"
#include "StarSystem.h"
#include "Sound.h"
#include "LuaChatForm.h"
#include "NameGenerator.h"
#include "HyperspaceCloud.h"

////////////////////////////////////////////////////////////

EXPORT_OOLUA_FUNCTIONS_11_NON_CONST(ObjectWrapper,
		ShipAIDoKill,
		ShipAIDoFlyTo,
		ShipAIDoLowOrbit,
		ShipAIDoMediumOrbit,
		ShipAIDoHighOrbit,
		SetMoney,
		AddMoney,
		SpaceStationAddAdvert,
		SpaceStationRemoveAdvert,
		GetDockedWith,
		GetSBody)
EXPORT_OOLUA_FUNCTIONS_3_CONST(ObjectWrapper,
		IsBody,
		GetMoney,
		GetLabel)

ObjectWrapper::ObjectWrapper(Object *o): m_obj(o) {
	if (o) {
		m_delCon = o->onDelete.connect(sigc::mem_fun(this, &ObjectWrapper::OnDelete));
	}
}
bool ObjectWrapper::IsBody() const {
	return Is(Object::BODY);
}
double ObjectWrapper::GetMoney() const {
	if (Is(Object::SHIP)) {
		Ship *s = static_cast<Ship*>(m_obj);
		return 0.01 * s->GetMoney();
	} else {
		return 0;
	}
}
void ObjectWrapper::ShipAIDoKill(ObjectWrapper &o)
{
	if (Is(Object::SHIP) && o.m_obj) {
		Ship *s = static_cast<Ship*>(m_obj);
		s->AIClearInstructions();
		s->AIInstruct(Ship::DO_KILL, o.m_obj);
	}
}
void ObjectWrapper::ShipAIDoFlyTo(ObjectWrapper &o)
{
	if (Is(Object::SHIP) && o.m_obj) {
		Ship *s = static_cast<Ship*>(m_obj);
		s->AIClearInstructions();
		s->AIInstruct(Ship::DO_FLY_TO, o.m_obj);
	}
}
void ObjectWrapper::ShipAIDoLowOrbit(ObjectWrapper &o)
{
	if (Is(Object::SHIP) && o.m_obj) {
		Ship *s = static_cast<Ship*>(m_obj);
		s->AIClearInstructions();
		s->AIInstruct(Ship::DO_LOW_ORBIT, o.m_obj);
	}
}
void ObjectWrapper::ShipAIDoMediumOrbit(ObjectWrapper &o)
{
	if (Is(Object::SHIP) && o.m_obj) {
		Ship *s = static_cast<Ship*>(m_obj);
		s->AIClearInstructions();
		s->AIInstruct(Ship::DO_MEDIUM_ORBIT, o.m_obj);
	}
}
void ObjectWrapper::ShipAIDoHighOrbit(ObjectWrapper &o)
{
	if (Is(Object::SHIP) && o.m_obj) {
		Ship *s = static_cast<Ship*>(m_obj);
		s->AIClearInstructions();
		s->AIInstruct(Ship::DO_HIGH_ORBIT, o.m_obj);
	}
}
void ObjectWrapper::SetMoney(double m) {
	if (Is(Object::SHIP)) {
		Ship *s = static_cast<Ship*>(m_obj);
		s->SetMoney((Sint64)(m*100.0));
	}
}
void ObjectWrapper::AddMoney(double m) {
	if (Is(Object::SHIP)) {
		Ship *s = static_cast<Ship*>(m_obj);
		s->SetMoney(s->GetMoney() + (Sint64)(m*100.0));
	}
}
const char *ObjectWrapper::GetLabel() const {
	if (Is(Object::BODY)) {
		return static_cast<Body*>(m_obj)->GetLabel().c_str();
	} else {
		return "";
	}
}
void ObjectWrapper::SpaceStationAddAdvert(const char *luaMod, int luaRef, const char *description) {
	if (Is(Object::SPACESTATION)) {
		static_cast<SpaceStation*>(m_obj)->BBAddAdvert(BBAdvert(luaMod, luaRef, description));
	}
}
void ObjectWrapper::SpaceStationRemoveAdvert(const char *luaMod, int luaRef) {
	if (Is(Object::SPACESTATION)) {
		static_cast<SpaceStation*>(m_obj)->BBRemoveAdvert(luaMod, luaRef);
	}
}
SBodyPath *ObjectWrapper::GetSBody()
{
	const SBody *sbody = 0;
	if (Is(Object::BODY)) {
		sbody = static_cast<Body*>(m_obj)->GetSBody();
		if (sbody) {
			SBodyPath *path = new SBodyPath;
			Pi::currentSystem->GetPathOf(sbody, path);
			return path;
		}
	}
	return 0;
}
ObjectWrapper *ObjectWrapper::GetDockedWith()
{
	if (Is(Object::SHIP) && static_cast<Ship*>(m_obj)->GetDockedWith()) {
		return new ObjectWrapper(static_cast<Ship*>(m_obj)->GetDockedWith());
	} else {
		return 0;
	}
}

ObjectWrapper::~ObjectWrapper() {
//	printf("ObjWrapper for %s is being deleted\n", GetLabel());
	m_delCon.disconnect();
}
bool ObjectWrapper::Is(Object::Type t) const {
	return m_obj && m_obj->IsType(t);
}
void ObjectWrapper::OnDelete() {
	// object got deleted out from under us
	m_obj = 0;
	m_delCon.disconnect();
}

/////////////////////////////////////////////////////////////
class Rand: public MTRand {
public:
	// don't add members, only methods (because dirty cast Pi::rng to Rand)
	Rand(): MTRand() {}
	Rand(unsigned long seed): MTRand(seed) {}
	std::string PersonName(bool isfemale) {
		return NameGenerator::FullName(*this, isfemale);
	}
	std::string Surname() {
		return NameGenerator::Surname(*this);
	}
};
OOLUA_CLASS_NO_BASES(Rand)
	OOLUA_NO_TYPEDEFS
	OOLUA_CONSTRUCTORS_BEGIN
		OOLUA_CONSTRUCTOR_1(unsigned long)
	OOLUA_CONSTRUCTORS_END
	OOLUA_MEM_FUNC_2_RENAME(Real, double, Double, double, double)
	OOLUA_MEM_FUNC_2_RENAME(Int, unsigned int, Int32, int, int)
	OOLUA_MEM_FUNC_1(std::string, PersonName, bool)
	OOLUA_MEM_FUNC_0(std::string, Surname)
OOLUA_CLASS_END

EXPORT_OOLUA_FUNCTIONS_0_CONST(Rand)
EXPORT_OOLUA_FUNCTIONS_4_NON_CONST(Rand,
		Real, Int, PersonName, Surname)

/////////////////////////////////////////////////////////////

// oolua doesn't like namespaces
class SoundEvent: public Sound::Event {};

OOLUA_CLASS_NO_BASES(SoundEvent)
	OOLUA_NO_TYPEDEFS
	OOLUA_ONLY_DEFAULT_CONSTRUCTOR
	OOLUA_MEM_FUNC_4(void, Play, const char *, float, float, Uint32)
	OOLUA_MEM_FUNC_0(bool, Stop)
OOLUA_CLASS_END

EXPORT_OOLUA_FUNCTIONS_2_NON_CONST(SoundEvent,
		Play, Stop)
EXPORT_OOLUA_FUNCTIONS_0_CONST(SoundEvent)

///////////////////////////////////////////////////////////////

static int UserDataSerialize(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TUSERDATA);
	if (mylua_checkudata(L, 1, "ObjectWrapper")) {
		char buf[256];
		ObjectWrapper *o;
		OOLUA::pull2cpp(L, o);
		snprintf(buf, sizeof(buf), "ObjectWrapper\n%d\n", Serializer::LookupBody((Body*)o->m_obj));
		lua_pushstring(L, buf);
		return 1;
	} else if (mylua_checkudata(L, 1, "SBodyPath")) {
		Serializer::Writer wr;
		SBodyPath *path;
		OOLUA::pull2cpp(L, path);
		path->Serialize(wr);
		std::string out = "SBodyPath\n";
		out += wr.GetData();
		OOLUA::push2lua(L, out);
		return 1;
	} else if (mylua_checkudata(L, 1, "SysLoc")) {
		Serializer::Writer wr;
		SysLoc *systemid;
		OOLUA::pull2cpp(L, systemid);
		systemid->Serialize(wr);
		std::string out = "SysLoc\n";
		out += wr.GetData();
		OOLUA::push2lua(L, out);
		return 1;
	} else {
		Error("Tried to serialize unknown userdata type.");
		return 0;
	}
}

static int UserDataUnserialize(lua_State *L)
{
	std::string str;
	OOLUA::pull2cpp(L, str);
	if (str.substr(0, 14) == "ObjectWrapper\n") {
		int idx = atoi(str.substr(14).c_str());
		Body *b = Serializer::LookupBody(idx);
		push2luaWithGc(L, new ObjectWrapper(b));
		return 1;
	} else if (str.substr(0, 10) == "SBodyPath\n") {
		Serializer::Reader r(str.substr(10));
		SBodyPath *p = new SBodyPath;
		SBodyPath::Unserialize(r, p);
		push2luaWithGc(L, p);
		return 1;
	} else if (str.substr(0, 7) == "SysLoc\n") {
		Serializer::Reader r(str.substr(7));
		SysLoc *p = new SysLoc;
		SysLoc::Unserialize(r, p);
		push2luaWithGc(L, p);
		return 1;
	}
	return 0;
}

namespace LuaPi {
	static int GetPlayer(lua_State *l) {
		push2luaWithGc(l, new ObjectWrapper((Object*)Pi::player));
		return 1;
	}
	static int GetGameTime(lua_State *l) {
		OOLUA_C_FUNCTION_0(double, Pi::GetGameTime)
	}
	static int Message(lua_State *l) {
		std::string from, msg;
		OOLUA::pull2cpp(l, msg);
		OOLUA::pull2cpp(l, from);
		Pi::cpan->MsgLog()->Message(from, msg);
		return 0;
	}
	static int ImportantMessage(lua_State *l) {
		std::string from, msg;
		OOLUA::pull2cpp(l, msg);
		OOLUA::pull2cpp(l, from);
		Pi::cpan->MsgLog()->ImportantMessage(from, msg);
		return 0;
	}
	static int GetCurrentSystem(lua_State *l) {
		// sadly must rebuild for the mo
		StarSystem *cur = Pi::currentSystem;
		SysLoc *s = new SysLoc(cur->SectorX(), cur->SectorY(), cur->SystemIdx());
		push2luaWithGc(l, s);
		return 1;
	}
	static int FormatDate(lua_State *l) {
		double t;
		OOLUA::pull2cpp(l, t);
		std::string s = format_date(t);
		OOLUA::push2lua(l, s.c_str());
		return 1;
	}
	static int SpawnShip(lua_State *l) {
		double due;
		std::string type;
		OOLUA::pull2cpp(l, due);
		OOLUA::pull2cpp(l, type);
		if (ShipType::Get(type.c_str()) == 0) {
			lua_pushnil(l);
			lua_pushstring(l, "Unknown ship type");
			return 2;
		} else {
			// for the mo, just put it near the player
			const vector3d pos = Pi::player->GetPosition() +
				10000.0 * vector3d(Pi::rng.Double(-1.0, 1.0), Pi::rng.Double(-1.0, 1.0), Pi::rng.Double(-1.0, 1.0));
			if (due <= Pi::GetGameTime()) {
				// already entered
				if (!Space::IsSystemBeingBuilt()) {
					lua_pushnil(l);
					lua_pushstring(l, "Insufficient time to generate ship entry");
					return 2;
				}
				if ((due <= 0) || (due < Pi::GetGameTime()-HYPERCLOUD_DURATION)) {
					// ship is supposed to have entered some time
					// ago and the hyperspace cloud is gone
					Ship *ship = new Ship(type.c_str());
					ship->SetFrame(Pi::player->GetFrame());
					ship->SetPosition(pos);
					ship->SetVelocity(Pi::player->GetVelocity());
					Space::AddBody(ship);
					push2luaWithGc(l, new ObjectWrapper(ship));
					return 1;
				} else {
					// hypercloud still present
					Ship *ship = new Ship(type.c_str());
					HyperspaceCloud *cloud = new HyperspaceCloud(ship, due, true);
					cloud->SetFrame(Pi::player->GetFrame());
					cloud->SetPosition(pos);
					cloud->SetVelocity(Pi::player->GetVelocity());
					Space::AddBody(cloud);
					push2luaWithGc(l, new ObjectWrapper(ship));
					return 1;
				}
			} else {
				// to hyperspace in shortly
				Ship *ship = new Ship(type.c_str());
				HyperspaceCloud *cloud = new HyperspaceCloud(ship, due, true);
				cloud->SetFrame(Pi::player->GetFrame());
				cloud->SetPosition(pos);
				cloud->SetVelocity(Pi::player->GetVelocity());
				Space::AddBody(cloud);
				push2luaWithGc(l, new ObjectWrapper(ship));
				return 1;
			}
		}
	}
}

/**
 * power 0 = unarmed, power 1 = armed to the teeth
 */
static Ship *make_random_ship(double power, int minMass, int maxMass)
{
	// find a ship that fits in the mass range
	std::vector<ShipType::Type> candidates;

	for (std::map<ShipType::Type, ShipType>::iterator i = ShipType::types.begin();
			i != ShipType::types.end(); ++i) {
		int hullMass = (*i).second.hullMass;
		if ((hullMass >= minMass) && (hullMass <= maxMass)) {
			candidates.push_back((*i).first);
		}
	}
	printf("%d candidates\n", candidates.size());
	if (candidates.size() == 0) return 0;

	for (int i=0; i<candidates.size(); i++) {
		printf("%s\n", candidates[i].c_str());
	}

	ShipType::Type &t = candidates[ Pi::rng.Int32(candidates.size()) ];
	Ship *ship = new Ship(t);
	
	/*
	ship->m_equipment.Set(Equip::SLOT_ENGINE, 0, Equip::DRIVE_CLASS1);

	switch (power) {
		case 1:
			ship->m_equipment.Set(Equip::SLOT_LASER, 0, Equip::PULSECANNON_2MW);
			break;
		case 2:
			ship->m_equipment.Set(Equip::SLOT_LASER, 0, Equip::PULSECANNON_4MW);
			break;
		case 0:
		default:
			ship->m_equipment.Set(Equip::SLOT_LASER, 0, Equip::PULSECANNON_1MW);
			break;
	}
	int amount = Pi::rng.Int32(5);
	while (amount--) ship->m_equipment.Add(Equip::HYDROGEN);*/
	return ship;
}

#define REG_FUNC(fnname, fnptr) \
	lua_pushcfunction(l, fnptr);\
	lua_setfield(l, -2, fnname)

void RegisterPiLuaAPI(lua_State *l)
{
//	printf("XXXXXXXXXXXXXXXXXXXXXXXX GET RID OF THIS SHIT!!!!!!!! XXXXXXXXXXXXX\n");
//	make_random_ship(1, 10, 100);
//	make_random_ship(1, 100, 1000);

	OOLUA::register_class<ObjectWrapper>(l);
	OOLUA::register_class<LuaChatForm>(l);
	OOLUA::register_class<SoundEvent>(l);
	OOLUA::register_class<SysLoc>(l);
	OOLUA::register_class<SBodyPath>(l);
	OOLUA::register_class<Rand>(l);
	
	lua_register(l, "UserDataSerialize", UserDataSerialize);
	lua_register(l, "UserDataUnserialize", UserDataUnserialize);

	lua_newtable(l);
	REG_FUNC("GetCurrentSystem", &LuaPi::GetCurrentSystem);
	REG_FUNC("GetPlayer", &LuaPi::GetPlayer);
	REG_FUNC("GetGameTime", &LuaPi::GetGameTime);
	REG_FUNC("Message", &LuaPi::Message);
	REG_FUNC("ImportantMessage", &LuaPi::ImportantMessage);
	REG_FUNC("SpawnShip", &LuaPi::SpawnShip);
	lua_setglobal(l, "Pi");
	
	lua_newtable(l);
	REG_FUNC("Format", &LuaPi::FormatDate);
	lua_setglobal(l, "Date");
}
