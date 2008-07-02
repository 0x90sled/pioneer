#ifndef __SBRE_H__
#define __SBRE_H__
#include "jjtypes.h"
#include "jjvector.h"


enum animsrc
{
	ASRC_GEAR = 0,	
	ASRC_SECFRAC,
	ASRC_MINFRAC,
	ASRC_HOURFRAC,
	ASRC_DAYFRAC,
};

enum animflag
{
	AFLAG_GEAR = 0, 
};

struct ObjParams
{
	float pAnim[10];
	uint8 pFlag[10];

	float linthrust[3];		// 1.0 to -1.0
	float angthrust[3];		// 1.0 to -1.0

	struct {
		float pDiff[3];
		float pSpec[3];
		float pEmis[3];
		float shiny;
	} pColor [3];

	char pText[3][256];
};

struct CollMesh
{
	int nv, ni;
	float *pVertex;
	int *pIndex;
	int *pFlag;

	int maxv, maxi;
	int cflag;
};

// if you don't call SetDepthRange, z bias and LOD will fail
// sd is screen depth in pixels, dn and df are like glDepthRange params
void sbreSetDepthRange (float sd, float dn, float df);
void sbreSetViewport (int w, int h, float d, float zn, float zf, float dn, float df);
void sbreSetDirLight (float *pColor, float *pDir);
void sbreSetWireframe (int val);
void sbreRenderModel (Vector *pPos, Matrix *pOrient, int model, ObjParams *pParam,
	float s=1.0f, Vector *pCompos=0);

// will preserve and realloc pointers in pCMesh
// maxv/maxi should match allocated sizes
void sbreGenCollMesh (CollMesh *pCMesh, int model, ObjParams *pParam, float s=1.0f);

#endif /* __SBRE_H__ */
