#ifndef __SBRE_MODELS_H__
#define __SBRE_MODELS_H__
#include "sbre_int.h"


extern Model curvetest;

extern Model dishmodel, nosewheelmodel, nwunitmodel, mainwheelmodel, mwunitmodel;
extern Model wing1model, wing2model;
extern Model ship1model, ship2model, ship3model, ship4model, ship5model, ship6model;
extern Model station1model, starport1model, tombstonemodel, cargomodel;
extern Model metalFrameTowerModel;
extern Model building1, building2, house1;

// common subobject indices

const int SUB_NOSEWHEEL = 1;
const int SUB_NWUNIT = 2;
const int SUB_MAINWHEEL = 3;
const int SUB_MWUNIT = 4;
const int SUB_DISH = 5;

// other subobject indices

const int SUB_WING1 = 30;
const int SUB_WING2 = 31;

Model * const ppModel[SBRE_COMPILED_MODELS] =
{
	// 0, current test object
	&ship6model,
	// 1, common subobjects
	&nosewheelmodel,
	&nwunitmodel,
	&mainwheelmodel,
	&mwunitmodel,
	&dishmodel,
	0, 0, 0, 0,
	// 10
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	// 20
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	// 30, single-use subobjects
	&wing1model,
	&wing2model,
	0, 0, 0, 0, 0, 0, 0, 0,
	// 40
	&building1, &building2, &house1, 0, 0, 0, 0, 0, 0, 0,
	// 50
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	// 60, JJ ships
	&ship1model,
	&ship2model,
	&ship3model,
	&ship4model,
	&ship5model,
	&station1model,
	&ship6model, 0, 0, 0,
	// 70
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	// 80
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	// 90, other people's ships
	&starport1model,&tombstonemodel,&cargomodel,0,0,0,0,0,0,0,
	// 100, more sub-objects
	&metalFrameTowerModel

};


#endif // __SBRE_MODELS_H__