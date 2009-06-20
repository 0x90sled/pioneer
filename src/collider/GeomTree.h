#ifndef _GEOMTREE_H
#define _GEOMTREE_H

#include "../Aabb.h"

class BIHNode;
struct tri_t;

struct isect_t {
	// triIdx = -1 if no intersection
	int triIdx;
	float dist;
};

class GeomTree {
public:
	GeomTree(int numVerts, int numTris, float *vertices, int *indices, int *triflags);
	~GeomTree();
	const Aabb &GetAabb() const { return m_aabb; }
	const Aabb &GetMaxAabb() const { return m_maxAabb; }
	// dir should be unit length,
	// isect.dist should be ray length
	// isect.triIdx should be -1 unless repeat calls with same isect_t
	void TraceRay(const vector3f &start, const vector3f &dir, isect_t *isect) const;
	vector3f GetTriNormal(int triIdx) const;
	int GetTriFlag(int triIdx) const { return m_triFlags[triIdx]; }
	
	const int m_numVertices;
	const float *m_vertices;
	static int stats_rayTriIntersections;
private:
	friend class BIHNode;
	void RayTriIntersect(const vector3f &a_origin, const vector3f &a_dir, int triIdx, isect_t *isect) const;
	void TraverseRay(const vector3f &a_origin, const vector3f &a_dir, isect_t *isect) const;
	BIHNode *AllocNode();
	void BihTreeGhBuild(BIHNode* a_node, Aabb &a_box, Aabb &a_splitBox, int a_depth, int a_prims);

	Aabb m_aabb;
	Aabb m_maxAabb; // an aabb that still contains the object regardless of rotation
	BIHNode *m_nodes;
	int m_nodesAllocPos;
	int m_nodesAllocSize;
	tri_t *m_triAlloc;
	int m_triAllocPos;
	int m_triAllocSize;

	const int *m_indices;
	const int *m_triFlags;
};

#endif /* _GEOMTREE_H */
