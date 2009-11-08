#ifndef _PROJECTILE_H
#define _PROJECTILE_H

#include "Body.h"

class Frame;

class Projectile: public Body {
public:
	OBJDEF(Projectile, Body, PROJECTILE);
	enum TYPE { TYPE_1MW_PULSE, TYPE_2MW_PULSE, TYPE_4MW_PULSE, TYPE_10MW_PULSE, TYPE_20MW_PULSE,
			TYPE_17MW_MINING };

	static void Add(Body *parent, TYPE t, const vector3d &pos, const vector3d &baseVel, const vector3d &dirVel);

	Projectile();
	//virtual ~Projectile();
	virtual void SetPosition(vector3d p);
	virtual vector3d GetPosition() const { return vector3d(m_orient[12], m_orient[13], m_orient[14]); }
	virtual double GetRadius() const { return 10; }
	virtual void Render(const Frame *camFrame);
	void TimeStepUpdate(const float timeStep);
	void StaticUpdate(const float timeStep);
	virtual void NotifyDeleted(const Body* const deletedBody);

	virtual void PostLoadFixup();
protected:
	virtual void Save();
	virtual void Load();
private:
	float GetDamage() const;
	Body *m_parent;
	matrix4x4d m_orient;
	vector3d m_baseVel;
	vector3d m_dirVel;
	float m_age;
	enum TYPE m_type;
};

#endif /* _PROJECTILE_H */
