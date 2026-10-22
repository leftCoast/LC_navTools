#include <navigation.h>
#include <navPrefs.h>
#include <GPS_NMEA2K.h>
#include <debug.h>

	
navigation*		ourNavigator = NULL;



navigation::navigation(netObj* inNMEA2K) {
	
	markMS = 0;												// Zero means no mark.
	ourNMEA2K = inNMEA2K;								// Our net object we will use.
	ourGPS = new GPSReader;								// We own the GPS reader, set it up.
	ourGPS->begin();										// Give it a kick to start it.
	ourGPS->setSpew(false);								// Shut up spew!
	Serial1.begin(9600);									// Fire up the GPS's serial port for it.
	while(Serial1.available()) Serial1.read();	// Flush out the GPS Serial data before letting it read nonsense.
	barometer		= NULL;								// You know the rule about pointers starting off with NULL..
	knotMeter 		= NULL;								//
	depthSounder	= NULL;								//
	haveNMEA2K = setupHandlers();						// Where else will we get speed, depth and barometer?
}


navigation::~navigation(void) {

	if (barometer) delete(barometer);
	if (knotMeter) delete(knotMeter);
	if (depthSounder) delete(depthSounder);
}


double navigation::trueToMag(double bearingVal) {
	
	bearingVal = bearingVal - magCorrect;		// We'll subtract the correction.
	if (bearingVal>=360) {							// If it's bigger n 360 now..
		bearingVal = bearingVal - 360;			// Calculate the real magnetic bearing.
	} else if (bearingVal<0) {						// If it's less n zero now..
		bearingVal = bearingVal + 360;			// Calculate the real magnetic bearing.
	}														//
	return bearingVal;								// Here you go.
}																				

				
double navigation::magToTrue(double bearingVal) {
	
	bearingVal = bearingVal + magCorrect;		// We'll add the correction.
	if (bearingVal>=360) {							// If it's bigger n 360 now..
		bearingVal = bearingVal - 360;			// Calculate the real true bearing.
	} else if (bearingVal<0) {						// If it's less n zero now..
		bearingVal = bearingVal + 360;			// Calculate the real true bearing.
	}														//
	return bearingVal;								// Here you go.
}	

				
bool navigation::setupHandlers(void) {

	if (addGPSHandlers(ourNMEA2K)) {
		barometer = new barometerObj(ourNMEA2K);
		if (barometer) {
			ourNMEA2K->addMsgHandler(barometer);
			knotMeter = new waterSpeedObj(ourNMEA2K);
			if (knotMeter) {
				ourNMEA2K->addMsgHandler(knotMeter);
				depthSounder = new waterDepthObj(ourNMEA2K);
				if (depthSounder) {
					ourNMEA2K->addMsgHandler(depthSounder);
					return true;
				}
			}
		}
	}
	return false;
}


// DateTime is weird in that you can't just fill it after it's been created. So I create
// one here and pass it back. Either with the GPS date, OR year 2000. So the end user
// needs to check the year to see if what she got was a valid date.
DateTime navigation::getDateTime(bool local) {

	DateTime noStamp;
	
	if (haveFix()) {
		DateTime	timeStamp(ourGPS->year,ourGPS->month,ourGPS->day,ourGPS->hours,ourGPS->min,ourGPS->sec);
		if (local) {
			TimeSpan	deltaTime(0,timeOffset,0,0);
			timeStamp = timeStamp + deltaTime;
		}
		return timeStamp;
	}
	return noStamp;
}


bool navigation::haveFix(void) {

	if (ourGPS) {
		return ourGPS->valid;
	}
	return false;
}


globalPos navigation::getFix(void) {

	globalPos	aFix;
	
	if (haveFix()) {
		aFix.copyPos(&(ourGPS->latLon));
	}
	return aFix;
}


float navigation::getSpeed(void) {

	if (haveNMEA2K) {
		return knotMeter->knots;
	}
	return NAN;
}

	
float navigation::getDepth(bool fathoms) {

if (haveNMEA2K) {
		if (fathoms) {
			return depthSounder->feet/6.0;
		}
		return depthSounder->feet;
	}
	return NAN;
}


float navigation::getCOG(bool magnetic) {

	if (haveFix()) {
		if (magnetic) {
			return trueToMag(ourGPS->trueCourse);
		}
		return ourGPS->trueCourse;
	}
	return NAN;
}


bool navigation::getSpew(void) { return ourGPS->getSpew(); }


void navigation::setSpew(bool onOff) { ourGPS->setSpew(onOff); }


void navigation::setMark(globalPos* inPos) {

	markMS = 0;								// Just in case..
	if (inPos) {							// They gave us a non NULL address?
		destMark.copyPos(inPos);	// Copy the current mark.
		markMS = millis();				// Toss in our current millis value as local ID.
		if (!markMS) {						// What's the odds it was actually a zero? million to one.
			markMS = millis();			// No sweat, if it was we just read it again. Done.
		}										//
	}											//
}


void navigation::clearMark(void) { markMS = 0; }


bool navigation::haveMark(void) {

	if (markMS) {
		return destMark.valid();
	}
	return false;
}

	
unsigned long navigation::getMarkMS(void) { return markMS; }


float navigation::getDistance(void) {

	globalPos	aFix;
	
	if (haveFix()&&haveMark()) {
		aFix = getFix();
		return aFix.distanceTo(&destMark);
	}
	return NAN;
}


float navigation::getBearing(bool magnetic) {

	globalPos	aFix;
	double		bearing;
	
	if (haveFix()&&haveMark()) {
		aFix = getFix();
		bearing = aFix.trueBearingTo(&destMark);
		if (magnetic) {
			return trueToMag(bearing);
		}
		return bearing;
	}
	return NAN;
}
