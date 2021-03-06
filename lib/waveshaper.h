/*
	waveshaper.h

	Antoine Lussier - R�mi Coulombe
	21 f�vrier 2020
	MUS3329

*/

/*
http://www.musicdsp.org/en/latest/Effects/114-waveshaper-simple-description.html

abs --> return absolute value of int. always positive. flip into positive value.

*/

#ifndef __WSHAPER_H__
#define __WSHAPER_H__

struct waveshaper {
	float sr;
	float lastout;
	float coeff;
	float drive;
};

struct waveshaper* waveshaper_init(float drive, float sr);

void waveshaper_delete(struct waveshaper* data);

float waveshaper_process(struct waveshaper* data, float input, float drive);

//void waveshaper_set_drive(struct waveshaper* data, float drive);



#endif