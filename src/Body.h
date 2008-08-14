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
	Body();
	virtual ~Body();
	virtual Object::Type GetType() { return Object::BODY; }
	virtual void SetPosition(vector3d p) = 0;
	virtual vector3d GetPosition() = 0; // within frame
	virtual void SetVelocity(vector3d v) { assert(0); }
	virtual vector3d GetVelocity() { assert(0); return vector3d(0.0); }
	virtual double GetRadius() const = 0;
	virtual void SetRotMatrix(const matrix4x4d &r) {};
	virtual void GetRotMatrix(matrix4x4d &m) { };
	virtual void Render(const Frame *camFrame) = 0;
	virtual void SetFrame(Frame *f) { m_frame = f; }
	// return true if to do collision response and apply damage
	virtual bool OnCollision(Body *b, Uint32 flags) { return false; }
	virtual void TimeStepUpdate(const float timeStep) {}
	// Override to clear any pointers you hold to the dying body.
	virtual void NotifyDeath(const Body* const dyingBody) {}
	
	vector3d GetPositionRelTo(const Frame *);
	Frame *GetFrame() { return m_frame; }
	void SetLabel(const char *label) { m_label = label; }
	std::string &GetLabel() { return m_label; }
	unsigned int GetFlags() { return m_flags; }
	void SetProjectedPos(const vector3d& projectedPos) { m_projectedPos = projectedPos; }
	// Only valid if IsOnscreen() is true.
	const vector3d& GetProjectedPos() const;
	bool IsOnscreen() const { return m_onscreen; }
	void SetOnscreen(const bool onscreen) { m_onscreen = onscreen; }
	// Only Space::KillBody() should call this method.
	void MarkDead() { m_dead = true; }

	enum { FLAG_CAN_MOVE_FRAME = 1 };
protected:
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
