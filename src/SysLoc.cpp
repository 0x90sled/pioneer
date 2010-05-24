#include "SysLoc.h"
#include "StarSystem.h"
#include "Pi.h"

EXPORT_OOLUA_FUNCTIONS_0_NON_CONST(SysLoc)
EXPORT_OOLUA_FUNCTIONS_6_CONST(SysLoc,
		GetSystemShortDescription, GetSystemName,
		GetSectorX, GetSectorY, GetSystemNum,
		GetRandomStarportNearButNotIn)

void SysLoc::Serialize(Serializer::Writer &wr) const
{
	wr.Int32(sectorX);
	wr.Int32(sectorY);
	wr.Int32(systemNum);
}

void SysLoc::Unserialize(Serializer::Reader &rd, SysLoc *loc)
{
	loc->sectorX = rd.Int32();
	loc->sectorY = rd.Int32();
	loc->systemNum = rd.Int32();
}

const char *SysLoc::GetSystemShortDescription() const
{
	return Sys()->GetShortDescription();
}

const char *SysLoc::GetSystemName() const
{
	return Sys()->GetName().c_str();
}

SBodyPath *SysLoc::GetRandomStarportNearButNotIn() const
{
	SBodyPath *path = new SBodyPath;
	if (Sys()->GetRandomStarportNearButNotIn(Pi::rng, path)) {
		return path;
	} else {
		delete path;
		return 0;
	}
}

const StarSystem *SysLoc::Sys() const
{
	return StarSystem::GetCached(*this);
}
