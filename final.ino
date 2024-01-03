#include <WiFi.h>
#include <LiquidCrystal_I2C.h>

const char* ssid = " REPLACE_WITH_YOUR_SSID";
const char* password = " REPLACE_WITH_YOUR_PASSWORD";

/**
PIN CONNECTIONS
**ESP32 -> RELAY MODULE**
GND -> GND
D13 -> IN1
D12 -> IN2
D14 -> IN3
D27 -> IN4
VIN -> VCC

**ESP32 -> DISPLAY**
GND -> GND
VIN -> VCC
D22 -> GDA
D21 -> SCL

*/


WiFiServer server(80);

String header;

String plugAState = "off";
String plugBState = "off";
String plugCState = "off";
String plugDState = "off";
int programState = 0; // 0 = started, 1 = connecting, 2 = connected !

const int plugA = 13;
const int plugB = 12;
const int plugC = 14;
const int plugD = 27;
const int power = 2;

unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 200;

LiquidCrystal_I2C lcd(0x27, 16, 2);  

void setupPin(int pin, int val = HIGH) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, val);
}

void wifiSetupCompleted() {
    programState = 2; // 0 = started, 1 = connecting, 2 = connected !    Serial.print("Connecting to ");
    printLcd();
    digitalWrite(power, HIGH);
}

void setupWifi() {
    programState = 1; // 0 = started, 1 = connecting, 2 = connected !    Serial.print("Connecting to ");
    printLcd();
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    server.begin();
    wifiSetupCompleted();
}

void setup() {
    Serial.begin(115200);
    lcd.init();
    lcd.backlight();
    setupPin(plugA);
    setupPin(plugB);
    setupPin(plugC);
    setupPin(plugD);
    setupPin(power, LOW);
    setupWifi();
}

void printLcd() {
  lcd.clear();
  if (programState == 0) {
    lcd.setCursor(0,0);
    lcd.print("Started");
  }
  else if (programState == 1) {
    lcd.setCursor(0,0);
    lcd.print("Connecting WiFi");
    lcd.setCursor(0,1);
    lcd.print(ssid);
  }
  else if (programState == 2) {
    lcd.setCursor(0,0);
    lcd.print(WiFi.localIP());
    lcd.setCursor(0,1);
    lcd.print("A-");
    if(plugAState == "on") lcd.print("1");
    else lcd.print("0");
    lcd.print(" B-");
    if(plugBState == "on") lcd.print("1");
    else lcd.print("0");
    lcd.print(" C-");
    if(plugCState == "on") lcd.print("1");
    else lcd.print("0");
    lcd.print(" D-");
    if(plugDState == "on") lcd.print("1");
    else lcd.print("0");
  }
  else {
    lcd.setCursor(0,0);
    lcd.print("Unknown State!");
  }
}

void updatePlugsForPin(String &header, int pin, String &stt, String nm) {
    if (header.indexOf("GET /"+nm+"/on") >= 0) {
        Serial.println("GPIO plug "+nm+" on");
        stt = "on";
        digitalWrite(pin, LOW);
    }
    if (header.indexOf("GET /"+nm+"/off") >= 0) {
        Serial.println("GPIO plug "+nm+" off");
        stt = "off";
        digitalWrite(pin, HIGH);
    }
}

void updatePlugs(String &header) {
    updatePlugsForPin(header, plugA, plugAState, "a");
    updatePlugsForPin(header, plugB, plugBState, "b");
    updatePlugsForPin(header, plugC, plugCState, "c");
    updatePlugsForPin(header, plugD, plugDState, "d");
    printLcd();
}

String printPlug(String name, String &state) {
    String path = "/" + name + "\/";
    if (state == "off") {
        path += "on";
    } else {
        path += "off";
    }
    path.toLowerCase();
    return String("<div class=\"plug\"><p class=\"txt\">Plug "+name+" - State " + String(state) + "</p><p><a href=\""+path+"\" onClick=\"clickAction()\" class=\"btn "+state+"\"><span class=\"btn-trig\"></span></a></p></div>");
}

void htmlResponse(WiFiClient &c) {
    
    c.println("HTTP/1.1 200 OK");
    c.println("Content-type:text/html");
    c.println("Connection: close");
    c.println();
    c.println("<!DOCTYPE html><html>");
    c.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
    c.println("<link rel=\"icon\" href=\"data:,\">");
    c.println("<meta http-equiv=\"refresh\" content=\"10;url=/\">");
    c.println("<style>");
    c.println("html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; }");
    c.println(".plug { border: 1px black dashed; display: flex; border-radius: 8px; align-items: center; margin: 1rem; padding: 5px; }");
    c.println(".txt { flex-grow: 1; text-align: left; padding-left: 18px; }");
    c.println(".btn { border: 1px solid gray; color: white; text-decoration: none; cursor: pointer; border-radius: 26px; display: block; height: 22px; background-color: #4CAF50; margin-right: 18px; }");
    c.println(".btn-trig { height: 22px; width: 22px; border-radius: 100%; display: inline-block; background-color: white; }");
    c.println(".btn.on { background-color: #4CAF50; }");
    c.println(".btn.on > .btn-trig { margin-left: 20px; }");
    c.println(".btn.off { background-color: gray; }");
    c.println(".btn.off > .btn-trig { margin-right: 20px; }");
    c.println("@media (min-width: 768px) { .row { display: flex; } .plug { flex: 1 1 0%; } }");
    c.println("</style></head><body>");
    c.println("<h1>Smart Switch Board</h1>");
    c.println(printPlug("A", plugAState));
    c.println(printPlug("B", plugBState));
    c.println(printPlug("C", plugCState));
    c.println(printPlug("D", plugDState));
    c.println("</body></html>");
    c.println();
}

void loop(){
    WiFiClient client = server.available();
    if (client) {
        
        currentTime = millis();
        previousTime = currentTime;
        Serial.println("New Client.");
        String currentLine = "";
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            updatePlugs(header);
            htmlResponse(client);
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
    }
}