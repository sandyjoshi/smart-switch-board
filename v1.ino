#include <WiFi.h>

const char* ssid = " REPLACE_WITH_YOUR_SSID";
const char* password = " REPLACE_WITH_YOUR_PASSWORD";

WiFiServer server(80);

String header;

String plugAState = "off";
String plugBState = "off";
String plugCState = "off";
String plugDState = "off";

const int plugA = 12;
const int plugB = 13;
const int plugC = 14;
const int plugD = 27;

unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

void setupPin(int pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

void setupWifi() {
    Serial.print("Connecting to ");
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
}

void setup() {
    Serial.begin(115200);
    setupPin(plugA);
    setupPin(plugB);
    setupPin(plugC);
    setupPin(plugD);
    setupWifi();
}

void updatePlugsForPin(int pin, String stt, String nm) {
    if (header.indexOf("GET /"+nm+"/on") >= 0) {
        Serial.println("GPIO plug "+nm+" on");
        stt = "on";
        digitalWrite(pin, HIGH);
    }
    if (header.indexOf("GET /"+nm+"/off") >= 0) {
        Serial.println("GPIO plug "+nm+" off");
        stt = "off";
        digitalWrite(pin, LOW);
    }
}

void updatePlugs(String header) {
    updatePlugsForPin(plugA, plugAState, "a");
    updatePlugsForPin(plugB, plugBState, "b");
    updatePlugsForPin(plugC, plugCState, "c");
    updatePlugsForPin(plugD, plugDState, "d");
}

String printPlug(String name, String state) {
    String path = "/" + name + "\/";
    if (state == "off") {
        path += "on";
    } else {
        path += "off";
    }
    path.toLowerCase();
    return "<div class=\"plug\"><p class=\"txt\">Plug "+name+" - State "+state+"</p><p><a href=\""+path+"\" onClick=\"clickAction()\" class=\"btn "+state+"\"><span class=\"btn-trig\"></span></a></p></div>";
}

void htmlResponse(WiFiClient client) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println("Connection: close");
    client.println();
    client.println("<!DOCTYPE html><html>");
    client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
    client.println("<link rel=\"icon\" href=\"data:,\">");
    client.println("<style>");
    client.println("html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; }");
    client.println(".plug { border: 1px black dashed; display: flex; border-radius: 8px; align-items: center; margin: 1rem; padding: 5px; }");
    client.println(".txt { flex-grow: 1; text-align: left; padding-left: 18px; }");
    client.println(".btn { border: 1px solid gray; color: white; text-decoration: none; cursor: pointer; border-radius: 26px; display: block; height: 22px; background-color: #4CAF50; margin-right: 18px; }");
    client.println(".btn-trig { height: 22px; width: 22px; border-radius: 100%; display: inline-block; background-color: white; }");
    client.println(".btn.on { background-color: #4CAF50; }");
    client.println(".btn.on > .btn-trig { margin-left: 20px; }");
    client.println(".btn.off { background-color: gray; }");
    client.println(".btn.off > .btn-trig { margin-right: 20px; }");
    client.println("@media (min-width: 768px) { .row { display: flex; } .plug { flex: 1 1 0%; } }");
    client.println("</style></head><body>");
    client.println("<h1>Smart Switch Board</h1>");
    client.println(printPlug("A", plugAState));
    client.println(printPlug("B", plugBState));
    client.println(printPlug("C", plugCState));
    client.println(printPlug("D", plugDState));
    client.println("</body></html>");
    client.println();
}

void loop(){
    WiFiClient client = server.available();
    if (client) {
        currentTime = millis();
        previousTime = currentTime;
        Serial.println("New Client.");
        String currentLine = "";
        while (client.connected() && currentTime - previousTime <= timeoutTime) 
        currentTime = millis();

        if (client.available()) {
            char c = client.read();
            Serial.write(c);
            header += c;
            if (c == '\n') {
                if (currentLine.length() == 0) {
                    updatePlugs(header);
                    htmlResponse(client);
                } else {
                    currentLine = "";
                }
            } else if (c != '\r') { 
                currentLine += c;     
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