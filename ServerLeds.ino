#define FASTLED_ESP8266_RAW_PIN_ORDER
#include "FastLED.h"

#define NUM_LEDS 300
#define DATA_PIN D1
#define led 13

CRGB leds[NUM_LEDS];

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>

const char* customssid = "led";
const int binterval = 10;
ESP8266WebServer httpServer(80);

int brightness = 255;

struct credential {
  String ssid;
  String pass;
};

int connected = 0;


//bool tryNetworks(){
//  WiFi.mode(WIFI_STA);
//  int n = WiFi.scanNetworks();
//  Serial.println("scan done");
//  if (n == 0)
//    Serial.println("no networks found");
//  else
//  {
//    Serial.print(n);
//    Serial.println("networks found");
//    for (int i = 0; i < n; ++i) {
//      Serial.print("Trying network:");
//      Serial.println(WiFi.SSID(i));
//      for (int j=0; j< ssids.length; j++) {
//      // Print SSID and RSSI for each network found
//        if (WiFi.SSID(i) == ssids[j]){
//          WiFi.mode(WIFI_STA);
//          WiFi.begin(ssid, password);
//          if (WiFi.waitForConnectResult() == WL_CONNECTED) {
//            Serial.println("Connection Established!...");
//            return true;
//          }
//        }
//      }
//     }
//  }
//  Serial.println("Could not connect to any network");
//  return false
//}

bool connect(struct credential cred) {
  WiFi.mode(WIFI_STA);
  if (cred.ssid == "" && cred.pass == "") return false;
  Serial.println("Trying to connect to " + cred.ssid + " with " + cred.pass);
  if (cred.ssid.length() < 1) {
    return false;
  }
  // Serial.println("Length " + cred.ssid.length());
  WiFi.begin(cred.ssid.c_str(), cred.pass.c_str());
  int c = 0;
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Ready");
      Serial.println("");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      return true;
    }
    delay(500);
    // Serial.print(WiFi.status());
    Serial.print("..");
    c++;
  }
  Serial.println("Connect timed out");
  return false;
}

void setCredentials(struct credential cred) {
  String qsid = cred.ssid;
  Serial.println("");
  Serial.println("SSID: " + qsid);
  String qpass = cred.pass;
  Serial.println("PASS: " + qpass);
  Serial.println("");

  Serial.println("writing eeprom ssid:");
  EEPROM.write(0, qsid.length());
  EEPROM.write(1, qpass.length());
  int j = 2;
  for (int i = 0; i < qsid.length(); ++i) {
    EEPROM.write(j, qsid[i]);
    Serial.print("Wrote: ");
    Serial.println(qsid[i]);
    j++;
  }
  Serial.println("writing eeprom pass:");
  for (int i = 0; i < qpass.length(); ++i) {
    EEPROM.write(j, qpass[i]);
    Serial.print("Wrote: ");
    Serial.println(qpass[i]);
    j++;
  }
  EEPROM.commit();
}

void handleSetCredentials() {
  struct credential cred;
  cred.ssid = "";
  cred.pass = "";

  cred.ssid = httpServer.arg("ssid");
  cred.pass = httpServer.arg("pass");

  if (cred.ssid != "" && cred.pass != "") {
    setCredentials(cred);
    String out = "Credentials, set as: " + cred.ssid + " : " + cred.pass;

    httpServer.send ( 200, "text/html", out );
    delay(2000);
    ESP.reset();
  } else {
    httpServer.send(400, "text/html", 
      " <html><head>\
        <meta http-equiv=\"refresh\" content=\"1, /credentials\">\
        <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
       </head>\
       <body>\
        <p>Invalid credentials</p>\
       </body></html> ");
  }
}

void handleCredentials() {
  digitalWrite ( led, 1 );
  char temp[2000];
  struct credential cred = getCredentials();
  char ssid[30];
  cred.ssid.toCharArray(ssid, 30);

  snprintf ( temp, 2000,

             "<html>\
    <head>\
      <title>ESP8266</title>\
      <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
      <style>\
        body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088;\
               font-size: 1em; margin: 0; padding: 0 20px 0 20px; }\
        input[type=\"text\"] { display: block; width: 100%; margin-top: 5px; }\
        input[type=\"submit\"] { width: 100%; margin-top: 10px; padding: 5px; background: #cccccc; \
                                 border: none; border-radius: 10px; box-shadow: 0px 0px 10px rgba(0, 0, 0, 0.5); }\
        ul { list-style-type: none; padding: 0; width: 100%; float: left; margin-bottom: 5%; margin-top: 0; }\
        ul li { width: 49%; float: left; position: relative; margin-bottom: 1%; }\
        li a { display: block; padding: 15px; text-align: center; position: relative; border-radius: 10px; \
                  text-decoration: none; box-shadow: 0px 0px 10px rgba(0, 0, 0, 0.5); padding: 20px; }\
      </style>\
    </head>\
    <body>\
      <h1 style=\"text-align:center; margin-top: 30px;\">Hello from ESP8266!</h1>\
      <p>Current Connection: %s</p>\
      <form action=\"/credentials/set\">\
        <span>SSID: </span>\
        <input type=\"text\" name=\"ssid\"><br>\
        <span>PASSWORD: </span>\
        <input type=\"text\" name=\"pass\"><br>\
        <input type=\"submit\" value=\"Submit\">\
      </form>\
      <ul>\
        <li style=\"width: 100%;\"><a href=\"/\">Back</a></li>\
      </ul>\
    </body>\
  </html>", ssid );
  
  httpServer.send ( 200, "text/html", temp );
  digitalWrite ( led, 0 );
}

void clearCredentials() {
  for (int i = 0; i < 96; ++i) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  ESP.reset();
}

void handleClearCredentials() {
  clearCredentials();
  httpServer.send(200, "text/plain", "Cleared!");
}

void handleWhite() {
  fill_solid(leds, NUM_LEDS, CRGB(255, 255, 255));
  FastLED.show();
  httpServer.send(400, "text/html", 
      " <html><head>\
        <meta http-equiv=\"refresh\" content=\"1, /\">\
        <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
       </head>\
       <body>\
        <p>White!</p>\
       </body></html> ");
}

void handleSetColor() {
  int r = httpServer.arg("r").toInt();
  int g = httpServer.arg("g").toInt();
  int b = httpServer.arg("b").toInt();
  char msg[400];
  fill_solid( leds, NUM_LEDS, CRGB(r,g,b));
  FastLED.show();
  snprintf ( msg, 400, 
   "<html><head>\
        <meta http-equiv=\"refresh\" content=\"1, /color\">\
        <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
       </head>\
       <body>\
        <p>RGB = ( %02d , %02d , %02d )</p>\
       </body></html>", r, g, b);
       
  httpServer.send(200, "text/html", msg);
}

void handleColor() {
  char temp[2000];

  snprintf ( temp, 2000,

             "<html>\
    <head>\
      <title>ESP8266</title>\
      <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
      <style>\
        body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088;\
               font-size: 1em; margin: 0; padding: 0 20px 0 20px; }\
        input[type=\"text\"] { display: block; width: 100%; margin-top: 5px; }\
        input[type=\"submit\"] { width: 100%; margin-top: 10px; padding: 5px; background: #cccccc; \
                                 border: none; border-radius: 10px; box-shadow: 0px 0px 10px rgba(0, 0, 0, 0.5); }\
        ul { list-style-type: none; padding: 0; width: 100%; float: left; margin-bottom: 5%; margin-top: 0; }\
        ul li { width: 49%; float: left; position: relative; margin-bottom: 1%; }\
        li a { display: block; padding: 15px; text-align: center; position: relative; border-radius: 10px; \
                  text-decoration: none; box-shadow: 0px 0px 10px rgba(0, 0, 0, 0.5); padding: 20px; }\
      </style>\
    </head>\
    <body>\
      <h1 style=\"text-align:center; margin-top: 30px;\">Hello from ESP8266!</h1>\
      <form action=\"/color/set\">\
        <span>R: </span>\
        <input type=\"text\" name=\"r\"><br>\
        <span>G: </span>\
        <input type=\"text\" name=\"g\"><br>\
        <span>B: </span>\
        <input type=\"text\" name=\"b\"><br>\
        <input type=\"submit\" value=\"Submit\">\
      </form></br>\
      <ul>\
        <li style=\"width: 100%;\"><a href=\"/\">Back</a></li>\
      </ul>\
    </body>\
  </html>" );
  
  httpServer.send ( 200, "text/html", temp );
}

void handleOff() {
  fill_solid( leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  httpServer.send(400, "text/html", 
      " <html><head>\
        <meta http-equiv=\"refresh\" content=\"1, /\">\
        <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
       </head>\
       <body>\
        <p>LEDs off!</p>\
       </body></html> ");
}

void handleBrightnessUp() {
  if (brightness > 255 - binterval) {
    brightness = 255;
  } else {
    brightness = brightness + binterval;
  }
  FastLED.setBrightness(brightness);
  char temp[400];
  snprintf ( temp, 400 ,
             "<html><head>\
        <meta http-equiv=\"refresh\" content=\"1, /brightness\">\
        <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
       </head>\
       <body>\
        <p>Brightness set to %02d </p>\
       </body></html> ", brightness);
  
  httpServer.send(200, "text/html", temp);
  FastLED.show();
}

void handleBrightnessDown() {
  if (brightness < binterval) {
    brightness = 0;
  } else {
    brightness = brightness - binterval;
  }
  FastLED.setBrightness(brightness);
  char temp[400];
  snprintf ( temp, 400 ,
             "<html><head>\
        <meta http-equiv=\"refresh\" content=\"1, /brightness\">\
        <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
       </head>\
       <body>\
        <p>Brightness set to %02d </p>\
       </body></html> ", brightness);
  
  httpServer.send(200, "text/html", temp);
  FastLED.show();
}

void handleSetBrightness() {
  int value = httpServer.arg("value").toInt();
  if (value > 0 && value < 256) {
    brightness = value;
 
    FastLED.setBrightness(brightness);
    char temp[400];
    snprintf ( temp, 400 ,
             "<html><head>\
        <meta http-equiv=\"refresh\" content=\"1, /brightness\">\
        <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
       </head>\
       <body>\
        <p>Brightness set to %02d </p>\
       </body></html> ", brightness);
  
    httpServer.send(200, "text/html", temp);
    FastLED.show();

  } else {
    httpServer.send(400, "text/html", 
      " <html><head>\
        <meta http-equiv=\"refresh\" content=\"1, /brightness\">\
        <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
       </head>\
       <body>\
        <p>Invalid value</p>\
       </body></html> ");
  }

}

void handleBrightness() {
  char temp[2000];
  int brightness = FastLED.getBrightness();

  snprintf ( temp, 2000,

             "<html>\
    <head>\
      <title>ESP8266</title>\
      <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
      <style>\
        body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088;\
               font-size: 1em; margin: 0; padding: 0 20px 0 20px; }\
        input[type=\"text\"] { display: block; width: 100%; margin-top: 5px; }\
        input[type=\"submit\"] { width: 100%; margin-top: 10px; padding: 5px; background: #cccccc; \
                                 border: none; border-radius: 10px; box-shadow: 0px 0px 10px rgba(0, 0, 0, 0.5); }\
        ul { list-style-type: none; padding: 0; width: 100%; float: left; margin-bottom: 5%; margin-top: 0; }\
        ul li { width: 49%; float: left; position: relative; margin-bottom: 1%; }\
        li a { display: block; padding: 15px; text-align: center; position: relative; border-radius: 10px; \
                  text-decoration: none; box-shadow: 0px 0px 10px rgba(0, 0, 0, 0.5); padding: 20px; }\
      </style>\
    </head>\
    <body>\
      <h1 style=\"text-align:center; margin-top: 30px;\">Hello from ESP8266!</h1>\
      <p>Current Brightness: %02d</p>\
      <form action=\"/brightness/set\">\
        <span>Brightness: </span>\
        <input type=\"text\" name=\"value\"><br>\
        <input type=\"submit\" value=\"Submit\">\
      </form></br>\
      <ul>\
        <li style=\"margin-right: 1%;\"><a href=\"/brightness/up\">UP</a></li>\
        <li style=\"margin-left: 1%;\"><a href=\"/brightness/down\">DOWN</a></li>\
      </ul>\
      <ul>\
        <li style=\"width: 100%;\"><a href=\"/\">Back</a></li>\
      </ul>\
    </body>\
  </html>", brightness );
  
  httpServer.send ( 200, "text/html", temp );
}

void handleRoot() {
  digitalWrite ( led, 1 );
  char temp[3000];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  snprintf ( temp, 3000,

             "<html>\
    <head>\
      <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
      <title>ESP8266</title>\
      <style>\
        body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088;\
               font-size: 1em; margin: 0; padding: 0 20px 0 20px; }\
        ul { list-style-type: none; padding: 0; width: 100%; float: left; margin-bottom: 5%; margin-top: 0; }\
        ul li { width: 49%; float: left; position: relative; margin-bottom: 1%; }\
        li a, p { display: block; padding: 15px; text-align: center; position: relative; \
                  text-decoration: none; box-shadow: 0px 0px 10px rgba(0, 0, 0, 0.5); padding: 20px; }\
      </style>\
    </head>\
    <body>\
      <h1 style=\"text-align:center; margin-top: 30px;\">Hello from ESP8266!</h1>\
      <p style=\"border-radius: 10px;\">Uptime: \
          <span id=\"hours\">00</span>:<span id=\"minutes\">00</span>:<span id=\"seconds\">00</span></p>\
      <p><em>Settings</em></p>\
      <ul>\
        <li style=\"margin-right: 1%;\"><a href=\"/credentials\">Set Credentials</a></li>\
        <li style=\"margin-left: 1%;\"><a href=\"/clear\">Clear</a></li>\
        <li style=\"margin-right: 1%;\"><a href=\"/off\">Off</a></li>\
        <li style=\"margin-left: 1%;\"><a href=\"/reboot\">Reboot</a></li>\
      </ul>\
      <p style=\"clear: both\"><em>LEDs</em></p>\
      <ul>\
        <li style=\"margin-right: 1%;\"><a href=\"color\">Set Color</a></li>\
        <li style=\"margin-left: 1%;\"><a href=\"brightness\">Set Brightness</a></li>\
        <li style=\"margin-right: 1%;\"><a href=\"white\">White</a></li>\
        <li style=\"margin-left: 1%;\"><a href=\"#\">Loop</a></li>\
      </ul>\
    </body>\
    <script type=\"text/javascript\">\
      hours = %02d;\
      mins = %02d;\
      secs = %02d;\
      setInterval(function() {\
        ++secs;\
        if (secs >= 60) { ++mins; secs = 0; }\
        if (mins >= 60) { ++hours; mins = 0; }\
        document.getElementById(\"hours\").innerHTML = new Intl.NumberFormat(\"en-IN\", {minimumIntegerDigits: 2}).format(hours);\
        document.getElementById(\"minutes\").innerHTML = new Intl.NumberFormat(\"en-IN\", {minimumIntegerDigits: 2}).format(mins);\
        document.getElementById(\"seconds\").innerHTML = new Intl.NumberFormat(\"en-IN\", {minimumIntegerDigits: 2}).format(secs);\
      }, 1000);\
    </script>\
  </html>",

             hr, min % 60, sec % 60
           );
  httpServer.send ( 200, "text/html", temp );
  digitalWrite ( led, 0 );
}

void setupAP() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.softAP(customssid);
  Serial.println("softap");
  Serial.println("");
  Serial.println("Ready");
  Serial.println("New network created");
  Serial.print("SSID: ");
  Serial.println(customssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
}

void handleLoop() {
  for (int dot = 0; dot < NUM_LEDS; dot++) {
    leds[dot] = CRGB::Blue;
    FastLED.show();
    // clear this led for the next time around the loop
    leds[dot] = CRGB::Black;
    delay(30);
  }
  httpServer.send ( 200, "text/plain", "Done!" );
}

void handleReboot() {
  httpServer.sendHeader("Location", String("/"), true);
  httpServer.send ( 302, "text/plain", "");
  delay(5000);
  ESP.reset();
}

void handleNotFound() {
  httpServer.send(404, "text/html", 
      "<html>\
    <head>\
      <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
      <title>ESP8266</title>\
      <style>\
        body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088;\
               font-size: 1em; margin: 0; padding: 0 20px 0 20px; }\
        ul { list-style-type: none; padding: 0; width: 100%; float: left; margin-bottom: 5%; margin-top: 0; }\
        ul li { width: 49%; float: left; position: relative; margin-bottom: 1%; }\
        li a { display: block; padding: 15px; text-align: center; position: relative; border-radius: 10px; \
                  text-decoration: none; box-shadow: 0px 0px 10px rgba(0, 0, 0, 0.5); padding: 20px; }\
      </style>\
    </head>\
    <body>\
      <h1 style=\"text-align:center; margin-top: 30px;\">Error 404</h1>\
      <p style=\"text-align:center;\">Woops. Looks like this page doesn't exist.</p>\
      <ul>\
        <li style=\"width: 100%; margin-top: 50px;\"><a href=\"/\">Back</a></li>\
      </ul>\
    </body>\
  </html>");
}

struct credential getCredentials() {
  Serial.println("Getting credentials from EEPROM...");
  struct credential cred;
  cred.ssid = "";
  cred.pass = "";
  int ssidlength = char(EEPROM.read(0));
  int passlength = char(EEPROM.read(1));
  int j = 2;
  for (int i = 0; i < ssidlength; ++i) {
    cred.ssid += char(EEPROM.read(j));
    j++;
  }
  Serial.print("SSID: ");
  if (cred.ssid == "") cred.ssid = "";
  Serial.println(cred.ssid);
  // Serial.println("Reading EEPROM pass");
  for (int i = 0; i < passlength; ++i) {
    cred.pass += char(EEPROM.read(j));
    j++;
  }
  Serial.print("PASS: ");
  if (cred.pass == "") cred.pass = "";
  Serial.println(cred.pass);
  return cred;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting...");

  EEPROM.begin(512);
  delay(10);
  WiFi.mode(WIFI_STA);

  struct credential cred = getCredentials();
  if (!connect(cred)) {
    setupAP();
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("led");

  // No authentication by default
  ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  if ( MDNS.begin ( "esp8266" ) ) {
    Serial.println ( "MDNS responder started" );
  }
  
  MDNS.addService("http", "tcp", 80);
  httpServer.on ( "/", handleRoot );
  httpServer.on ( "/loop", handleLoop );
  httpServer.on ( "/clear", handleClearCredentials );
  httpServer.on ( "/credentials", handleCredentials );
  httpServer.on ( "/credentials/set", handleSetCredentials );
  httpServer.on ( "/white", handleWhite );
  httpServer.on ( "/off", handleOff );
  httpServer.on ( "/color", handleColor );
  httpServer.on ( "/color/set", handleSetColor );
  httpServer.on ( "/brightness", handleBrightness );
  httpServer.on ( "/brightness/set", handleSetBrightness );
  httpServer.on ( "/brightness/up", handleBrightnessUp );
  httpServer.on ( "/brightness/down", handleBrightnessDown );
  httpServer.on ( "/reboot", handleReboot );

  httpServer.onNotFound(handleNotFound);
  
  httpServer.begin();

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  // FastLED.setDither(0);
  FastLED.setBrightness(255);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 20000);
  fill_solid(leds, NUM_LEDS, CRGB(255, 255, 255));
  FastLED.show();
}

void loop() {
  ArduinoOTA.handle();
  httpServer.handleClient();
  FastLED.show();
}
