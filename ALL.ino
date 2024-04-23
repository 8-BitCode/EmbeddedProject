#include <TFT_eSPI.h>
#include <Timezone.h>
#include <Time.h>
#include <TimeLib.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <RTClib.h>

#define ENC_A 26
#define ENC_B 25
#define BUTTON_PIN 35
String Allconfession = "";
TFT_eSPI tft = TFT_eSPI(); // Create an instance of the TFT_eSPI class

RTC_DS3231 rtc;

// Define your time zone using the Timezone library
TimeChangeRule myDST = {"BST", Last, Sun, Mar, 1, 60}; // British Summer Time (GMT+1)
TimeChangeRule mySTD = {"GMT", Last, Sun, Oct, 2, 0};  // Greenwich Mean Time (GMT+0)
Timezone myTZ(myDST, mySTD);

const int numOptions = 3;
const char *options[] = {"Clock", "StopWatch", ";3 Mode"};
int counter = 0; // Declare the counter variable here
int currentOption = 0;

const unsigned long longPressDuration = 2000; // 2 seconds for long press
unsigned long buttonPressStartTime = 0;
bool isLongPress = false;

enum ScreenState {
  MENU,
  COLOR
};

ScreenState screenState = MENU; // Initial state is MENU


volatile bool buttonPressed = false;
volatile unsigned long lastButtonPressTime = 0;
volatile int state = 0;  // 0: Stopped, 1: Running, 2: Stopped and Displayed, 3: Reset

unsigned long startTime = 0;
unsigned long elapsedTime = 0;

void IRAM_ATTR buttonISR() {
  unsigned long now = millis();
  if (now - lastButtonPressTime > 300) {  // Debouncing, adjust as needed
    buttonPressed = true;
    lastButtonPressTime = now;
  }
}
volatile int counterP = 0;
#ifndef SSID
#define SSID "Secret ;3 see what happens"
#endif
int yPos = 70;
const char *softAP_ssid = SSID;

IPAddress apIP(8, 8, 8, 8);
IPAddress netMsk(255, 255, 255, 0);

const byte DNS_PORT = 53;
DNSServer dnsServer;

WebServer server(80);

boolean isIp(String str) {
  for (size_t i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

boolean captivePortal() {
  if (!isIp(server.hostHeader())) {
    Serial.println("Request redirected to captive portal");
    server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()), true);
    server.send(302, "text/plain", "");
    server.client().stop();
    return true;
  }
  return false;
}
bool wasInThreeMode = false;
String previousConfessions;
void handleRoot() {
   if (captivePortal()) {
    if (wasInThreeMode) {
      Allconfession = "";  // Clear previous confessions
      wasInThreeMode = false;
    }
    return;
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  String p;
  p += F(
      "<!DOCTYPE html>"
      "<html lang='en'>"
      "<head>"
      "    <meta charset='UTF-8'>"
      "    <meta name='viewport' content='width=device-width, initial-scale=1.0'>"
      "    <title>Anonymous Confessionals</title>"
      "    <style>"
      "        body {"
      "            background-color: #808080; /* Grey background */"
      "            font-family: 'Lucida Console', 'Courier New', monospace; /* Custom font */"
      "            text-align: center;"
      "            margin: 50px;"
      "        }"
      "        .hidden {"
      "            display: none;"
      "        }"
      "        h1, h2 {"
      "            font-size: 2em; /* Larger font for titles */"
      "        }"
      "        input, button {"
      "            margin: 10px;"
      "            padding: 8px;"
      "            font-size: 18px; /* Larger font for input */"
      "            background-color: #ffcc00;"
      "            font-family: 'Lucida Console', 'Courier New', monospace; /* Custom font */"
      "        }"
      "        textarea {"
      "            margin: 10px;"
      "            width: 80%;"
      "            height: 200px;"
      "            font-size: 2em;"
      "            padding: 10px;"
      "        }"
      "        #description {"
      "            margin: 10px;"
      "            font-size: 1.2em;"
      "        }"
      "    </style>"
      "</head>"
      "<body>"
      "    <h1>Anonymous Confessions</h1>"
      "    <div id='nameInput'>"
      "        <h3>Enter Your Name</h3>"
      "        <h5>(please refrain from using your real name)</h5>"
      "        <input type='text' id='name'>"
      "        <br>"
      "        <button onclick='hideNameInput()'>Submit</button>"
      "        <p id='description'>Fun thoughts or otherwise<br>this won't be saved</p>"
      "    </div>"
      "    <div id='textInput' class='hidden'>"
      "        <h2>Say what you want to say:</h2>"
      "        <textarea id='userText' rows='4' cols='50'></textarea>"
      "        <br>"
      "        <button onclick='showResult()'>Submit</button>"
      "    </div>"
      "    <div id='result' class='hidden'>"
"        <h2>Confession:</h2>"
"        <p id='output'></p>"
"    </div>"
"    <script>"
"        function hideNameInput() {"
"            document.getElementById('nameInput').classList.add('hidden');"
"            document.getElementById('textInput').classList.remove('hidden');"
"            document.getElementById('description').classList.add('hidden');"
"        }"
"        function showResult() {"
"            var userName = document.getElementById('name').value;"
"            var userText = document.getElementById('userText').value;"
"            var xhr = new XMLHttpRequest();"
"            xhr.open('GET', '/submit?name=' + encodeURIComponent(userName) + '&confession=' + encodeURIComponent(userText), true);"
"            xhr.send();"
"            document.getElementById('textInput').classList.add('hidden');"
"            document.getElementById('result').classList.remove('hidden');"
"            var output = document.getElementById('output');"
"            output.innerHTML += '<p>' + userName + ': ' + userText + '</p>';"
"        }"
"    </script>"
"</body>"
"</html>"
  );
  server.send(200, "text/html", p);
}

void handleNotFound() {
  if (captivePortal()) {
    return;
  }
  String message = F("File Not Found\n\n");
  message += F("URI: ");
  message += server.uri();
  message += F("\nMethod: ");
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += F("\nArguments: ");
  message += server.args();
  message += F("\n");

  for (uint8_t i = 0; i < server.args(); i++) {
    message += String(F(" ")) + server.argName(i) + F(": ") + server.arg(i) + F("\n");
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(404, "text/plain", message);
}
String confession = "";
void handleSubmit() {
  String name = server.arg("name");
  confession = server.arg("confession");

  Serial.print("Name: ");
  Serial.println(name);
  Serial.print("Confession: ");
  Serial.println(confession);

  // Check if the current confession is not already present in the full text
  if (Allconfession.indexOf(confession) == -1) {
    // Append the current confession to the previous confessions with an empty line
    if (!Allconfession.isEmpty()) {
      Allconfession += "\n\n\n" + server.arg("name") + ":";  // Add an empty line if there are previous confessions
    }
    Allconfession += confession;
  }

  // Set text datum point to top center for the name
  tft.setTextDatum(MC_DATUM);
  // Display name on LCD
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.drawString(name, tft.width() / 2, yPos - 30);

  // Set text datum point to top left for the confession
  tft.setTextDatum(TL_DATUM);
  
  // Display all confessions on LCD with line breaks and dashes
  int maxLineLength = 15; // Adjust the maximum line length as needed
  int lineCount = 0;

  for (int i = 0; i < Allconfession.length(); ) {
    int remainingChars = min(static_cast<unsigned int>(maxLineLength), static_cast<unsigned int>(Allconfession.length() - i));
    String line = Allconfession.substring(i, i + remainingChars);

    // Check if the next character is not a space and it's not the last line
    if (i + remainingChars < Allconfession.length() && Allconfession[i + remainingChars] != ' ' && Allconfession[i + remainingChars - 1] != ' ') {
      // Add a dash at the end of the line
      line += "-";
    }

    tft.drawString(line, 25, yPos + lineCount * tft.fontHeight() * 2);
    lineCount++;
    i += remainingChars;
  }

  server.send(200, "text/plain", "Data received successfully");
}


void setup() {
  String Allconfession = "";
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(115200);
  delay(500);

  tft.begin();
  tft.setRotation(1); // Adjust the rotation if needed

  displayOptions();
}

bool Runlock = false;
bool Gunlock = false;
bool Bunlock = false;

void loop() {
  static int lastCounter = 0;
  static int lastButtonState = HIGH;
  int buttonState = digitalRead(BUTTON_PIN);

  readEncoder();

  if (screenState == MENU && counter != lastCounter) {
    displayOptions();
    lastCounter = counter;
  }
  if(screenState == MENU){
    Runlock = false;
    Gunlock = false;
    Bunlock = false;
  }

  handleButtonPress(buttonState, lastButtonState);
  handleLongPress(buttonState, lastButtonState);

  lastButtonState = buttonState;
  if(Runlock){
    RedLoop();
  }
  if(Gunlock){
    GreenLoop();
  }
  if(Bunlock){
    BlueLoop();
  }
}
void handleButtonPress(int buttonState, int lastButtonState) {
  if (buttonState == LOW && lastButtonState == HIGH) {
    // Button is pressed
    buttonPressStartTime = millis();
  }
}

void handleLongPress(int buttonState, int lastButtonState) {
  if (buttonState == HIGH && lastButtonState == LOW) {
    // Button is released
    unsigned long buttonPressDuration = millis() - buttonPressStartTime;

    if (buttonPressDuration >= longPressDuration) {
      isLongPress = true;
    } else {
      isLongPress = false;
      if (screenState == MENU) {
        displayColor(options[currentOption]);
        screenState = COLOR; // Change state to COLOR when displaying color
        displayOptions();    // Update the display with color information
      } else {
        // Additional logic for handling button press in COLOR state if needed
      }
    }
  }

  if (isLongPress) {
    // Perform action for long press (return to menu screen)
    displayOptions();
    screenState = MENU; // Change state to MENU on long press
    isLongPress = false;
    Serial.println("beans");
  }
}


void readEncoder() {
  if (screenState == MENU) {
    static uint8_t old_AB = 3;
    static int8_t encval = 0;
    static const int8_t enc_states[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

    old_AB <<= 2;

    if (digitalRead(ENC_A)) old_AB |= 0x02;
    if (digitalRead(ENC_B)) old_AB |= 0x01;

    encval += enc_states[(old_AB & 0x0f)];

    if (encval > 3) {
      counter++;
      encval = 0;
    } else if (encval < -3) {
      counter--;
      encval = 0;
    }

    // Ensure the counter stays within the range of options
    counter = constrain(counter, 0, numOptions - 1);
    currentOption = counter;
  }
}

void displayOptions() {
  tft.fillScreen(TFT_BLACK);

  for (int i = 0; i < numOptions; i++) {
    int textColor = (i == currentOption) ? TFT_GREEN : TFT_WHITE;
    tft.setTextColor(textColor);
    tft.setTextSize(2.6);
    int xPos = (tft.width() - tft.textWidth(options[i])) / 2;
    int yPos = i * 80 + 20;
    tft.drawString(options[i], xPos, yPos, 2);

    // Set wasInThreeMode when ;3 Mode is selected
    if (i == numOptions - 1 && currentOption == i) {
      wasInThreeMode = true;
    }
  }
}

void displayColor(const char *color) {
  if (strcmp(color, "Clock") == 0) {
    displayRed();
  } else if (strcmp(color, "StopWatch") == 0) {
    displayGreen();
  } else if (strcmp(color, ";3 Mode") == 0) {
    displayBlue();
  } else {
    // Default to black if color is not recognized
    tft.fillScreen(TFT_BLACK);
  }
}

void displayRed() {
  Runlock = true;

  tft.init();
  tft.setRotation(1); // Adjust the rotation if needed

  // SETUP RTC MODULE
  if (!rtc.begin()) {
    Serial.println("RTC module is NOT found");
    Serial.flush();
    while (1);
  }
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  DateTime compiled = DateTime(F(__DATE__), F(__TIME__));

  // Check if RTC lost power and set the time if needed
  if (rtc.now() < compiled) {
    Serial.println("RTC lost power, setting the time!");
    rtc.adjust(compiled);
  } else {
    Serial.println("RTC is running, not setting the time!");
  }
  
}

void displayGreen() {
  Gunlock = true;

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2.5);
  tft.setTextColor(TFT_WHITE);

  attachInterrupt(digitalPinToInterrupt(35), buttonISR, FALLING);
}

void displayBlue() {
   Bunlock = true;

  delay(1000);
  Serial.begin(9600);
  Serial.println();
  Serial.println("Configuring access point...");
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(softAP_ssid);
  delay(1000);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  
  // Clear previous confessions and names when entering ;3 Mode
  if (wasInThreeMode) {
    Allconfession = "";
    wasInThreeMode = false;
  }
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  server.on("/", handleRoot);
  server.on("/generate_204", handleRoot);
  server.on("/submit", HTTP_GET, handleSubmit);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void RedLoop(){
  // Record the start time
  unsigned long startTime = millis();

  // Get the local time from RTC
  DateTime rtcTime = rtc.now();

  // Check if DST is in effect (manually)
  bool isDST = (rtcTime.month() > 3 && rtcTime.month() < 10) || 
               (rtcTime.month() == 3 && rtcTime.day() > (31 - ((5 * rtcTime.year() / 4 + 1) % 7))) || 
               (rtcTime.month() == 10 && rtcTime.day() <= (31 - ((5 * rtcTime.year() / 4 + 1) % 7)));

  // Calculate the local time with the UTC offset and DST adjustment
  time_t local = myTZ.toLocal(rtcTime.unixtime());

  // Adjust for DST if needed
  if (isDST) {
    local += 3600; // Add an extra hour for DST
  }

  // Clear the screen
  tft.fillScreen(TFT_BLACK);

  // Display the time in the middle of the screen
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(5);

  // Calculate the coordinates to center the time text
  int timeX = (tft.width() - tft.textWidth("00:00")) / 2;
  int timeY = (tft.height() - tft.fontHeight()) / 2;

  tft.setCursor(timeX, timeY);
  tft.printf("%.2d:%.2d", hour(local), minute(local));

  // Display the date below the time
  tft.setTextSize(2); // Smaller text for the date
  int dateX = (tft.width() - tft.textWidth("00/00/00")) / 2;
  int dateY = timeY + tft.fontHeight() + 45; // Adjust the vertical spacing

  tft.setCursor(dateX, dateY);
  tft.printf("%.2d/%.2d/%.2d", rtcTime.day(), rtcTime.month(), rtcTime.year() % 100);
  
  delay(10);

}
void GreenLoop(){
  if (buttonPressed) {
    buttonPressed = false;

    if (state == 0 || state == 3) {
      // Start the timer or restart from 0
      startTime = millis();
      elapsedTime = 0;
      state = 1;
    } else if (state == 1) {
      // Stop the timer
      elapsedTime += millis() - startTime;
      state = 2;
    } else if (state == 2) {
      // Display the stopped time
      drawStoppedTime();
      state = 3;
    } else {
      // Reset the timer
      elapsedTime = 0;
      state = 0;
    }
  }

  if (state == 1) {
    drawDigitalClock();
  } else if (state == 0 || state == 3) {
    drawInitialScreen();
  }

  delay(10);  // Update every 10 milliseconds
}

void BlueLoop(){
  dnsServer.processNextRequest();
  server.handleClient();
  static int lastCounter = 0;

  read_encoder();
  // If count has changed, print the new value to serial
  if (counterP != lastCounter) {
    if (counterP > lastCounter) {
      Serial.println("Clockwise");
      tft.fillScreen(TFT_BLACK);
      yPos+=50;
      handleSubmit();
    } else {
      Serial.println("Anticlockwise");
      tft.fillScreen(TFT_BLACK);
      yPos-=50;
      handleSubmit();
    }
    lastCounter = counterP;
  }
}



void drawDigitalClock() {
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(1); // Set 180-degree rotation

  // Running
  unsigned long totalMillis = elapsedTime + (millis() - startTime);
  int hours = totalMillis / 3600000;
  int minutes = (totalMillis % 3600000) / 60000;
  int seconds = (totalMillis % 60000) / 1000;
  int milliseconds = totalMillis % 1000;

  tft.setCursor(50, 115);
  tft.printf("%02d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds);
}

void drawStoppedTime() {
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(1); // Set 180-degree rotation

  // Display the stopped time
  int hours = elapsedTime / 3600000;
  int minutes = (elapsedTime % 3600000) / 60000;
  int seconds = (elapsedTime % 60000) / 1000;
  int milliseconds = elapsedTime % 1000;

  tft.setCursor(50, 115);
  tft.printf("%02d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds);
}

void drawInitialScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(1); // Set 180-degree rotation

  tft.setCursor(50, 115);
  tft.printf("00:00:00.000");
}
void read_encoder() {
  // Encoder routine. Updates counter if they are valid
  // and if rotated a full indent

  static uint8_t old_AB = 3;  // Lookup table index
  static int8_t encval = 0;   // Encoder value
  static const int8_t enc_states[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0}; // Lookup table

  old_AB <<= 2; // Remember previous state

  if (digitalRead(ENC_A)) old_AB |= 0x02; // Add current state of pin A
  if (digitalRead(ENC_B)) old_AB |= 0x01; // Add current state of pin B

  encval += enc_states[(old_AB & 0x0f)];

  // Update counter if encoder has rotated a full indent, that is at least 4 steps
  if (encval > 3) {         // Four steps forward
    counterP++;              // Increase counter
    encval = 0;
  } else if (encval < -3) { // Four steps backwards
    counterP--;              // Decrease counter
    encval = 0;
  }
}


