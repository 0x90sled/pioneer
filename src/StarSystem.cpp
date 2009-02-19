#include "StarSystem.h"
#include "Sector.h"
#include "custom_starsystems.h"
#include "Serializer.h"

#define CELSIUS	273.15
#define DEBUG_DUMP

// minimum moon mass a little under Europa's
const fixed MIN_MOON_MASS = fixed(6,1000); // earth masses
const fixed MIN_MOON_DIST = fixed(15,10000); // AUs
const fixed MAX_MOON_DIST = fixed(2, 100); // AUs

// indexed by enum type turd  
float StarSystem::starColors[][3] = {
	{ 0, 0, 0 }, // gravpoint
	{ 0.5, 0.0, 0.0 }, // brown dwarf
	{ 1.0, 0.2, 0.0 }, // M
	{ 1.0, 0.6, 0.1 }, // K
	{ 0.4, 0.4, 0.8 }, // white dwarf
	{ 1.0, 1.0, 0.4 }, // G
	{ 1.0, 1.0, 0.8 }, // F
	{ 1.0, 1.0, 1.0 }, // A
	{ 0.7, 0.7, 1.0 }, // B
	{ 1.0, 0.7, 1.0 }  // O
};

// indexed by enum type turd  
float StarSystem::starRealColors[][3] = {
	{ 0, 0, 0 }, // gravpoint
	{ 0.5, 0.0, 0.0 }, // brown dwarf
	{ 1.0, 0.2, 0.0 }, // M
	{ 1.0, 0.7, 0.1 }, // K
	{ 1.0, 1.0, 1.0 }, // white dwarf
	{ 1.0, 1.0, 0.9 }, // G
	{ 1.0, 1.0, 1.0 }, // F
	{ 1.0, 1.0, 1.0 }, // A
	{ 0.7, 0.7, 1.0 }, // B
	{ 1.0, 0.7, 1.0 }  // O
};

static const struct SBodySubTypeInfo {
	SBody::BodySuperType supertype;
	int mass[2]; // min,max % sol for stars, unused for planets
	int radius; // % sol radii for stars, % earth radii for planets
	const char *description;
	const char *icon;
	int tempMin, tempMax;
} bodyTypeInfo[SBody::TYPE_MAX] = {
	{
		SBody::SUPERTYPE_NONE, {}, 0, "Shouldn't see this!",
	}, {
		SBody::SUPERTYPE_STAR,
		{2,8}, 30, "Brown dwarf sub-stellar object",
		"icons/object_brown_dwarf.png",
		1000, 2000
	}, {
		SBody::SUPERTYPE_STAR,
		{10,47}, 50, "Type 'M' red star",
		"icons/object_star_m.png",
		2000, 3500
	}, {
		SBody::SUPERTYPE_STAR,
		{50,78}, 90, "Type 'K' orange star",
		"icons/object_star_k.png",
		3500, 5000
	}, {
		SBody::SUPERTYPE_STAR,
		{20,100}, 1, "White dwarf",
		"icons/object_white_dwarf.png",
		4000, 40000
	}, { 
		SBody::SUPERTYPE_STAR,
		{80,110}, 110, "Type 'G' yellow star",
		"icons/object_star_g.png",
		5000, 6000
	}, {
		SBody::SUPERTYPE_STAR,
		{115,170}, 140, "Type 'F' white star",
		"icons/object_star_f.png",
		6000, 7500
	}, {
		SBody::SUPERTYPE_STAR,
		{180,320}, 210, "Type 'A' hot white star",
		"icons/object_star_a.png",
		7500, 10000
	}, {
		SBody::SUPERTYPE_STAR,
		{400,1800}, 700, "Bright type 'B' blue star",
		"icons/object_star_b.png",
		10000, 30000
	}, {
		SBody::SUPERTYPE_STAR,
		{2000,4000}, 1600, "Hot, massive type 'O' blue star",
		"icons/object_star_o.png",
		30000, 60000
	}, {
		SBody::SUPERTYPE_GAS_GIANT,
		{}, 390, "Small gas giant",
		"icons/object_planet_small_gas_giant.png"
	}, {
		SBody::SUPERTYPE_GAS_GIANT,
		{}, 950, "Medium gas giant",
		"icons/object_planet_medium_gas_giant.png"
	}, {
		SBody::SUPERTYPE_GAS_GIANT,
		{}, 1110, "Large gas giant",
		"icons/object_planet_large_gas_giant.png"
	}, {
		SBody::SUPERTYPE_GAS_GIANT,
		{}, 1500, "Very large gas giant",
		"icons/object_planet_large_gas_giant.png"
	}, {
		SBody::SUPERTYPE_ROCKY_PLANET,
		{}, 26, "Small, rocky dwarf planet", // moon radius
		"icons/object_planet_dwarf.png"
	}, {
		SBody::SUPERTYPE_ROCKY_PLANET,
		{}, 52, "Small, rocky planet with a thin atmosphere", // mars radius
		"icons/object_planet_small.png"
	}, {
		SBody::SUPERTYPE_ROCKY_PLANET,
		{}, 100, "Rocky planet with liquid water and a nitrogen atmosphere", // earth radius
		"icons/object_planet_water_n2.png"
	}, {
		SBody::SUPERTYPE_ROCKY_PLANET,
		{}, 100, "Rocky planet with a carbon dioxide atmosphere",
		"icons/object_planet_co2.png"
	}, {
		SBody::SUPERTYPE_ROCKY_PLANET,
		{}, 100, "Rocky planet with a methane atmosphere",
		"icons/object_planet_methane.png"
	}, {
		SBody::SUPERTYPE_ROCKY_PLANET,
		{}, 100, "Rocky planet with liquid water and a thick nitrogen atmosphere",
		"icons/object_planet_water_n2.png"
	}, {
		SBody::SUPERTYPE_ROCKY_PLANET,
		{}, 100, "Rocky planet with a thick carbon dioxide atmosphere",
		"icons/object_planet_co2.png"
	}, {
		SBody::SUPERTYPE_ROCKY_PLANET,
		{}, 100, "Rocky planet with a thick methane atmosphere",
		"icons/object_planet_methane.png"
	}, {
		SBody::SUPERTYPE_ROCKY_PLANET,
		{}, 100, "Highly volcanic world",
		"icons/object_planet_volcanic.png"
	}, {
		SBody::SUPERTYPE_ROCKY_PLANET,
		{}, 100, "World with indigenous life and an oxygen atmosphere",
		"icons/object_planet_life.png"
	}, {
		SBody::SUPERTYPE_STARPORT,
		{}, 0, "Orbital starport",
		"icons/object_orbital_starport.png"
	}, {
		SBody::SUPERTYPE_STARPORT,
		{}, 0, "Starport",
	}
};

SBody::BodySuperType SBody::GetSuperType() const
{
	return bodyTypeInfo[type].supertype;
}

const char *SBody::GetAstroDescription()
{
	return bodyTypeInfo[type].description;
}

const char *SBody::GetIcon()
{
	return bodyTypeInfo[type].icon;
}

double SBody::GetMaxChildOrbitalDistance() const
{
	double max = 0;
	for (unsigned int i=0; i<children.size(); i++) {
		if (children[i]->orbMax.ToDouble() > max) {
			max = children[i]->orbMax.ToDouble();	
		}
	}
	return AU * max;
}


static inline Sint64 isqrt(Sint64 a)
{
	Sint64 ret=0;
	Sint64 s;
	Sint64 ret_sq=-a-1;
	for(s=62; s>=0; s-=2){
		Sint64 b;
		ret+= ret;
		b=ret_sq + ((2*ret+1)<<s);
		if(b<0){
			ret_sq=b;
			ret++;
		}
	}
	return ret;
}


/*
 * These are the nice floating point surface temp calculating turds.
 *
static const double boltzman_const = 5.6704e-8;
static double calcEnergyPerUnitAreaAtDist(double star_radius, double star_temp, double object_dist)
{
	const double total_solar_emission = boltzman_const *
		star_temp*star_temp*star_temp*star_temp*
		4*M_PI*star_radius*star_radius;

	return total_solar_emission / (4*M_PI*object_dist*object_dist);
}

// bond albedo, not geometric
static double CalcSurfaceTemp(double star_radius, double star_temp, double object_dist, double albedo, double greenhouse)
{
	const double energy_per_meter2 = calcEnergyPerUnitAreaAtDist(star_radius, star_temp, object_dist);
	const double surface_temp = pow(energy_per_meter2*(1-albedo)/(4*(1-greenhouse)*boltzman_const), 0.25);
	return surface_temp;
}
*/
/*
 * Instead we use these butt-ugly overflow-prone spat of ejaculate:
 */
/*
 * star_radius in sol radii
 * star_temp in kelvin,
 * object_dist in AU
 * return Watts/m^2
 */
static fixed calcEnergyPerUnitAreaAtDist(fixed star_radius, int star_temp, fixed object_dist)
{
	fixed temp = star_temp * fixed(1,10000);
	const fixed total_solar_emission =
		temp*temp*temp*temp*star_radius*star_radius;
	
	return fixed(1744665451,100000)*(total_solar_emission / (object_dist*object_dist));
}

static int CalcSurfaceTemp(SBody *primary, fixed distToPrimary, fixed albedo, fixed greenhouse)
{
	fixed energy_per_meter2;
	if (primary->type == SBody::TYPE_GRAVPOINT) {
		// binary. take energies of both stars
		energy_per_meter2 = calcEnergyPerUnitAreaAtDist(primary->children[0]->radius,
			primary->children[0]->averageTemp, distToPrimary);
		energy_per_meter2 += calcEnergyPerUnitAreaAtDist(primary->children[1]->radius,
			primary->children[1]->averageTemp, distToPrimary);
	} else {
		energy_per_meter2 = calcEnergyPerUnitAreaAtDist(primary->radius, primary->averageTemp, distToPrimary);
	}
	const fixed surface_temp_pow4 = energy_per_meter2*(1-albedo)/(1-greenhouse);
	return isqrt(isqrt((surface_temp_pow4.v>>16)*4409673));
}

void Orbit::KeplerPosAtTime(double t, double *dist, double *ang)
{
	double e = eccentricity;
	double a = semiMajorAxis;
	// mean anomaly
	double M = 2*M_PI*t / period;
	// eccentric anomaly
	double E = M + (e - (1/8.0)*e*e*e)*sin(M) +
	               (1/2.0)*e*e*sin(2*M) +
		       (3/8.0)*e*e*e*sin(3*M);
	// true anomaly (angle of orbit position)
	double v = 2*atan(sqrt((1+e)/(1-e)) * tan(E/2.0));
	// heliocentric distance
	double r = a * (1 - e*e) / (1 + e*cos(v));
	*ang = v;
	*dist = r;
}
			
vector3d Orbit::CartesianPosAtTime(double t)
{
	double dist, ang;
	KeplerPosAtTime(t, &dist, &ang);
	vector3d pos = vector3d(cos(ang)*dist, sin(ang)*dist, 0);
	pos = rotMatrix * pos;
	return pos;
}


struct ring_t {
	fixed dist;
	fixed mass;
};

#define BAND_SIZE	25
static std::list<ring_t> *AccreteDisc2(fixed size, fixed density, MTRand &rand)
{
	std::list<ring_t> *disc = new std::list<ring_t>;
	const int NUM_RINGS = rand.Int32(250, 500);
	fixed scale = size/NUM_RINGS;

	fixed bandDensity = fixed(0);
	for (int i=0; i<NUM_RINGS; i++) {
		if ((i%BAND_SIZE)==0) bandDensity = rand.Fixed() * scale;
		ring_t r;
		r.dist = (i+1)*size/NUM_RINGS;
		r.dist -= rand.Fixed() * scale;
		r.mass = bandDensity * rand.Fixed() * density;
		disc->push_back(r);
	}

	bool changed;
	do {
		changed = false;
		for (std::list<ring_t>::iterator i = disc->begin(); i != disc->end();) {
			std::list<ring_t>::iterator n = i;
			++n;
			if (n == disc->end()) break;
			
			fixed totalmass = (*i).mass + (*n).mass;
			if (!totalmass.v) { ++i; continue; }
	
			fixed range = (*i).dist / 3;
			fixed dist = (*n).dist - (*i).dist;
			if ((dist < range) && (
				((*i).mass > (*n).mass) ||
				((*n).mass > (*i).mass))) {
				
				(*n).dist = ((*n).dist*(*n).mass + (*i).dist*(*i).mass) / totalmass;
				(*n).mass += (*i).mass;
				i = disc->erase(i);
				changed = true;
				// because otherwise one blob can acrete and accrete in one step
				++i;
			} else {
				++i;
			}
		}
	} while (changed);

	return disc;
}

double calc_orbital_period(double semiMajorAxis, double centralMass)
{
	return 2.0*M_PI*sqrt((semiMajorAxis*semiMajorAxis*semiMajorAxis)/(G*centralMass));
}

void SBody::EliminateBadChildren()
{
	for (std::vector<SBody*>::iterator i = children.begin(); i != children.end(); ++i) {
		(*i)->tmp = 0;
	}
	// now check for overlapping & unacceptably close orbits. merge planets
	for (std::vector<SBody*>::iterator i = children.begin(); i != children.end(); ++i) {
		if ((*i)->GetSuperType() == SBody::SUPERTYPE_STAR) continue;
		if ((*i)->tmp) continue;

		for (std::vector<SBody*>::iterator j = children.begin(); j != children.end(); ++j) {
			if ((*j)->GetSuperType() == SBody::SUPERTYPE_STAR) continue;
			if ((*j) == (*i)) continue;
			// don't eat anything bigger than self
			if ((*j)->mass > (*i)->mass) continue;
			fixed i_min = (*i)->orbMin;
			fixed i_max = (*i)->orbMax;
			fixed j_min = (*j)->orbMin;
			fixed j_max = (*j)->orbMax;
			fixed i_avg = (i_min+i_max)>>1;
			fixed j_avg = (j_min+j_max)>>1;
			bool eat = false;
			if (i_avg > j_avg) {
				if (i_min < j_max*fixed(13,10)) eat = true;
			} else {
				if (i_max > j_min*fixed(7,10)) eat = true;
			}
			if (eat) {
				(*i)->mass += (*j)->mass;
				(*j)->tmp = 1;
			}
		}

	}

	// kill the eaten ones
	for (std::vector<SBody*>::iterator i = children.begin(); i != children.end();) {
		if ((*i)->tmp) {
			i = children.erase(i);
		}
		else
			++i;
	}
}

SBodyPath::SBodyPath()
{
	sectorX = sectorY = systemIdx = 0;
	for (int i=0; i<SBODYPATHLEN; i++) elem[i] = -1;
}
SBodyPath::SBodyPath(int sectorX, int sectorY, int systemIdx)
{
	this->sectorX = sectorX;
	this->sectorY = sectorY;
	this->systemIdx = systemIdx;
	for (int i=0; i<SBODYPATHLEN; i++) elem[i] = -1;
}

void SBodyPath::Serialize() const
{
	using namespace Serializer::Write;
	wr_int(sectorX);
	wr_int(sectorY);
	wr_int(systemIdx);
	for (int i=0; i<SBODYPATHLEN; i++) wr_byte(elem[i]);
}

void SBodyPath::Unserialize(SBodyPath *path)
{
	using namespace Serializer::Read;
	path->sectorX = rd_int();
	path->sectorY = rd_int();
	path->systemIdx = rd_int();
	for (int i=0; i<SBODYPATHLEN; i++) path->elem[i] = rd_byte();
}

SBody *StarSystem::GetBodyByPath(const SBodyPath *path) const
{
	assert((m_secx == path->sectorX) || (m_secy == path->sectorY) ||
	       (m_sysIdx == path->systemIdx));

	SBody *body = rootBody;
	for (int i=0; i<SBODYPATHLEN; i++) {
		if (path->elem[i] == -1) continue;
		else {
			body = body->children[path->elem[i]];
		}
	}
	return body;
}

void StarSystem::GetPathOf(const SBody *sbody, SBodyPath *path) const
{
	*path = SBodyPath();

	int pos = SBODYPATHLEN-1;

	for (const SBody *parent = sbody->parent;;) {
		if (!parent) break;
		assert((pos>=0) && (pos < SBODYPATHLEN));

		// find position of sbody in parent
		unsigned int index = 0;
		bool found = false;
		for (; index < parent->children.size(); index++) {
			if (parent->children[index] == sbody) {
				assert(index < 128);
				path->elem[pos--] = index;
				sbody = parent;
				parent = parent->parent;
				found = true;
				break;
			}
		}
		assert(found);
	}
	path->sectorX = m_secx;
	path->sectorY = m_secy;
	path->systemIdx = m_sysIdx;
}

/*
struct CustomSBody {
	const char *name; // null to end system
	SBody::BodyType type;
	int primaryIdx;  // -1 for primary
	fixed radius; // in earth radii for planets, sol radii for stars
	fixed mass; // earth masses or sol masses
	int averageTemp; // kelvin
	fixed semiMajorAxis; // in AUs
	fixed eccentricity;
};
*/
void StarSystem::CustomGetKidsOf(SBody *parent, const CustomSBody *customDef, const int primaryIdx)
{
	const CustomSBody *c = customDef;
	for (int i=0; c->name; c++, i++) {
		if (c->primaryIdx != primaryIdx) continue;
		
		SBody *kid = new SBody;
		SBody::BodyType type = c->type;
		kid->type = type;
		kid->parent = parent;
		kid->radius = c->radius;
		kid->mass = c->mass;
		kid->econType = c->econType;
		kid->averageTemp = c->averageTemp;
		kid->name = c->name;
		kid->rotationPeriod = c->rotationPeriod;
		
		kid->orbit.eccentricity = c->eccentricity.ToDouble();
		kid->orbit.semiMajorAxis = c->semiMajorAxis.ToDouble() * AU;
		kid->orbit.period = calc_orbital_period(kid->orbit.semiMajorAxis, parent->GetMass());
		kid->orbit.rotMatrix = matrix4x4d::RotateYMatrix(c->inclination) *
					  matrix4x4d::RotateZMatrix(rand.Double(M_PI));
		parent->children.push_back(kid);

		// perihelion and aphelion (in AUs)
		kid->orbMin = c->semiMajorAxis - c->eccentricity*c->semiMajorAxis;
		kid->orbMax = 2*c->semiMajorAxis - kid->orbMin;

		PickEconomicStuff(kid);
		CustomGetKidsOf(kid, customDef, i);
	}
}

void StarSystem::GenerateFromCustom(const CustomSBody *customDef)
{
	// find primary
	const CustomSBody *csbody = customDef;

	int idx = 0;
	while ((csbody->name) && (csbody->primaryIdx != -1)) { csbody++; idx++; }
	assert(csbody->primaryIdx == -1);

	rootBody = new SBody;
	SBody::BodyType type = csbody->type;
	rootBody->type = type;
	rootBody->parent = NULL;
	rootBody->radius = csbody->radius;
	rootBody->mass = csbody->mass;
	rootBody->averageTemp = csbody->averageTemp;
	rootBody->name = csbody->name;
	
	CustomGetKidsOf(rootBody, customDef, idx);

}

void StarSystem::MakeStarOfType(SBody *sbody, SBody::BodyType type, MTRand &rand)
{
	sbody->type = type;
	sbody->radius = fixed(bodyTypeInfo[type].radius, 100);
	sbody->mass = fixed(rand.Int32(bodyTypeInfo[type].mass[0],
				bodyTypeInfo[type].mass[1]), 100);
	sbody->averageTemp = rand.Int32(bodyTypeInfo[type].tempMin,
				bodyTypeInfo[type].tempMax);
}

void StarSystem::MakeRandomStar(SBody *sbody, MTRand &rand)
{
	SBody::BodyType type = (SBody::BodyType)rand.Int32((int)SBody::TYPE_STAR_MIN, (int)SBody::TYPE_STAR_MAX);
	MakeStarOfType(sbody, type, rand);
}

void StarSystem::MakeStarOfTypeLighterThan(SBody *sbody, SBody::BodyType type, fixed maxMass, MTRand &rand)
{
	int tries = 16;
	do {
		MakeStarOfType(sbody, type, rand);
	} while ((sbody->mass > maxMass) && (--tries));
}

void StarSystem::MakeBinaryPair(SBody *a, SBody *b, fixed minDist, MTRand &rand)
{
	fixed ecc = rand.NFixed(3);
	fixed m = a->mass + b->mass;
	fixed a0 = b->mass / m;
	fixed a1 = a->mass / m;
	fixed semiMajorAxis;
	int mul = 1;

	do {
		switch (rand.Int32(3)) {
			case 2: semiMajorAxis = fixed(rand.Int32(100,10000), 100); break;
			case 1: semiMajorAxis = fixed(rand.Int32(10,1000), 100); break;
			default:
			case 0: semiMajorAxis = fixed(rand.Int32(1,100), 100); break;
		}
		semiMajorAxis *= mul;
		mul *= 2;
	} while (semiMajorAxis < minDist);

	a->orbit.eccentricity = ecc.ToDouble();
	a->orbit.semiMajorAxis = AU * (semiMajorAxis * a0).ToDouble();
	a->orbit.period = 60*60*24*365* semiMajorAxis.ToDouble() * sqrt(semiMajorAxis.ToDouble() / m.ToDouble());
	
	const float rotY = rand.Double()*M_PI/2.0;
	const float rotZ = rand.Double(M_PI);
	a->orbit.rotMatrix = matrix4x4d::RotateYMatrix(rotY) * matrix4x4d::RotateZMatrix(rotZ);
	b->orbit.rotMatrix = matrix4x4d::RotateYMatrix(rotY) * matrix4x4d::RotateZMatrix(rotZ-M_PI);

	b->orbit.eccentricity = ecc.ToDouble();
	b->orbit.semiMajorAxis = AU * (semiMajorAxis * a1).ToDouble();
	b->orbit.period = a->orbit.period;
	
	fixed orbMin = semiMajorAxis - ecc*semiMajorAxis;
	fixed orbMax = 2*semiMajorAxis - orbMin;
	a->orbMin = orbMin;
	b->orbMin = orbMin;
	a->orbMax = orbMax;
	b->orbMax = orbMax;
}

SBody::SBody()
{
	econType = 0;
	memset(tradeLevel, 0, sizeof(tradeLevel));
}

/*
 * As my excellent comrades have pointed out, choices that depend on floating
 * point crap will result in different universes on different platforms.
 *
 * We must be sneaky and avoid floating point in these places.
 */
StarSystem::StarSystem(int sector_x, int sector_y, int system_idx)
{
	unsigned long _init[4] = { system_idx, sector_x, sector_y, UNIVERSE_SEED };
	m_secx = sector_x;
	m_secy = sector_y;
	m_sysIdx = system_idx;
	rootBody = 0;
	if (system_idx == -1) return;
	rand.seed(_init, 4);

	Sector s = Sector(sector_x, sector_y);

	if (s.m_systems[system_idx].customDef) {
		GenerateFromCustom(s.m_systems[system_idx].customDef);
		return;
	}

	SBody *star[4];
	SBody *centGrav1, *centGrav2;

	const int numStars = s.m_systems[system_idx].numStars;
	assert((numStars >= 1) && (numStars <= 4));

	if (numStars == 1) {
		SBody::BodyType type = s.m_systems[system_idx].starType[0];
		star[0] = new SBody;
		star[0]->parent = NULL;
		star[0]->name = s.m_systems[system_idx].name;
		star[0]->orbMin = 0;
		star[0]->orbMax = 0;
		MakeStarOfType(star[0], type, rand);
		rootBody = star[0];
		m_numStars = 1;
	} else {
		centGrav1 = new SBody;
		centGrav1->type = SBody::TYPE_GRAVPOINT;
		centGrav1->parent = NULL;
		centGrav1->name = s.m_systems[system_idx].name+" A,B";
		rootBody = centGrav1;

		SBody::BodyType type = s.m_systems[system_idx].starType[0];
		star[0] = new SBody;
		star[0]->name = s.m_systems[system_idx].name+" A";
		star[0]->parent = centGrav1;
		MakeStarOfType(star[0], type, rand);
		
		star[1] = new SBody;
		star[1]->name = s.m_systems[system_idx].name+" B";
		star[1]->parent = centGrav1;
		MakeStarOfTypeLighterThan(star[1], s.m_systems[system_idx].starType[1],
				star[0]->mass, rand);

try_that_again_guvnah:
		MakeBinaryPair(star[0], star[1], fixed(0), rand);

		centGrav1->mass = star[0]->mass + star[1]->mass;
		centGrav1->children.push_back(star[0]);
		centGrav1->children.push_back(star[1]);
		m_numStars = 2;

		if (numStars > 2) {
			if (star[0]->orbMax > fixed(100,1)) {
				// reduce to < 100 AU...
				goto try_that_again_guvnah;
			}
			// 3rd and maybe 4th star
			if (numStars == 3) {
				star[2] = new SBody;
				star[2]->name = s.m_systems[system_idx].name+" C";
				star[2]->orbMin = 0;
				star[2]->orbMax = 0;
				MakeStarOfTypeLighterThan(star[2], s.m_systems[system_idx].starType[2],
					star[0]->mass, rand);
				centGrav2 = star[2];
				m_numStars = 3;
			} else {
				centGrav2 = new SBody;
				centGrav2->type = SBody::TYPE_GRAVPOINT;
				centGrav2->name = s.m_systems[system_idx].name+" C,D";
				centGrav2->orbMax = 0;

				star[2] = new SBody;
				star[2]->name = s.m_systems[system_idx].name+" C";
				star[2]->parent = centGrav2;
				MakeStarOfTypeLighterThan(star[2], s.m_systems[system_idx].starType[2],
					star[0]->mass, rand);
				
				star[3] = new SBody;
				star[3]->name = s.m_systems[system_idx].name+" D";
				star[3]->parent = centGrav2;
				MakeStarOfTypeLighterThan(star[3], s.m_systems[system_idx].starType[3],
					star[2]->mass, rand);

				MakeBinaryPair(star[2], star[3], fixed(0), rand);
				centGrav2->mass = star[2]->mass + star[3]->mass;
				centGrav2->children.push_back(star[2]);
				centGrav2->children.push_back(star[3]);
				m_numStars = 4;
			}
			SBody *superCentGrav = new SBody;
			superCentGrav->type = SBody::TYPE_GRAVPOINT;
			superCentGrav->parent = NULL;
			superCentGrav->name = s.m_systems[system_idx].name;
			centGrav1->parent = superCentGrav;
			centGrav2->parent = superCentGrav;
			rootBody = superCentGrav;
			const fixed minDist = star[0]->orbMax + star[2]->orbMax;
			MakeBinaryPair(centGrav1, centGrav2, 4*minDist, rand);
			superCentGrav->children.push_back(centGrav1);
			superCentGrav->children.push_back(centGrav2);

		}
	}

	{ /* decide how infested the joint is */
		const int dist = 1+MAX(abs(sector_x), abs(sector_y));
		m_humanInfested = (fixed(1,2)+fixed(1,2)*rand.Fixed()) / dist;
	}

	for (int i=0; i<m_numStars; i++) MakePlanetsAround(star[i]);

	if (m_numStars > 1) MakePlanetsAround(centGrav1);
	if (m_numStars == 4) MakePlanetsAround(centGrav2);
}

void StarSystem::MakePlanetsAround(SBody *primary)
{
	int disc_size = rand.Int32(10,100) + rand.Int32(60,140)*(10*primary->mass*primary->mass).ToInt64();
	//printf("disc_size %.1fAU\n", disc_size/10.0);
	
	// some restrictions on planet formation due to binary star orbits
	fixed orbMinKill = fixed(0);
	fixed orbMaxKill = fixed(disc_size, 10);
	if (primary->type == SBody::TYPE_GRAVPOINT) {
		SBody *star = primary->children[0];
		orbMinKill = star->orbMax*10;
	}
	else if ((primary->GetSuperType() == SBody::SUPERTYPE_STAR) && (primary->parent)) {
		// limit planets out to 10% distance to star's binary companion
		orbMaxKill = primary->orbMin * fixed(1,10);
	}
	if (m_numStars >= 3) {
		orbMaxKill = MIN(orbMaxKill, fixed(5,100)*rootBody->children[0]->orbMin);
	}

	std::list<ring_t> *disc = AccreteDisc2(fixed(disc_size,10), 10*rand.Fixed(), rand);
	for (std::list<ring_t>::iterator i = disc->begin(); i != disc->end(); ++i) {
		if ((*i).dist < fixed(1,10)) continue;
		fixed mass = (*i).mass;
		if (mass == 0) continue;
		fixed semiMajorAxis = (*i).dist;
		fixed ecc = rand.NFixed(3);
		// perihelion and aphelion (in AUs)
		fixed orbMin = semiMajorAxis - ecc*semiMajorAxis;
		fixed orbMax = 2*semiMajorAxis - orbMin;

		if ((orbMin < orbMinKill) ||
		    (orbMax > orbMaxKill)) continue;
		
		SBody *planet = new SBody;
		planet->type = SBody::TYPE_PLANET_DWARF;
		planet->seed = rand.Int32();
		planet->humanActivity = m_humanInfested * rand.Fixed();
		planet->tmp = 0;
		planet->parent = primary;
	//	planet->radius = EARTH_RADIUS*bodyTypeInfo[type].radius;
		planet->mass = mass;
		planet->rotationPeriod = fixed(rand.Int32(1,200), 24);

		planet->orbit.eccentricity = ecc.ToDouble();
		planet->orbit.semiMajorAxis = semiMajorAxis.ToDouble() * AU;
		planet->orbit.period = calc_orbital_period(planet->orbit.semiMajorAxis, SOL_MASS*primary->mass.ToDouble());
		planet->orbit.rotMatrix = matrix4x4d::RotateYMatrix(rand.NDouble(5)*M_PI/2.0) *
					  matrix4x4d::RotateZMatrix(rand.Double(M_PI));
		planet->orbMin = orbMin;
		planet->orbMax = orbMax;
		primary->children.push_back(planet);
	}
	delete disc;

	// merge children with overlapping or very close orbits
	primary->EliminateBadChildren();
	int idx=0;
	
	for (std::vector<SBody*>::iterator i = primary->children.begin(); i != primary->children.end(); ++i) {
		if ((*i)->GetSuperType() == SBody::SUPERTYPE_STAR) continue;
		// Turn them into something!!!!!!!
		char buf[3];
		buf[0] = ' ';
		buf[1] = 'b'+(idx++);
		buf[2] = 0;
		(*i)->name = primary->name+buf;
		fixed d = ((*i)->orbMin + (*i)->orbMax) >> 1;
		(*i)->PickPlanetType(this, primary, d, rand, true);

#ifdef DEBUG_DUMP
//		printf("%s: mass %f, semi-major axis %fAU, ecc %f\n", (*i)->name.c_str(), (*i)->mass.ToDouble(), (*i)->orbit.semiMajorAxis/AU, (*i)->orbit.eccentricity);
#endif /* DEBUG_DUMP */

	}
}

void SBody::PickPlanetType(StarSystem *system, SBody *star, const fixed distToPrimary, MTRand &rand, bool genMoons)
{
	fixed albedo = rand.Fixed() * fixed(1,2);
	fixed globalwarming = rand.Fixed() * fixed(9,10);
	// light planets have bugger all atmosphere
	if (mass < 1) globalwarming *= mass;
	// big planets get high global warming due to thick atmos
	if (mass > 3) globalwarming *= (mass-2);
	globalwarming = CLAMP(globalwarming, fixed(0), fixed(95,100));

	/* this is all of course a total fucking joke and un-physical */
	int bbody_temp;
	bool fiddle = false;
	for (int i=0; i<10; i++) {
		bbody_temp = CalcSurfaceTemp(star, distToPrimary, albedo, globalwarming);
		//printf("temp %f, albedo %f, globalwarming %f\n", bbody_temp, albedo, globalwarming);
		// extreme high temperature and low mass causes atmosphere loss
#define ATMOS_LOSS_MASS_CUTOFF	2
#define ATMOS_TEMP_CUTOFF	400
#define FREEZE_TEMP_CUTOFF	220
		if ((bbody_temp > ATMOS_TEMP_CUTOFF) &&
		   (mass < ATMOS_LOSS_MASS_CUTOFF)) {
		//	printf("atmos loss\n");
			globalwarming = globalwarming * (mass/ATMOS_LOSS_MASS_CUTOFF);
			fiddle = true;
		}
		if (!fiddle) break;
		fiddle = false;
	}
	// this is utter rubbish. should decide atmosphere composition and then freeze out
	// components of it in the previous loop
	if ((bbody_temp < FREEZE_TEMP_CUTOFF) && (mass < 5)) {
		globalwarming *= 0.2;
		albedo = rand.Double(0.05) + 0.9;
	}
	bbody_temp = CalcSurfaceTemp(star, distToPrimary, albedo, globalwarming);
//	printf("= temp %f, albedo %f, globalwarming %f\n", bbody_temp, albedo, globalwarming);

	averageTemp = bbody_temp;
	econType = 0;

	if (mass > 317*13) {
		// more than 13 jupiter masses can fuse deuterium - is a brown dwarf
		type = SBody::TYPE_BROWN_DWARF;
		// XXX should prevent mass exceeding 65 jupiter masses or so,
		// when it becomes a star
	} else if (mass > 300) {
		type = SBody::TYPE_PLANET_LARGE_GAS_GIANT;
	} else if (mass > 90) {
		type = SBody::TYPE_PLANET_MEDIUM_GAS_GIANT;
	} else if (mass > 6) {
		type = SBody::TYPE_PLANET_SMALL_GAS_GIANT;
	} else {
		// terrestrial planets
		if (mass < fixed(2,100)) {
			type = SBody::TYPE_PLANET_DWARF;
		} else if ((mass < fixed(2,10)) && (globalwarming < fixed(5,100))) {
			type = SBody::TYPE_PLANET_SMALL;
		} else if (mass < 3) {
			if ((averageTemp > CELSIUS-10) && (averageTemp < CELSIUS+70)) {
				// try for life
				int minTemp = CalcSurfaceTemp(star, orbMax, albedo, globalwarming);
				int maxTemp = CalcSurfaceTemp(star, orbMin, albedo, globalwarming);

				if ((minTemp > CELSIUS-10) && (minTemp < CELSIUS+70) &&
				    (maxTemp > CELSIUS-10) && (maxTemp < CELSIUS+70)) {
					type = SBody::TYPE_PLANET_INDIGENOUS_LIFE;
				} else {
					type = SBody::TYPE_PLANET_WATER;
				}
			} else {
				if (rand.Int32(0,1)) type = SBody::TYPE_PLANET_CO2;
				else type = SBody::TYPE_PLANET_METHANE;
			}
		} else /* 3 < mass < 6 */ {
			if ((averageTemp > CELSIUS-10) && (averageTemp < CELSIUS+70)) {
				type = SBody::TYPE_PLANET_WATER_THICK_ATMOS;
			} else {
				if (rand.Int32(0,1)) type = SBody::TYPE_PLANET_CO2_THICK_ATMOS;
				else type = SBody::TYPE_PLANET_METHANE_THICK_ATMOS;
			}
		}
		// kind of crappy
		if ((mass > fixed(8,10)) && (!rand.Int32(0,15))) type = SBody::TYPE_PLANET_HIGHLY_VOLCANIC;
	}
	radius = fixed(bodyTypeInfo[type].radius, 100);

	// generate moons
	if ((genMoons) && (mass > fixed(1,2))) {
		// min/max distances taken roughly from jovian and .. um.. saturnianish moons
		std::list<ring_t> *disc = AccreteDisc2(fixed(1,1), fixed(rand.Int32(1,5),1000)*mass, rand);
		for (std::list<ring_t>::iterator i = disc->begin(); i != disc->end(); ++i) {
//		std::vector<int> *disc = AccreteDisc(isqrt(mass.v>>13), 10, rand.Int32(1,10), rand);
//		for (unsigned int i=0; i<disc->size(); i++) {
			if ((*i).dist * MAX_MOON_DIST < MIN_MOON_DIST) continue;
			fixed moonmass = (*i).mass;
			if (moonmass < MIN_MOON_MASS) continue;
			SBody *moon = new SBody;
			moon->type = SBody::TYPE_PLANET_DWARF;
			moon->seed = rand.Int32();
			moon->tmp = 0;
			moon->parent = this;
		//	moon->radius = EARTH_RADIUS*bodyTypeInfo[type].radius;
			moon->rotationPeriod = fixed(rand.Int32(1,200), 24);
			moon->humanActivity = system->m_humanInfested * rand.Fixed();

			moon->mass = moonmass;
			fixed ecc = rand.NFixed(3);
			fixed semiMajorAxis = (*i).dist/100;
			moon->orbit.eccentricity = ecc.ToDouble();
			moon->orbit.semiMajorAxis = semiMajorAxis.ToDouble()*AU;
			moon->orbit.period = calc_orbital_period(moon->orbit.semiMajorAxis, this->mass.ToDouble() * EARTH_MASS);
			moon->orbit.rotMatrix = matrix4x4d::RotateYMatrix(rand.NDouble(5)*M_PI/2.0) *
						  matrix4x4d::RotateZMatrix(rand.Double(M_PI));
			this->children.push_back(moon);

			moon->orbMin = semiMajorAxis - ecc*semiMajorAxis;
			moon->orbMax = 2*semiMajorAxis - moon->orbMin;
		}
		delete disc;
	
		// merge moons with overlapping or very close orbits
		EliminateBadChildren();

		int idx=0;
		for(std::vector<SBody*>::iterator i = children.begin(); i!=children.end(); ++i) {
			// Turn them into something!!!!!!!
			char buf[2];
			buf[0] = '1'+(idx++);
			buf[1] = 0;
			(*i)->name = name+buf;
			(*i)->PickPlanetType(system, star, distToPrimary, rand, false);
		}

	}

	bool has_starports = false;
	// starports - orbital
	if ((genMoons) && (averageTemp < CELSIUS+100) && (averageTemp > 100) &&
		(rand.Fixed() < humanActivity)) {
		has_starports = true;
		SBody *sp = new SBody;
		sp->type = SBody::TYPE_STARPORT_ORBITAL;
		sp->seed = rand.Int32();
		sp->tmp = 0;
		sp->econType = econType;
		sp->parent = this;
		sp->rotationPeriod = fixed(1,3600);
		sp->averageTemp = this->averageTemp;
		sp->mass = 0;
		sp->name = "Starport";
		sp->humanActivity = humanActivity;
		fixed semiMajorAxis;
		if (children.size()) {
			semiMajorAxis = fixed(1,2) * children[0]->orbMin;
		} else {
			semiMajorAxis = fixed(1, 3557);
		}
		sp->orbit.eccentricity = 0;
		sp->orbit.semiMajorAxis = semiMajorAxis.ToDouble()*AU;
		sp->orbit.period = calc_orbital_period(sp->orbit.semiMajorAxis, this->mass.ToDouble() * EARTH_MASS);
		sp->orbit.rotMatrix = matrix4x4d::Identity();
		children.insert(children.begin(), sp);
		sp->orbMin = semiMajorAxis;
		sp->orbMax = semiMajorAxis;

		if (rand.Fixed() < humanActivity) {
			SBody *sp2 = new SBody;
			*sp2 = *sp;
			sp2->orbit.rotMatrix = matrix4x4d::RotateZMatrix(M_PI);
			children.insert(children.begin(), sp2);
		}
	}
	// starports - surface
	if ((averageTemp < CELSIUS+80) && (averageTemp > 100) &&
		((type == SBody::TYPE_PLANET_DWARF) ||
		(type == SBody::TYPE_PLANET_SMALL) ||
		(type == SBody::TYPE_PLANET_WATER) ||
		(type == SBody::TYPE_PLANET_CO2) ||
		(type == SBody::TYPE_PLANET_METHANE) ||
		(type == SBody::TYPE_PLANET_INDIGENOUS_LIFE))) {

		fixed activ = humanActivity;
		if (type == SBody::TYPE_PLANET_INDIGENOUS_LIFE) humanActivity *= 2;

		int max = 6;
		while ((max-- > 0) && (rand.Fixed() < activ)) {
			has_starports = true;
			SBody *sp = new SBody;
			sp->type = SBody::TYPE_STARPORT_SURFACE;
			sp->seed = rand.Int32();
			sp->tmp = 0;
			sp->parent = this;
			sp->averageTemp = this->averageTemp;
			sp->humanActivity = activ;
			sp->mass = 0;
			sp->name = "Starport";
			// used for orientation on planet surface
			sp->orbit.rotMatrix = matrix4x4d::RotateZMatrix(2*M_PI*rand.Double()) *
					      matrix4x4d::RotateYMatrix(2*M_PI*rand.Double());
			children.insert(children.begin(), sp);
		}
	}

	if (has_starports) {
		if (type == SBody::TYPE_PLANET_INDIGENOUS_LIFE)
			econType |= ECON_AGRICULTURE;
		else
			econType |= ECON_MINING;
		if (rand.Int32(2)) econType |= ECON_INDUSTRY;
		else econType |= ECON_MINING;
		system->PickEconomicStuff(this);
	}
}

void StarSystem::PickEconomicStuff(SBody *b)
{
	int tries = rand.Int32(10, 20);
	// This is a fucking mess. why is econType being decided before
	// getting here...
	
	// things we produce, plus their inputs
	while (tries--) {
		Equip::Type t = static_cast<Equip::Type>(rand.Int32(Equip::FIRST_COMMODITY, Equip::LAST_COMMODITY));
		const EquipType &type = EquipType::types[t];
		if (!(type.econType & b->econType)) continue;
		// XXX techlevel??
		int howmuch = rand.Int32(1,5);
		b->tradeLevel[t] += -howmuch;
		for (int i=0; i<EQUIP_INPUTS; i++) {
			b->tradeLevel[type.inputs[i]] += howmuch;
		}
	}
	// Add a bunch of things people consume
	const int NUM_CONSUMABLES = 10;
	const Equip::Type consumables[NUM_CONSUMABLES] = { 
		Equip::AIR_PROCESSORS,
		Equip::GRAIN,
		Equip::FRUIT_AND_VEG,
		Equip::ANIMAL_MEAT,
		Equip::LIQUOR,
		Equip::CONSUMER_GOODS,
		Equip::MEDICINES,
		Equip::HAND_WEAPONS,
		Equip::NARCOTICS,
		Equip::LIQUID_OXYGEN
	};

	tries = rand.Int32(3,6);
	while (tries--) {
		Equip::Type t = consumables[rand.Int32(0, NUM_CONSUMABLES - 1)];
		if ((t == Equip::AIR_PROCESSORS) ||
		    (t == Equip::LIQUID_OXYGEN)) {
			if (b->type == SBody::TYPE_PLANET_INDIGENOUS_LIFE)
				continue;
		}
		if (b->tradeLevel[t] >= 0) {
			b->tradeLevel[t] += rand.Int32(1,4);
		}
	}
}

StarSystem::~StarSystem()
{
	if (rootBody) delete rootBody;
}

bool StarSystem::IsSystem(int sector_x, int sector_y, int system_idx)
{
	return (sector_x == m_secx) && (sector_y == m_secy) && (system_idx == m_sysIdx);
}

SBody::~SBody()
{
	for (std::vector<SBody*>::iterator i = children.begin(); i != children.end(); ++i) {
		delete (*i);
	}
}

void StarSystem::Serialize(StarSystem *s)
{
	using namespace Serializer::Write;
	if (s) {
		wr_byte(1);
		wr_int(s->m_secx);
		wr_int(s->m_secy);
		wr_int(s->m_sysIdx);
	} else {
		wr_byte(0);
	}
}

StarSystem *StarSystem::Unserialize()
{
	using namespace Serializer::Read;
	if (rd_byte()) {
		int sec_x = rd_int();
		int sec_y = rd_int();
		int sys_idx = rd_int();
		return new StarSystem(sec_x, sec_y, sys_idx);
	} else {
		return 0;
	}
}

