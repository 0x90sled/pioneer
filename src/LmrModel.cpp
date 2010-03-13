#include "libs.h"
#include <map>
#include "glfreetype.h"
#include "LmrModel.h"
#include "collider/collider.h"
#include "perlin.h"
#include "Render.h"

#define MODEL "Model"
struct RenderState {
	/* For the root model this will be identity matrix.
	 * For sub-models called with call_model() then this will be the
	 * transform from sub-model coords to root-model coords.
	 * It is needed by the RenderThruster stuff so we know the centre of
	 * the root model and orientation when rendering thrusters on
	 * sub-models */
	matrix4x4f subTransform;
	// combination of model scale, call_model scale, and all parent scalings
	float combinedScale;
};

namespace ShipThruster {
	struct Thruster
	{
		// cannot be used as an angular thruster
		bool linear_only;
		vector3f pos;
		vector3f dir;
		float power;
	};

	
	static vector3f ResolveHermiteSpline (const vector3f &p0, const vector3f &p1, const vector3f &n0, const vector3f &n1, float t)
	{
		float t2 = t*t, t3 = t*t*t;
		vector3f tv1, tv2, tv;
		tv1 = p0 * (2*t3-3*t2+1);
		tv = n0 * (t3-2*t2+t);
		tv1 = tv+tv1;
		tv2 = p1 * (-2*t3+3*t2);
		tv = n1 *  (t3-t2);
		tv2 = tv + tv2;
		return tv1 + tv2;
	}

	static const int pNumIndex[3] =
		{ (2*4+1*8)*3, (2*8+5*16)*3, (2*16+13*32)*3 };

	static vector3f pTVertex4pt[2*4+2];
	static vector3f pTVertex8pt[6*8+2];
	static vector3f pTVertex16pt[14*16+2];

	static Uint16 pTIndex4pt[(2*4+1*8)*3];
	static Uint16 pTIndex8pt[(2*8+5*16)*3];
	static Uint16 pTIndex16pt[(2*16+13*32)*3];

	static vector3f *ppTVertex[3] =
		{ pTVertex4pt, pTVertex8pt, pTVertex16pt };

	static Uint16 *ppTIndex[3] =
		{ pTIndex4pt, pTIndex8pt, pTIndex16pt };


	static void GenerateThrusters ()
	{
		vector3f pos0 = vector3f(0.0f, 0.0f, 0.0f);
		vector3f pos1 = vector3f(0.0f, 0.0f, 1.0f);
		vector3f tan0 = vector3f(0.0f, 1.0f, 0.2f);
		vector3f tan1 = vector3f(0.0f, -0.2f, 1.0f);

		int j, n;
		for (j=0, n=4; j<3; j++, n<<=1)
		{
			vector3f *pCur = ppTVertex[j];
			float t, incstep = 1.0f / (n-1);
			int i; for (i=0, t=incstep; i<n-2; i++, t+=incstep)
			{
				vector3f *pos = pCur;
				*pos = ResolveHermiteSpline (pos0, pos1, tan0, tan1, t);
				pCur++;

				float angstep = 2.0f * 3.141592f / n, ang = angstep;
				int i; for (i=1; i<n; i++, ang+=angstep, pCur++)
				{
					pCur->x = sin(ang) * pos->y;
					pCur->y = cos(ang) * pos->y;
					pCur->z = pos->z;
				}
			}
			*pCur = pos0; pCur++;
			*pCur = pos1; pCur++;

			int ni=0, k;
			Uint16 *pIndex = ppTIndex[j];

			for (k=0; k<n; k++) {
				int k1 = k+1==n ? 0 : k+1;
				pIndex[ni++] = (n-2)*n; pIndex[ni++] = k; pIndex[ni++] = k1;
				pIndex[ni++] = (n-2)*n+1; pIndex[ni++] = k1+(n-3)*n; pIndex[ni++] = k+(n-3)*n;
			}
			for (i=0; i<n-3; i++)
			{
				for (k=0; k<n; k++) {
					int k1 = k+1==n ? 0 : k+1;
					pIndex[ni++] = k+i*n; pIndex[ni++] = k+i*n+n; pIndex[ni++] = k1+i*n;
					pIndex[ni++] = k1+i*n; pIndex[ni++] = k+i*n+n;	pIndex[ni++] = k1+i*n+n;
				}
			}
		}
	}

	static const float s_black[4] = { 0, 0, 0, 0 };
	static const float s_alpha[4] = { 0, 0, 0, 0.6 };
	static bool inittted = false;

	static void RenderThruster(const RenderState *rstate, const LmrObjParams *params, Thruster *pThruster)
	{
		if (!inittted) GenerateThrusters();
		inittted = true;
		const float scale = 1.0;
		// to find v(0,0,0) position of root model (when putting thrusters on sub-models)
		vector3f compos = vector3f(rstate->subTransform[12], rstate->subTransform[13], rstate->subTransform[14]);
		matrix4x4f invSubModelMat = matrix4x4f::MakeRotMatrix(
					vector3f(rstate->subTransform[0], rstate->subTransform[1], rstate->subTransform[2]),
					vector3f(rstate->subTransform[4], rstate->subTransform[5], rstate->subTransform[6]),
					vector3f(rstate->subTransform[8], rstate->subTransform[9], rstate->subTransform[10]));

		vector3f start, end, dir = pThruster->dir;
		start = pThruster->pos * scale;
		float power = -vector3f::Dot(dir, invSubModelMat * vector3f(params->linthrust));

		if (!pThruster->linear_only) {
			vector3f angdir, cpos;
			const vector3f at = invSubModelMat * vector3f(params->angthrust);
			cpos = compos + start;
			angdir = vector3f::Cross(cpos, dir);
			float xp = angdir.x * at.x;
			float yp = angdir.y * at.y;
			float zp = angdir.z * at.z;
			if (xp+yp+zp > 0) {
				if (xp > yp && xp > zp && fabs(at.x) > power) power = fabs(at.x);
				else if (yp > xp && yp > zp && fabs(at.y) > power) power = fabs(at.y);
				else if (zp > xp && zp > yp && fabs(at.z) > power) power = fabs(at.z);
			}
		}

		if (power <= 0.001f) return;
		power *= scale;
		float width = sqrt(power)*pThruster->power*0.6f;
		float len = power*pThruster->power;
		end = dir * len;
		end += start;

		vector3f v1, v2, pos;
		matrix4x4f m2;
		matrix4x4f m = matrix4x4f::Identity();
		v1.x = dir.y; v1.y = dir.z; v1.z = dir.x;
		v2 = vector3f::Cross(v1, dir).Normalized();
		v1 = vector3f::Cross(v2, dir);
		m[0] = v1.x; m[4] = v2.x; m[8] = dir.x;
		m[1] = v1.y; m[5] = v2.y; m[9] = dir.y;
		m[2] = v1.z; m[6] = v2.z; m[10] = dir.z;
		m2 = /*objorient **/ m;

		pos = /*objorient **/ start;

		m2[12] = pos.x;
		m2[13] = pos.y;
		m2[14] = pos.z;
		
		glPushMatrix ();
		glMultMatrixf (&m2[0]);

		glScalef (width*0.5f, width*0.5f, len*0.666f);
		
		glMaterialfv (GL_FRONT, GL_AMBIENT_AND_DIFFUSE, s_alpha);
		glMaterialfv (GL_FRONT, GL_SPECULAR, s_black);
		float col[4] = { 0.0f, 0.4f, 1.0f, 1.0f };
		glMaterialfv (GL_FRONT, GL_EMISSION, col);

		glVertexPointer (3, GL_FLOAT, sizeof(vector3f), pTVertex8pt);
		glDrawElements (GL_TRIANGLES, pNumIndex[1], GL_UNSIGNED_SHORT, pTIndex8pt);

		glScalef (2.0f, 2.0f, 1.5f);
		col[0] = 0.4f; col[1] = 0; col[2] = 1.0f;
		glMaterialfv (GL_FRONT, GL_EMISSION, col);
		glVertexPointer (3, GL_FLOAT, sizeof(vector3f), pTVertex8pt);
		glDrawElements (GL_TRIANGLES, pNumIndex[1], GL_UNSIGNED_SHORT, pTIndex8pt);

		glPopMatrix ();
	}


}

class LmrGeomBuffer;

static Render::Shader *s_normalShader;
static float s_scrWidth = 800.0f;
static bool s_buildDynamic;
static FontFace *s_font;
static float NEWMODEL_ZBIAS = 0.0002f;
static LmrGeomBuffer *s_curBuf;
static const LmrObjParams *s_curParams;
static std::map<std::string, LmrModel*> s_models;
static lua_State *sLua;
static int s_numTrisRendered;

lua_State *LmrGetLuaState() { return sLua; }

void LmrNotifyScreenWidth(float width)
{
	s_scrWidth = width;
}

int LmrModelGetStatsTris() { return s_numTrisRendered; }
void LmrModelClearStatsTris() { s_numTrisRendered = 0; }

#define BUFFER_OFFSET(i) ((char *)NULL + (i))
//#define USE_VBO 0
	
class LmrGeomBuffer {
public:
	LmrGeomBuffer(LmrModel *model) {
		curOp.type = OP_NONE;
		curTriFlag = 0;
		m_vertexBuffer = 0;
		m_indexBuffer = 0;
		m_model = model;
	}
	~LmrGeomBuffer() {
		if (m_vertexBuffer) glDeleteBuffersARB(1, &m_vertexBuffer);
		if (m_indexBuffer) glDeleteBuffersARB(1, &m_indexBuffer);
	}
	int GetIndicesPos() const {
		return m_indices.size();
	}
	int GetVerticesPos() const {
		return m_vertices.size();
	}
	void SetGeomFlag(Uint16 flag) {
		curTriFlag = flag;
	}
	Uint16 GetGeomFlag() const {
		return curTriFlag;
	}
	void PreBuild() {
		FreeGeometry();
		curTriFlag = 0;
	}
	void PostBuild() {
		PushCurOp();
		// we are using Uint16 index arrays
		assert(m_indices.size() < 65536);
	//		printf("%d vertices, %d indices, %d ops\n", m_vertices.size(), m_indices.size(), m_ops.size());
		if (USE_VBO) {
			if (m_vertexBuffer == 0) glGenBuffersARB(1, &m_vertexBuffer);
			if (m_indexBuffer == 0) glGenBuffersARB(1, &m_indexBuffer);
			
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
			glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER, sizeof(Uint16)*m_indices.size(),
					0, GL_STATIC_DRAW);
			if (m_indices.size())
				glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER, sizeof(Uint16)*m_indices.size(),
					&m_indices[0], GL_STATIC_DRAW);
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);
		
			glBindBufferARB(GL_ARRAY_BUFFER, m_vertexBuffer);
			glBufferDataARB(GL_ARRAY_BUFFER, sizeof(Vertex)*m_vertices.size(), 0, GL_STATIC_DRAW);
			if (m_vertices.size())
				glBufferDataARB(GL_ARRAY_BUFFER, sizeof(Vertex)*m_vertices.size(), &m_vertices[0], GL_STATIC_DRAW);
			glBindBufferARB(GL_ARRAY_BUFFER, 0);
		}
		curOp.type = OP_NONE;
	}
	void FreeGeometry() {
		m_vertices.clear();
		m_indices.clear();
		m_triflags.clear();
		m_ops.clear();
		m_thrusters.clear();
	}
	void Render(const RenderState *rstate, const vector3f &cameraPos, const LmrObjParams *params) {
		s_numTrisRendered += m_indices.size()/3;
		
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	
		glEnable(GL_LIGHTING);

		BindBuffers();

		glDepthRange(0.0, 1.0);

		const unsigned int opEndIdx = m_ops.size();
		for (unsigned int i=0; i<opEndIdx; i++) {
			const Op &op = m_ops[i];
			switch (op.type) {
			case OP_DRAW_ELEMENTS:
				if (USE_VBO) {
					glDrawRangeElements(GL_TRIANGLES, op.elems.elemMin, op.elems.elemMax, op.elems.count, GL_UNSIGNED_SHORT, BUFFER_OFFSET(op.elems.start*sizeof(Uint16)));
				} else {
					glDrawElements(GL_TRIANGLES, op.elems.count, GL_UNSIGNED_SHORT, &m_indices[op.elems.start]);
				}
				break;
			case OP_DRAW_BILLBOARDS:
				// XXX not using vbo yet
				if (USE_VBO) UnbindBuffers();
				Render::PutPointSprites(op.billboards.count, &m_vertices[op.billboards.start].v, op.billboards.size,
						op.billboards.col, op.billboards.tex);
				BindBuffers();
				break;
			case OP_SET_MATERIAL:
				{
					const LmrMaterial &m = m_model->m_materials[op.col.material_idx];
					glMaterialfv (GL_FRONT, GL_AMBIENT_AND_DIFFUSE, m.diffuse);
					glMaterialfv (GL_FRONT, GL_SPECULAR, m.specular);
					glMaterialfv (GL_FRONT, GL_EMISSION, m.emissive);
					glMaterialf (GL_FRONT, GL_SHININESS, m.shininess);
					if (m.diffuse[3] == 1.0) {
						glDisable(GL_BLEND);
					} else {
						glEnable(GL_BLEND);
					}
				}
				break;
			case OP_ZBIAS:
				if (op.zbias.amount == 0) {
					glDepthRange(0.0, 1.0);
				} else {
					vector3f tv = cameraPos - vector3f(op.zbias.pos);
				//	if (vector3f::Dot(tv, vector3f(op.zbias.norm)) < 0.0f) {
						glDepthRange(0.0, 1.0 - op.zbias.amount*NEWMODEL_ZBIAS);
				//	} else {
				//		glDepthRange(0.0, 1.0);
				//	}
				}
				break;
			case OP_CALL_MODEL:
				{
				// XXX materials fucked up after this
				const matrix4x4f trans = matrix4x4f(op.callmodel.transform);
				vector3f cam_pos = cameraPos - vector3f(trans[12], trans[13], trans[14]);
				RenderState rstate2;
				rstate2.subTransform = rstate->subTransform * trans;
				rstate2.combinedScale = rstate->combinedScale * op.callmodel.scale * op.callmodel.model->m_scale;
				op.callmodel.model->Render(&rstate2, cam_pos, trans, params);
				// XXX re-binding buffer may not be necessary
				BindBuffers();
				}
				break;
			case OP_NONE:
				break;
			}
		}
		
		glDisableClientState (GL_VERTEX_ARRAY);
		glDisableClientState (GL_NORMAL_ARRAY);

		if (USE_VBO) {
			UnbindBuffers();
		}

		if (m_thrusters.size()) RenderThrusters(rstate, cameraPos, params);
	}

	void UnbindBuffers() {
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void RenderThrusters(const RenderState *rstate, const vector3f &cameraPos, const LmrObjParams *params) {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);	
		glEnableClientState (GL_VERTEX_ARRAY);
		glDisableClientState (GL_NORMAL_ARRAY);
		glEnable (GL_BLEND);
		for (unsigned int i=0; i<m_thrusters.size(); i++) {
			ShipThruster::RenderThruster (rstate, params, &m_thrusters[i]);
		}
		glDisable (GL_BLEND);
		glDisableClientState (GL_VERTEX_ARRAY);
	}
	void PushThruster(const vector3f &pos, const vector3f &dir, const float power, bool linear_only) {
		unsigned int i = m_thrusters.size();
		m_thrusters.resize(i+1);
		m_thrusters[i].pos = pos;
		m_thrusters[i].dir = dir;
		m_thrusters[i].power = power;
		m_thrusters[i].linear_only = linear_only;
	}
	int PushVertex(const vector3f &pos, const vector3f &normal) {
		m_vertices.push_back(Vertex(pos, normal));
		return m_vertices.size() - 1;
	}
	void SetVertex(int idx, const vector3f &pos, const vector3f &normal) {
		m_vertices[idx] = Vertex(pos, normal);
	}
	void PushTri(int i1, int i2, int i3) {
		OpDrawElements(3);
		PushIdx(i1);
		PushIdx(i2);
		PushIdx(i3);
		m_triflags.push_back(curTriFlag);
	}
	
	void PushZBias(float amount, const vector3f &pos, const vector3f &norm) {
		if (curOp.type) m_ops.push_back(curOp);
		curOp.type = OP_ZBIAS;
		curOp.zbias.amount = amount;
		memcpy(curOp.zbias.pos, &pos.x, 3*sizeof(float));
		memcpy(curOp.zbias.norm, &norm.x, 3*sizeof(float));
	}

	void PushCallModel(LmrModel *m, const matrix4x4f &transform, float scale) {
		if (curOp.type) m_ops.push_back(curOp);
		curOp.type = OP_CALL_MODEL;
		memcpy(curOp.callmodel.transform, &transform[0], 16*sizeof(float));
		curOp.callmodel.model = m;
		curOp.callmodel.scale = scale;
	}

	void PushInvisibleTri(int i1, int i2, int i3) {
		if (curOp.type != OP_NONE) m_ops.push_back(curOp);
		curOp.type = OP_NONE;
		PushIdx(i1);
		PushIdx(i2);
		PushIdx(i3);
		m_triflags.push_back(curTriFlag);
	}
	
	void PushBillboards(const char *texname, const float size, const vector3f &color, const int numPoints, const vector3f *points)
	{
		char buf[256];
		snprintf(buf, sizeof(buf), "data/textures/%s", texname);
		GLuint tex = util_load_tex_rgba(buf);

		if (curOp.type) m_ops.push_back(curOp);
		curOp.type = OP_DRAW_BILLBOARDS;
		curOp.billboards.start = m_vertices.size();
		curOp.billboards.count = numPoints;
		curOp.billboards.tex = tex;
		curOp.billboards.size = size;
		curOp.billboards.col[0] = color.x;
		curOp.billboards.col[1] = color.y;
		curOp.billboards.col[2] = color.z;
		curOp.billboards.col[3] = 1.0f;

		// Hack -- store in v and n of Vertex structure...
		for (int i=0; i<numPoints; i+=2) {
			if (i < numPoints-1) {
				PushVertex(points[i], points[i+1]);
			} else {
				PushVertex(points[i], vector3f(0.0f));
			}
		}
	}

	void SetMaterial(const char *mat_name, const float mat[11]) {
		std::map<std::string, int>::iterator i = m_model->m_materialLookup.find(mat_name);
		if (i != m_model->m_materialLookup.end()) {
			LmrMaterial &m = m_model->m_materials[(*i).second];
			m.diffuse[0] = mat[0];
			m.diffuse[1] = mat[1];
			m.diffuse[2] = mat[2];
			m.diffuse[3] = mat[3];
			m.specular[0] = mat[4];
			m.specular[1] = mat[5];
			m.specular[2] = mat[6];
			m.specular[3] = 1.0f;
			m.shininess = mat[7];
			m.emissive[0] = mat[8];
			m.emissive[1] = mat[9];
			m.emissive[2] = mat[10];
			m.emissive[3] = 1.0f;
		} else {
			luaL_error(sLua, "Unknown material name '%s'.", mat_name);
			exit(0);
		}
	}

	void PushUseMaterial(const char *mat_name) {
		if (curOp.type) m_ops.push_back(curOp);
		curOp.type = OP_SET_MATERIAL;
		
		std::map<std::string, int>::iterator i = m_model->m_materialLookup.find(mat_name);
		if (i != m_model->m_materialLookup.end()) {
			curOp.col.material_idx = (*i).second;
		} else {
			printf("Unknown material name '%s'.\n", mat_name);
			exit(0);
		}
	}

	/* return start vertex index */
	int AllocVertices(int num) {
		int start = m_vertices.size();
		m_vertices.resize(start + num);
		return start;
	}

	const vector3f &GetVertex(int num) const {
		return m_vertices[num].v;
	}

	void GetCollMeshGeometry(LmrCollMesh *c, const matrix4x4f &transform, const LmrObjParams *params) {
		const int vtxBase = c->nv;
		const int idxBase = c->ni;
		const int flagBase = c->nf;
		c->nv += m_vertices.size();
		c->ni += m_indices.size();
		c->nf += m_indices.size()/3;
		assert(m_triflags.size() == m_indices.size()/3);
		c->m_numTris += m_triflags.size();

		if (m_vertices.size()) {
			c->pVertex = (float*)realloc(c->pVertex, 3*sizeof(float)*c->nv);
		
			for (unsigned int i=0; i<m_vertices.size(); i++) {
				const vector3f v = transform * m_vertices[i].v;
				c->pVertex[3*vtxBase + 3*i] = v.x;
				c->pVertex[3*vtxBase + 3*i+1] = v.y;
				c->pVertex[3*vtxBase + 3*i+2] = v.z;
				c->m_aabb.Update(v);
			}
		}
		if (m_indices.size()) {
			c->pIndex = (int*)realloc(c->pIndex, sizeof(int)*c->ni);
			c->pFlag = (int*)realloc(c->pFlag, sizeof(int)*c->nf);
			for (unsigned int i=0; i<m_indices.size(); i++) {
				c->pIndex[idxBase + i] = vtxBase + m_indices[i];
			}
			for (unsigned int i=0; i<m_triflags.size(); i++) {
				c->pFlag[flagBase + i] = m_triflags[i];
			}
		}
		
		// go through Ops to see if we call other models
		const unsigned int opEndIdx = m_ops.size();
		for (unsigned int i=0; i<opEndIdx; i++) {
			const Op &op = m_ops[i];
			if (op.type == OP_CALL_MODEL) {
				matrix4x4f _trans = transform * matrix4x4f(op.callmodel.transform);
				op.callmodel.model->GetCollMeshGeometry(c, _trans, params);
			}
		}
	}

private:
	void BindBuffers() {
		glEnableClientState (GL_VERTEX_ARRAY);
		glEnableClientState (GL_NORMAL_ARRAY);
		if (USE_VBO) {
			glBindBufferARB(GL_ARRAY_BUFFER, m_vertexBuffer);
			glVertexPointer(3, GL_FLOAT, sizeof(Vertex), 0);
			glNormalPointer(GL_FLOAT, sizeof(Vertex), (void *)(3*sizeof(float)));
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
		} else {
			glNormalPointer(GL_FLOAT, 2*sizeof(vector3f), &m_vertices[0].n);
			glVertexPointer(3, GL_FLOAT, 2*sizeof(vector3f), &m_vertices[0].v);
		}
	}
	void OpDrawElements(int numIndices) {
		if (curOp.type != OP_DRAW_ELEMENTS) {
			if (curOp.type) m_ops.push_back(curOp);
			curOp.type = OP_DRAW_ELEMENTS;
			curOp.elems.start = m_indices.size();
			curOp.elems.count = 0;
			curOp.elems.elemMin = 1<<30;
			curOp.elems.elemMax = 0;
		}
		curOp.elems.count += numIndices;
	}
	void PushCurOp() {
		m_ops.push_back(curOp);
	}
	void PushIdx(Uint16 v) {
		curOp.elems.elemMin = MIN(v, curOp.elems.elemMin);
		curOp.elems.elemMax = MAX(v, curOp.elems.elemMax);
		m_indices.push_back(v);
	}

	enum OpType { OP_NONE, OP_DRAW_ELEMENTS, OP_DRAW_BILLBOARDS, OP_SET_MATERIAL, OP_ZBIAS,
			OP_CALL_MODEL };

	struct Op {
		enum OpType type;
		union {
			struct { int start, count, elemMin, elemMax; } elems;
			struct { int material_idx; } col;
			struct { float amount; float pos[3]; float norm[3]; } zbias;
			struct { LmrModel *model; float transform[16]; float scale; } callmodel;
			struct { int start, count; GLuint tex; float size; float col[4]; } billboards;
		};
	};
	struct Vertex {
		Vertex() {}
		Vertex(const vector3f &v, const vector3f &n): v(v), n(n) {}
		vector3f v, n;
	};

	Op curOp;
	Uint16 curTriFlag;
	std::vector<Vertex> m_vertices;
	std::vector<Uint16> m_indices;
	std::vector<Uint16> m_triflags;
	std::vector<Op> m_ops;
	std::vector<ShipThruster::Thruster> m_thrusters;
	GLuint m_vertexBuffer;
	GLuint m_indexBuffer;
	LmrModel *m_model;
};

LmrModel::LmrModel(const char *model_name)
{
	m_name = model_name;
	m_boundingRadius = 1.0f;
	m_scale = 1.0f;

	char buf[256];
	snprintf(buf, sizeof(buf), "%s_info", model_name);
	lua_getglobal(sLua, buf);
	if (lua_istable(sLua, -1)) {
		m_numLods = 0;

		lua_getfield(sLua, -1, "bounding_radius");
		if (lua_isnumber(sLua, -1)) m_boundingRadius = luaL_checknumber(sLua, -1);
		else luaL_error(sLua, "model %s_info missing bounding_radius=", model_name);
		lua_pop(sLua, 1);

		lua_getfield(sLua, -1, "lod_pixels");
		if (lua_istable(sLua, -1)) {
			for(int i=1;; i++) {
				lua_pushinteger(sLua, i);
				lua_gettable(sLua, -2);
				bool is_num = lua_isnumber(sLua, -1);
				if (is_num) {
					m_lodPixelSize[i-1] = luaL_checknumber(sLua, -1);
					m_numLods++;
				}
				lua_pop(sLua, 1);
				if (!is_num) break;
				if (i > LMR_MAX_LOD) {
					luaL_error(sLua, "Too many LODs (maximum %d)", LMR_MAX_LOD);
				}
			}
		} else {
			m_numLods = 1;
			m_lodPixelSize[0] = 0;
		}
		lua_pop(sLua, 1);

		lua_getfield(sLua, -1, "materials");
		if (lua_istable(sLua, -1)) {
			for(int i=1;; i++) {
				lua_pushinteger(sLua, i);
				lua_gettable(sLua, -2);
				bool is_string = lua_isstring(sLua, -1);
				if (is_string) {
					const char *mat_name = luaL_checkstring(sLua, -1);
					m_materialLookup[mat_name] = m_materials.size();
					m_materials.push_back(LmrMaterial());
				}
				lua_pop(sLua, 1);
				if (!is_string) break;
			}
		}
		lua_pop(sLua, 1);

		lua_getfield(sLua, -1, "scale");
		if (lua_isnumber(sLua, -1)) {
			m_scale = lua_tonumber(sLua, -1);
		}
		lua_pop(sLua, 1);

		/* pop model_info table */
		lua_pop(sLua, 1);
	} else {
		luaL_error(sLua, "Could not find function %s_info()", model_name);
	}
	
	snprintf(buf, sizeof(buf), "%s_dynamic", model_name);
	lua_getglobal(sLua, buf);
	m_hasDynamicFunc = lua_isfunction(sLua, -1);
	lua_pop(sLua, 1);

	for (int i=0; i<m_numLods; i++) {
		m_staticGeometry[i] = new LmrGeomBuffer(this);
		m_dynamicGeometry[i] = new LmrGeomBuffer(this);
	}

	for (int i=0; i<m_numLods; i++) {
		m_staticGeometry[i]->PreBuild();
		s_curBuf = m_staticGeometry[i];
		// call model static building function
		lua_getfield(sLua, LUA_GLOBALSINDEX, (m_name+"_static").c_str());
		// lod as first argument
		lua_pushnumber(sLua, i+1);
		lua_call(sLua, 1, 0);
		s_curBuf = 0;
		m_staticGeometry[i]->PostBuild();
	}
}

LmrModel::~LmrModel()
{
	for (int i=0; i<m_numLods; i++) {
		delete m_staticGeometry[i];
		delete m_dynamicGeometry[i];
	}
}

//static std::map<std::string, LmrModel*> s_models;
void LmrGetModelsWithTag(const char *tag, std::vector<LmrModel*> &outModels)
{
	for (std::map<std::string, LmrModel*>::iterator i = s_models.begin();
			i != s_models.end(); ++i) {
		LmrModel *model = (*i).second;
		
		char buf[256];
		snprintf(buf, sizeof(buf), "%s_info", model->GetName());
		lua_getglobal(sLua, buf);
		lua_getfield(sLua, -1, "tags");
		if (lua_istable(sLua, -1)) {
			for(int i=1;; i++) {
				lua_pushinteger(sLua, i);
				lua_gettable(sLua, -2);
				if (lua_isstring(sLua, -1)) {
					const char *s = luaL_checkstring(sLua, -1);
					if (0 == strcmp(tag, s)) {
						outModels.push_back(model);
						lua_pop(sLua, 1);
						break;
					}
				} else if (lua_isnil(sLua, -1)) {
					lua_pop(sLua, 1);
					break;
				}
				lua_pop(sLua, 1);
			}
		}
		lua_pop(sLua, 2);
	}
}

float LmrModel::GetFloatAttribute(const char *attr_name) const
{
	char buf[256];
	snprintf(buf, sizeof(buf), "%s_info", m_name.c_str());
	lua_getglobal(sLua, buf);
	lua_getfield(sLua, -1, attr_name);
	float result = luaL_checknumber(sLua, -1);
	lua_pop(sLua, 2);
	return result;
}

int LmrModel::GetIntAttribute(const char *attr_name) const
{
	char buf[256];
	snprintf(buf, sizeof(buf), "%s_info", m_name.c_str());
	lua_getglobal(sLua, buf);
	lua_getfield(sLua, -1, attr_name);
	int result = luaL_checkint(sLua, -1);
	lua_pop(sLua, 2);
	return result;
}

void LmrModel::PushAttributeToStack(const char *attr_name) const
{
	char buf[256];
	snprintf(buf, sizeof(buf), "%s_info", m_name.c_str());
	lua_getglobal(sLua, buf);
	lua_getfield(sLua, -1, attr_name);
}

void LmrModel::Render(const matrix4x4f &trans, const LmrObjParams *params)
{
	RenderState rstate;
	rstate.subTransform = matrix4x4f::Identity();
	rstate.combinedScale = m_scale;
	Render(&rstate, vector3f(-trans[12], -trans[13], -trans[14]), trans, params);
}

void LmrModel::Render(const RenderState *rstate, const vector3f &cameraPos, const matrix4x4f &trans, const LmrObjParams *params)
{
	Render::UseProgram(s_normalShader);
	glPushMatrix();
	glMultMatrixf(&trans[0]);
	glScalef(m_scale, m_scale, m_scale);
	glEnable(GL_NORMALIZE);

	float pixrad = 0.5f * s_scrWidth * rstate->combinedScale * m_boundingRadius / cameraPos.Length();
	//printf("%s: %fpx\n", m_name.c_str(), pixrad);

	int lod = m_numLods-1;
	for (int i=lod-1; i>=0; i--) {
		if (pixrad < m_lodPixelSize[i]) lod = i;
	}
	//printf("%s: lod %d\n", m_name.c_str(), lod);

	Build(lod, params);

	m_staticGeometry[lod]->Render(rstate, cameraPos, params);
	if (m_hasDynamicFunc) {
		m_dynamicGeometry[lod]->Render(rstate, cameraPos, params);
	}
	s_curBuf = 0;

	glDisable(GL_NORMALIZE);
	glPopMatrix();
	Render::UseProgram(0);
}

void LmrModel::Build(int lod, const LmrObjParams *params)
{
	if (m_hasDynamicFunc) {
		m_dynamicGeometry[lod]->PreBuild();
		s_curBuf = m_dynamicGeometry[lod];
		s_curParams = params;
		// call model dynamic bits
		lua_getfield(sLua, LUA_GLOBALSINDEX, (m_name+"_dynamic").c_str());
		// lod as first argument
		lua_pushnumber(sLua, lod+1);
		lua_call(sLua, 1, 0);
		s_curBuf = 0;
		s_curParams = 0;
		m_dynamicGeometry[lod]->PostBuild();
	}
}

void LmrModel::GetCollMeshGeometry(LmrCollMesh *mesh, const matrix4x4f &transform, const LmrObjParams *params)
{
	// use lowest LOD
	Build(0, params);
	matrix4x4f m = transform * matrix4x4f::ScaleMatrix(m_scale);
	m_staticGeometry[0]->GetCollMeshGeometry(mesh, m, params);
	if (m_hasDynamicFunc) m_dynamicGeometry[0]->GetCollMeshGeometry(mesh, m, params);
}

LmrCollMesh::LmrCollMesh(LmrModel *m, const LmrObjParams *params)
{
	memset(this, 0, sizeof(LmrCollMesh));
	m_aabb.min = vector3f(FLT_MAX, FLT_MAX, FLT_MAX);
	m_aabb.max = vector3f(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	m->GetCollMeshGeometry(this, matrix4x4f::Identity(), params);
	m_radius = MAX(m_aabb.min.Length(), m_aabb.max.Length());
	geomTree = new GeomTree(nv, m_numTris, pVertex, pIndex, pFlag);
}

/** returns number of tris found (up to 'num') */
int LmrCollMesh::GetTrisWithGeomflag(unsigned int flags, int num, vector3d *outVtx) const
{
	int found = 0;
	for (int i=0; (i<m_numTris) && (found<num); i++) {
		if (pFlag[i] == flags) {
			*(outVtx++) = vector3d(&pVertex[3*pIndex[3*i]]);
			*(outVtx++) = vector3d(&pVertex[3*pIndex[3*i+1]]);
			*(outVtx++) = vector3d(&pVertex[3*pIndex[3*i+2]]);
			found++;
		}
	}
	return found;
}

LmrCollMesh::~LmrCollMesh()
{
	// nice. mixed allocation. for the love of realloc...
	delete geomTree;
	free(pVertex);
	free(pIndex);
	free(pFlag);
}

LmrModel *LmrLookupModelByName(const char *name) throw (LmrModelNotFoundException)
{
	std::map<std::string, LmrModel*>::iterator i = s_models.find(name);

	if (i == s_models.end()) {
		throw LmrModelNotFoundException();
	}
	return (*i).second;
}	

namespace ModelFuncs {
	static int call_model(lua_State *L)
	{
		const char *obj_name = luaL_checkstring(L, 1);
//	subobject(object_name, vector pos, vector xaxis, vector yaxis [, scale=float, onflag=])
		if (!obj_name) return 0;
		if (!obj_name[0]) return 0;
		LmrModel *m = s_models[obj_name];
		if (!m) {
			luaL_error(L, "call_model() to undefined model '%s'. Referenced model must be registered before calling model", obj_name);
		} else {
			const vector3f *pos = MyLuaVec::checkVec(L, 2);
			const vector3f *_xaxis = MyLuaVec::checkVec(L, 3);
			const vector3f *_yaxis = MyLuaVec::checkVec(L, 4);
			float scale = luaL_checknumber(L, 5);

			vector3f zaxis = vector3f::Cross(*_xaxis, *_yaxis).Normalized();
			vector3f xaxis = vector3f::Cross(*_yaxis, zaxis).Normalized();
			vector3f yaxis = vector3f::Cross(zaxis, xaxis);

			matrix4x4f trans = matrix4x4f::MakeInvRotMatrix(scale*xaxis, scale*yaxis, scale*zaxis);
			trans[12] = pos->x;
			trans[13] = pos->y;
			trans[14] = pos->z;

			s_curBuf->PushCallModel(m, trans, scale);
		}
		return 0;
	}

	static int extrusion(lua_State *L)
	{
		const vector3f *start = MyLuaVec::checkVec(L, 1);
		const vector3f *end = MyLuaVec::checkVec(L, 2);
		const vector3f *updir = MyLuaVec::checkVec(L, 3);
		const float radius = luaL_checknumber(L, 4);

#define EXTRUSION_MAX_VTX 32
		int steps = lua_gettop(L)-4;
		if (steps > EXTRUSION_MAX_VTX) {
			luaL_error(L, "extrusion() takes at most %d points", EXTRUSION_MAX_VTX);
		}
		vector3f evtx[EXTRUSION_MAX_VTX];

		for (int i=0; i<steps; i++) {
			evtx[i] = *MyLuaVec::checkVec(L, i+5);
		}

		const int vtxStart = s_curBuf->AllocVertices(6*steps);

		vector3f yax = *updir;
		vector3f xax, zax;
		zax = ((*end) - (*start)).Normalized();
		xax = vector3f::Cross(yax, zax);

		for (int i=0; i<steps; i++) {
			vector3f tv, norm;
			tv = xax * evtx[i].x;
			norm = yax * evtx[i].y;
			norm = norm + tv;

			vector3f p1 = norm * radius;
			s_curBuf->SetVertex(vtxStart + i, (*start) + p1, -zax);
			s_curBuf->SetVertex(vtxStart + i + steps, (*end) + p1, zax);
		}

		for (int i=0; i<steps-1; i++) {
			// top cap
			s_curBuf->PushTri(vtxStart, vtxStart+i+1, vtxStart+i);
			// bottom cap
			s_curBuf->PushTri(vtxStart+steps, vtxStart+steps+i, vtxStart+steps+i+1);
		}

		// sides
		for (int i=0; i<steps; i++) {
			const vector3f &v1 = s_curBuf->GetVertex(vtxStart + i);
			const vector3f &v2 = s_curBuf->GetVertex(vtxStart + (i + 1)%steps);
			const vector3f &v3 = s_curBuf->GetVertex(vtxStart + i + steps);
			const vector3f &v4 = s_curBuf->GetVertex(vtxStart + (i + 1)%steps + steps);
			const vector3f norm = vector3f::Cross(v2-v1, v3-v1).Normalized();

			const int idx = vtxStart + 2*steps + i*4;
			s_curBuf->SetVertex(idx, v1, norm);
			s_curBuf->SetVertex(idx+1, v2, norm);
			s_curBuf->SetVertex(idx+2, v3, norm);
			s_curBuf->SetVertex(idx+3, v4, norm);

			s_curBuf->PushTri(idx, idx+1, idx+3);
			s_curBuf->PushTri(idx, idx+3, idx+2);
		}

		return 0;
	}
	
	static vector3f eval_cubic_bezier_u(const vector3f p[4], float u)
	{
		vector3f out(0.0f);
		float Bu[4] = { (1.0f-u)*(1.0f-u)*(1.0f-u),
			3.0f*(1.0f-u)*(1.0f-u)*u,
			3.0f*(1.0f-u)*u*u,
			u*u*u };
		for (int i=0; i<4; i++) {
			out += p[i] * Bu[i];
		}
		return out;
	}

	static vector3f eval_quadric_bezier_u(const vector3f p[3], float u)
	{
		vector3f out(0.0f);
		float Bu[3] = { (1.0f-u)*(1.0f-u), 2.0f*u*(1.0f-u), u*u };
		for (int i=0; i<3; i++) {
			out += p[i] * Bu[i];
		}
		return out;
	}

	static int _flat(lua_State *L, bool xref)
	{
		const int divs = luaL_checkinteger(L, 1);
		const vector3f *normal = MyLuaVec::checkVec(L, 2);
		vector3f xrefnorm(0.0f);
		if (xref) xrefnorm = vector3f(-normal->x, normal->y, normal->z);
#define FLAT_MAX_SEG 32
		struct {
			const vector3f *v[3];
			int nv;
		} segvtx[FLAT_MAX_SEG];

		if (!lua_istable(L, 3)) {
			luaL_error(L, "argment 3 to flat() must be a table of line segments");
			return 0;
		}

		int argmax = lua_gettop(L);
		int seg = 0;
		int numPoints = 0;
		// iterate through table of line segments
		for (int n=3; n<=argmax; n++, seg++) {
			if (lua_istable(L, n)) {
				// this table is a line segment itself
				// 1 vtx = straight line
				// 2     = quadric bezier
				// 3     = cubic bezier
				int nv = 0;
				for (int i=1; i<4; i++) {
					lua_pushinteger(L, i);
					lua_gettable(L, n);
					if (lua_isnil(L, -1)) {
						lua_pop(L, 1);
						break;
					} else {
						segvtx[seg].v[nv++] = MyLuaVec::checkVec(L, -1);
						lua_pop(L, 1);
					}
				}
				segvtx[seg].nv = nv;

				if (!nv) {
					luaL_error(L, "number of points in a line segment must be 1-3 (straight, quadric, cubic)");
					return 0;
				} else if (nv == 1) {
					numPoints++;
				} else if (nv > 1) {
					numPoints += divs;
				}
			} else {
				luaL_error(L, "invalid crap in line segment list");
				return 0;
			}
		}

		const int vtxStart = s_curBuf->AllocVertices(xref ? 2*numPoints : numPoints);
		int vtxPos = vtxStart;

		const vector3f *prevSegEnd = segvtx[seg-1].v[ segvtx[seg-1].nv-1 ];
		// evaluate segments
		int maxSeg = seg;
		for (seg=0; seg<maxSeg; seg++) {
			if (segvtx[seg].nv == 1) {
				if (xref) {
					vector3f p = *segvtx[seg].v[0]; p.x = -p.x;
					s_curBuf->SetVertex(vtxPos + numPoints, p, xrefnorm);
				}
				s_curBuf->SetVertex(vtxPos++, *segvtx[seg].v[0], *normal);
				prevSegEnd = segvtx[seg].v[0];
			} else if (segvtx[seg].nv == 2) {
				vector3f _p[3];
				_p[0] = *prevSegEnd;
				_p[1] = *segvtx[seg].v[0];
				_p[2] = *segvtx[seg].v[1];
				float inc = 1.0f / (float)divs;
				float u = inc;
				for (int i=1; i<=divs; i++, u+=inc) {
					vector3f p = eval_quadric_bezier_u(_p, u);
					s_curBuf->SetVertex(vtxPos, p, *normal);
					if (xref) {
						p.x = -p.x;
						s_curBuf->SetVertex(vtxPos+numPoints, p, xrefnorm);
					}
					vtxPos++;
				}
				prevSegEnd = segvtx[seg].v[1];
			} else if (segvtx[seg].nv == 3) {
				vector3f _p[4];
				_p[0] = *prevSegEnd;
				_p[1] = *segvtx[seg].v[0];
				_p[2] = *segvtx[seg].v[1];
				_p[3] = *segvtx[seg].v[2];
				float inc = 1.0f / (float)divs;
				float u = inc;
				for (int i=1; i<=divs; i++, u+=inc) {
					vector3f p = eval_cubic_bezier_u(_p, u);
					s_curBuf->SetVertex(vtxPos, p, *normal);
					if (xref) {
						p.x = -p.x;
						s_curBuf->SetVertex(vtxPos+numPoints, p, xrefnorm);
					}
					vtxPos++;
				}
				prevSegEnd = segvtx[seg].v[2];
			}
		}

		for (int i=1; i<numPoints-1; i++) {
			s_curBuf->PushTri(vtxStart, vtxStart+i, vtxStart+i+1);
			if (xref) {
				s_curBuf->PushTri(vtxStart+numPoints, vtxStart+numPoints+1+i, vtxStart+numPoints+i);
			}
		}
		return 0;
	}
	
	static int flat(lua_State *L) { return _flat(L, false); }
	static int xref_flat(lua_State *L) { return _flat(L, true); }

	static vector3f eval_quadric_bezier_triangle(const vector3f p[6], float s, float t, float u)
	{
		vector3f out(0.0f);
		const float coef[6] = { s*s, 2.0f*s*t, t*t, 2.0f*s*u, 2.0f*t*u, u*u };
		for (int i=0; i<6; i++) {
			out += p[i] * coef[i];
		}
		return out;
	}

	static vector3f eval_cubic_bezier_triangle(const vector3f p[10], float s, float t, float u)
	{
		vector3f out(0.0f);
		const float coef[10] = { s*s*s, 3.0f*s*s*t, 3.0f*s*t*t, t*t*t, 3.0f*s*s*u, 6.0f*s*t*u, 3.0f*t*t*u, 3.0f*s*u*u, 3.0f*t*u*u, u*u*u };
		for (int i=0; i<10; i++) {
			out += p[i] * coef[i];
		}
		return out;
	}
	template <int BEZIER_ORDER>
	static void _bezier_triangle(lua_State *L, bool xref)
	{
		vector3f pts[10];
		const int divs = luaL_checkint(L, 1) + 1;
		assert(divs > 0);
		if (BEZIER_ORDER == 2) {
			for (int i=0; i<6; i++) {
				pts[i] = *MyLuaVec::checkVec(L, i+2);
			}
		} else if (BEZIER_ORDER == 3) {
			for (int i=0; i<10; i++) {
				pts[i] = *MyLuaVec::checkVec(L, i+2);
			}
		}

		const int numVertsInPatch = divs*(1+divs)/2;
		const int vtxStart = s_curBuf->AllocVertices(numVertsInPatch * (xref ? 2 : 1));
		int vtxPos = vtxStart;

		float inc = 1.0f / (float)(divs-1);
		float s,t,u;
		s = t = u = 0;
		for (int i=0; i<divs; i++, u += inc) {
			float pos = 0;
			float inc2 = 1.0f / (float)(divs-1-i);
			for (int j=i; j<divs; j++, pos += inc2) {
				s = (1.0f-u)*(1.0f-pos);
				t = (1.0f-u)*pos;
				vector3f p, pu, pv;
				if (BEZIER_ORDER == 2) {
					p = eval_quadric_bezier_triangle(pts, s, t, u);
					pu = eval_quadric_bezier_triangle(pts, s+0.1f*inc, t-0.1f*inc, u);
					pv = eval_quadric_bezier_triangle(pts, s-0.05f*inc, t-0.05f*inc, u+0.1f*inc);
				} else if (BEZIER_ORDER == 3) {
					p = eval_cubic_bezier_triangle(pts, s, t, u);
					pu = eval_cubic_bezier_triangle(pts, s+0.1f*inc, t-0.1f*inc, u);
					pv = eval_cubic_bezier_triangle(pts, s-0.05f*inc, t-0.05f*inc, u+0.1f*inc);
				}
				vector3f norm = vector3f::Cross(pu-p, pv-p).Normalized();
				s_curBuf->SetVertex(vtxPos, p, norm);

				if (xref) {
					norm.x = -norm.x;
					p.x = -p.x;
					s_curBuf->SetVertex(vtxPos + numVertsInPatch, p, norm);
				}
				vtxPos++;
			}
		}
		//assert((vtxPos - vtxStart) == numVertsInPatch);

		vtxPos = vtxStart;
		for (int y=0; y<divs-1; y++) {
			const int adv = divs-y;
			s_curBuf->PushTri(vtxPos, vtxPos+adv, vtxPos+1);
			for (int x=1; x<adv-1; x++) {
				s_curBuf->PushTri(vtxPos+x, vtxPos+x+adv-1, vtxPos+x+adv);
				s_curBuf->PushTri(vtxPos+x, vtxPos+x+adv, vtxPos+x+1);
			}
			if (xref) {
				const int refVtxPos = vtxPos + numVertsInPatch;
				s_curBuf->PushTri(refVtxPos, refVtxPos+1, refVtxPos+adv);
				for (int x=1; x<adv-1; x++) {
					s_curBuf->PushTri(refVtxPos+x, refVtxPos+x+adv, refVtxPos+x+adv-1);
					s_curBuf->PushTri(refVtxPos+x, refVtxPos+x+1, refVtxPos+x+adv);
				}
			}
			vtxPos += adv;
		}
	}

	static int cubic_bezier_triangle(lua_State *L) { _bezier_triangle<3>(L, false); return 0; }
	static int xref_cubic_bezier_triangle(lua_State *L) { _bezier_triangle<3>(L, true); return 0; }
	static int quadric_bezier_triangle(lua_State *L) { _bezier_triangle<2>(L, false); return 0; }
	static int xref_quadric_bezier_triangle(lua_State *L) { _bezier_triangle<2>(L, true); return 0; }


	static vector3f eval_quadric_bezier_u_v(const vector3f p[9], float u, float v)
	{
		vector3f out(0.0f);
		float Bu[3] = { (1.0f-u)*(1.0f-u), 2.0f*u*(1.0f-u), u*u };
		float Bv[3] = { (1.0f-v)*(1.0f-v), 2.0f*v*(1.0f-v), v*v };
		for (int i=0; i<3; i++) {
			for (int j=0; j<3; j++) {
				out += p[i+3*j] * Bu[i] * Bv[j];
			}
		}
		return out;
	}

	static void _quadric_bezier_quad(lua_State *L, bool xref)
	{
		vector3f pts[9];
		const int divs_u = luaL_checkint(L, 1);
		const int divs_v = luaL_checkint(L, 2);
		for (int i=0; i<9; i++) {
			pts[i] = *MyLuaVec::checkVec(L, i+3);
		}

		const int numVertsInPatch = (divs_u+1)*(divs_v+1);
		const int vtxStart = s_curBuf->AllocVertices(numVertsInPatch * (xref ? 2 : 1));

		float inc_u = 1.0f / (float)divs_u;
		float inc_v = 1.0f / (float)divs_v;
		float u,v;
		u = v = 0;
		for (int i=0; i<=divs_u; i++, u += inc_u) {
			v = 0;
			for (int j=0; j<=divs_v; j++, v += inc_v) {
				vector3f p = eval_quadric_bezier_u_v(pts, u, v);
				// this is a very inefficient way of
				// calculating normals...
				vector3f pu = eval_quadric_bezier_u_v(pts, u+0.01f*inc_u, v);
				vector3f pv = eval_quadric_bezier_u_v(pts, u, v+0.01f*inc_v);
				vector3f norm = vector3f::Cross(pu-p, pv-p).Normalized();

				s_curBuf->SetVertex(vtxStart + i*(divs_v+1) + j, p, norm);
				if (xref) {
					p.x = -p.x;
					norm.x = -norm.x;
					s_curBuf->SetVertex(vtxStart + numVertsInPatch + i*(divs_v+1) + j, p, norm);
				}
			}
		}

		for (int i=0; i<divs_u; i++) {
			int baseVtx = vtxStart + i*(divs_v+1);
			for (int j=0; j<divs_v; j++) {
				s_curBuf->PushTri(baseVtx+j, baseVtx+j+1+(divs_v+1), baseVtx+j+1);
				s_curBuf->PushTri(baseVtx+j, baseVtx+j+(divs_v+1), baseVtx+j+1+(divs_v+1));
			}
		}
		if (xref) for (int i=0; i<divs_u; i++) {
			int baseVtx = vtxStart + numVertsInPatch + i*(divs_v+1);
			for (int j=0; j<divs_v; j++) {
				s_curBuf->PushTri(baseVtx+j, baseVtx+j+1, baseVtx+j+1+(divs_v+1));
				s_curBuf->PushTri(baseVtx+j, baseVtx+j+1+(divs_v+1), baseVtx+j+(divs_v+1));
			}
		}
	}
	
	static int quadric_bezier_quad(lua_State *L) { _quadric_bezier_quad(L, false); return 0; }
	static int xref_quadric_bezier_quad(lua_State *L) { _quadric_bezier_quad(L, true); return 0; }

	static vector3f eval_cubic_bezier_u_v(const vector3f p[16], float u, float v)
	{
		vector3f out(0.0f);
		float Bu[4] = { (1.0f-u)*(1.0f-u)*(1.0f-u),
			3.0f*(1.0f-u)*(1.0f-u)*u,
			3.0f*(1.0f-u)*u*u,
			u*u*u };
		float Bv[4] = { (1.0f-v)*(1.0f-v)*(1.0f-v),
			3.0f*(1.0f-v)*(1.0f-v)*v,
			3.0f*(1.0f-v)*v*v,
			v*v*v };
		for (int i=0; i<4; i++) {
			for (int j=0; j<4; j++) {
				out += p[i+4*j] * Bu[i] * Bv[j];
			}
		}
		return out;
	}

	static void _cubic_bezier_quad(lua_State *L, bool xref)
	{
		vector3f pts[16];
		const int divs_v = luaL_checkint(L, 1);
		const int divs_u = luaL_checkint(L, 2);
		if (lua_istable(L, 3)) {
			for (int i=0; i<16; i++) {
				lua_pushinteger(L, i+1);
				lua_gettable(L, 3);
				pts[i] = *MyLuaVec::checkVec(L, -1);
				lua_pop(L, 1);
			}
		} else {
			for (int i=0; i<16; i++) {
				pts[i] = *MyLuaVec::checkVec(L, i+3);
			}
		}

		const int numVertsInPatch = (divs_v+1)*(divs_u+1);
		const int vtxStart = s_curBuf->AllocVertices(numVertsInPatch * (xref ? 2 : 1));


		float inc_v = 1.0f / (float)divs_v;
		float inc_u = 1.0f / (float)divs_u;
		float u,v;
		u = v = 0;
		for (int i=0; i<=divs_u; i++, u += inc_u) {
			v = 0;
			for (int j=0; j<=divs_v; j++, v += inc_v) {
				vector3f p = eval_cubic_bezier_u_v(pts, u, v);
				// this is a very inefficient way of
				// calculating normals...
				vector3f pu = eval_cubic_bezier_u_v(pts, u+0.01f*inc_u, v);
				vector3f pv = eval_cubic_bezier_u_v(pts, u, v+0.01f*inc_v);
				vector3f norm = vector3f::Cross(pu-p, pv-p).Normalized();

				s_curBuf->SetVertex(vtxStart + i*(divs_v+1) + j, p, norm);
				if (xref) {
					p.x = -p.x;
					norm.x = -norm.x;
					s_curBuf->SetVertex(vtxStart + numVertsInPatch + i*(divs_v+1) + j, p, norm);
				}
			}
		}

		for (int i=0; i<divs_u; i++) {
			int baseVtx = vtxStart + i*(divs_v+1);
			for (int j=0; j<divs_v; j++) {
				s_curBuf->PushTri(baseVtx+j, baseVtx+j+1+(divs_v+1), baseVtx+j+1);
				s_curBuf->PushTri(baseVtx+j, baseVtx+j+(divs_v+1), baseVtx+j+1+(divs_v+1));
			}
		}
		if (xref) for (int i=0; i<divs_u; i++) {
			int baseVtx = vtxStart + numVertsInPatch + i*(divs_v+1);
			for (int j=0; j<divs_v; j++) {
				s_curBuf->PushTri(baseVtx+j, baseVtx+j+1, baseVtx+j+1+(divs_v+1));
				s_curBuf->PushTri(baseVtx+j, baseVtx+j+1+(divs_v+1), baseVtx+j+(divs_v+1));
			}
		}
	}

	static int cubic_bezier_quad(lua_State *L) { _cubic_bezier_quad(L, false); return 0; }
	static int xref_cubic_bezier_quad(lua_State *L) { _cubic_bezier_quad(L, true); return 0; }

	static int set_material(lua_State *L)
	{
		const char *mat_name = luaL_checkstring(L, 1);
		float mat[11];
		if (lua_istable(L, 2)) {
			// material as table of 11 values
			for (int i=0; i<11; i++) {
				lua_pushinteger(L, i+1);
				lua_gettable(L, 2);
				mat[i] = luaL_checknumber(L, -1);
				lua_pop(L, 1);
			}
		} else {
			for (int i=0; i<11; i++) {
				mat[i] = lua_tonumber(L, i+2);
			}
		}
		s_curBuf->SetMaterial(mat_name, mat);
		return 0;
	}

	static int use_material(lua_State *L)
	{
		const char *mat_name = luaL_checkstring(L, 1);
		s_curBuf->PushUseMaterial(mat_name);
		return 0;
	}

		static matrix4x4f _textTrans;
		static vector3f _textNorm;
		static void _text_index_callback(int num, Uint16 *vals) {
			const int base = s_curBuf->GetVerticesPos();
			for (int i=0; i<num; i+=3) {
				s_curBuf->PushTri(vals[i]+base, vals[i+1]+base, vals[i+2]+base);
			}
		}
		static void _text_vertex_callback(int num, float offsetX, float offsetY, float *vals) {
			for (int i=0; i<num*3; i+=3) {
				vector3f p = vector3f(offsetX+vals[i], offsetY+vals[i+1], vals[i+2]);
				p = _textTrans * p;
				s_curBuf->PushVertex(p, _textNorm);
			}
		}
	static int text(lua_State *L)
	{
		const char *str = luaL_checkstring(L, 1);
		vector3f pos = *MyLuaVec::checkVec(L, 2);
		vector3f *norm = MyLuaVec::checkVec(L, 3);
		vector3f *textdir = MyLuaVec::checkVec(L, 4);
		float scale = luaL_checknumber(L, 5);
		vector3f yaxis = vector3f::Cross(*norm, *textdir).Normalized();
		vector3f zaxis = vector3f::Cross(*textdir, yaxis).Normalized();
		vector3f xaxis = vector3f::Cross(yaxis, zaxis);
		_textTrans = matrix4x4f::MakeInvRotMatrix(scale*xaxis, scale*yaxis, scale*zaxis);
		
		bool do_center = false;
		if (lua_istable(L, 6)) {
			lua_pushstring(L, "center");
			lua_gettable(L, 6);
			do_center = lua_toboolean(L, -1);
			lua_pop(L, 1);

			lua_pushstring(L, "xoffset");
			lua_gettable(L, 6);
			float xoff = lua_tonumber(L, -1);
			lua_pop(L, 1);
			
			lua_pushstring(L, "yoffset");
			lua_gettable(L, 6);
			float yoff = lua_tonumber(L, -1);
			lua_pop(L, 1);
			pos += _textTrans * vector3f(xoff, yoff, 0);
		}
	
		if (do_center) {
			float xoff = 0, yoff = 0;
			s_font->MeasureString(str, xoff, yoff);
			pos -= 0.5f * (_textTrans * vector3f(xoff, yoff, 0));
		}
		_textTrans[12] = pos.x;
		_textTrans[13] = pos.y;
		_textTrans[14] = pos.z;
		_textNorm = *norm;
		s_font->GetStringGeometry(str, &_text_index_callback, &_text_vertex_callback);
//text("some literal string", vector pos, vector norm, vector textdir, [xoff=, yoff=, scale=, onflag=])
		return 0;
	}
	
	static int geomflag(lua_State *L)
	{
		Uint16 flag = luaL_checkint(L, 1);
		s_curBuf->SetGeomFlag(flag);
		return 0;
	}

	static int zbias(lua_State *L)
	{
		float amount = luaL_checknumber(L, 1);
		if (amount == 0) {
			s_curBuf->PushZBias(0, vector3f(0.0), vector3f(0.0));
		} else {
			vector3f *pos = MyLuaVec::checkVec(L, 2);
			vector3f *norm = MyLuaVec::checkVec(L, 3);
			s_curBuf->PushZBias(amount, *pos, *norm);
		}
		return 0;
	}

	static void _circle(int steps, const vector3f &center, const vector3f &normal, const vector3f &updir, float radius) {
		const int vtxStart = s_curBuf->AllocVertices(steps);

		const vector3f axis1 = updir.Normalized();
		const vector3f axis2 = vector3f::Cross(updir, normal).Normalized();

		const float inc = 2.0f*M_PI / (float)steps;
		float ang = 0.5f*inc;
		radius /= cosf(ang);
		for (int i=0; i<steps; i++, ang += inc) {
			vector3f p = center + radius * (sin(ang)*axis1 + cos(ang)*axis2);
			s_curBuf->SetVertex(vtxStart+i, p, normal);
		}

		for (int i=2; i<steps; i++) {
			// top cap
			s_curBuf->PushTri(vtxStart, vtxStart+i-1, vtxStart+i);
		}
	}

	static int circle(lua_State *L)
	{
		int steps = luaL_checkint(L, 1);
		const vector3f *center = MyLuaVec::checkVec(L, 2);
		const vector3f *normal = MyLuaVec::checkVec(L, 3);
		const vector3f *updir = MyLuaVec::checkVec(L, 4);
		float radius = lua_tonumber(L, 5);
		_circle(steps, *center, *normal, *updir, radius);
		return 0;
	}

	static int xref_circle(lua_State *L)
	{
		int steps = luaL_checkint(L, 1);
		vector3f center = *MyLuaVec::checkVec(L, 2);
		vector3f normal = *MyLuaVec::checkVec(L, 3);
		vector3f updir = *MyLuaVec::checkVec(L, 4);
		float radius = lua_tonumber(L, 5);
		_circle(steps, center, normal, updir, radius);
		center.x = -center.x;
		normal.x = -normal.x;
		updir.x = -updir.x;
		_circle(steps, center, normal, updir, radius);
		return 0;
	}

	static void _tube(int steps, const vector3f &start, const vector3f &end, const vector3f &updir, float inner_radius, float outer_radius) {
		const int vtxStart = s_curBuf->AllocVertices(8*steps);

		const vector3f dir = (end-start).Normalized();
		const vector3f axis1 = updir.Normalized();
		const vector3f axis2 = vector3f::Cross(updir, dir).Normalized();

		const float inc = 2.0f*M_PI / (float)steps;
		float ang = 0.5*inc;
		const float radmod = 1.0f/cosf(ang);
		inner_radius *= radmod;
		outer_radius *= radmod;
		for (int i=0; i<steps; i++, ang += inc) {
			vector3f p = (sin(ang)*axis1 + cos(ang)*axis2);
			vector3f p_inner = inner_radius * p;
			vector3f p_outer = outer_radius * p;

			s_curBuf->SetVertex(vtxStart+i, start+p_outer, p);
			s_curBuf->SetVertex(vtxStart+i+steps, end+p_outer, p);
			s_curBuf->SetVertex(vtxStart+i+2*steps, start+p_inner, -p);
			s_curBuf->SetVertex(vtxStart+i+3*steps, end+p_inner, -p);

			s_curBuf->SetVertex(vtxStart+i+4*steps, start+p_outer, -dir);
			s_curBuf->SetVertex(vtxStart+i+5*steps, end+p_outer, dir);
			s_curBuf->SetVertex(vtxStart+i+6*steps, start+p_inner, -dir);
			s_curBuf->SetVertex(vtxStart+i+7*steps, end+p_inner, dir);
		}

		for (int i=0; i<steps-1; i++) {
			s_curBuf->PushTri(vtxStart+i, vtxStart+i+1, vtxStart+i+steps);
			s_curBuf->PushTri(vtxStart+i+1, vtxStart+i+steps+1, vtxStart+i+steps);
			s_curBuf->PushTri(vtxStart+i+2*steps, vtxStart+i+steps+2*steps, vtxStart+i+1+2*steps);
			s_curBuf->PushTri(vtxStart+i+1+2*steps, vtxStart+i+steps+2*steps, vtxStart+i+steps+1+2*steps);
		}
		s_curBuf->PushTri(vtxStart+steps-1, vtxStart, vtxStart+2*steps-1);
		s_curBuf->PushTri(vtxStart, vtxStart+steps, vtxStart+2*steps-1);
		
		s_curBuf->PushTri(vtxStart+3*steps-1, vtxStart+4*steps-1, vtxStart+2*steps);
		s_curBuf->PushTri(vtxStart+2*steps, vtxStart+4*steps-1, vtxStart+3*steps);

		for (int i=0; i<steps-1; i++) {
			// 'start' end
			s_curBuf->PushTri(vtxStart+4*steps+i, vtxStart+6*steps+i, vtxStart+4*steps+i+1);
			
			s_curBuf->PushTri(vtxStart+4*steps+i+1, vtxStart+6*steps+i, vtxStart+6*steps+i+1);
			// 'end' end *cough*
			s_curBuf->PushTri(vtxStart+5*steps+i, vtxStart+5*steps+i+1, vtxStart+7*steps+i);
			
			s_curBuf->PushTri(vtxStart+5*steps+i+1, vtxStart+7*steps+i+1, vtxStart+7*steps+i);
		}
		// 'start' end
		s_curBuf->PushTri(vtxStart+5*steps-1, vtxStart+7*steps-1, vtxStart+4*steps);
		s_curBuf->PushTri(vtxStart+4*steps, vtxStart+7*steps-1, vtxStart+6*steps);
		// 'end' end
		s_curBuf->PushTri(vtxStart+6*steps-1, vtxStart+5*steps, vtxStart+8*steps-1);
		s_curBuf->PushTri(vtxStart+5*steps, vtxStart+7*steps, vtxStart+8*steps-1);
	}
	
	static int tube(lua_State *L)
	{
		int steps = luaL_checkint(L, 1);
		const vector3f *start = MyLuaVec::checkVec(L, 2);
		const vector3f *end = MyLuaVec::checkVec(L, 3);
		const vector3f *updir = MyLuaVec::checkVec(L, 4);
		float inner_radius = lua_tonumber(L, 5);
		float outer_radius = lua_tonumber(L, 6);
		_tube(steps, *start, *end, *updir, inner_radius, outer_radius);
		return 0;
	}

	static int xref_tube(lua_State *L)
	{
		int steps = luaL_checkint(L, 1);
		vector3f start = *MyLuaVec::checkVec(L, 2);
		vector3f end = *MyLuaVec::checkVec(L, 3);
		vector3f updir = *MyLuaVec::checkVec(L, 4);
		float inner_radius = lua_tonumber(L, 5);
		float outer_radius = lua_tonumber(L, 6);
		_tube(steps, start, end, updir, inner_radius, outer_radius);
		start.x = -start.x;
		end.x = -end.x;
		updir.x = -updir.x;
		_tube(steps, start, end, updir, inner_radius, outer_radius);
		return 0;
	}

	static void _tapered_cylinder(int steps, const vector3f &start, const vector3f &end, const vector3f &updir, float radius1, float radius2) {
		const int vtxStart = s_curBuf->AllocVertices(4*steps);

		const vector3f dir = (end-start).Normalized();
		const vector3f axis1 = updir.Normalized();
		const vector3f axis2 = vector3f::Cross(updir, dir).Normalized();

		const float inc = 2.0f*M_PI / (float)steps;
		float ang = 0.5*inc;
		radius1 /= cosf(ang);
		radius2 /= cosf(ang);
		for (int i=0; i<steps; i++, ang += inc) {
			vector3f p1 = radius1 * (sin(ang)*axis1 + cos(ang)*axis2);
			vector3f p2 = radius2 * (sin(ang)*axis1 + cos(ang)*axis2);
			vector3f n = vector3d::Cross(vector3f::Cross((end+p2)-(start+p1), p1), (end+p2)-(start+p1))
				.Normalized();

			s_curBuf->SetVertex(vtxStart+i, start+p1, n);
			s_curBuf->SetVertex(vtxStart+i+steps, end+p2, n);
			s_curBuf->SetVertex(vtxStart+i+2*steps, start+p1, -dir);
			s_curBuf->SetVertex(vtxStart+i+3*steps, end+p2, dir);
		}

		for (int i=0; i<steps-1; i++) {
			s_curBuf->PushTri(vtxStart+i, vtxStart+i+1, vtxStart+i+steps);
			s_curBuf->PushTri(vtxStart+i+1, vtxStart+i+steps+1, vtxStart+i+steps);
		}
		s_curBuf->PushTri(vtxStart+steps-1, vtxStart, vtxStart+2*steps-1);
		s_curBuf->PushTri(vtxStart, vtxStart+steps, vtxStart+2*steps-1);

		for (int i=2; i<steps; i++) {
			// bottom cap
			s_curBuf->PushTri(vtxStart+2*steps, vtxStart+2*steps+i, vtxStart+2*steps+i-1);
			// top cap
			s_curBuf->PushTri(vtxStart+3*steps, vtxStart+3*steps+i-1, vtxStart+3*steps+i);
		}
	}
	
	static int tapered_cylinder(lua_State *L)
	{
		int steps = luaL_checkint(L, 1);
		const vector3f *start = MyLuaVec::checkVec(L, 2);
		const vector3f *end = MyLuaVec::checkVec(L, 3);
		const vector3f *updir = MyLuaVec::checkVec(L, 4);
		float radius1 = lua_tonumber(L, 5);
		float radius2 = lua_tonumber(L, 6);
		_tapered_cylinder(steps, *start, *end, *updir, radius1, radius2);
		return 0;
	}

	static int xref_tapered_cylinder(lua_State *L)
	{
		/* could optimise for x-reflection but fuck it */
		int steps = luaL_checkint(L, 1);
		vector3f start = *MyLuaVec::checkVec(L, 2);
		vector3f end = *MyLuaVec::checkVec(L, 3);
		vector3f updir = *MyLuaVec::checkVec(L, 4);
		float radius1 = lua_tonumber(L, 5);
		float radius2 = lua_tonumber(L, 6);
		_tapered_cylinder(steps, start, end, updir, radius1, radius2);
		start.x = -start.x;
		end.x = -end.x;
		updir.x = -updir.x;
		_tapered_cylinder(steps, start, end, updir, radius1, radius2);
		return 0;
	}

	static void _cylinder(int steps, const vector3f &start, const vector3f &end, const vector3f &updir, float radius) {
		const int vtxStart = s_curBuf->AllocVertices(4*steps);

		const vector3f dir = (end-start).Normalized();
		const vector3f axis1 = updir.Normalized();
		const vector3f axis2 = vector3f::Cross(updir, dir).Normalized();

		const float inc = 2.0f*M_PI / (float)steps;
		float ang = 0.5*inc;
		radius /= cosf(ang);
		for (int i=0; i<steps; i++, ang += inc) {
			vector3f p = radius * (sin(ang)*axis1 + cos(ang)*axis2);
			vector3f n = p.Normalized();

			s_curBuf->SetVertex(vtxStart+i, start+p, n);
			s_curBuf->SetVertex(vtxStart+i+steps, end+p, n);
			s_curBuf->SetVertex(vtxStart+i+2*steps, start+p, -dir);
			s_curBuf->SetVertex(vtxStart+i+3*steps, end+p, dir);
		}

		for (int i=0; i<steps-1; i++) {
			s_curBuf->PushTri(vtxStart+i, vtxStart+i+1, vtxStart+i+steps);
			s_curBuf->PushTri(vtxStart+i+1, vtxStart+i+steps+1, vtxStart+i+steps);
		}
		s_curBuf->PushTri(vtxStart+steps-1, vtxStart, vtxStart+2*steps-1);
		s_curBuf->PushTri(vtxStart, vtxStart+steps, vtxStart+2*steps-1);

		for (int i=2; i<steps; i++) {
			// bottom cap
			s_curBuf->PushTri(vtxStart+2*steps, vtxStart+2*steps+i, vtxStart+2*steps+i-1);
			// top cap
			s_curBuf->PushTri(vtxStart+3*steps, vtxStart+3*steps+i-1, vtxStart+3*steps+i);
		}
	}
	
	static int cylinder(lua_State *L)
	{
		int steps = luaL_checkint(L, 1);
		const vector3f *start = MyLuaVec::checkVec(L, 2);
		const vector3f *end = MyLuaVec::checkVec(L, 3);
		const vector3f *updir = MyLuaVec::checkVec(L, 4);
		float radius = lua_tonumber(L, 5);
		_cylinder(steps, *start, *end, *updir, radius);
		return 0;
	}

	static int xref_cylinder(lua_State *L)
	{
		/* could optimise for x-reflection but fuck it */
		int steps = luaL_checkint(L, 1);
		vector3f start = *MyLuaVec::checkVec(L, 2);
		vector3f end = *MyLuaVec::checkVec(L, 3);
		vector3f updir = *MyLuaVec::checkVec(L, 4);
		float radius = lua_tonumber(L, 5);
		_cylinder(steps, start, end, updir, radius);
		start.x = -start.x;
		end.x = -end.x;
		updir.x = -updir.x;
		_cylinder(steps, start, end, updir, radius);
		return 0;
	}

	static void _ring(int steps, const vector3f &start, const vector3f &end, const vector3f &updir, float radius) {

		const vector3f dir = (end-start).Normalized();
		const vector3f axis1 = updir.Normalized();
		const vector3f axis2 = vector3f::Cross(updir, dir).Normalized();

		const int vtxStart = s_curBuf->AllocVertices(2*steps);

		const float inc = 2.0f*M_PI / (float)steps;
		float ang = 0.5*inc;
		radius /= cosf(ang);
		for (int i=0; i<steps; i++, ang += inc) {
			vector3f p = radius * (sin(ang)*axis1 + cos(ang)*axis2);
			vector3f n = p.Normalized();

			s_curBuf->SetVertex(vtxStart+i, start+p, n);
			s_curBuf->SetVertex(vtxStart+i+steps, end+p, n);
		}

		for (int i=0; i<steps-1; i++) {
			s_curBuf->PushTri(vtxStart+i, vtxStart+i+1, vtxStart+i+steps);
			s_curBuf->PushTri(vtxStart+i+1, vtxStart+i+steps+1, vtxStart+i+steps);
		}
		s_curBuf->PushTri(vtxStart+steps-1, vtxStart, vtxStart+2*steps-1);
		s_curBuf->PushTri(vtxStart, vtxStart+steps, vtxStart+2*steps-1);
	}

	/* Cylinder with no top or bottom caps */
	static int ring(lua_State *L)
	{
		int steps = luaL_checkint(L, 1);
		const vector3f *start = MyLuaVec::checkVec(L, 2);
		const vector3f *end = MyLuaVec::checkVec(L, 3);
		const vector3f *updir = MyLuaVec::checkVec(L, 4);
		float radius = lua_tonumber(L, 5);
		_ring(steps, *start, *end, *updir, radius);
		return 0;
	}

	static int xref_ring(lua_State *L)
	{
		int steps = luaL_checkint(L, 1);
		vector3f start = *MyLuaVec::checkVec(L, 2);
		vector3f end = *MyLuaVec::checkVec(L, 3);
		vector3f updir = *MyLuaVec::checkVec(L, 4);
		float radius = lua_tonumber(L, 5);
		_ring(steps, start, end, updir, radius);
		start.x = -start.x;
		end.x = -end.x;
		updir.x = -updir.x;
		_ring(steps, start, end, updir, radius);
		return 0;
	}

	static int invisible_tri(lua_State *L)
	{
		const vector3f *v1 = MyLuaVec::checkVec(L, 1);
		const vector3f *v2 = MyLuaVec::checkVec(L, 2);
		const vector3f *v3 = MyLuaVec::checkVec(L, 3);
		
		vector3f n = vector3f::Cross((*v1)-(*v2), (*v1)-(*v3)).Normalized();
		int i1 = s_curBuf->PushVertex(*v1, n);
		int i2 = s_curBuf->PushVertex(*v2, n);
		int i3 = s_curBuf->PushVertex(*v3, n);
		s_curBuf->PushInvisibleTri(i1, i2, i3);
		return 0;
	}

	static int tri(lua_State *L)
	{
		const vector3f *v1 = MyLuaVec::checkVec(L, 1);
		const vector3f *v2 = MyLuaVec::checkVec(L, 2);
		const vector3f *v3 = MyLuaVec::checkVec(L, 3);
		
		vector3f n = vector3f::Cross((*v1)-(*v2), (*v1)-(*v3)).Normalized();
		int i1 = s_curBuf->PushVertex(*v1, n);
		int i2 = s_curBuf->PushVertex(*v2, n);
		int i3 = s_curBuf->PushVertex(*v3, n);
		s_curBuf->PushTri(i1, i2, i3);
		return 0;
	}
	
	static int xref_tri(lua_State *L)
	{
		vector3f v1 = *MyLuaVec::checkVec(L, 1);
		vector3f v2 = *MyLuaVec::checkVec(L, 2);
		vector3f v3 = *MyLuaVec::checkVec(L, 3);
		
		vector3f n = vector3f::Cross((v1)-(v2), (v1)-(v3)).Normalized();
		int i1 = s_curBuf->PushVertex(v1, n);
		int i2 = s_curBuf->PushVertex(v2, n);
		int i3 = s_curBuf->PushVertex(v3, n);
		s_curBuf->PushTri(i1, i2, i3);
		v1.x = -v1.x; v2.x = -v2.x; v3.x = -v3.x; n.x = -n.x;
		i1 = s_curBuf->PushVertex(v1, n);
		i2 = s_curBuf->PushVertex(v2, n);
		i3 = s_curBuf->PushVertex(v3, n);
		s_curBuf->PushTri(i1, i3, i2);
		return 0;
	}
	
	static int quad(lua_State *L)
	{
		const vector3f *v1 = MyLuaVec::checkVec(L, 1);
		const vector3f *v2 = MyLuaVec::checkVec(L, 2);
		const vector3f *v3 = MyLuaVec::checkVec(L, 3);
		const vector3f *v4 = MyLuaVec::checkVec(L, 4);
		
		vector3f n = vector3f::Cross((*v1)-(*v2), (*v1)-(*v3)).Normalized();
		int i1 = s_curBuf->PushVertex(*v1, n);
		int i2 = s_curBuf->PushVertex(*v2, n);
		int i3 = s_curBuf->PushVertex(*v3, n);
		int i4 = s_curBuf->PushVertex(*v4, n);
		s_curBuf->PushTri(i1, i2, i3);
		s_curBuf->PushTri(i1, i3, i4);
		return 0;
	}
	
	static int xref_quad(lua_State *L)
	{
		vector3f v1 = *MyLuaVec::checkVec(L, 1);
		vector3f v2 = *MyLuaVec::checkVec(L, 2);
		vector3f v3 = *MyLuaVec::checkVec(L, 3);
		vector3f v4 = *MyLuaVec::checkVec(L, 4);
		
		vector3f n = vector3f::Cross((v1)-(v2), (v1)-(v3)).Normalized();
		int i1 = s_curBuf->PushVertex(v1, n);
		int i2 = s_curBuf->PushVertex(v2, n);
		int i3 = s_curBuf->PushVertex(v3, n);
		int i4 = s_curBuf->PushVertex(v4, n);
		s_curBuf->PushTri(i1, i2, i3);
		s_curBuf->PushTri(i1, i3, i4);
		v1.x = -v1.x; v2.x = -v2.x; v3.x = -v3.x; v4.x = -v4.x; n.x = -n.x;
		i1 = s_curBuf->PushVertex(v1, n);
		i2 = s_curBuf->PushVertex(v2, n);
		i3 = s_curBuf->PushVertex(v3, n);
		i4 = s_curBuf->PushVertex(v4, n);
		s_curBuf->PushTri(i1, i3, i2);
		s_curBuf->PushTri(i1, i4, i3);
		return 0;
	}

	static int thruster(lua_State *L)
	{
		const vector3f *pos = MyLuaVec::checkVec(L, 1);
		const vector3f *dir = MyLuaVec::checkVec(L, 2);
		const float power = luaL_checknumber(L, 3);
		bool linear_only = false;
		if (lua_isboolean(L, 4)) {
			linear_only = lua_toboolean(L, 4);
		}
		s_curBuf->PushThruster(*pos, *dir, power, linear_only);
		return 0;
	}

	static int xref_thruster(lua_State *L)
	{
		vector3f pos = *MyLuaVec::checkVec(L, 1);
		const vector3f *dir = MyLuaVec::checkVec(L, 2);
		const float power = luaL_checknumber(L, 3);
		bool linear_only = false;
		if (lua_isboolean(L, 4)) {
			linear_only = lua_toboolean(L, 4);
		}
		s_curBuf->PushThruster(pos, *dir, power, linear_only);
		pos.x = -pos.x;
		s_curBuf->PushThruster(pos, *dir, power, linear_only);
		return 0;
	}

	static int get_arg(lua_State *L)
	{
		assert(s_curParams != 0);
		int i = luaL_checkint(L, 1);
		lua_pushnumber(L, s_curParams->argFloats[i]);
		return 1;
	}
	
	static int get_arg_string(lua_State *L)
	{
		assert(s_curParams != 0);
		int i = luaL_checkint(L, 1);
		if (s_curParams->argStrings[i])
			lua_pushstring(L, s_curParams->argStrings[i]);
		else
			lua_pushstring(L, "");
		return 1;
	}
	
	static int get_arg_material(lua_State *L)
	{
		assert(s_curParams != 0);
		int n = luaL_checkint(L, 1);
		lua_createtable (L, 11, 0);

		const LmrMaterial &mat = s_curParams->pMat[n];

		for (int i=0; i<4; i++) {
			lua_pushinteger(L, 1+i);
			lua_pushnumber(L, mat.diffuse[i]);
			lua_settable(L, -3);
		}
		for (int i=0; i<3; i++) {
			lua_pushinteger(L, 5+i);
			lua_pushnumber(L, mat.specular[i]);
			lua_settable(L, -3);
		}
		lua_pushinteger(L, 8);
		lua_pushnumber(L, mat.shininess);
		lua_settable(L, -3);
		for (int i=0; i<3; i++) {
			lua_pushinteger(L, 9+i);
			lua_pushnumber(L, mat.emissive[i]);
			lua_settable(L, -3);
		}
		return 1;
	}

	static int billboard(lua_State *L)
	{
//		billboard('texname', size, color, { p1, p2, p3, p4 })
		const char *texname = luaL_checkstring(L, 1);
		const float size = luaL_checknumber(L, 2);
		const vector3f color = *MyLuaVec::checkVec(L, 3);
		std::vector<vector3f> points;

		if (lua_istable(L, 4)) {
			for (int i=1;; i++) {
				lua_pushinteger(L, i);
				lua_gettable(L, 4);
				if (lua_isnil(L, -1)) {
					lua_pop(L, 1);
					break;
				}
				points.push_back(*MyLuaVec::checkVec(L, -1));
				lua_pop(L, 1);
			}
		}
		s_curBuf->PushBillboards(texname, size, color, points.size(), &points[0]);
		return 0;
	}

} /* namespace ModelFuncs */

namespace ObjLoader {
	static int load_obj_file(lua_State *L)
	{
		const char *obj_name = luaL_checkstring(L, 1);
		int numArgs = lua_gettop(L);
		matrix4x4f *transform = 0;
		if (numArgs > 1) {
			transform = MyLuaMatrix::checkMat4x4(L, 2);
		}
	
		char buf[1024];
		snprintf(buf, sizeof(buf), "data/models/%s", obj_name);
		FILE *f = fopen(buf, "r");
		if (!f) {
			printf("Could not open %s\n", buf);
			exit(0);
		}
		std::vector<vector3f> vertices;
		std::vector<vector3f> normals;
		// maps obj file vtx_idx,norm_idx to a single GeomBuffer vertex index
		std::map< std::pair<int,int>, int> vtxmap;

		for (int line_no=1; fgets(buf, sizeof(buf), f); line_no++) {
			if ((buf[0] == 'v') && buf[1] == ' ') {
				// vertex
				vector3f v;
				PiVerify(3 == sscanf(buf, "v %f %f %f", &v.x, &v.y, &v.z));
				if (transform) v = (*transform) * v;
				vertices.push_back(v);
			}
			else if ((buf[0] == 'v') && (buf[1] == 'n') && (buf[2] == ' ')) {
				// normal
				vector3f v;
				PiVerify(3 == sscanf(buf, "vn %f %f %f", &v.x, &v.y, &v.z));
				if (transform) v = ((*transform) * v).Normalized();
				normals.push_back(v);
			}
			else if ((buf[0] == 'f') && (buf[1] == ' ')) {
				// how many vertices in this face?
				const int MAX_VTX_FACE = 4;
				char *bit[MAX_VTX_FACE];
				char *pos = &buf[2];
				int numBits = 0;
				while ((pos[0] != '\0') && (numBits < MAX_VTX_FACE)) {
					bit[numBits++] = pos;
					while(pos[0] && !isspace(pos[0])) pos++;
					if (isspace(pos[0])) pos++;
				}
				printf("%d bits\n", numBits);

				int realVtxIdx[MAX_VTX_FACE];
				for (int i=0; i<numBits; i++) {
					int vi, ni, ti;
					if (3 != sscanf(bit[i], "%d/%d/%d", &vi, &ti, &ni)) {
						if (2 != sscanf(bit[i], "%d//%d", &vi, &ni)) {
							printf("Obj file has no normals or is otherwise too weird at line %d\n", line_no);
							exit(0);
						}
					}
					// indices start from 1 in obj file
					vi--; ni--;
					// SHARE THE PAIN!
					std::map< std::pair<int, int>, int>::iterator it = vtxmap.find(std::pair<int,int>(vi, ni));
					if (it == vtxmap.end()) {
						// insert the horrible thing
						int vtxStart = s_curBuf->AllocVertices(1);
						s_curBuf->SetVertex(vtxStart, vertices[vi], normals[ni]);
						vtxmap[std::pair<int,int>(vi, ni)] = vtxStart;
						realVtxIdx[i] = vtxStart;
					} else {
						realVtxIdx[i] = (*it).second;
					}
				}

				if (numBits == 3) {
					s_curBuf->PushTri(realVtxIdx[0], realVtxIdx[1], realVtxIdx[2]);
				} else if (numBits == 4) {
					s_curBuf->PushTri(realVtxIdx[0], realVtxIdx[1], realVtxIdx[2]);
					s_curBuf->PushTri(realVtxIdx[0], realVtxIdx[2], realVtxIdx[3]);
				} else {
					printf("Obj file must have faces with 3 or 4 vertices (quads or triangles)\n");
					exit(0);
				}
			}
			else if (strncmp("usemtl ", buf, 7) == 0) {
				char mat_name[128];
				if (1 == sscanf(buf, "usemtl %s", mat_name)) {
					s_curBuf->PushUseMaterial(mat_name);
				} else {
					printf("Obj file has no normals or is otherwise too weird at line %d\n", line_no);
					exit(0);
				}
			}
		}
		return 0;
	}
};

namespace UtilFuncs {
	
	int noise(lua_State *L) {
		vector3f v;
		if (lua_isnumber(L, 1)) {
			v.x = lua_tonumber(L, 1);
			v.y = lua_tonumber(L, 2);
			v.z = lua_tonumber(L, 3);
		} else {
			v = *MyLuaVec::checkVec(L, 1);
		}
		lua_pushnumber(L, noise(v));
		return 1;
	}
} /* UtilFuncs */

static int define_model(lua_State *L)
{
	int n = lua_gettop(L);
	if (n != 2) {
		luaL_error(L, "define_model takes 2 arguments");
		return 0;
	}

	const char *model_name = luaL_checkstring(L, 1);

	if (!lua_istable(L, 2)) {
		luaL_error(L, "define_model 2nd argument must be a table");
		return 0;
	}

	// table is passed containing info, static and dynamic, which are
	// functions. we then stuff them into the globals, named
	// modelName_info, _static, etc.
	char buf[256];

	lua_pushstring(L, "info");
	lua_gettable(L, 2);
	snprintf(buf, sizeof(buf), "%s_info", model_name);
	lua_setglobal(L, buf);

	lua_pushstring(L, "static");
	lua_gettable(L, 2);
	snprintf(buf, sizeof(buf), "%s_static", model_name);
	lua_setglobal(L, buf);

	lua_pushstring(L, "dynamic");
	lua_gettable(L, 2);
	snprintf(buf, sizeof(buf), "%s_dynamic", model_name);
	lua_setglobal(L, buf);
	
	s_models[model_name] = new LmrModel(model_name);
	return 0;
}

void LmrModelCompilerInit()
{
	s_normalShader = new Render::Shader("model");
	PiVerify(s_font = new FontFace ("font.ttf"));

	lua_State *L = lua_open();
	sLua = L;
	luaL_openlibs(L);


	lua_pushinteger(L, 1234);
	lua_setglobal(L, "x");


	MyLuaVec::Vec_register(L);
	lua_pop(L, 1); // why again?
	MyLuaMatrix::Mat4x4_register(L);
	lua_pop(L, 1); // why again?
	// shorthand for Vec.new(x,y,z)
	lua_register(L, "v", MyLuaVec::Vec_new);
	lua_register(L, "norm", MyLuaVec::Vec_newNormalized);
	lua_register(L, "define_model", define_model);
	lua_register(L, "set_material", ModelFuncs::set_material);
	lua_register(L, "use_material", ModelFuncs::use_material);
	lua_register(L, "get_arg_material", ModelFuncs::get_arg_material);
	
	lua_register(L, "invisible_tri", ModelFuncs::invisible_tri);
	lua_register(L, "tri", ModelFuncs::tri);
	lua_register(L, "xref_tri", ModelFuncs::xref_tri);
	lua_register(L, "quad", ModelFuncs::quad);
	lua_register(L, "xref_quad", ModelFuncs::xref_quad);
	lua_register(L, "cylinder", ModelFuncs::cylinder);
	lua_register(L, "xref_cylinder", ModelFuncs::xref_cylinder);
	lua_register(L, "tapered_cylinder", ModelFuncs::tapered_cylinder);
	lua_register(L, "xref_tapered_cylinder", ModelFuncs::xref_tapered_cylinder);
	lua_register(L, "tube", ModelFuncs::tube);
	lua_register(L, "xref_tube", ModelFuncs::xref_tube);
	lua_register(L, "ring", ModelFuncs::ring);
	lua_register(L, "xref_ring", ModelFuncs::xref_ring);
	lua_register(L, "circle", ModelFuncs::circle);
	lua_register(L, "xref_circle", ModelFuncs::xref_circle);
	lua_register(L, "text", ModelFuncs::text);
	lua_register(L, "quadric_bezier_quad", ModelFuncs::quadric_bezier_quad);
	lua_register(L, "xref_quadric_bezier_quad", ModelFuncs::xref_quadric_bezier_quad);
	lua_register(L, "cubic_bezier_quad", ModelFuncs::cubic_bezier_quad);
	lua_register(L, "xref_cubic_bezier_quad", ModelFuncs::xref_cubic_bezier_quad);
	lua_register(L, "cubic_bezier_tri", ModelFuncs::cubic_bezier_triangle);
	lua_register(L, "xref_cubic_bezier_tri", ModelFuncs::xref_cubic_bezier_triangle);
	lua_register(L, "quadric_bezier_tri", ModelFuncs::quadric_bezier_triangle);
	lua_register(L, "xref_quadric_bezier_tri", ModelFuncs::xref_quadric_bezier_triangle);
	lua_register(L, "extrusion", ModelFuncs::extrusion);
	lua_register(L, "thruster", ModelFuncs::thruster);
	lua_register(L, "xref_thruster", ModelFuncs::xref_thruster);
	lua_register(L, "get_arg", ModelFuncs::get_arg);
	lua_register(L, "get_arg_string", ModelFuncs::get_arg_string);
	lua_register(L, "flat", ModelFuncs::flat);
	lua_register(L, "xref_flat", ModelFuncs::xref_flat);
	lua_register(L, "billboard", ModelFuncs::billboard);
	lua_register(L, "geomflag", ModelFuncs::geomflag);
	lua_register(L, "zbias", ModelFuncs::zbias);
	lua_register(L, "call_model", ModelFuncs::call_model);
	lua_register(L, "noise", UtilFuncs::noise);
	lua_register(L, "load_obj", ObjLoader::load_obj_file);

	s_buildDynamic = false;
	if (luaL_dofile(L, "data/models/models.lua")) {
		printf("%s\n", lua_tostring(L, -1));
	}
	s_buildDynamic = true;
	//lua_close(L);
}
