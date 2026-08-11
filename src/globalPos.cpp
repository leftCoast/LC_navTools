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


void showGPosPack(gPosPack* aGPP) {

	Serial.print(" latValid : ");Serial.println(aGPP->latValid);
	Serial.print(" latDeg   : ");Serial.println(aGPP->latDeg);
	Serial.print(" latMin   : ");Serial.println(aGPP->latMin,8);
	Serial.print(" latQuad  : ");Serial.println(aGPP->latQuad);
	Serial.print(" lonValid : ");Serial.println(aGPP->lonValid);
	Serial.print(" lonDeg   : ");Serial.println(aGPP->lonDeg);
	Serial.print(" lonMin   : ");Serial.println(aGPP->lonMin,8);
	Serial.print(" lonQuad  : ");Serial.println(aGPP->lonQuad);
}



mapper			degMinMapper(0,1,0,60);
mapper 			secMinMapper(0,60,0,1);
posParser		ourPosParser;
posFormatter	ourPosFormatter;


// **********************************************
// *****************   navMark  *****************
// **********************************************


navMark::navMark(void) { markName = NULL; }


navMark::navMark(const char* inName,gPosPack* inPos) {

	markName = NULL;
	setName(inName);
	setPos(inPos);
}


navMark::~navMark(void) { freeStr(&markName); }


void navMark::setName(const char* inName) { heapStr(&markName,inName); }


void navMark::setPos(gPosPack* inPos) { latLon = *inPos; }


const char* navMark::getName(void) { return markName; }


gPosPack navMark::getPos(void) { return latLon; }


uint32_t navMark::numBytes(void) {

	uint32_t	numBytes;
	
	numBytes = sizeof(gPosPack);
	if (markName) {
		numBytes = numBytes + strlen(markName) + 1;
	}
	return numBytes;
}

		
void navMark::fillBuff(uint8_t* buff) {

	gPosPack*	packPtr;
	char*			strPtr;
	
	packPtr = (gPosPack*)buff;
	*packPtr = latLon;
	strPtr = (char*)&(buff[sizeof(gPosPack)]);
	strcpy(strPtr,markName);
}

	
void navMark::readBuff(uint8_t* buff) {			
	
	gPosPack*	packPtr;		
	char*			strPtr;
	
	packPtr = (gPosPack*)buff;
	setPos(packPtr);
	strPtr = (char*)&(buff[sizeof(gPosPack)]);
	setName(strPtr);
}

	
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
	return ourPos; 
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
	//Serial.print("Cleaned param : ");
	//Serial.println(inParam);
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
			if (degAsInt<90 && degAsInt>=0) {										// If it passes sanity check.
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
			if (degAsDbl<180 && degAsDbl>=0) {										// If sanity check passes..
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
// *************** posFormatter *****************
// **********************************************


posFormatter::posFormatter(void) {  }


posFormatter::~posFormatter(void) {  }

	
char* posFormatter::getLatStr(gPosPack* aPos,posFormat format) {

	double	degrees;
	int		minutes;
	double	seconds;
	int		iSeconds;
	char		quadStr[4];
	
	strcpy(outStr,"Invalid");
	if (aPos) {
		if (aPos->latValid) {
			if (aPos->latQuad==south) {
				strcpy(quadStr,"S");
			} else {
				strcpy(quadStr,"N");
			}
			switch(format) {
				case floatDeg							:
					degrees = aPos->latDeg + (aPos->latMin / 60.0);
					if (aPos->latQuad==south) {
						degrees = -degrees;
					}
					sprintf(outStr,"%11.6f",degrees);
				break;
				case floatDeg_quad					:
					degrees = aPos->latDeg + (aPos->latMin / 60.0);
					sprintf(outStr,"%10.6f%s%s",degrees," ",quadStr);
				break;
				case intDeg_floatMin_quad			:
					sprintf(outStr,"%3u%s%8.5f%s%s",aPos->latDeg," ",aPos->latMin," ",quadStr);
				break;
				case intDeg_intMin_intSec_quad	:
					minutes = trunc(aPos->latMin);
					iSeconds = round((aPos->latMin - minutes) * 60.0);
					sprintf(outStr,"%3u%s%2u%s%2u%s%s",aPos->latDeg," ",minutes," ",iSeconds," ",quadStr);
				break;
				case intDeg_intMin_floatSec_quad	:
					minutes = trunc(aPos->latMin);
					seconds = (aPos->latMin - minutes) * 60.0;
					sprintf(outStr,"%3u%s%2u%s%6.3f%s%s",aPos->latDeg," ",minutes," ",seconds," ",quadStr);
				break;
				case quad_floatDeg					:
					degrees = aPos->latDeg + (aPos->latMin / 60.0);
					sprintf(outStr,"%s%s%10.6f",quadStr," ",degrees);
				break;
				case quad_intDeg_floatMin			:
					sprintf(outStr,"%s%s%3u%s%10.6f",quadStr," ",aPos->latDeg," ",aPos->latMin);
				break;
				case quad_intDeg_intMin_intSec	:
					minutes = trunc(aPos->latMin);
					iSeconds = round((aPos->latMin - minutes) * 60.0);
					sprintf(outStr,"%s%s%3u%s%2u%s%2u",quadStr," ",aPos->latDeg," ",minutes," ",iSeconds);
				break;
				case quad_intDeg_intMin_floatSec	:
					minutes = trunc(aPos->latMin);
					seconds = (aPos->latMin - minutes) * 60.0;
					sprintf(outStr,"%s%s%3u%s%2u%s%6.3f",quadStr," ",aPos->latDeg," ",minutes," ",seconds);
				break;
			}
		}
	}
	return outStr;
}


char* posFormatter::getLonStr(gPosPack* aPos,posFormat format) {

	double	degrees;
	int		minutes;
	double	seconds;
	int		iSeconds;
	char		quadStr[4];
	
	strcpy(outStr,"Invalid");
	if (aPos) {
		if (aPos->lonValid) {
			if (aPos->lonQuad==east) {
				strcpy(quadStr,"E");
			} else {
				strcpy(quadStr,"W");
			}
			switch(format) {
				case floatDeg							:
					degrees = aPos->lonDeg + (aPos->lonMin / 60.0);
					if (aPos->lonQuad==west) {
						degrees = -degrees;
					}
					sprintf(outStr,"%11.6f",degrees);
				break;
				case floatDeg_quad					:
					degrees = aPos->lonDeg + (aPos->lonMin / 60.0);
					sprintf(outStr,"%10.6f%s%s",degrees," ",quadStr);
				break;
				case intDeg_floatMin_quad			:
					sprintf(outStr,"%3u%s%8.5f%s%s",aPos->lonDeg," ",aPos->lonMin," ",quadStr);
				break;
				case intDeg_intMin_intSec_quad	:
					minutes = trunc(aPos->lonMin);
					iSeconds = round((aPos->lonMin - minutes) * 60.0);
					sprintf(outStr,"%3u%s%2u%s%2u%s%s",aPos->lonDeg," ",minutes," ",iSeconds," ",quadStr);
				break;
				case intDeg_intMin_floatSec_quad	:
					minutes = trunc(aPos->lonMin);
					seconds = (aPos->lonMin - minutes) * 60.0;
					sprintf(outStr,"%3u%s%2u%s%6.3f%s%s",aPos->lonDeg," ",minutes," ",seconds," ",quadStr);
				break;
				case quad_floatDeg					:
					degrees = aPos->lonDeg + (aPos->lonMin / 60.0);
					sprintf(outStr,"%s%s%10.6f",quadStr," ",degrees);
				break;
				case quad_intDeg_floatMin			:
					sprintf(outStr,"%s%s%3u%s%10.6f",quadStr," ",aPos->lonDeg," ",aPos->lonMin);
				break;
				case quad_intDeg_intMin_intSec	:
					minutes = trunc(aPos->lonMin);
					iSeconds = round((aPos->lonMin - minutes) * 60.0);
					sprintf(outStr,"%s%s%3u%s%2u%s%2u",quadStr," ",aPos->lonDeg," ",minutes," ",iSeconds);
				break;
				case quad_intDeg_intMin_floatSec	:
					minutes = trunc(aPos->lonMin);
					seconds = (aPos->lonMin - minutes) * 60.0;
					sprintf(outStr,"%s%s%3u%s%2u%s%6.3f",quadStr," ",aPos->lonDeg," ",minutes," ",seconds);
				break;
			}
		}
	}
	return outStr;
}

	
	
	
// **********************************************
// ****************  globalPos  *****************
// **********************************************


// Constructor, we ain't valid yet.
globalPos::globalPos(void) {

	ourPos.latValid = false;
	ourPos.lonValid = false;
}


// Destructor, nothing to recycle.
globalPos::~globalPos(void) {  }


// Make sure everything is in spec.x	
bool globalPos::valid(void) { return ourPos.latValid && ourPos.lonValid; }


/*	
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
*/
								
// I wanna' be like you euooo.
void globalPos::copyPos(globalPos* aLatLon) {

	if (aLatLon) {
		ourPos = aLatLon->getPos();
		//copyLat(aLatLon);
		//copyLon(aLatLon);
	}
}


void globalPos::setPos(gPosPack* inPos) { ourPos = *inPos; }

	
void globalPos::setLat(gPosPack* aPos) {

	ourPos.latValid	= false;
	if (aPos) {
		ourPos.latValid	= aPos->latValid;
		ourPos.latDeg		= aPos->latDeg;
		ourPos.latMin		= aPos->latMin;
		ourPos.latQuad		= aPos->latQuad;
	}
}


void globalPos::setLon(gPosPack* aPos) {

	ourPos.lonValid	= false;
	if (aPos) {
		ourPos.lonValid	= aPos->lonValid;
		ourPos.lonDeg		= aPos->lonDeg;
		ourPos.lonMin		= aPos->lonMin;
		ourPos.lonQuad		= aPos->lonQuad;
	}
}

	
void globalPos::setPos(double inLat, double inLon) {
	
	ourPos.latValid = false;
	ourPos.lonValid = false;
	if ((inLat<=90 && inLat>=-90)&&
		(inLon>=-180 && inLon<=180)) {
		ourPos.latDeg = trunc(inLat);
		ourPos.latDeg = abs(ourPos.latDeg);
		ourPos.latMin = abs(inLat) - ourPos.latDeg;
		ourPos.latMin = ourPos.latMin * 60.0;
		if (inLat>=0) {
			ourPos.latQuad = north;
		} else {
			ourPos.latQuad = south;
		}
		ourPos.lonDeg = trunc(inLon);
		ourPos.lonDeg = abs(ourPos.lonDeg);
		ourPos.lonMin = abs(inLon) - ourPos.lonDeg;
		ourPos.lonMin = ourPos.lonMin * 60.0;
		if (inLon>=0) {
			ourPos.lonQuad = east;
		} else {
			ourPos.lonQuad = west;
		}
		ourPos.latValid = true;
		ourPos.lonValid = true;
	}
}


void globalPos::setPosition(int inLatDeg, double inLatMin, quad inLatQuad, int inLonDeg, double inLonMin, quad inLonQuad) {

	ourPos.latValid = false;
	ourPos.lonValid = false;
	if (checkLatDeg(inLatDeg)) {
		if (checkLonDeg(inLonDeg)) {
			if (checkMin(inLatMin)) {
				if (checkMin(inLonMin)) {
					if (inLatQuad==north||inLatQuad==south) {
						if (inLonQuad==east||inLonQuad==west) {
							ourPos.latDeg	= inLatDeg;
							ourPos.latMin	= inLatMin;
							ourPos.latQuad	= inLatQuad;
							ourPos.lonDeg	= inLonDeg;
							ourPos.lonMin	= inLonMin;
							ourPos.lonQuad	= inLonQuad;
							ourPos.latValid = true;
							ourPos.lonValid = true;
						}
					}
				}
			}
		}
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
				ourPos.latDeg = atoi(latStr);								// From start to new end is degrees.
				ourPos.latMin = atof(minStr);								// minute string is already saved.
			}																		//
		}																			//
		freeStr(&latStr);														// Recycle the local string.
	}																				//
	setValid();																	// Check to see if our position is valid.
}


// Looks for N, S, NORTH or SOUTH. Ignores case.
void globalPos::setLatQuad(const char* inQuad) {

	char*		quadStr;
	
	quadStr = NULL;																// ALWAYS initialize at NULL for this.
	if (heapStr(&quadStr,inQuad)) {											// If we can alocate a copy.
		upCase(quadStr);															// Make it all uppercase.
		if (!strcmp("N",quadStr)||!strcmp("NORTH",quadStr)) {			// We'll take either of these.
			ourPos.latQuad = north;												// Call it north.
		}else if (!strcmp("S",quadStr)||!strcmp("SOUTH",quadStr)) {	// Else we'll take either of these.
			ourPos.latQuad = south;												// Calling them south.
		}																				//
		freeStr(&quadStr);														// Recycle the local string.
	}																					//
	setValid();																		// Check to see if our position is valid.
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
				ourPos.lonDeg = atoi(lonStr);								// From start to new end is degrees.
				ourPos.lonMin = atof(minStr);								// minute string is already saved.
			}																		//
		}																			//
		freeStr(&lonStr);														// Recycle the local string.
	}																				//
	setValid();																	// Check to see if our position is valid.
}


// Looks for E, W, EAST or WEST. Ignores case.
void globalPos::setLonQuad(const char* inQuad) {

	char*		quadStr;
	
	quadStr = NULL;																// ALWAYS initialize at NULL for this.
	if (heapStr(&quadStr,inQuad)) {											// If we can alocate a copy.
		upCase(quadStr);															// Make it all uppercase.
		if (!strcmp("E",quadStr)||!strcmp("EAST",quadStr)) {			// We'll take either of these.
			ourPos.lonQuad = east;												// Call it north.
		} else if (!strcmp("W",quadStr)||!strcmp("WEST",quadStr)) {	// Else we'll take either of these.
			ourPos.lonQuad = west;												// Calling them south.
		}																				//
		freeStr(&quadStr);														// Recycle the local string.
	}																					//
	setValid();																		// Check to see if our position is valid.
}


// From the values we have saved, are we holding a valid position? Good for when you shove
// in a value from, wherever, and need know if it was valid or not. For example the GPS
// reader code stuffs in position values from the hardware. Then as the last value  comes
// in, it calls this to set the valid flags. For whomever wants to use this data.
void globalPos::setValid(void) {

	ourPos.latValid = false;
	ourPos.lonValid = false;
	if (ourPos.latDeg<=90 && ourPos.latDeg>=0) {
		if (ourPos.latMin<60 && ourPos.latMin>=0) {
			if (ourPos.latQuad==north || ourPos.latQuad==south) {
				ourPos.latValid = true;
			}
		}
	}
	if (ourPos.lonDeg<=180 && ourPos.lonDeg>=0) {
		if (ourPos.lonMin<60 && ourPos.lonMin>=0) {
			if (ourPos.latQuad==east || ourPos.latQuad==west) {
				ourPos.lonValid = true;
			}
		}
	}
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


// Formatted for humans.
char* globalPos::getLatStr(posFormat format) { return ourPosFormatter.getLatStr(&ourPos,format); }


// Formatted for humans as well.
char* globalPos::getLonStr(posFormat format) { return ourPosFormatter.getLonStr(&ourPos,format); }


// Want our position? Have a copy!
gPosPack globalPos::getPos(void) { return ourPos; }


// For NMEA2k messages.	And MATH!
double globalPos::getLatAsDbl(void) {
	
	double	result;
	
	result = ourPos.latMin/60.0;
	result = result + ourPos.latDeg;
	if (ourPos.latQuad==south) {
		result = result * -1;
	}
	return result;
}


// For NMEA2k messages.	And MATH!
double globalPos::getLonAsDbl(void) {

	double	result;
	
	result = ourPos.lonMin/60.0;
	result = result + ourPos.lonDeg;
	if (ourPos.lonQuad==west) {
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

