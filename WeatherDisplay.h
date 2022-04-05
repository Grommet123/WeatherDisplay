//////////////////////////////////////////////////////////
//    Weather Forecast Display                          //
//           GK Grotsky                                 //
//            12/29/16                                  //
//////////////////////////////////////////////////////////

#ifndef WeatherDisplay_h
#define WeatherDisplay_h
#include <Arduino.h>

#define VERSION  "13.2"        // Version number of the software
#define GETDATACOUNT 60 * 60   // Time to get more data (in seconds)
#define PORT 80                // Port number of remote server
#define ERROROFFSET            // Add time/date error offset
#define CREDIT "GK Grotsky"    // Yours truly :-)
#define TOGGLEDISPLAY_SW D1    // Toggle display switch
#define ALIVE_LED D2           // Flashes to indicate that the system is alive
#define ALIVEBRIGHTNESS 150    // Brightness of the alive LED
#define ArduinoJsonVer         // ArduinoJson version 5 or 6 (comment out for version 5)
#define TFT_CS   D8            // TFT chip select pin
#define TFT_DC   D4            // TFT AO/DC pin
#define TFT_RST  D0            // TFT reset pin
//#define PERMANENT_DST          // Permanent DST (Comment out if and when this takes affect)
#define DEBUG                  // Display debug data

// Color definitions
#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0
#define WHITE    0xFFFF
#define GREY     0xC618

/* A macro to compute the number of days elapsed since 2000 Jan 0.0 */
/* (which is equal to 1999 Dec 31, 0h UT)                           */
#define days_since_2000_Jan_0(y,m,d) \
  (367L*(y)-((7*((y)+(((m)+9)/12)))/4)+((275*(m))/9)+(d)-730530L)

/* Some conversion factors between radians and degrees */
#ifndef PI
#define PI        3.1415926535897932384
#endif

#define RADEG     ( 180.0 / PI )
#define DEGRAD    ( PI / 180.0 )

/* The trigonometric functions in degrees */
#define sind(x)  sin((x)*DEGRAD)
#define cosd(x)  cos((x)*DEGRAD)
#define tand(x)  tan((x)*DEGRAD)

#define atand(x)    (RADEG*atan(x))
#define asind(x)    (RADEG*asin(x))
#define acosd(x)    (RADEG*acos(x))
#define atan2d(y,x) (RADEG*atan2(y,x))

/* Following are some macros around the "workhorse" function __daylen__ */
/* They mainly fill in the desired values for the reference altitude    */
/* below the horizon, and also selects whether this altitude should     */
/* refer to the Sun's center or its upper limb.                         */
/* This macro computes the length of the day, from sunrise to sunset. */
/* Sunrise/set is considered to occur when the Sun's upper limb is    */
/* 35 arc minutes below the horizon (this accounts for the refraction */
/* of the Earth's atmosphere).                                        */
#define day_length(year,month,day,lon,lat)  \
  __daylen__( year, month, day, lon, lat, -35.0/60.0, 1 )

/* This macro computes the length of the day, including civil twilight. */
/* Civil twilight starts/ends when the Sun's center is 6 degrees below  */
/* the horizon.                                                         */
#define day_civil_twilight_length(year,month,day,lon,lat)  \
  __daylen__( year, month, day, lon, lat, -6.0, 0 )

/* This macro computes the length of the day, incl. nautical twilight.  */
/* Nautical twilight starts/ends when the Sun's center is 12 degrees    */
/* below the horizon.                                                   */
#define day_nautical_twilight_length(year,month,day,lon,lat)  \
  __daylen__( year, month, day, lon, lat, -12.0, 0 )

/* This macro computes the length of the day, incl. astronomical twilight. */
/* Astronomical twilight starts/ends when the Sun's center is 18 degrees   */
/* below the horizon.                                                      */
#define day_astronomical_twilight_length(year,month,day,lon,lat)  \
  __daylen__( year, month, day, lon, lat, -18.0, 0 )

/* This macro computes times for sunrise/sunset.                      */
/* Sunrise/set is considered to occur when the Sun's upper limb is    */
/* 35 arc minutes below the horizon (this accounts for the refraction */
/* of the Earth's atmosphere).                                        */
#define sun_rise_set(year,month,day,lon,lat,rise,set)  \
  __sunriset__( year, month, day, lon, lat, -35.0/60.0, 1, rise, set )

/* This macro computes the start and end times of civil twilight.       */
/* Civil twilight starts/ends when the Sun's center is 6 degrees below  */
/* the horizon.                                                         */
#define civil_twilight(year,month,day,lon,lat,start,end)  \
  __sunriset__( year, month, day, lon, lat, -6.0, 0, start, end )

/* This macro computes the start and end times of nautical twilight.    */
/* Nautical twilight starts/ends when the Sun's center is 12 degrees    */
/* below the horizon.                                                   */
#define nautical_twilight(year,month,day,lon,lat,start,end)  \
  __sunriset__( year, month, day, lon, lat, -12.0, 0, start, end )

/* This macro computes the start and end times of astronomical twilight.   */
/* Astronomical twilight starts/ends when the Sun's center is 18 degrees   */
/* below the horizon.                                                      */
#define astronomical_twilight(year,month,day,lon,lat,start,end)  \
  __sunriset__( year, month, day, lon, lat, -18.0, 0, start, end )

#endif // #ifndef WeatherDisplay_h
