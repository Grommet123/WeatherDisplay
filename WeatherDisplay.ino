////////////////////////////////////////////////////////////////////////
//    Weather Forecast Display                                       //
//           GK Grotsky                                              //
//            12/29/16                                               //
//     Based on the following:                                       //
// http://educ8s.tv/art-deco-weather-forecast-display/               //
//                                                                   //
// https://github.com/Grommet123/WeatherDisplay                      //
//                                                                   //
///////////////////////////////////////////////////////////////////////

#include "WeatherDisplay.h"
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>

WiFiClient client; // WIFI client object
String serverName = "api.openweathermap.org"; // Remote server we will connect to
String result; // WIFI buffer from NodeMCU ESP12E
bool night = false;
bool toggleDisplay = HIGH;
bool pastToggleDisplay = !toggleDisplay;

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST); // TFT display object

// Icon data (from WeatherIcons.c) for TFT display
extern unsigned char cloud[];
extern unsigned char thunder[];
extern unsigned char wind[];
/* Fill in these four variables within NetworkConnection.c
   with your ssid, password, APIKey and cityID */
extern const char* ssid; // SSID of local network
extern const char* password; // Password on network
extern const char* APIKey; // openweathermap.org API key
extern const char* cityID; // openweathermap.org city

void setup() {
#ifdef DEBUG
  Serial.begin(9600);
#endif

  pinMode(TOGGLEDISPLAY_SW, INPUT);
  pinMode(ALIVE_LED, OUTPUT);

  tft.initR(INITR_BLACKTAB);   // initialize the ST7735S chip, black tab
  tft.fillScreen(BLACK);

#ifdef DEBUG
  Serial.println("Connecting");
#endif
  byte maxLeghtOfParam;

  tft.setTextColor(YELLOW);
  tft.setTextSize(1);
  maxLeghtOfParam = min((int)sizeof("SW Version ") + (int)sizeof(VERSION), 22); // Limit characters
  tft.setCursor((11 - (maxLeghtOfParam / 2)) * 4, 5); // Center text
  tft.print("SW version ");
  tft.print(VERSION);
  maxLeghtOfParam = min((int)sizeof("HW Version ") + (int)sizeof(HWVERSION), 22); // Limit characters
  tft.setCursor((11 - (maxLeghtOfParam / 2)) * 4, 15); // Center text
  tft.print("HW version ");
  tft.print(HWVERSION);
  tft.setTextColor(WHITE);
  maxLeghtOfParam = min((int)sizeof("Connecting to"), 22); // Limit characters
  tft.setCursor((11 - (maxLeghtOfParam / 2)) * 4, 80); // Center text
  tft.print("Connecting to");
  maxLeghtOfParam = min((int)sizeof(ssid), 22); // Limit characters
  tft.setCursor((11 - (maxLeghtOfParam / 2)) * 4, 90); // Center text
  tft.print(ssid);
  maxLeghtOfParam = min((int)sizeof("Written by ") + (int)sizeof(CREDIT), 22); // Limit characters
  tft.setCursor((11 - (maxLeghtOfParam / 2)) * 4, 150); // Center text
  tft.setTextColor(YELLOW);
  tft.print("Written by ");
  tft.print(CREDIT);

  int startCounter = 0;

  WiFi.begin (ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
#ifdef DEBUG
    Serial.print(".");
#endif
    startCounter++;
    tft.setTextSize(1);
    tft.setTextColor(YELLOW);
    if (startCounter <= 1) {
      maxLeghtOfParam = min((int)sizeof("          "), 22); // Limit characters
      tft.setCursor((11 - (maxLeghtOfParam / 2)) * 4, 110); // Center text
    }
    tft.print(startCounter);
    if (startCounter >= 10) {
#ifdef DEBUG
      Serial.println("\nTimed out. Starting again...");
#endif
      return;
    }
    delay(1000);
  }
#ifdef DEBUG
  Serial.println("WiFi connected");
#endif
} // setup

void loop() {
  static int counter = GETDATACOUNT;
  bool flasher;

  toggleDisplay = digitalRead(TOGGLEDISPLAY_SW);
  if (toggleDisplay != pastToggleDisplay) {
    pastToggleDisplay = toggleDisplay;
    counter = GETDATACOUNT;
  }

  if (counter >= GETDATACOUNT) //Get new data
  {
    counter = 0;
    getWeatherData();
  }
  else {
    counter++;
    flasher = (counter % 2 == 0); // 0 or 1
    // Flash the alive LED so we know the system has not crashed
    analogWrite(ALIVE_LED, flasher * ALIVEBRIGHTNESS);
    // Seed random number generator
    randomSeed(counter + flasher);
    delay(1000);
#ifdef DEBUG
    if (counter % 60 == 0) { // Once per minutes
      int count = counter / 60;
      Serial.print("Time since last update = ");
      Serial.print(count);
      if ((count) > 1) {
        Serial.print(" minutes");
      }
      else {
        Serial.print(" minute");
      }
      Serial.print(" out of ");
      Serial.println(GETDATACOUNT / 60);
    }
#endif
  }
} // loop

void getWeatherData() //client function to send/receive GET request data.
{
#ifdef DEBUG
  Serial.println("Getting Weather Data");
#endif

  if (client.connect(serverName, PORT)) { // Starts client connection, checks for connection
    client.println("GET /data/2.5/forecast?id=" + String(cityID) + "&units=imperial&cnt=1&APPID=" + String(APIKey));
    client.println("Host: " + serverName);
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Connection: close");  // Close 1.1 persistent connection
    client.println();  // End of get request
  }
  else {
#ifdef DEBUG
    Serial.println("connection failed"); // Error message if no client connect
    Serial.println();
#endif
    return;
  }

  while (client.connected() && !client.available()) delay(1); // Waits for data

#ifdef DEBUG
  Serial.println("Waiting for data");
#endif

  while (client.connected() || client.available()) { // Connected or data available
    char c = client.read(); // Gets JSON data from WIFI buffer
    result = result + c;
  }
  client.stop(); // Stop client

#ifdef DEBUG
  Serial.println("Received data");
  Serial.println("Printing JSON data...");
  Serial.println(result); // Print out the JSON data
#endif
  result.replace('[', ' ');
  result.replace(']', ' ');

  // See the following for a description on JSON decoding:
  // https://randomnerdtutorials.com/decoding-and-encoding-json-with-arduino-or-esp8266/
  char jsonArray [result.length() + 1];
  result.toCharArray(jsonArray, sizeof(jsonArray));
  jsonArray[result.length() + 1] = '\0';
#ifdef ArduinoJsonVer // Version 6
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, jsonArray);

  String location = doc["city"]["name"];
  String temperature = doc["list"]["main"]["temp"];
  String humidity = doc["list"]["main"]["humidity"];
  String weather = doc["list"]["weather"]["main"];
  String description = doc["list"]["weather"]["description"];
  String idString = doc["list"]["weather"]["id"];
  String timeS = doc["list"]["dt_txt"];
  String latitude = doc["city"]["coord"]["lat"];
  String longitude = doc["city"]["coord"]["lon"];
  String windSpeed = doc["list"]["wind"]["speed"];
  String windDirection = doc["list"]["wind"]["deg"];
#else // Version 5
  StaticJsonBuffer<1024> json_buf;
  JsonObject &root = json_buf.parseObject(jsonArray);
  if (!root.success())
  {
#ifdef DEBUG
    Serial.println("parseObject() failed");
#endif
    return;
  }
  String location = root["city"]["name"];
  String temperature = root["list"]["main"]["temp"];
  String humidity = root["list"]["main"]["humidity"];
  String weather = root["list"]["weather"]["main"];
  String description = root["list"]["weather"]["description"];
  String idString = root["list"]["weather"]["id"];
  String timeS = root["list"]["dt_txt"];
  String latitude = root["city"]["coord"]["lat"];
  String longitude = root["city"]["coord"]["lon"];
  String windSpeed = root["list"]["wind"]["speed"];
  String windDirection = root["list"]["wind"]["deg"];
#endif // Version 5 or 6
  bool DST = false;
  int riseI, setI, monthI, dayI, yearI;
  String timeUTC = timeS;
  String timeRaw = timeUTC;
  timeUTC = timeUTC.substring(timeUTC.length() - 8, timeUTC.length()); // Strip out the time (XX:XX:XX)
  timeS = convertGMTTimeToLocal(timeS, latitude, longitude,
                                &riseI, &setI, &monthI, &dayI, &yearI, &DST);
  String dateS = String(monthI) + '/' + String(dayI) + '/' + String(yearI);

  int length = temperature.length();
  if (length == 5)
  {
    temperature.remove(length - 1);
  }

  String latitudeS = latitude;
  String longitudeS = longitude;
  String latDir, lonDir;
  if ((latitude.toFloat() >= 0)) {
    latDir = 'N';
    latitudeS = (' ' + latitudeS);
  }
  else {
    latDir = 'S';
    latitudeS.replace('-', ' ');
  }
  if ((longitude.toFloat() >= 0)) {
    lonDir = 'E';
    longitudeS = (' ' + longitudeS);
  }
  else {
    lonDir = 'W';
    longitudeS.replace('-', ' ');
  }
  latitudeS  = (latitudeS  + latDir + " Degs");
  longitudeS = (longitudeS + lonDir + " Degs");
#ifdef DEBUG
  Serial.println();
  Serial.println("Weather Display");
  Serial.println("SW version " + String(VERSION));
  Serial.println("HW version " + String(HWVERSION));
  Serial.println("Written by " + String(CREDIT));
  Serial.println("Remote server " + serverName);
  Serial.println("Raw UTC date/time " + timeRaw);
  Serial.println(dateS);
  Serial.println("Today is " + getDayOfWeek(dayOfWeek(yearI, monthI, dayI)));
  Serial.println((DST) ? "It's DST" : "It's ST");
  Serial.println((night) ? "It is Night time" : "It is Day time");
  Serial.println("City " + location);
  Serial.println("Weather Icon ID " + idString);
  Serial.println("Weather " + weather);
  Serial.println("Description " + description);
  Serial.println("Temperature " + temperature + " Degs Fahrenheit");
  Serial.println("Humidity " + humidity + "% RH");
  Serial.println("Wind speed " + windSpeed + " MPH");
  Serial.println("Wind direction " + windDirection + " Degs");
  Serial.println("Latitude" + latitudeS);
  Serial.println("Longitude" + longitudeS);
  Serial.println("Sunrise " + String(riseI) + "am");
  Serial.println("Sunset " + String(setI) + "pm");
  Serial.println("Time last updated " + timeS);
  Serial.println("UTC " + timeUTC);
#endif // #ifdef DEBUG

  clearScreen();
  if (toggleDisplay) {
    // Display the aux weather
    printAuxData(location,
                 latitudeS,
                 longitudeS,
                 temperature,
                 humidity,
                 description,
                 windSpeed,
                 windDirection,
                 timeS,
                 riseI,
                 setI,
                 idString.toInt(),
                 dateS,
                 timeUTC,
                 dayI,
                 monthI,
                 yearI,
                 DST,
                 timeRaw);
  }
  else {
    // Display the main weather
    printMainData(humidity,
                  temperature,
                  timeS,
                  idString.toInt(),
                  dateS,
                  dayI,
                  monthI,
                  yearI,
                  description,
                  DST);
  }
} // void getWeatherData()

// Displays the aux weather
void printAuxData(String location,
                  String latitudeS,
                  String longitudeS,
                  String temperature,
                  String humidity,
                  String description,
                  String windSpeed,
                  String windDirection,
                  String time,
                  int rise,
                  int set,
                  int weatherID,
                  String dateS,
                  String timeUTC,
                  int dayI,
                  int monthI,
                  int yearI,
                  bool DST,
                  String timeRaw)
{
  static long lastRandomNumber;
  long randomColor;
  long anotherRandomColor;
  unsigned int textColor[] = {
    BLUE,
    RED,
    GREEN,
    CYAN,
    MAGENTA,
    YELLOW,
    WHITE
  };

  // Get random number
  byte maxRandomNumber = sizeof(textColor) / sizeof(unsigned int);
  randomColor = random(maxRandomNumber);
  // If random number repeats, try again
  while (randomColor == lastRandomNumber) {
    randomColor = random(maxRandomNumber);
  }
  lastRandomNumber = randomColor;
  anotherRandomColor = random(maxRandomNumber);
  // If random number repeats first random number, try again
  while (anotherRandomColor == randomColor) {
    anotherRandomColor = random(maxRandomNumber);
  }
  tft.setTextColor(textColor[randomColor]);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.print("Location: ");
  tft.setTextColor(textColor[anotherRandomColor]);
  tft.println(location);
  tft.setTextColor(textColor[randomColor]);
  tft.print("Lat");
  tft.setTextColor(textColor[anotherRandomColor]);
  tft.println(latitudeS);
  tft.setTextColor(textColor[randomColor]);
  tft.print("Long");
  tft.setTextColor(textColor[anotherRandomColor]);
  tft.println(longitudeS);
  tft.setTextColor(textColor[randomColor]);
  tft.print("Temperature: ");
  tft.setTextColor(textColor[anotherRandomColor]);
  tft.print(temperature.toInt());
  tft.println(" F");
  tft.setTextColor(textColor[randomColor]);
  tft.print("Humidity: ");
  tft.setTextColor(textColor[anotherRandomColor]);
  tft.print(humidity.toInt());
  tft.println("% RH");
  tft.setTextColor(textColor[randomColor]);
  tft.print("Weather ID: ");
  tft.setTextColor(textColor[anotherRandomColor]);
  tft.println(weatherID);
  tft.setTextColor(textColor[randomColor]);
  tft.print("Sky: ");
  tft.setTextColor(textColor[anotherRandomColor]);
  if ((sizeof("Sky: ") + description.length()) >= 21 /*Length of line*/) description.replace(' ', '\n');
  tft.println(description);
  tft.setTextColor(textColor[randomColor]);
  tft.print("Wind Spd: ");
  tft.setTextColor(textColor[anotherRandomColor]);
  tft.print(windSpeed.toInt());
  tft.println(" Knots");
  tft.setTextColor(textColor[randomColor]);
  tft.print("Wind Dir: ");
  tft.setTextColor(textColor[anotherRandomColor]);
  tft.print(windDirection.toInt());
  tft.println(" Degs");
  tft.setTextColor(textColor[randomColor]);
  tft.print("Time LU: ");
  tft.setTextColor(textColor[anotherRandomColor]);
  tft.println(time);
  tft.setTextColor(textColor[randomColor]);
  tft.print("UTC: ");
  tft.setTextColor(textColor[anotherRandomColor]);
  tft.println(timeUTC);
  tft.setTextColor(textColor[randomColor]);
  tft.print("Date: ");
  tft.setTextColor(textColor[anotherRandomColor]);
  tft.println(dateS);
  tft.setTextColor(textColor[randomColor]);
  tft.print("Sunrise: ");
  tft.setTextColor(textColor[anotherRandomColor]);
  tft.print(rise);
  tft.println(":00am");
  tft.setTextColor(textColor[randomColor]);
  tft.print("Sunset: ");
  tft.setTextColor(textColor[anotherRandomColor]);
  tft.print(set);
  tft.println(":00pm");
  tft.setTextColor(textColor[randomColor]);
  tft.print("It's ");
  tft.setTextColor(textColor[anotherRandomColor]);
  tft.println((DST) ? "DST" : "ST");
  tft.setTextColor(textColor[randomColor]);
  tft.print("Today is ");
  tft.setTextColor(textColor[anotherRandomColor]);
  tft.println(getDayOfWeek(dayOfWeek(yearI, monthI, dayI)));
  tft.setTextColor(textColor[randomColor]);
  tft.print("It is ");
  tft.setTextColor(textColor[anotherRandomColor]);
  tft.println((night) ? "Night time" : "Day time");
  tft.setTextColor(textColor[randomColor]);
  tft.print("Raw UTC ");
  tft.setTextColor(textColor[anotherRandomColor]);
  if ((sizeof("Raw UTC ") + timeRaw.length()) >= 21 /*Length of line*/) timeRaw.replace(' ', '\n');
  tft.print(timeRaw);
}

// Displays the main weather
void printMainData(String humidityString,
                   String temperature,
                   String time,
                   int weatherID,
                   String dateS,
                   int dayI,
                   int monthI,
                   int yearI,
                   String description,
                   bool DST)
{
  byte maxLeghtOfParam;
  String isDST;

  tft.setTextColor(YELLOW);
  tft.setTextSize(1);

  (DST) ? isDST = " DST" : isDST = " ST";
  maxLeghtOfParam = min((int)sizeof("LU ") + (int)time.length() + (int)isDST.length(), 22); // Limit characters
  tft.setCursor((11 - (maxLeghtOfParam / 2)) * 4, 5); // Center text
  tft.print("LU " + time + isDST);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(27, 20);
  tft.print(temperature);
  tft.setTextSize(1);
  tft.print("o");
  tft.setTextSize(2);
  tft.print(" F");

  printWeatherIcon(weatherID);

  tft.setTextColor(YELLOW);
  tft.setTextSize(1);
  maxLeghtOfParam = min((int)description.length(), 22); // Limit characters
  tft.setCursor((11 - (maxLeghtOfParam / 2)) * 4, 120); // Center text
  tft.print(description);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(27, 132);
  tft.print(humidityString);
  tft.print("% RH");
  tft.setTextColor(YELLOW);
  tft.setTextSize(1);
  maxLeghtOfParam = min((int)dateS.length(), 22); // Limit characters
  String theDayOfWeek = getDayOfWeek(dayOfWeek(yearI, monthI, dayI));
  maxLeghtOfParam += min((int)theDayOfWeek.length() + 1, 22); // Limit characters
  tft.setCursor((11 - (maxLeghtOfParam / 2)) * 4, 150); // Center text
  tft.print(theDayOfWeek + " " + dateS);
}

void printWeatherIcon(int id)
{
  switch (id)
  {
    case 800: drawClearWeather(); break;
    case 801: drawFewClouds(); break;
    case 802: drawFewClouds(); break;
    case 803: drawFewClouds(); break;
    case 804: drawCloud(); break;

    case 200: drawThunderstorm(); break;
    case 201: drawThunderstorm(); break;
    case 202: drawThunderstorm(); break;
    case 210: drawThunderstorm(); break;
    case 211: drawThunderstorm(); break;
    case 212: drawThunderstorm(); break;
    case 221: drawThunderstorm(); break;
    case 230: drawThunderstorm(); break;
    case 231: drawThunderstorm(); break;
    case 232: drawThunderstorm(); break;

    case 300: drawLightRain(); break;
    case 301: drawLightRain(); break;
    case 302: drawLightRain(); break;
    case 310: drawLightRain(); break;
    case 311: drawLightRain(); break;
    case 312: drawLightRain(); break;
    case 313: drawLightRain(); break;
    case 314: drawLightRain(); break;
    case 321: drawLightRain(); break;

    case 500: drawLightRainWithSunOrMoon(); break;
    case 501: drawModerateRain(); break;
    case 502: drawLightRainWithSunOrMoon(); break;
    case 503: drawLightRainWithSunOrMoon(); break;
    case 504: drawLightRainWithSunOrMoon(); break;
    case 511: drawLightRain(); break;
    case 520: drawModerateRain(); break;
    case 521: drawModerateRain(); break;
    case 522: drawHeavyRain(); break;
    case 531: drawHeavyRain(); break;

    case 600: drawLightSnowfall(); break;
    case 601: drawModerateSnowfall(); break;
    case 602: drawHeavySnowfall(); break;
    case 611: drawLightSnowfall(); break;
    case 612: drawLightSnowfall(); break;
    case 615: drawLightSnowfall(); break;
    case 616: drawLightSnowfall(); break;
    case 620: drawLightSnowfall(); break;
    case 621: drawModerateSnowfall(); break;
    case 622: drawHeavySnowfall(); break;

    case 701: drawFog(); break;
    case 711: drawFog(); break;
    case 721: drawFog(); break;
    case 731: drawFog(); break;
    case 741: drawFog(); break;
    case 751: drawFog(); break;
    case 761: drawFog(); break;
    case 762: drawFog(); break;
    case 771: drawFog(); break;
    case 781: drawFog(); break;

    default: break;
  }
}

String convertGMTTimeToLocal(String timeS, String latitude, String longitude,
                             int* riseI, int* setI, int* monthI, int* dayI, int* yearI, bool* DST)
{
  double rise, set;

  int length = timeS.length();
  String yearS = timeS.substring(0, 4);
  int year = yearS.toInt();
  String monthS = timeS.substring(5, 7);
  int month = monthS.toInt();
  String dayS = timeS.substring(8, 10);
  int day = dayS.toInt();

  timeS = timeS.substring(length - 8, length - 6); // Strip out the hours
  int time = timeS.toInt();

#ifdef ERROROFFSET
  // For some reason, openweathermap.org time is off by some (???)
  // hours from UTC. Need to compensate
  int TimeErrorOffset = -3;
  if (((time > 18) && (time < 21)) ||
      (time == 12)) {
    time += (TimeErrorOffset + 2);
  }
  else if (time == 0) {
    time += TimeErrorOffset;
    convertDate((double)longitude.toFloat(), &year, &month, &day);
  }
  else {
    time += TimeErrorOffset;
  }
#endif

  // Convert UTC time & date to local time & date
  *DST = convertToLocal(&time, &year, &month,
                        &day, (double)longitude.toFloat(), true); // true means date and DST conversion

  *monthI = month;
  *dayI = day;
  *yearI = year;

  // Get sunrise and sunset time in UTC
  int rs = sun_rise_set(year, month, day,
                        (double)longitude.toFloat(),
                        (double)latitude.toFloat(),
                        &rise, &set);
  *riseI = (int)round(rise);
  *setI = (int)round(set);

  // Convert UTC "sunrise" time to local "sunrise" time
  (void) convertToLocal(riseI, &year, &month,
                        &day, (double)longitude.toFloat(), true); // true means date and DST conversion

  // Convert UTC "sunset" time to local "sunset" time
  (void) convertToLocal(setI, &year, &month,
                        &day, (double)longitude.toFloat(), true); // true means date and DST conversion

  // Determine night or day
  night = (time >= *setI || time <= *riseI);

  if (*riseI >= 12) { // Convert to 12 hour format
    if (*riseI > 12) *riseI -= 12;
  }
  if (*setI >= 12) { // Convert to 12 hour format
    if (*setI > 12) *setI -= 12;
  }

  char AMPM[] = "am";
  if (time >= 12) { // Convert to 12 hour format
    if (time > 12) time -= 12;
    strcpy(AMPM, "pm");
  }

  timeS = String(time) + ":00" + AMPM;

  return timeS;
}

void clearScreen()
{
  tft.fillScreen(BLACK);
}

void drawClearWeather()
{
  if (night)
  {
    drawTheMoon();
  } else
  {
    drawTheSun();
  }
}

void drawFewClouds()
{
  if (night)
  {
    drawCloudAndTheMoon();
  } else
  {
    drawCloudWithSun();
  }
}

void drawTheSun()
{
  tft.fillCircle(64, 80, 26, YELLOW);
}

void drawTheFullMoon()
{
  tft.fillCircle(64, 80, 26, GREY);
}

void drawTheMoon()
{
  tft.fillCircle(64, 80, 26, GREY);
  tft.fillCircle(75, 73, 26, BLACK);
}

void drawCloud()
{
  tft.drawBitmap(0, 35, cloud, 128, 90, GREY);
}

void drawCloudWithSun()
{
  tft.fillCircle(73, 70, 20, YELLOW);
  tft.drawBitmap(0, 36, cloud, 128, 90, BLACK);
  tft.drawBitmap(0, 40, cloud, 128, 90, GREY);
}

void drawLightRainWithSunOrMoon()
{
  if (night)
  {
    drawCloudTheMoonAndRain();
  } else
  {
    drawCloudSunAndRain();
  }
}

void drawLightRain()
{
  tft.drawBitmap(0, 35, cloud, 128, 90, GREY);
  tft.fillRoundRect(50, 105, 3, 13, 1, BLUE);
  tft.fillRoundRect(65, 105, 3, 13, 1, BLUE);
  tft.fillRoundRect(80, 105, 3, 13, 1, BLUE);
}

void drawModerateRain()
{
  tft.drawBitmap(0, 35, cloud, 128, 90, GREY);
  tft.fillRoundRect(50, 105, 3, 15, 1, BLUE);
  tft.fillRoundRect(57, 102, 3, 15, 1, BLUE);
  tft.fillRoundRect(65, 105, 3, 15, 1, BLUE);
  tft.fillRoundRect(72, 102, 3, 15, 1, BLUE);
  tft.fillRoundRect(80, 105, 3, 15, 1, BLUE);
}

void drawHeavyRain()
{
  tft.drawBitmap(0, 35, cloud, 128, 90, GREY);
  tft.fillRoundRect(43, 102, 3, 15, 1, BLUE);
  tft.fillRoundRect(50, 105, 3, 15, 1, BLUE);
  tft.fillRoundRect(57, 102, 3, 15, 1, BLUE);
  tft.fillRoundRect(65, 105, 3, 15, 1, BLUE);
  tft.fillRoundRect(72, 102, 3, 15, 1, BLUE);
  tft.fillRoundRect(80, 105, 3, 15, 1, BLUE);
  tft.fillRoundRect(87, 102, 3, 15, 1, BLUE);
}

void drawThunderstorm()
{
  tft.drawBitmap(0, 40, thunder, 128, 90, YELLOW);
  tft.drawBitmap(0, 35, cloud, 128, 90, GREY);
  tft.fillRoundRect(48, 102, 3, 15, 1, BLUE);
  tft.fillRoundRect(55, 102, 3, 15, 1, BLUE);
  tft.fillRoundRect(74, 102, 3, 15, 1, BLUE);
  tft.fillRoundRect(82, 102, 3, 15, 1, BLUE);
}

void drawLightSnowfall()
{
  tft.drawBitmap(0, 30, cloud, 128, 90, GREY);
  tft.fillCircle(50, 100, 3, GREY);
  tft.fillCircle(65, 103, 3, GREY);
  tft.fillCircle(82, 100, 3, GREY);
}

void drawModerateSnowfall()
{
  tft.drawBitmap(0, 35, cloud, 128, 90, GREY);
  tft.fillCircle(50, 105, 3, GREY);
  tft.fillCircle(50, 115, 3, GREY);
  tft.fillCircle(65, 108, 3, GREY);
  tft.fillCircle(65, 118, 3, GREY);
  tft.fillCircle(82, 105, 3, GREY);
  tft.fillCircle(82, 115, 3, GREY);
}

void drawHeavySnowfall()
{
  tft.drawBitmap(0, 35, cloud, 128, 90, GREY);
  tft.fillCircle(40, 105, 3, GREY);
  tft.fillCircle(52, 105, 3, GREY);
  tft.fillCircle(52, 115, 3, GREY);
  tft.fillCircle(65, 108, 3, GREY);
  tft.fillCircle(65, 118, 3, GREY);
  tft.fillCircle(80, 105, 3, GREY);
  tft.fillCircle(80, 115, 3, GREY);
  tft.fillCircle(92, 105, 3, GREY);
}

void drawCloudSunAndRain()
{
  tft.fillCircle(73, 70, 20, YELLOW);
  tft.drawBitmap(0, 32, cloud, 128, 90, BLACK);
  tft.drawBitmap(0, 35, cloud, 128, 90, GREY);
  tft.fillRoundRect(50, 105, 3, 13, 1, BLUE);
  tft.fillRoundRect(65, 105, 3, 13, 1, BLUE);
  tft.fillRoundRect(80, 105, 3, 13, 1, BLUE);
}

void drawCloudAndTheMoon()
{
  tft.fillCircle(94, 60, 18, GREY);
  tft.fillCircle(105, 53, 18, BLACK);
  tft.drawBitmap(0, 32, cloud, 128, 90, BLACK);
  tft.drawBitmap(0, 35, cloud, 128, 90, GREY);
}

void drawCloudTheMoonAndRain()
{
  tft.fillCircle(94, 60, 18, GREY);
  tft.fillCircle(105, 53, 18, BLACK);
  tft.drawBitmap(0, 32, cloud, 128, 90, BLACK);
  tft.drawBitmap(0, 35, cloud, 128, 90, GREY);
  tft.fillRoundRect(50, 105, 3, 11, 1, BLUE);
  tft.fillRoundRect(65, 105, 3, 11, 1, BLUE);
  tft.fillRoundRect(80, 105, 3, 11, 1, BLUE);
}

void drawWind()
{
  tft.drawBitmap(0, 35, wind, 128, 90, GREY);
}

void drawFog()
{
  tft.fillRoundRect(45, 60, 40, 4, 1, GREY);
  tft.fillRoundRect(40, 70, 50, 4, 1, GREY);
  tft.fillRoundRect(35, 80, 60, 4, 1, GREY);
  tft.fillRoundRect(40, 90, 50, 4, 1, GREY);
  tft.fillRoundRect(45, 100, 40, 4, 1, GREY);
}

void clearIcon()
{
  tft.fillRect(0, 40, 128, 100, BLACK);
}

/* Helper functions for determing local time & date (from UTC)
  The following three functions ripped off from Electrical Engineering Stack Exchange
  http://electronics.stackexchange.com/questions/66285/how-to-calculate-day-of-the-week-for-rtc
   Returns the number of days to the start of the specified year, taking leap
   years into account, but not the shift from the Julian calendar to the
   Gregorian calendar. Instead, it is as though the Gregorian calendar is
   extrapolated back in time to a hypothetical "year zero".
*/
uint16_t leap(uint16_t year)
{
  return year * 365 + (year / 4) - (year / 100) + (year / 400);
}
/* Returns a number representing the number of days since March 1 in the
   hypothetical year 0, not counting the change from the Julian calendar
   to the Gregorian calendar that occurred in the 16th century. This
   algorithm is loosely based on a function known as "Zeller's Congruence".
   This number MOD 7 gives the day of week, where 0 = Monday and 6 = Sunday.
*/
uint16_t zeller(uint16_t year, uint8_t month, uint8_t day)
{
  year += ((month + 9) / 12) - 1;
  month = (month + 9) % 12;
  return leap(year) + month * 30 + ((6 * month + 5) / 10) + day + 1;
}

// Returns the day of week for a given date.
uint8_t dayOfWeek(uint16_t year, uint8_t month, uint8_t day)
{
  return (zeller(year, month, day) % 7) + 1;
}

/* Ripped off from Stackoverflow
  http://stackoverflow.com/questions/5590429/calculating-daylight-saving-time-from-only-date
  Check to see if it's Daylight Savings Time (DST)
*/
bool IsDST(uint8_t day, uint8_t month , uint8_t DOW)
{
  // If and when permanent DST takes affect
#ifdef PERMANENT_DST
  return true;
#else
  // Make Day of Week (DOW) match with what Stackoverflow suggests
  // for DOW (Sunday = 0 to Saturday = 6)
  switch (DOW)
  {
    case 6:  DOW = 0; break; // Sun
    case 7:  DOW = 1; break; // Mon
    case 1:  DOW = 2; break; // Tue
    case 2:  DOW = 3; break; // Wed
    case 3:  DOW = 4; break; // Thu
    case 4:  DOW = 5; break; // Fri
    case 5:  DOW = 6; break; // Sat
    default: break;
  }
  // January, February, and December are out
  if (month < 3 || month > 11) {
    return false;
  }
  // April to October are in
  if (month > 3 && month < 11) {
    return true;
  }
  int8_t previousSunday = (int8_t)(day - DOW);
  // In march, we are DST if our previous Sunday was on or after the 8th
  if (month == 3) {
    return previousSunday >= 8;
  }
  // In November we must be before the first Sunday to be DST
  // That means the previous Sunday must be before the 1st
  return previousSunday <= 0;
#endif // #ifdef PERMANENT_DTS
}

/* Convert UTC time and date to local time and date
   Difference between UTC time/date (at Greenwich) and local time/date is 15 minutes
   per 1 degree of longitude. See the following:
   http://www.edaboard.com/thread101516.html
*/
bool convertToLocal(int* hour, int* year, int* month,
                    int* day, double lon, bool convertDate) {

  uint8_t DaysAMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  // Get Day of Week (DOW)
  uint8_t DOW = dayOfWeek(*year, *month, *day);
  // Get Daylight Saving Time (DST) or Standard Time (ST)
  bool DST = IsDST(*day, *month, DOW);
  // Compute local time (hours)
  int8_t UTCOffset = (int8_t)round((lon / 15.0d)); // UTC offset
  if (UTCOffset < 0) {
    // West of Greenwich, subtract
    UTCOffset = abs(UTCOffset); // Make offset positive
    if (DST && convertDate) --UTCOffset; // Compensate for DST
    if (*hour <= UTCOffset) *hour += 24;
    *hour -= UTCOffset; // Subtract offset
  }
  else {
    // East of Greenwich, add
    if (DST && convertDate) --UTCOffset; // Compensate for DST
    if (*hour <= UTCOffset) *hour += 24;
    *hour += UTCOffset; // Add offset
  }
  // Convert date if convertDate flag is set
  // Portions of the following code (with some modifications) ripped off from Instructables
  // http://www.instructables.com/id/GPS-time-UTC-to-local-time-conversion-using-Arduin/?ALLSTEPS
  if (convertDate) {
    if ((24 - *hour) <= UTCOffset) { // A new UTC day started
      if (*year % 4 == 0) DaysAMonth[1] = 29; //leap year check (the simple method)
      if (*hour < 24) {
        *day -= 1;
        if (*day < 1) {
          if (*month == 1) {
            *month = 12;
            *year -= 1;
          } // if (*month == 1)
          else {
            *month -= 1;
          }
          *day = DaysAMonth[*month - 1];
        } // if (*day < 1)
      } // if (*hour < 24)
      else if (*hour >= 24) {
        *day += 1;
        if (*day > DaysAMonth[*month - 1]) {
          *day = 1;
          *month += 1;
          if (*month > 12) *year += 1;
        } // if (*day > DaysAMonth[*month - 1])
      } // if (*hour >= 24)
    } // if ((24 - *hour) <= UTCOffset)
  } // if (convertDate)
  return (DST);
}

void convertDate(double lon, int* year, int* month, int* day) {

  uint8_t DaysAMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  if (*year % 4 == 0) DaysAMonth[1] = 29; //leap year check (the simple method)
  if (round((lon / 15.0d)) < 0.0d) { // East of Greenwich, subtract
    *day -= 1;
    if (*day < 1) {
      if (*month == 1) {
        *month = 12;
        *year -= 1;
      } // if (*month == 1)
      else {
        *month -= 1;
      }
      *day = DaysAMonth[*month - 1];
    } // if (*day < 1)
  } // if (round((lon / 15.0d)) < 0.0d)
  else { // West of Greenwich, add
    *day += 1;
    if (*day > DaysAMonth[*month - 1]) {
      *day = 1;
      *month += 1;
      if (*month > 12) *year += 1;
    } // if (*day > DaysAMonth[*month - 1])
  } // if (round((lon / 15.0d)) < 0.0d)
}

// Get the day of the week
String getDayOfWeek(uint8_t day)
{
  String DOW;
  if (day == 7) day = 0;
  switch (day)
  {
    case 0:  DOW = "Monday"; break;
    case 1:  DOW = "Tuesday"; break;
    case 2:  DOW = "Wensday"; break;
    case 3:  DOW = "Thursday"; break;
    case 4:  DOW = "Friday"; break;
    case 5:  DOW = "Saturday" ; break;
    case 6:  DOW = "Sunday"; break;
    default: DOW = "Error" ;
  }
  return (DOW);
}

/*
  Computes Sun rise/set times, start/end of twilight, and
  the length of the day at any date and latitude
  Written as DAYLEN.C, 1989-08-16
  Modified to SUNRISET.C, 1992-12-01
  (c) Paul Schlyter, 1989, 1992
  Released to the public domain by Paul Schlyter, December 1992
  http://stjarnhimlen.se/comp/sunriset.c
*/

/* The "workhorse" function for sun rise/set times */
int __sunriset__( int year, int month, int day, double lon, double lat,
                  double altit, int upper_limb, double * trise, double * tset )
/***************************************************************************/
/* Note: year,month,date = calendar date, 1801-2099 only.             */
/*       Eastern longitude positive, Western longitude negative       */
/*       Northern latitude positive, Southern latitude negative       */
/*       The longitude value IS critical in this function!            */
/*       altit = the altitude which the Sun should cross              */
/*               Set to -35/60 degrees for rise/set, -6 degrees       */
/*               for civil, -12 degrees for nautical and -18          */
/*               degrees for astronomical twilight.                   */
/*         upper_limb: non-zero -> upper limb, zero -> center         */
/*               Set to non-zero (e.g. 1) when computing rise/set     */
/*               times, and to zero when computing start/end of       */
/*               twilight.                                            */
/*        *rise = where to store the rise time                        */
/*        *set  = where to store the set  time                        */
/*                Both times are relative to the specified altitude,  */
/*                and thus this function can be used to compute       */
/*                various twilight times, as well as rise/set times   */
/* Return value:  0 = sun rises/sets this day, times stored at        */
/*                    *trise and *tset.                               */
/*               +1 = sun above the specified "horizon" 24 hours.     */
/*                    *trise set to time when the sun is at south,    */
/*                    minus 12 hours while *tset is set to the south  */
/*                    time plus 12 hours. "Day" length = 24 hours     */
/*               -1 = sun is below the specified "horizon" 24 hours   */
/*                    "Day" length = 0 hours, *trise and *tset are    */
/*                    both set to the time when the sun is at south.  */
/*                                                                    */
/**********************************************************************/
{
  double  d,  /* Days since 2000 Jan 0.0 (negative before) */
          sr,         /* Solar distance, astronomical units */
          sRA,        /* Sun's Right Ascension */
          sdec,       /* Sun's declination */
          sradius,    /* Sun's apparent radius */
          t,          /* Diurnal arc */
          tsouth,     /* Time when Sun is at south */
          sidtime;    /* Local sidereal time */

  int rc = 0; /* Return cde from function - usually 0 */

  /* Compute d of 12h local mean solar time */
  d = days_since_2000_Jan_0(year, month, day) + 0.5 - lon / 360.0;

  /* Compute the local sidereal time of this moment */
  sidtime = revolution( GMST0(d) + 180.0 + lon );

  /* Compute Sun's RA, Decl and distance at this moment */
  sun_RA_dec( d, &sRA, &sdec, &sr );

  /* Compute time when Sun is at south - in hours UT */
  tsouth = 12.0 - rev180(sidtime - sRA) / 15.0;

  /* Compute the Sun's apparent radius in degrees */
  sradius = 0.2666 / sr;

  /* Do correction to upper limb, if necessary */
  if ( upper_limb )
    altit -= sradius;

  /* Compute the diurnal arc that the Sun traverses to reach */
  /* the specified altitude altit: */
  {
    double cost;
    cost = ( sind(altit) - sind(lat) * sind(sdec) ) /
           ( cosd(lat) * cosd(sdec) );
    if ( cost >= 1.0 )
      rc = -1, t = 0.0;       /* Sun always below altit */
    else if ( cost <= -1.0 )
      rc = +1, t = 12.0;      /* Sun always above altit */
    else
      t = acosd(cost) / 15.0; /* The diurnal arc, hours */
  }

  /* Store rise and set times - in hours UT */
  *trise = tsouth - t;
  *tset  = tsouth + t;

  return rc;
}  /* __sunriset__ */

/* The "workhorse" function */
double __daylen__( int year, int month, int day, double lon, double lat,
                   double altit, int upper_limb )
/**********************************************************************/
/* Note: year,month,date = calendar date, 1801-2099 only.             */
/*       Eastern longitude positive, Western longitude negative       */
/*       Northern latitude positive, Southern latitude negative       */
/*       The longitude value is not critical. Set it to the correct   */
/*       longitude if you're picky, otherwise set to to, say, 0.0     */
/*       The latitude however IS critical - be sure to get it correct */
/*       altit = the altitude which the Sun should cross              */
/*               Set to -35/60 degrees for rise/set, -6 degrees       */
/*               for civil, -12 degrees for nautical and -18          */
/*               degrees for astronomical twilight.                   */
/*         upper_limb: non-zero -> upper limb, zero -> center         */
/*               Set to non-zero (e.g. 1) when computing day length   */
/*               and to zero when computing day+twilight length.      */
/**********************************************************************/
{
  double  d,  /* Days since 2000 Jan 0.0 (negative before) */
          obl_ecl,    /* Obliquity (inclination) of Earth's axis */
          sr,         /* Solar distance, astronomical units */
          slon,       /* True solar longitude */
          sin_sdecl,  /* Sine of Sun's declination */
          cos_sdecl,  /* Cosine of Sun's declination */
          sradius,    /* Sun's apparent radius */
          t;          /* Diurnal arc */

  /* Compute d of 12h local mean solar time */
  d = days_since_2000_Jan_0(year, month, day) + 0.5 - lon / 360.0;

  /* Compute obliquity of ecliptic (inclination of Earth's axis) */
  obl_ecl = 23.4393 - 3.563E-7 * d;

  /* Compute Sun's ecliptic longitude and distance */
  sunpos( d, &slon, &sr );

  /* Compute sine and cosine of Sun's declination */
  sin_sdecl = sind(obl_ecl) * sind(slon);
  cos_sdecl = sqrt( 1.0 - sin_sdecl * sin_sdecl );

  /* Compute the Sun's apparent radius, degrees */
  sradius = 0.2666 / sr;

  /* Do correction to upper limb, if necessary */
  if ( upper_limb )
    altit -= sradius;

  /* Compute the diurnal arc that the Sun traverses to reach */
  /* the specified altitude altit: */
  {
    double cost;
    cost = ( sind(altit) - sind(lat) * sin_sdecl ) /
           ( cosd(lat) * cos_sdecl );
    if ( cost >= 1.0 )
      t = 0.0;                      /* Sun always below altit */
    else if ( cost <= -1.0 )
      t = 24.0;                     /* Sun always above altit */
    else  t = (2.0 / 15.0) * acosd(cost); /* The diurnal arc, hours */
  }
  return t;
}  /* __daylen__ */

/* This function computes the Sun's position at any instant */
void sunpos( double d, double * lon, double * r )
/******************************************************/
/* Computes the Sun's ecliptic longitude and distance */
/* at an instant given in d, number of days since     */
/* 2000 Jan 0.0.  The Sun's ecliptic latitude is not  */
/* computed, since it's always very near 0.           */
/******************************************************/
{
  double M,         /* Mean anomaly of the Sun */
         w,         /* Mean longitude of perihelion */
         /* Note: Sun's mean longitude = M + w */
         e,         /* Eccentricity of Earth's orbit */
         E,         /* Eccentric anomaly */
         x, y,      /* x, y coordinates in orbit */
         v;         /* True anomaly */

  /* Compute mean elements */
  M = revolution( 356.0470 + 0.9856002585 * d );
  w = 282.9404 + 4.70935E-5 * d;
  e = 0.016709 - 1.151E-9 * d;

  /* Compute true longitude and radius vector */
  E = M + e * RADEG * sind(M) * ( 1.0 + e * cosd(M) );
  x = cosd(E) - e;
  y = sqrt( 1.0 - e * e ) * sind(E);
  *r = sqrt( x * x + y * y );          /* Solar distance */
  v = atan2d( y, x );                  /* True anomaly */
  *lon = v + w;                        /* True solar longitude */
  if ( *lon >= 360.0 )
    *lon -= 360.0;                   /* Make it 0..360 degrees */
}

void sun_RA_dec( double d, double * RA, double * dec, double * r )
/******************************************************/
/* Computes the Sun's equatorial coordinates RA, Decl */
/* and also its distance, at an instant given in d,   */
/* the number of days since 2000 Jan 0.0.             */
/******************************************************/
{
  double lon, obl_ecl, x, y, z;

  /* Compute Sun's ecliptical coordinates */
  sunpos( d, &lon, r );

  /* Compute ecliptic rectangular coordinates (z=0) */
  x = *r * cosd(lon);
  y = *r * sind(lon);

  /* Compute obliquity of ecliptic (inclination of Earth's axis) */
  obl_ecl = 23.4393 - 3.563E-7 * d;

  /* Convert to equatorial rectangular coordinates - x is unchanged */
  z = y * sind(obl_ecl);
  y = y * cosd(obl_ecl);

  /* Convert to spherical coordinates */
  *RA = atan2d( y, x );
  *dec = atan2d( z, sqrt(x * x + y * y) );

}  /* sun_RA_dec */

/******************************************************************/
/* This function reduces any angle to within the first revolution */
/* by subtracting or adding even multiples of 360.0 until the     */
/* result is >= 0.0 and < 360.0                                   */
/******************************************************************/
#define INV360    ( 1.0 / 360.0 )

double revolution( double x )
/*****************************************/
/* Reduce angle to within 0..360 degrees */
/*****************************************/
{
  return ( x - 360.0 * floor( x * INV360 ) );
}  /* revolution */

double rev180( double x )
/*********************************************/
/* Reduce angle to within +180..+180 degrees */
/*********************************************/
{
  return ( x - 360.0 * floor( x * INV360 + 0.5 ) );
}  /* revolution */


/*******************************************************************/
/* This function computes GMST0, the Greenwich Mean Sidereal Time  */
/* at 0h UT (i.e. the sidereal time at the Greenwhich meridian at  */
/* 0h UT).  GMST is then the sidereal time at Greenwich at any     */
/* time of the day.  I've generalized GMST0 as well, and define it */
/* as:  GMST0 = GMST - UT  --  this allows GMST0 to be computed at */
/* other times than 0h UT as well.  While this sounds somewhat     */
/* contradictory, it is very practical:  instead of computing      */
/* GMST like:                                                      */
/*                                                                 */
/*  GMST = (GMST0) + UT * (366.2422/365.2422)                      */
/*                                                                 */
/* where (GMST0) is the GMST last time UT was 0 hours, one simply  */
/* computes:                                                       */
/*                                                                 */
/*  GMST = GMST0 + UT                                              */
/*                                                                 */
/* where GMST0 is the GMST "at 0h UT" but at the current moment!   */
/* Defined in this way, GMST0 will increase with about 4 min a     */
/* day.  It also happens that GMST0 (in degrees, 1 hr = 15 degr)   */
/* is equal to the Sun's mean longitude plus/minus 180 degrees!    */
/* (if we neglect aberration, which amounts to 20 seconds of arc   */
/* or 1.33 seconds of time)                                        */
/*                                                                 */
/*******************************************************************/

double GMST0( double d )
{
  double sidtim0;
  /* Sidtime at 0h UT = L (Sun's mean longitude) + 180.0 degr  */
  /* L = M + w, as defined in sunpos().  Since I'm too lazy to */
  /* add these numbers, I'll let the C compiler do it for me.  */
  /* Any decent C compiler will add the constants at compile   */
  /* time, imposing no runtime or code overhead.               */
  sidtim0 = revolution( ( 180.0 + 356.0470 + 282.9404 ) +
                        ( 0.9856002585 + 4.70935E-5 ) * d );
  return sidtim0;
}  /* GMST0 */
