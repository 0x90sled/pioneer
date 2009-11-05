#ifndef _BODY_H
#define _BODY_H

#include "vector3.h"
#include "matrix4x4.h"
#include "Object.h"
#include <string>

class Frame;
class ObjMesh;

class Body: public Object {
public:
	OBJDEF(Body, Object, BODY);
	Body();
	virtual ~Body();
	void Serialize();
	static Body *Unserialize();
	virtual void PostLoadFixup() {};
	virtual void SetPosition(vector3d p) = 0;
	virtual vector3d GetPosition() const = 0; // within frame
	virtual void SetVelocity(vector3d v) { assert(0); }
	virtual vector3d GetVelocity() const { return vector3d(0.0); }
	virtual double GetRadius() const = 0;
	virtual double GetMass() const { assert(0); return 0; }
	virtual void SetRotMatrix(const matrix4x4d &r) {};
	virtual void GetRotMatrix(matrix4x4d &m) const { };
	virtual void Render(const Frame *camFrame) = 0;
	virtual void SetFrame(Frame *f) { m_frame = f; }
	// return true if to do collision response and apply damage
	virtual bool OnCollision(Object *o, Uint32 flags, double relVel) { return false; }
	// Attacker may be null
	virtual bool OnDamage(Object *attacker, float kgDamage) { return false; }
	virtual void OnHaveKilled(Body *guyWeKilled) {}
	virtual void TimeStepUpdate(const float timeStep) {}
	// after all bodies have had TimeStepUpdate (their moving step),
	// StaticUpdate() is called. Good for special collision testing (Projectiles)
	// as you can't test for collisions if different objects are on different 'steps'
	virtual void StaticUpdate(const float timeStep) {}
	// Note: Does not mean killed, just deleted.
	// Override to clear any pointers you hold to the body
	virtual void NotifyDeleted(const Body* const deletedBody) {}
	vector3d GetVelocityRelativeTo(const Body *other) const;
	vector3d GetVelocityRelativeTo(const Frame *f) const;
	// for putting on planet surface, oriented +y up
	void OrientOnSurface(double radius, double latitude, double longitude);
	vector3d GetPositionRelTo(const Frame *) const;
	Frame *GetFrame() const { return m_frame; }
	void SetLabel(const char *label) { m_label = label; }
	const std::string &GetLabel() const { return m_label; }
	unsigned int GetFlags() { return m_flags; }
	void SetProjectedPos(const vector3d& projectedPos) { m_projectedPos = projectedPos; }
	// Only valid if IsOnscreen() is true.
	const vector3d& GetProjectedPos() const;
	bool IsOnscreen() const { return m_onscreen; }
	void SetOnscreen(const bool onscreen) { m_onscreen = onscreen; }
	// Only Space::KillBody() should call this method.
	void MarkDead() { m_dead = true; }
	bool IsDead() const { return m_dead; }

	enum { FLAG_CAN_MOVE_FRAME = (1<<0),
               FLAG_LABEL_HIDDEN = (1<<1) };
protected:
	virtual void Save();
	virtual void Load();
	unsigned int m_flags;
private:
	// frame of reference
	Frame *m_frame;
	std::string m_label;
	bool m_onscreen;
	vector3d m_projectedPos;
	// Checked in destructor to make sure body has been marked dead.
	bool m_dead;
};

#endif /* _BODY_H */
