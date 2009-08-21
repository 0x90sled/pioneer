#ifndef _COLLISION_CONTACT_H
#define _COLLISION_CONTACT_H

struct CollisionContact {
	vector3d pos;
	vector3d normal;
	double depth;
	double dist; // distance travelled to hit point
	int triIdx;
	void *userData1, *userData2;
	int geomFlag;
	CollisionContact() {
		depth = 0; triIdx = -1; userData1 = userData2 = 0; geomFlag = 0; dist = 0;
	}
};

#endif /* _COLLISION_CONTACT_H */
