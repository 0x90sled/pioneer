#include "StarSystem.h"
#include "Sector.h"

#define CELSIUS	273.15
#define DEBUG_DUMP

// indexed by enum type turd  
float StarSystem::starColors[7][3] = {
	{ 1.0, 0.2, 0.0 }, // M
	{ 1.0, 0.6, 0.1 }, // K
	{ 1.0, 1.0, 0.4 }, // G
	{ 1.0, 1.0, 0.8 }, // F
	{ 1.0, 1.0, 1.0 }, // A
	{ 0.7, 0.7, 1.0 }, // B
	{ 1.0, 0.7, 1.0 } // O
};

static const struct SBodySubTypeInfo {
	int mass; // % sol for stars, unused for planets
	int radius; // % sol radii for stars, % earth radii for planets
	const char *description;
	const char *icon;
	int tempMin, tempMax;
} bodyTypeInfo[StarSystem::TYPE_MAX] = {
	{
		40, 50, "Type 'M' red star",
		"icons/object_star_m.png",
		2000, 3500
	}, {
		80, 90, "Type 'K' orange star",
		"icons/object_star_k.png",
		3500, 5000
	}, { 
		110, 110, "Type 'G' yellow star",
		"icons/object_star_g.png",
		5000, 6000
	}, {
		170, 140, "Type 'F' white star",
		"icons/object_star_f.png",
		6000, 7500
	}, {
		310, 210, "Type 'A' hot white star",
		"icons/object_star_a.png",
		7500, 10000
	}, {
		1800, 700, "Bright type 'B' blue star",
		"icons/object_star_b.png",
		10000, 30000
	}, {
		6400, 1600, "Hot, massive type 'O' blue star",
		"icons/object_star_o.png",
		30000, 60000
	}, {
		0, 30, "Brown dwarf sub-stellar object",
		"icons/object_brown_dwarf.png"
	}, {
		0, 390, "Small gas giant",
		"icons/object_planet_small_gas_giant.png"
	}, {
		0, 950, "Medium gas giant",
		"icons/object_planet_medium_gas_giant.png"
	}, {
		0, 1110, "Large gas giant",
		"icons/object_planet_large_gas_giant.png"
	}, {
		0, 1500, "Very large gas giant",
		"icons/object_planet_large_gas_giant.png"
	}, {
		0, 26, "Small, rocky dwarf planet", // moon radius
		"icons/object_planet_dwarf.png"
	}, {
		0, 52, "Small, rocky planet with a thin atmosphere", // mars radius
		"icons/object_planet_small.png"
	}, {
		0, 100, "Rocky planet with liquid water and a nitrogen atmosphere", // earth radius
		"icons/object_planet_water_n2.png"
	}, {
		0, 100, "Rocky planet with a carbon dioxide atmosphere",
		"icons/object_planet_co2.png"
	}, {
		0, 100, "Rocky planet with a methane atmosphere",
		"icons/object_planet_methane.png"
	}, {
		0, 100, "Rocky planet with liquid water and a thick nitrogen atmosphere",
		"icons/object_planet_water_n2.png"
	}, {
		0, 100, "Rocky planet with a thick carbon dioxide atmosphere",
		"icons/object_planet_co2.png"
	}, {
		0, 100, "Rocky planet with a thick methane atmosphere",
		"icons/object_planet_methane.png"
	}, {
		0, 100, "Highly volcanic world",
		"icons/object_planet_volcanic.png"
	}, {
		0, 100, "World with indigenous life and an oxygen atmosphere",
		"icons/object_planet_life.png"
	}
};

const char *StarSystem::SBody::GetAstroDescription()
{
	return bodyTypeInfo[type].description;
}

const char *StarSystem::SBody::GetIcon()
{
	return bodyTypeInfo[type].icon;
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
static double calcSurfaceTemp(double star_radius, double star_temp, double object_dist, double albedo, double greenhouse)
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

static int calcSurfaceTemp(fixed star_radius, int star_temp, fixed object_dist, fixed albedo, fixed greenhouse)
{
	const fixed energy_per_meter2 = calcEnergyPerUnitAreaAtDist(star_radius, star_temp, object_dist);
	const fixed surface_temp_pow4 = energy_per_meter2*(1-albedo)/(1-greenhouse);
	return isqrt(isqrt((surface_temp_pow4.v>>16)*4409673));
}



void StarSystem::Orbit::KeplerPosAtTime(double t, double *dist, double *ang)
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
			
vector3d StarSystem::Orbit::CartesianPosAtTime(double t)
{
	double dist, ang;
	KeplerPosAtTime(t, &dist, &ang);
	vector3d pos = vector3d(cos(ang)*dist, sin(ang)*dist, 0);
	pos = rotMatrix * pos;
	return pos;
}

static std::vector<int> *AccreteDisc(int size, int bandSize, int density, MTRand &rand)
{
	std::vector<int> *disc = new std::vector<int>(size);

	int bandDensity = 0;
	for (int i=0; i<size; i++) {
		if (!(i%bandSize)) bandDensity = rand.Int32(density);
		(*disc)[i] = bandDensity * rand.Int32(density);
	}

	for (int iter=0; iter<20; iter++) {
		for (int i=0; i<(signed)disc->size(); i++) {
			int d=1+(i/3);

			for (; d>0; d--) {
				if ((i+d < (signed)disc->size()) && ((*disc)[i] > (*disc)[i+d])) {
					(*disc)[i] += (*disc)[i+d];
					(*disc)[i+d] = 0;
				}
				if (((i-d) >= 0) && ((*disc)[i] > (*disc)[i-d])) {
					(*disc)[i] += (*disc)[i-d];
					(*disc)[i-d] = 0;
				}
			}
		}
	}
	return disc;
}

double calc_orbital_period(double semiMajorAxis, double centralMass)
{
	return 2.0*M_PI*sqrt((semiMajorAxis*semiMajorAxis*semiMajorAxis)/(G*centralMass));
}

void StarSystem::SBody::EliminateBadChildren()
{
	// now check for overlapping & unacceptably close orbits. merge planets
	for (std::vector<SBody*>::iterator i = children.begin(); i != children.end(); ++i) {
		if ((*i)->temp) continue;

		for (std::vector<SBody*>::iterator j = children.begin(); j != children.end(); ++j) {
			if ((*j) == (*i)) continue;
			// don't eat anything bigger than self
			if ((*j)->mass > (*i)->mass) continue;
			fixed i_min = (*i)->radMin;
			fixed i_max = (*i)->radMax;
			fixed j_min = (*j)->radMin;
			fixed j_max = (*j)->radMax;
			fixed i_avg = (i_min+i_max)>>1;
			fixed j_avg = (j_min+j_max)>>1;
			bool eat = false;
			if (i_avg > j_avg) {
				if (i_min < j_max*fixed(12,10)) eat = true;
			} else {
				if (i_max > j_min*fixed(8,10)) eat = true;
			}
			if (eat) {
				(*i)->mass += (*j)->mass;
				(*j)->temp = 1;
			}
		}

	}

	// kill the eaten ones
	for (std::vector<SBody*>::iterator i = children.begin(); i != children.end();) {
		if ((*i)->temp) {
			i = children.erase(i);
		}
		else
			++i;
	}
}

/*
 * As my excellent comrades have pointed out, choices that depend on floating
 * point crap will result in different universes on different platforms.
 *
 * We must be sneaky and avoid floating point in these places.
 */
StarSystem::StarSystem(int sector_x, int sector_y, int system_idx)
{
	unsigned long _init[3] = { system_idx, sector_x, sector_y };
	loc.secX = sector_x;
	loc.secY = sector_y;
	loc.sysIdx = system_idx;
	rootBody = 0;
	if (system_idx == -1) return;
	rand.seed(_init, 3);

	Sector s = Sector(sector_x, sector_y);
	// primary
	SBody *primary = new SBody;

	StarSystem::BodyType type = s.m_systems[system_idx].primaryStarClass;
	primary->type = type;
	primary->parent = NULL;
	primary->radius = fixed(bodyTypeInfo[type].radius, 100);
	primary->mass = fixed(bodyTypeInfo[type].mass, 100);
	primary->supertype = SUPERTYPE_STAR;
	primary->averageTemp = rand.Int32(bodyTypeInfo[type].tempMin,
				bodyTypeInfo[type].tempMax);
	rootBody = primary;

	// XXX bad if the enum is fiddled with........
	int disc_size = rand.Int32(6,100) + rand.Int32(60,140)*primary->type*primary->type;
	//printf("disc_size %.1fAU\n", disc_size/10.0);

	std::vector<int> *disc = AccreteDisc(disc_size, 10, rand.Int32(10,400), rand);
	for (unsigned int i=0; i<disc->size(); i++) {
		fixed mass = fixed((*disc)[i]);
		if (mass == 0) continue;

		SBody *planet = new SBody;
		planet->type = TYPE_PLANET_DWARF;
		planet->supertype = SUPERTYPE_NONE;
		planet->seed = rand.Int32();
		planet->temp = 0;
		planet->parent = primary;
	//	planet->radius = EARTH_RADIUS*bodyTypeInfo[type].radius;
		planet->mass = mass;

		fixed ecc = rand.NFixed(3);
		fixed semiMajorAxis = fixed(i+1, 10); // in AUs
		planet->orbit.eccentricity = ecc.ToDouble();
		planet->orbit.semiMajorAxis = semiMajorAxis.ToDouble() * AU;
		planet->orbit.period = calc_orbital_period(planet->orbit.semiMajorAxis, SOL_MASS*primary->mass.ToDouble());
		planet->orbit.rotMatrix = matrix4x4d::RotateYMatrix(rand.NDouble(5)*M_PI/2.0) *
					  matrix4x4d::RotateZMatrix(rand.Double(M_PI));
		primary->children.push_back(planet);

		// perihelion and aphelion (in AUs)
		planet->radMin = semiMajorAxis - ecc*semiMajorAxis;
		planet->radMax = 2*semiMajorAxis - planet->radMin;
	}
	delete disc;

	// merge children with overlapping or very close orbits
	primary->EliminateBadChildren();
	primary->name = s.m_systems[system_idx].name;
	int idx=0;
	for (std::vector<SBody*>::iterator i = primary->children.begin(); i != primary->children.end(); ++i) {
		// Turn them into something!!!!!!!
		char buf[3];
		buf[0] = ' ';
		buf[1] = 'b'+(idx++);
		buf[2] = 0;
		(*i)->name = primary->name+buf;
		fixed d = ((*i)->radMin + (*i)->radMax) >> 1;
		(*i)->PickPlanetType(primary, d, rand, true);

#ifdef DEBUG_DUMP
		printf("%s: mass %f, semi-major axis %fAU, ecc %f\n", (*i)->name.c_str(), (*i)->mass.ToDouble(), (*i)->orbit.semiMajorAxis/AU, (*i)->orbit.eccentricity);
#endif /* DEBUG_DUMP */

	}
}

void StarSystem::SBody::PickPlanetType(SBody *star, const fixed distToPrimary, MTRand &rand, bool genMoons)
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
		bbody_temp = calcSurfaceTemp(star->radius, star->averageTemp, distToPrimary, albedo, globalwarming);
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
	bbody_temp = calcSurfaceTemp(star->radius, star->averageTemp, distToPrimary, albedo, globalwarming);
//	printf("= temp %f, albedo %f, globalwarming %f\n", bbody_temp, albedo, globalwarming);

	averageTemp = bbody_temp;

	if (mass > 317*13) {
		// more than 13 jupiter masses can fuse deuterium - is a brown dwarf
		type = TYPE_BROWN_DWARF;
		// XXX should prevent mass exceeding 65 jupiter masses or so,
		// when it becomes a star
	} else if (mass > 300) {
		type = TYPE_PLANET_LARGE_GAS_GIANT;
	} else if (mass > 90) {
		type = TYPE_PLANET_MEDIUM_GAS_GIANT;
	} else if (mass > 6) {
		type = TYPE_PLANET_SMALL_GAS_GIANT;
	} else {
		// terrestrial planets
		if (mass < fixed(2,100)) {
			type = TYPE_PLANET_DWARF;
		} else if ((mass < fixed(2,10)) && (globalwarming < fixed(5,100))) {
			type = TYPE_PLANET_SMALL;
		} else if (mass < 3) {
			if ((averageTemp > CELSIUS-10) && (averageTemp < CELSIUS+70)) {
				// try for life
				int minTemp = calcSurfaceTemp(star->radius, star->averageTemp, radMax, albedo, globalwarming);
				int maxTemp = calcSurfaceTemp(star->radius, star->averageTemp, radMin, albedo, globalwarming);

				if ((minTemp > CELSIUS-10) && (minTemp < CELSIUS+70) &&
				    (maxTemp > CELSIUS-10) && (maxTemp < CELSIUS+70)) {
					type = TYPE_PLANET_INDIGENOUS_LIFE;
				} else {
					type = TYPE_PLANET_WATER;
				}
			} else {
				if (rand.Int32(0,1)) type = TYPE_PLANET_CO2;
				else type = TYPE_PLANET_METHANE;
			}
		} else /* 3 < mass < 6 */ {
			if ((averageTemp > CELSIUS-10) && (averageTemp < CELSIUS+70)) {
				type = TYPE_PLANET_WATER_THICK_ATMOS;
			} else {
				if (rand.Int32(0,1)) type = TYPE_PLANET_CO2_THICK_ATMOS;
				else type = TYPE_PLANET_METHANE_THICK_ATMOS;
			}
		}
		// kind of crappy
		if ((mass > fixed(8,10)) && (!rand.Int32(0,15))) type = TYPE_PLANET_HIGHLY_VOLCANIC;
	}
	radius = fixed(bodyTypeInfo[type].radius, 100);

	// generate moons
	if ((genMoons) && (mass > fixed(1,2))) {
		std::vector<int> *disc = AccreteDisc(isqrt(mass.v>>12), 10, rand.Int32(1,10), rand);
		for (unsigned int i=0; i<disc->size(); i++) {
			fixed mass = fixed((*disc)[i]);
			if (mass == 0) continue;

			SBody *moon = new SBody;
			moon->type = TYPE_PLANET_DWARF;
			moon->supertype = SUPERTYPE_NONE;
			moon->seed = rand.Int32();
			moon->temp = 0;
			moon->parent = this;
		//	moon->radius = EARTH_RADIUS*bodyTypeInfo[type].radius;
			moon->mass = mass;
			fixed ecc = rand.NFixed(3);
			fixed semiMajorAxis = fixed(i+1, 2000);
			moon->orbit.eccentricity = ecc.ToDouble();
			moon->orbit.semiMajorAxis = semiMajorAxis.ToDouble()*AU;
			moon->orbit.period = calc_orbital_period(moon->orbit.semiMajorAxis, this->mass.ToDouble() * EARTH_MASS);
			moon->orbit.rotMatrix = matrix4x4d::RotateYMatrix(rand.NDouble(5)*M_PI/2.0) *
						  matrix4x4d::RotateZMatrix(rand.Double(M_PI));
			this->children.push_back(moon);

			moon->radMin = semiMajorAxis - ecc*semiMajorAxis;
			moon->radMax = 2*semiMajorAxis - moon->radMin;
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
			(*i)->PickPlanetType(star, distToPrimary, rand, false);
		}
	}
}

StarSystem::~StarSystem()
{
	if (rootBody) delete rootBody;
}

bool StarSystem::IsSystem(int sector_x, int sector_y, int system_idx)
{
	return (sector_x == loc.secX) && (sector_y == loc.secY) && (system_idx == loc.sysIdx);
}

StarSystem::SBody::~SBody()
{
	for (std::vector<SBody*>::iterator i = children.begin(); i != children.end(); ++i) {
		delete (*i);
	}
}

