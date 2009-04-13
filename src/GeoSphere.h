#ifndef _GEOSPHERE_H
#define _GEOSPHERE_H

#include "vector3.h"
#include "mtrand.h"

class SBody;
class GeoPatch;
class GeoSphere {
public:
	GeoSphere(const SBody *body);
	~GeoSphere();
	void Render(vector3d campos);
	void SetColor(const float col[4]);
	void AddCraters(MTRand &rand, int num, double minAng, double maxAng);
	double GetHeight(const vector3d &p);
	friend class GeoPatch;
private:
	GeoPatch *m_patches[6];
	struct crater_t {
		vector3d pos;
		double size;
	} *m_craters;
	int m_numCraters;
	float m_diffColor[4], m_ambColor[4];
	const SBody *m_sbody;

	double m_maxHeight;
	double m_invMaxHeight;
	double m_crap[16];
	double m_sealevel;

	vector3d GetColor(vector3d &p, double height);
};

#endif /* _GEOSPHERE_H */
