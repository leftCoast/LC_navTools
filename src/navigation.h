#ifndef navigation_h
#define navigation_h

/*
Let's discuss the tasks of a navigator.

• Determine current location (Fix) By..
• Dead reckoning from a known point.
• Compass bearings from known navigation marks.
• GPS Fix.
• Following a known depth contour line.
• Shooting the stars.. Noon sighting, star sighting and plotting.

Once you know where you are, you need to calculate where to head, to get to where you
should be going.

• Great circle rout calculation.
• Course to mark
• Distance to mark

• Keeping an up to date position plot.
• Accurate time & date.


Fixes.. They should have time calculated and how..

Logbook? Time, date, Fixes, how calculated, destination mark? COG calculation? Water depth?

Fixes may need to have optional notes? If by bearings from known points. What points? What
values? If dead reconning speed, delta time.. 

*/
#include <RTClib.h>
#include <globalPos.h>		// Brings in global locations.
#include <GPSReader.h>
#include <handlers.h>

class navigation {

public:
			navigation(netObj* inNMEA2K);
virtual	~navigation(void);
		
			
			double			trueToMag(double bearingVal);
			double			magToTrue(double bearingVal);
			bool				setupHandlers(void);
			DateTime			getDateTime(bool local);
			
			bool				haveFix(void);
			globalPos		getFix(void);
			float				getBoatSpeed(void);
			float				getSOG(void);
			float				getDepth(bool fathoms);
			bool				moving(float minSpeed=0.75);
			float				getCOG(bool magnetic);
			bool				getSpew(void);
			void				setSpew(bool onOff);

			void				setMark(globalPos* inPos);
			void				clearMark(void);
			bool				haveMark(void);
			unsigned long	getMarkMS(void);
			float				getDistance(void);
			float				getBearing(bool magnetic);

			barometerObj*	barometer;
			waterSpeedObj*	knotMeter;
			waterDepthObj*	depthSounder;
			
protected:
			bool				haveNMEA2K;
			netObj*			ourNMEA2K;
			globalPos		destMark;
			unsigned long	markMS;
			runningAvg*		GPSSpeedSmooth;
			float				avgGPSSpeed;
			
};

extern navigation*	ourNavigator;

#endif