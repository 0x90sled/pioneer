// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef GALAXYGENERATOR_H
#define GALAXYGENERATOR_H

#include <list>
#include <string>
#include "RefCounted.h"
#include "Sector.h"
#include "StarSystem.h"
#include "SystemPath.h"

class SectorGeneratorStage;
class StarSystemGeneratorStage;

class GalaxyGenerator : public RefCounted {
public:
	typedef int Version;
	static const Version LAST_VERSION = -1;

	static RefCountedPtr<Galaxy> Create(const std::string& name, Version version = LAST_VERSION);
	static RefCountedPtr<Galaxy> Create() {
		return Create(s_defaultGenerator, s_defaultVersion);
	}

	static std::string GetDefaultGeneratorName() { return s_defaultGenerator; }
	static Version GetDefaultGeneratorVersion() { return s_defaultVersion; }
	static Version GetLastVersion(const std::string& name);
	static void SetDefaultGenerator(const std::string& name, Version version = LAST_VERSION);

	virtual ~GalaxyGenerator();

	const std::string& GetName() const { return m_name; }
	Version GetVersion() const { return m_version; }

	bool IsDefault() const { return m_name == s_defaultGenerator && m_version == s_defaultVersion; }

	// Templated for the template cache class.
	template <typename T, typename Cache>
	RefCountedPtr<T> Generate(const SystemPath& path, Cache* cache);

	GalaxyGenerator* AddSectorStage(SectorGeneratorStage* sectorGenerator);
	GalaxyGenerator* AddStarSystemStage(StarSystemGeneratorStage* starSystemGenerator);

	struct SectorConfig {
		bool isCustomOnly;

		SectorConfig() : isCustomOnly(false) { }
	};

	struct StarSystemConfig {
		bool isCustomOnly;

		StarSystemConfig() : isCustomOnly(false) { }
	};

private:
	GalaxyGenerator(const std::string& name, Version version = LAST_VERSION) : m_name(name), m_version(version) { }

	virtual RefCountedPtr<Sector> GenerateSector(const SystemPath& path, SectorCache* cache);
	virtual RefCountedPtr<StarSystem> GenerateStarSystem(const SystemPath& path, StarSystemCache* cache);

	const std::string m_name;
	const Version m_version;

	std::list<SectorGeneratorStage*> m_sectorStage;
	std::list<StarSystemGeneratorStage*> m_starSystemStage;

	static std::string s_defaultGenerator;
	static Version s_defaultVersion;
};

template <>
inline RefCountedPtr<Sector> GalaxyGenerator::Generate<Sector,SectorCache>(const SystemPath& path, SectorCache* cache) {
	return GenerateSector(path, cache);
}

template <>
inline RefCountedPtr<StarSystem> GalaxyGenerator::Generate<StarSystem,StarSystemCache>(const SystemPath& path, StarSystemCache* cache) {
	return GenerateStarSystem(path, cache);
}

class GalaxyGeneratorStage {
public:
	virtual ~GalaxyGeneratorStage() { }

protected:
	GalaxyGeneratorStage() : m_galaxyGenerator(nullptr) { }

	friend class GalaxyGenerator;
	void AssignToGalaxyGenerator(GalaxyGenerator* galaxyGenerator) { m_galaxyGenerator = galaxyGenerator; }

	GalaxyGenerator* m_galaxyGenerator;
};

class SectorGeneratorStage : public GalaxyGeneratorStage {
public:
	virtual ~SectorGeneratorStage() { }

	virtual bool Apply(Random& rng, RefCountedPtr<Sector> sector, GalaxyGenerator::SectorConfig* config) = 0;
};

class StarSystemGeneratorStage : public GalaxyGeneratorStage {
public:
	virtual ~StarSystemGeneratorStage() { }

	virtual bool Apply(Random& rng, RefCountedPtr<StarSystem::GeneratorAPI> system, GalaxyGenerator::StarSystemConfig* config) = 0;
};

#endif
