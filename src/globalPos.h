#ifndef globalPos_h
#define globalPos_h

#include <Arduino.h>
#include <lilParser.h>

#define	G_POS_BUFF_BYTES	40


// Quadrant choices.
enum quad {
	north,
	south,
	east,
	west
};


enum posFormat {
	floatDeg,
	floatDeg_quad,
	intDeg_floatMin_quad,
	intDeg_intMin_intSec_quad,
	intDeg_intMin_floatSec_quad,
	quad_floatDeg,
	quad_intDeg_floatMin	,
	quad_intDeg_intMin_intSec	,
	quad_intDeg_intMin_floatSec
};



// Need a position packed up for passing about?
struct gPosPack {

	bool		latValid;
	int		latDeg;
	double	latMin;
	quad		latQuad;
	bool		lonValid;
	int		lonDeg;
	double	lonMin;
	quad		lonQuad;
};


extern gPosPack	nullPos;
extern void			showGPosPack(gPosPack* aGPP);
extern bool			checkLatDeg(int degrees);
extern bool			checkLonDeg(int degrees);
extern bool			checkMin(double minutes);
extern double		rad2deg(double angleRad);
extern double		deg2rad(double angleDeg);




// **********************************************
// *****************   navMark  *****************
// **********************************************


// Navigation mark is a named position.
class navMark {

	public :
				navMark(const char* inName,gPosPack* inPos);
				navMark(void);
	virtual	~navMark(void);
	
	virtual	void 				setName(const char* inName);
	virtual	void 				setPos(gPosPack* inPos);
				const char* 	getName(void);
				gPosPack			getPos(void);
				uint32_t			numBytes(void);
				void				fillBuff(uint8_t* buff);
				void				readBuff(uint8_t* buff);			
				
	protected:
				gPosPack latLon;
				char*		markName;
};




// **********************************************
// ****************   posParser  ****************
// **********************************************


enum parseCmd { noCmd, latCmd, lonCmd };


class posParser :	public lilParser {

	public:
				posParser(void);
	virtual	~posParser(void);

				gPosPack	parsePos(const char* inLatPos,const char* inLonPos);
	
	protected:
				void		parseStr(const char* inStr);
				void		cleanParam(char* inParam);
				void		parseLat(void);
				void		parseLon(void);
				
				gPosPack	ourPos;
};

extern posParser ourPosParser;


	
// **********************************************
// *************** posFormatter *****************
// **********************************************


class posFormatter :	public lilParser {

	public:
				posFormatter(void);
	virtual	~posFormatter(void);
	
				char*	getLatStr(gPosPack* aPos,posFormat format);
				char*	getLonStr(gPosPack* aPos,posFormat format);
				
				char	outStr[G_POS_BUFF_BYTES];
	};
	
extern posFormatter ourPosFormatter;	



// **********************************************
// ****************  globalPos  *****************
// **********************************************

class globalPos {

	public:
				globalPos(void);
	virtual	~globalPos(void);
	
				bool		valid(void);
				
				//int		writeToEEPROM(int addr);
				//int		copyFromEEPROM(int addr);
				
				void		copyPos(globalPos* aLatLon);
				
				void		setPos(gPosPack* inPos);
				void		setLat(gPosPack* aPos);
				void		setLon(gPosPack* aPos);
				void		setPos(double inLat, double inLon);
				void		setPosition(int inLatDeg, double inLatMin, quad inLatQuad, int inLonDeg, double inLonMin, quad inLonQuad);
				
				void		setLatValue(const char* inParam);	// This set is for the GPS reader.
				void		setLatQuad(const char* inParam);
				void		setLonValue(const char* inParam);
				void		setLonQuad(const char* inParam);
				void		setValid(void);
				
				double	trueBearingTo(globalPos* inDest);	// Calculating the good stuff.
				double	distanceTo(globalPos* inDest);
				
				char*		getLatStr(posFormat format=floatDeg_quad);		// Formatted for humans.
				char*		getLonStr(posFormat format=floatDeg_quad);		// This one too.
				
				gPosPack	getPos(void);
				//int		getLatDeg(void);
				//double	getLatMin(void);
				//quad		getLatQuad(void);
				//int		getLonDeg(void);
				//double	getLonMin(void);
				//quad		getLonQuad(void);
				double	getLatAsDbl(void);		// These last six kinda' need a 32 bit processer.
				double	getLonAsDbl(void);		// Otherwise you may run into rounding errors.
				int32_t	getLatAsInt32(void);		// For NMEA2k messages.
				int32_t	getLonAsInt32(void);		// For NMEA2k messages.
				int64_t	getLatAsInt64(void);		// For NMEA2k messages.
				int64_t	getLonAsInt64(void);		// For NMEA2k messages.
				
	protected:
				gPosPack ourPos;
				//int		latDeg;
				//double	latMin;
				//quad		latQuad;
				//int		lonDeg;
				//double	lonMin;
				//quad		lonQuad;
				
};


#endif