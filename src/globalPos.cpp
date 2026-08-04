#include <globalPos.h>
#include <strTools.h>
#include <EEPROM.h>
#include <mapper.h>
#include <strTools.h>

#include <debug.h>

//#define RADIUS_EARTH_KNOTS  3443.98		// Calculated this from www map.
//#define RADIUS_EARTH_KNOTS  3440			// Saw this on google earth.
#define RADIUS_EARTH_KNOTS  3443.92			// Google's unit calculator gives this.

gPosPack nullPos = { false,0,0,north,false,0,0,west };

bool	checkLatDeg(int degrees) { return (degrees>=0 && degrees<90); }

bool	checkLonDeg(int degrees) { return (degrees>=0 && degrees<180); }

bool	checkMin(double minutes) { return (minutes>=0 && minutes<60); }

double rad2deg(double angleRad) { return angleRad*180/M_PI; }

double deg2rad(double angleDeg) { return angleDeg*M_PI/180.0; }

double hav(double theta) { return ((1-cos(theta))/2.0); }		// Haversine function

mapper		degMinMapper(0,1,0,60);
mapper 		secMinMapper(0,60,0,1);
posParser	ourPosParser;

	
// **********************************************
// ****************   posParser  ****************
// **********************************************


posParser::posParser(void) 
	:lilParser() {
	
	addCmd(latCmd,"LAT");
	addCmd(lonCmd,"LON");
}
	
	
posParser::~posParser(void) {  }


// Just stuffs this string in ignoring the parsing result. Does NOT pass in the '\0' at
// the end of the string.
void posParser::parseStr(const char* inStr) {

	int	i;
	
	if (inStr) {
		i = 0;
		while(inStr[i]) {
			addChar(inStr[i]);
			i++;
		}
	}	
}


// Pass in lat and lon strings and this should return a packed position.
gPosPack	posParser::parsePos(const char* inLatPos,const char* inLonPos) {
	
	ourPos.latValid = false;
	ourPos.lonValid = false;
	if (inLatPos && inLonPos) {						// Sanity non NULL..
		parseStr("LAT ");
		parseStr(inLatPos);
		if (addChar('\n')==latCmd) {
			parseLat();
		}
		parseStr("LON ");
		parseStr(inLonPos);
		if (addChar('\n')==lonCmd) {
			parseLon();
		}
	}
	return nullPos; 
}


void posParser::cleanParam(char* inParam) {

	int	i;
	
	if (inParam) {
		upCase(inParam);
		i = 0;
		while(inParam[i]!='\0') {
			if (inParam[i]>='A'&&inParam[i]<='Z') i++;
			else if (inParam[i]>='0'&&inParam[i]<='9') i++;
			else if (inParam[i]=='-'||inParam[i]=='.') i++;
			else delChar(inParam,i);
		}
	}
}


// We are being told that this should be a latitude string.
void posParser::parseLat(void) {
	
	int			degAsInt;
	double		degAsDbl;
	int			minAsInt;
	double		minAsDbl;
	double		secAsDbl;
	quad			ourQuad;
	char*			firstParam;
	char*			secondParam;
	char*			thirdParam;
	char*			fourthParam;
	char*			quadStr;
	char*			degStr;
	char*			minStr;
	char*			secStr;
	bool			success;
	
	firstParam	= NULL;																	// Set anything we'll allocate to NULL.
	secondParam	= NULL;																	//
	thirdParam	= NULL;																	//
	fourthParam	= NULL;																	//
	success = false;																		// Default to no success.
	switch(numParams()) {																// Let's see how many prams we got.
		case 1	:																			// We got one param. Signed double.
			heapStr(&firstParam,getNextParam());									// Grab the param.
			cleanParam(firstParam);														// Clean out gunk..
			degAsDbl = atof(firstParam);												// Grab the value.
			if (degAsDbl<=90 && degAsDbl>=-90) {									// If it passes sanity check.
				ourQuad = north;															// Let's say it's north.
				if (degAsDbl<0) {															// If negative.. 
					ourQuad = south;														// it's south.
					degAsDbl = degAsDbl * -1;											// Do the abs() thing.
				}																				//
				degAsInt = trunc(degAsDbl);											// Save off the degree int.
				degAsDbl = degAsDbl - degAsInt;										// Sub off the degree int.
				minAsDbl = degMinMapper.map(degAsDbl);								// Map the remainder to minutes.
				success = true;															// We have been a success!
			}																					//
		break;																				//
		case 2	:																			// We got two params. Pos double & quad. 
			heapStr(&firstParam,getNextParam());									// Grab first value.
			cleanParam(firstParam);														// Clean up to make things simpler.
			heapStr(&secondParam,getNextParam());									// Grab second.
			cleanParam(secondParam);													// Scrub scrub!
			if (firstParam[0]=='N'||firstParam[0]=='S') {						// If the first param is quad..
				quadStr = firstParam;													// Point quadStr at it.
				degStr = secondParam;													// POint degStr at second.
			} else if (secondParam[0]=='N'||secondParam[0]=='S') {			// Or.. Second param has the quad.
				quadStr = secondParam;													// Point quadStr at second param.
				degStr = firstParam;														// First param must be degree string.
			} else {																			// Else?
				break;																		// There is no else, we give up here.
			}																					//
			degAsDbl = atof(degStr);													// We have one float value for degrees
			if (degAsDbl<=90 && degAsDbl>=0) {										// If sanity check passes..
				degAsInt = trunc(degAsDbl);											// Save off the degree int.
				degAsDbl = degAsDbl - degAsInt;										// Sub off the degree int.
				minAsDbl = degMinMapper.map(degAsDbl);							 	// Map the remainder to minutes.
				ourQuad = north;															// Assume north..
				if (quadStr[0]=='S') {													// Unless it's an 'S'..
					ourQuad = south;														// Then, hey, it's south.
				}																				//
				ourPos.latDeg	= degAsInt;												// Fill in our bits of the output.
				ourPos.latMin	= minAsDbl;												//
				ourPos.latQuad	= ourQuad;												//
				success = true;															// We have been a success!
			}																					//
		break;																				//
		case 3	:																			// Four prams, int deg, int min, double sec & quad
			heapStr(&firstParam,getNextParam());									// Grab and clean the four params.
			cleanParam(firstParam);														//
			heapStr(&secondParam,getNextParam());									//
			cleanParam(secondParam);													//
			heapStr(&thirdParam,getNextParam());									//
			cleanParam(thirdParam);														//
			if (firstParam[0]=='N'||firstParam[0]=='S') {						// If quad is first..
				quadStr = firstParam;													// Point quadStr at it.
				degStr = secondParam;													// Deg str will be second.
				minStr = thirdParam;
			} else if (fourthParam[0]=='N'||fourthParam[0]=='S') {			// Else quad is last, only other choice we allow.
				quadStr = thirdParam;													// Quad third param
				degStr = firstParam;														// deg will be first.
				minStr = secondParam;													// Min will be second.
			} else {																			// else?
				break;																		// Can't find quad, bail!
			}																					//
			degAsInt = atoi(degStr);													// Grab degrees as an int.
			if (degAsInt<=90 && degAsInt>=0) {										// If it passes sanity check.
				minAsDbl = atof(minStr);												// Grab minutes as a double.
				if (minAsDbl<=60 && minAsDbl>=0) {									// If minutes passes sanity.
					ourQuad = north;														// Assume north..
					if (quadStr[0]=='S') {												// Unless it's an 'S'..
						ourQuad = south;													// Then, hey, it's south.
					}																			//
					ourPos.latDeg	= degAsInt;											// Fill in our bits of the output.
					ourPos.latMin	= minAsDbl;											//
					ourPos.latQuad	= ourQuad;											//
					success = true;														// We have been a success!
				}																				//
			}																					//
		break;																				//
		case 4	:																			// 
			heapStr(&firstParam,getNextParam());									// Grab and clean the three params.
			cleanParam(firstParam);														//
			heapStr(&secondParam,getNextParam());									//
			cleanParam(secondParam);													//
			heapStr(&thirdParam,getNextParam());									//
			cleanParam(thirdParam);														//
			heapStr(&fourthParam,getNextParam());									//
			cleanParam(fourthParam);													//
			if (firstParam[0]=='N'||firstParam[0]=='S') {						// If quad is first..
				quadStr = firstParam;													// Point quadStr at it.
				degStr = secondParam;													// Deg str will be second.
				minStr = thirdParam;														// Min str will be third.
				secStr = fourthParam;													// Sec str will be fourth.
			} else if (fourthParam[0]=='N'||fourthParam[0]=='S') {			// Else quad is last, only other choice we allow.
				quadStr = fourthParam;													// Quad third param
				degStr = firstParam;														// deg will be first.
				minStr = secondParam;													// Min will be second.
				secStr = thirdParam;														// Sec str will be third.
			} else {																			// Quad is not on either end?
				break;																		// We're done here.
			}																					//
			degAsInt = atoi(degStr);													// Grab degrees as an int.
			if (degAsInt<=90 && degAsInt>=0) {										// If it passes sanity check.
				minAsInt = atoi(minStr);												// Grab minutes as a double.
				if (minAsInt<60 && minAsInt>=0) {									// If minutes passes sanity.
					secAsDbl = atof(secStr);											// Grab seconds.
					if (secAsDbl<60 && secAsDbl>=0) {								// If seconds passes sanity.
						ourQuad = north;													// Assume north..
						if (quadStr[0]=='S') {											// Unless it's an 'S'..
							ourQuad = south;												// Then, hey, it's south.
						}																		//
						minAsDbl = minAsInt + secMinMapper.map(secAsDbl);		// Convert our min & sec to double min.
						ourPos.latDeg	= degAsInt;										// Fill in our bits of the output.
						ourPos.latMin	= minAsDbl;										//
						ourPos.latQuad	= ourQuad;										//
						success = true;													// We have been a success!
					}																			//
				}																				//
			}																					//
		break;																				// Time to go.
	}																							//
	freeStr(&firstParam);																// Recycle the RAM we used.
	freeStr(&secondParam);																//
	freeStr(&thirdParam);																//
	freeStr(&fourthParam);																//
	ourPos.latValid = success;															// Note our success.
}


// We are being told that this should be a longitude string.
void posParser::parseLon(void) {

	int			degAsInt;
	double		degAsDbl;
	int			minAsInt;
	double		minAsDbl;
	double		secAsDbl;
	quad			ourQuad;
	char*			firstParam;
	char*			secondParam;
	char*			thirdParam;
	char*			fourthParam;
	char*			quadStr;
	char*			degStr;
	char*			minStr;
	char*			secStr;
	bool			success;
	
	firstParam	= NULL;																	// Set anything we'll allocate to NULL.
	secondParam	= NULL;																	//
	thirdParam	= NULL;																	//
	fourthParam	= NULL;																	//
	success = false;																		// Well, we ain't been a success yet.
	switch(numParams()) {																// Choose the select by num parameters.
		case 1	:																			// We got one param. Signed double.
			heapStr(&firstParam,getNextParam());									// Grab the param.
			cleanParam(firstParam);														// Clean out gunk..
			degAsDbl = atof(firstParam);												// Grab the value.
			if (degAsDbl<180 && degAsDbl>-180) {									// If it passes sanity check.
				ourQuad = east;															// Let's say it's north.
				if (degAsDbl<0) {															// If negative.. 
					ourQuad = west;														// it's south.
					degAsDbl = degAsDbl * -1;											// Do the abs() thing.
				}																				//
				degAsInt = trunc(degAsDbl);											// Save off the degree int.
				degAsDbl = degAsDbl - degAsInt;										// Sub off the degree int.
				minAsDbl = degMinMapper.map(degAsDbl);								// Map the remainder to minutes.
				ourPos.lonDeg	= degAsInt;												// Fill in our bits of the output.
				ourPos.lonMin	= minAsDbl;												//
				ourPos.lonQuad	= ourQuad;												//
				success = true;															// We have been a success!
			}																					//
		break;																				//
		case 2	:																			// We got two params. Pos double & quad. 
			heapStr(&firstParam,getNextParam());									// Grab first value.
			cleanParam(firstParam);														// Clean up to make things simpler.
			heapStr(&secondParam,getNextParam());									// Grab second.
			cleanParam(secondParam);													// Scrub scrub!
			if (firstParam[0]=='W'||firstParam[0]=='E') {						// If the first param is quad..
				quadStr = firstParam;													// Point quadStr at it.
				degStr = secondParam;													// POint degStr at second.
			} else if (secondParam[0]=='W'||secondParam[0]=='E') {			// Or.. Second param has the quad.
				quadStr = secondParam;													// Point quadStr at second param.
				degStr = firstParam;														// First param must be degree string.
			} else {																			// Else?
				break;																		// There is no else, we give up here.
			}																					//
			degAsDbl = atof(degStr);													// We have one float value for degrees
			if (degAsDbl<180 && degAsDbl>0) {										// If sanity check passes..
				degAsInt = trunc(degAsDbl);											// Save off the degree int.
				degAsDbl = degAsDbl - degAsInt;										// Sub off the degree int.
				minAsDbl = degMinMapper.map(degAsDbl);								// Map the remainder to minutes.
				ourQuad = west;															// Assume west..
				if (quadStr[0]=='E') {													// Unless it's an 'S'..
					ourQuad = east;														// Then, hey, it's south.
				}																				//
				ourPos.lonDeg	= degAsInt;												// Fill in our bits of the output.
				ourPos.lonMin	= minAsDbl;												//
				ourPos.lonQuad	= ourQuad;												//
				success = true;															// We have been a success!
			}
		break;																				//
		case 3	:																			// Four prams, int deg, int min, double sec & quad
			heapStr(&firstParam,getNextParam());									// Grab and clean the three params.
			cleanParam(firstParam);														//
			heapStr(&secondParam,getNextParam());									//
			cleanParam(secondParam);													//
			heapStr(&thirdParam,getNextParam());									//									//
			cleanParam(thirdParam);														//
			if (firstParam[0]=='W'||firstParam[0]=='E') {						// If quad is first..
				quadStr = firstParam;													// Point quadStr at it.
				degStr = secondParam;													// Deg str will be second.
				minStr = thirdParam;														// Min str will be third.
			} else if (thirdParam[0]=='W'||thirdParam[0]=='E') {				// Else quad is last, only other choice we allow.
				quadStr = thirdParam;													// Quad third param
				degStr = firstParam;														// deg will be first.
				minStr = secondParam;													// Min will be second.
			} else {																			// else?
				break;																		// Can't find quad, bail!
			}
			degAsInt = atoi(degStr);												// Grab degrees as an int.
			if (degAsInt<180 && degAsInt>=0) {									// If it passes sanity check.
				minAsDbl = atof(minStr);											// Grab minutes as a double.
				if (minAsDbl<60 && minAsDbl>=0) {								// If minutes passes sanity.
					ourQuad = west;													// Assume west..
					if (quadStr[0]=='E') {											// Unless it's an 'S'..
						ourQuad = east;												// Then, hey, it's south.
					}																		//
					ourPos.lonDeg	= degAsInt;												// Fill in our bits of the output.
					ourPos.lonMin	= minAsDbl;												//
					ourPos.lonQuad	= ourQuad;												//
					success = true;															// We have been a success!
				}																			//
			}																					//
		break;																				//
		case 4	:																			//
			heapStr(&firstParam,getNextParam());									// Grab and clean the four params.
			cleanParam(firstParam);														//
			heapStr(&secondParam,getNextParam());									//
			cleanParam(secondParam);													//
			heapStr(&thirdParam,getNextParam());									//
			cleanParam(thirdParam);														//
			heapStr(&fourthParam,getNextParam());									//
			cleanParam(fourthParam);													//
			if (firstParam[0]=='W'||firstParam[0]=='E') {						// If quad is first..
				quadStr = firstParam;													// Point quadStr at it.
				degStr = secondParam;													// Deg str will be second.
				minStr = thirdParam;														// Min str will be third.
				secStr = fourthParam;													// Sec str will be fourth.
			} else if (fourthParam[0]=='W'||fourthParam[0]=='E') {			// Else quad is last, only other choice we allow.
				quadStr = fourthParam;													// Quad third param
				degStr = firstParam;														// deg will be first.
				minStr = secondParam;													// Min will be second.
				secStr = thirdParam;														// Sec str will be third.
			} else {																			// Quad is not on either end?
				break;																		// We're done here.
			}																					//
			degAsInt = atoi(degStr);													// Grab degrees as an int.
			if (degAsInt<180 && degAsInt>=0) {										// If it passes sanity check.
				minAsInt = atoi(minStr);												// Grab minutes as a double.
				if (minAsInt<60 && minAsInt>=0) {									// If minutes passes sanity.
					secAsDbl = atof(secStr);											// Grab seconds.
					if (secAsDbl<60 && secAsDbl>=0) {								// If seconds passes sanity.
						ourQuad = west;													// Assume west..
						if (quadStr[0]=='E') {											// Unless it's an 'S'..
							ourQuad = east;												// Then, hey, it's south.
						}																		//
						minAsDbl = minAsInt + secMinMapper.map(secAsDbl);		// Convert our min & sec to double min.
						ourPos.lonDeg	= degAsInt;										// Fill in our bits of the output.
						ourPos.lonMin	= minAsDbl;										//
						ourPos.lonQuad	= ourQuad;										//
						success = true;													// We have been a success!
					}																			//
				}																				//
			}																					//
		break;																				// All done.
	}																							//
	freeStr(&firstParam);																// Recycle the RAM we used.
	freeStr(&secondParam);																//
	freeStr(&thirdParam);																//
	freeStr(&fourthParam);																//
	ourPos.lonValid = success;															// Return our success, or not..
}



// **********************************************
// ****************  globalPos  *****************
// **********************************************


// Constructor, fill in some legal default values.
globalPos::globalPos(void) {

	latDeg	= 48;
	latMin	= 29.41;
	latQuad	= north;
	lonDeg	= 122;
	lonMin	= 39.72;
	lonQuad	= west;
}


// Destructor, nothing to recycle.
globalPos::~globalPos(void) {  }


// Make sure everything is in spec.x	
bool globalPos::valid(void) {

	if (!checkLatDeg(latDeg))	return false;
	if (!checkMin(latMin))		return false;
	if (!checkLonDeg(lonDeg))	return false;
	if (!checkMin(lonMin))		return false;
	if (latQuad==east||latQuad==west) return false;
	if (lonQuad==north||lonQuad==south) return false;
	return true;
}
	
int globalPos::writeToEEPROM(int addr) {

	
	double	value;
	
	value = getLatAsDbl();
	EEPROM.put(addr,value);
	addr = addr + sizeof(double);
	value = getLonAsDbl();
	EEPROM.put(addr,value);
	return 2*sizeof(double);
}


int globalPos::copyFromEEPROM(int addr) {

	double	value;
	
	EEPROM.get(addr,value);
	setLat(value);
	addr = addr + sizeof(double);
	EEPROM.get(addr,value);
	setLon(value);
	return 2*sizeof(double);
}

								
// I wanna' be like you euooo.
void globalPos::copyPos(globalPos* aLatLon) {

	if (aLatLon) {
		copyLat(aLatLon);
		copyLon(aLatLon);
	}
}


// I wanna' be like.. Half of you euooo.
void globalPos::copyLat(globalPos* aLatLon) {

	if (aLatLon) {
		latDeg	= aLatLon->latDeg;
		latMin	= aLatLon->latMin;
		latQuad	= aLatLon->latQuad;
	}
}


// Or the other half of you euooo.
void globalPos::copyLon(globalPos* aLatLon) {

	if (aLatLon) {
		lonDeg	= aLatLon->lonDeg;
		lonMin	= aLatLon->lonMin;
		lonQuad	= aLatLon->lonQuad;
	}
}

				
// In the format DD MM.MMM	Does not look for Quadrent. See below.
void globalPos::setLatValue(const char* inLatStr) {

	int		dotIndex;
	char*		latStr;                                      
	char		minStr[G_POS_BUFF_BYTES];
	
	latStr = NULL;																// ALWAYS initialize at NULL for these.
	if (heapStr(&latStr,inLatStr)) {										// If we can alocate a copy.
		dotIndex = 0;															// Starting at zero.
		while(latStr[dotIndex]!='.'&&latStr[dotIndex]!='\0') {	// Looking for the dot..
			dotIndex++;															// Cruise!
		}																			//
		if (latStr[dotIndex]=='.') {										// If we are pointing at the dot.
			if (dotIndex>2) {													// If we can back up a couple.                                                                                                                                                                                                                                                                                                     
				strcpy(minStr,&(latStr[dotIndex-2]));					// From here to end is the minutes.
				latStr[dotIndex-2] = '\0';									// Break the string where we started.
				latDeg = atoi(latStr);										// From start to new end is degrees.
				latMin = atof(minStr);										// minute string is already saved.
			}																		//
		}																			//
		freeStr(&latStr);														// Recycle the local string.
	}
}


// Looks for N, S, NORTH or SOUTH. Ignores case.
void globalPos::setLatQuad(const char* inQuad) {

	char*		quadStr;
	
	quadStr = NULL;																// ALWAYS initialize at NULL for this.
	if (heapStr(&quadStr,inQuad)) {											// If we can alocate a copy.
		upCase(quadStr);															// Make it all uppercase.
		if (!strcmp("N",quadStr)||!strcmp("NORTH",quadStr)) {			// We'll take either of these.
			latQuad = north;														// Call it north.
		}else if (!strcmp("S",quadStr)||!strcmp("SOUTH",quadStr)) {	// Else we'll take either of these.
			latQuad = south;														// Calling them south.
		}																				//
		freeStr(&quadStr);														// Recycle the local string.
	}																					//
}
	 

// In the format DDD MM.MMM	Does not look for Quadrent. See below.		 
void globalPos::setLonValue(const char* inLonStr) {

	int		dotIndex;
	char*		lonStr;                                      
	char		minStr[G_POS_BUFF_BYTES];
	
	lonStr = NULL;																// ALWAYS initialize at NULL for this.
	if (heapStr(&lonStr,inLonStr)) {										// If we can alocate a copy.
		dotIndex = 0;															// Starting at zero.
		while(lonStr[dotIndex]!='.'&&lonStr[dotIndex]!='\0') {	// Looking for the dot..
			dotIndex++;															// Cruise!
		}																			//
		if (lonStr[dotIndex]=='.') {										// If we are pointing at the dot.
			if (dotIndex>2) {													// If we can back up a couple.                                                                                                                                                                                                                                                                                                     
				strcpy(minStr,&(lonStr[dotIndex-2]));					// From here to end is the minutes.
				lonStr[dotIndex-2] = '\0';									// Break the string where we started.
				lonDeg = atoi(lonStr);										// From start to new end is degrees.
				lonMin = atof(minStr);										// minute string is already saved.
			}																		//
		}																			//
		freeStr(&lonStr);														// Recycle the local string.
	}
}


// Looks for E, W, EAST or WEST. Ignores case.
void globalPos::setLonQuad(const char* inQuad) {

	char*		quadStr;
	
	quadStr = NULL;															// ALWAYS initialize at NULL for this.
	if (heapStr(&quadStr,inQuad)) {										// If we can alocate a copy.
		upCase(quadStr);														// Make it all uppercase.
		if (!strcmp("E",quadStr)||!strcmp("EAST",quadStr)) {		// We'll take either of these.
			lonQuad = east;													// Call it north.
		} else if (!strcmp("W",quadStr)||!strcmp("WEST",quadStr)) {	// Else we'll take either of these.
			lonQuad = west;													// Calling them south.
		}																			//
		freeStr(&quadStr);													// Recycle the local string.
	}																				//
}


void globalPos::setLat(double inLat) {
	
	int		latDeg;
	double	latMin;
	quad		latQuad;
	
	latDeg = trunc(inLat);
	latDeg = abs(latDeg);
	latMin = abs(inLat) - latDeg;
	latMin = latMin * 60.0;
	if (inLat>=0) {
		latQuad = north;
	} else {
		latQuad = south;
	}
	setLatValue(latDeg,latMin);
	setLatQuad(latQuad);
}


void globalPos::setLon(double inLon) {

	
	int		lonDeg;
	double	lonMin;
	quad		lonQuad;
	
	lonDeg = trunc(inLon);
	lonDeg = abs(lonDeg);
	lonMin = abs(inLon) - lonDeg;
	lonMin = lonMin * 60.0;
	if (inLon>=0) {
		lonQuad = east;
	} else {
		lonQuad = west;
	}
	setLonValue(lonDeg,lonMin);
	setLonQuad(lonQuad);
	
}
				
				
void globalPos::setPosValues(const char* latStr,const char* lonStr) {

	//setLatValue(latStr);
	//setLonValue(lonStr);
	gPosPack	aPos;
	
	aPos = ourPosParser.parsePos(latStr,lonStr);
	if (aPos.latValid && aPos.lonValid) {
		setPosition(aPos.latDeg,aPos.latMin,aPos.latQuad,aPos.lonDeg,aPos.lonMin,aPos.lonQuad);
	}
}


void globalPos::setQuads(const char*  inLatQuad,const char*  inLonQuad) {

	setLatQuad(inLatQuad);
	setLonQuad(inLonQuad);
}


void globalPos::setPos(double inLat, double inLon) {

	int		latDeg;
	double	latMin;
	quad		latQuad;
	int		lonDeg;
	double	lonMin;
	quad		lonQuad;
	
	latDeg = trunc(inLat);
	latDeg = abs(latDeg);
	latMin = abs(inLat) - latDeg;
	latMin = latMin * 60.0;
	if (inLat>=0) {
		latQuad = north;
	} else {
		latQuad = south;
	}
	lonDeg = trunc(inLon);
	lonDeg = abs(lonDeg);
	lonMin = abs(inLon) - lonDeg;
	lonMin = lonMin * 60.0;
	if (inLon>=0) {
		lonQuad = east;
	} else {
		lonQuad = west;
	}
	setPosition(latDeg,latMin,latQuad,lonDeg,lonMin,lonQuad);
}


void globalPos::setLatValue(int inLatDeg, double inLatMin) {

	if (checkLatDeg(inLatDeg)&&checkMin(inLatMin)) {
		latDeg = inLatDeg;
		latMin = inLatMin;
	}
}


void globalPos::setLatQuad(quad inLatQuad) {

	if (inLatQuad==north||inLatQuad==south) {
		latQuad = inLatQuad;
	}
}


void globalPos::setLonValue(int inLonDeg, double inLonMin) {

	if (checkLonDeg(inLonDeg)&&checkMin(inLonMin)) {
		lonDeg = inLonDeg;
		lonMin = inLonMin;
	}
}


void globalPos::setLonQuad(quad inLonQuad) {

	if (inLonQuad==east||inLonQuad==west) {
		lonQuad = inLonQuad;
	}
}


void globalPos::setQuads(quad inLatQuad,quad inLonQuad) {

	setLatQuad(inLatQuad);
	setLonQuad(inLonQuad);
}


void globalPos::setPosition(int inLatDeg, double inLatMin, quad inLatQuad, int inLonDeg, double inLonMin, quad inLonQuad) {

	setLatValue(inLatDeg,inLatMin);
	setLonValue(inLonDeg,inLonMin);
	setQuads(inLatQuad,inLonQuad);
}


// We are at point A, we want to sail to point B. Where should we head? Well, the internet
// furnished this formula :
//
// ----------------------------------------------------------------- 
// Bearing from point A to B, can be calculated as,
// 
// β = atan2(X,Y),
// 
// where, X and Y are two quantities and can be calculated as:
// 
// X = cos θb * sin ∆L
// 
// Y = cos θa * sin θb – sin θa * cos θb * cos ∆L
// ----------------------------------------------------------------- 
//
// Ok.. Let's give it a go!
//
double globalPos::trueBearingTo(globalPos* inDest) { 

	double	latA;
	double	lonA;
	double	latB;
	double	lonB;
	double	bearing;
	double	X;				// Some quantity?
	double	Y;				// Some other quantity?
	double	deltaLon;
	
	latA = getLatAsDbl();																// Grab our location
	latA = deg2rad(latA);																// Convert to radians, for doing trig.
	lonA = getLonAsDbl();																// Other value as wel..
	lonA = deg2rad(lonA);																// Trig..
	
	latB = inDest->getLatAsDbl();														// Do the same for the destination
	latB = deg2rad(latB);																// Convert.
	lonB = inDest->getLonAsDbl();														// Other value.
	lonB = deg2rad(lonB);																// Convert.
	
	
	deltaLon = lonB - lonA;																// Formula wants delta longitude.
	X = cos(latB) * sin(deltaLon);													// Calculate the X thing.
	Y = cos(latA) * sin(latB) - sin(latA) * cos(latB) * cos(deltaLon);	// Calculate the Y thing.
	bearing = atan2(X,Y);																// Do the atan2() thing.
	bearing = rad2deg(bearing);														// Convert it back to degrees. (For sailors)
	if (bearing<0) {																		// Negative values?
		bearing = 360 + bearing;														// Would this be the fix?
	}																							// Seems so.
	return bearing;																		// Hand it off.
}


// How far away is point B?
// On using great circle route and an average value or earth's radius.. We'll use the 
// Haversine formula :
//
// ----------------------------------------------------------------- 
// https://www.movable-type.co.uk/scripts/latlong.html
//
//Haversine formula:	
//a = sin²(Δφ/2) + cos φ1 ⋅ cos φ2 ⋅ sin²(Δλ/2)
//c = 2 ⋅ atan2( √a, √(1−a) )
//d = R ⋅ c
//where:	φ is latitude, λ is longitude, R is earth’s radius (mean radius = 6,371km);
//note that angles need to be in radians to pass to trig functions!
// -----------------------------------------------------------------
//
// Well, here it goes..
double globalPos::distanceTo(globalPos* inDest) {

	double	latA;
	double	lonA;
	double	latB;
	double	lonB;
	double	deltaLat;
	double	deltaLon;
	double	step1;
	double	angle;
	double	dist;
	
	latA = getLatAsDbl();																										// Grab our latitude as a numerical value.
	latA = deg2rad(latA);																										// Convert to radians, for doing trig.
	lonA = getLonAsDbl();																										// Grab our longitude as a numerical value.
	lonA = deg2rad(lonA);																										// Trig..
	
	latB = inDest->getLatAsDbl();																								// Do the same for the destination
	latB = deg2rad(latB);																										// Convert.
	lonB = inDest->getLonAsDbl();																								// Grab..
	lonB = deg2rad(lonB);																										// Convert.
	
	deltaLat = (inDest->getLatAsDbl() - getLatAsDbl());																// Deltas are all calculated dest - start.
	deltaLat = deg2rad(deltaLat);
	deltaLon = (inDest->getLonAsDbl() - getLonAsDbl());																// Calculate..
	deltaLon = deg2rad(deltaLon);
	
	step1 = sin(deltaLat/2) * sin(deltaLat/2) + cos(latA)*cos(latB)*sin(deltaLon/2)*sin(deltaLon/2);	// Calculate first step.
	angle = 2 * atan2(sqrt(step1), sqrt(1-step1));
	dist = angle * RADIUS_EARTH_KNOTS; //6371.2; for km
	return dist;
}


// These value to string methods end up with all this monkey motion because typical
// Arduinos don't do float to string conversions in the normal sprintf() ways. So I
// convert everything to ints and paste the string bits all together "by hand".

// This assumes the values are "good" IE in range. Otherwise you'll get wacky answers.
char* globalPos::getLatStr(void) {

	char		tempStr[G_POS_BUFF_BYTES];
	int		lMin;
	double	lMinDec;
	int		lMinOut;
	
	sprintf(tempStr,"%03u",latDeg);
	strcpy(outStr,tempStr);
	lMin = trunc(latMin);
	sprintf(tempStr,"%02u",lMin);
	strcat(outStr,tempStr);
	strcat(outStr,".");
	lMinDec = latMin - lMin;
	lMinOut = round(lMinDec * 10000);
	sprintf(tempStr,"%04u",lMinOut);
	strcat(outStr,tempStr);
	return outStr;
}

	
char* globalPos::getLatQuadStr(void) {

	if (latQuad==north) {
		strcpy(outStr,"N");
	} else {
		strcpy(outStr,"S");
	}
	return outStr;
}


char* globalPos::getLonStr(void) {

	char	tempStr[G_POS_BUFF_BYTES];
	int	lMin;
	double	lMinDec;
	int	lMinOut;
	
	sprintf(tempStr,"%03u",lonDeg);
	strcpy(outStr,tempStr);
	lMin = trunc(lonMin);
	sprintf(tempStr,"%02u",lMin);
	strcat(outStr,tempStr);
	strcat(outStr,".");
	lMinDec = lonMin - lMin;
	lMinOut = round(lMinDec * 10000);
	sprintf(tempStr,"%04u",lMinOut);
	strcat(outStr,tempStr);
	return outStr;
}


char* globalPos::getLonQuadStr(void) {
	
	if (lonQuad==east) {
		strcpy(outStr,"E");
	} else {
		strcpy(outStr,"W");
	}
	return outStr;
}


// Formatted for humans.	
char* globalPos::showLatStr(void) {

	char	tempStr[G_POS_BUFF_BYTES];
	
	if (valid()) {
		switch(latQuad) {
			case north	: strcpy(outStr, "North ");	break;
			case south	: strcpy(outStr, "South ");	break;
			default		:										break;
		}
		sprintf(tempStr,"%3u%s",latDeg," Deg. ");
		strcat(outStr,tempStr);
		sprintf(tempStr,"%6.3lf%s",latMin," Min.");
		strcat(outStr,tempStr);
	} else {
		strcpy(outStr,"Invalid fix");
	}
	return outStr;
}


// This one too.			
char* globalPos::showLonStr(void) {

	char	tempStr[G_POS_BUFF_BYTES];
	
	if (valid()) {
		switch(lonQuad) {
			case east	: strcpy(outStr, "East  ");	break;
			case west	: strcpy(outStr, "West  ");	break;
			default		:										break;
		}
		sprintf(tempStr,"%3u%s",lonDeg," Deg. ");
		strcat(outStr,tempStr);
		sprintf(tempStr,"%6.3lf%s",lonMin," Min.");
		strcat(outStr,tempStr);
	} else {
		strcpy(outStr,"Invalid fix");
	}
	return outStr;
}			

				
int globalPos::getLatDeg(void)	{ return latDeg; }

double globalPos::getLatMin(void)	{ return latMin; }

quad globalPos::getLatQuad(void)	{ return latQuad; }

int globalPos::getLonDeg(void)	{ return lonDeg; }

double globalPos::getLonMin(void)	{ return lonMin; }

quad globalPos::getLonQuad(void)	{ return lonQuad; }


double globalPos::getLatAsDbl(void) {
	
	double	result;
	
	result = latMin/60.0;
	result = result + latDeg;
	if (latQuad==south) {
		result = result * -1;
	}
	return result;
}


double globalPos::getLonAsDbl(void) {

	double	result;
	
	result = lonMin/60.0;
	result = result + lonDeg;
	if (lonQuad==west) {
		result = result * -1;
	}
	return result;
}



// For NMEA2k messages.				
int32_t  globalPos::getLatAsInt32(void) {	

	double	temp;
	int32_t	result;
	
	temp = getLatAsDbl();
	temp = temp * 10000000;
	result = round(temp);
	return result;
}


// For NMEA2k messages.
int32_t  globalPos::getLonAsInt32(void) {
		
	double	temp;
	int32_t	result;
	
	temp = getLonAsDbl();
	temp = temp * 10000000;
	result = round(temp);
	return result;
}


// For NMEA2k messages.
int64_t	globalPos::getLatAsInt64(void) {
	
	double	temp;
	int64_t	result;
	
	temp = getLatAsDbl();
	temp = temp * 10000000000000000.0;
	result = round(temp);
	return result;
}


// For NMEA2k messages.
int64_t	globalPos::getLonAsInt64(void) {
	
	double	temp;
	int64_t	result;
	
	temp = getLonAsDbl();
	temp = temp * 10000000000000000.0;
	result = round(temp);
	return result;
}