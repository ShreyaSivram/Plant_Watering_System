#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Replace these with your WiFi credentials
const char* ssid = "techno";      // Your WiFi SSID
const char* password = "techno@123";  // Your WiFi password

// Define NTP Server address
const char* ntpServer = "in.pool.ntp.org";  // NTP server in your region
const long gmtOffset_sec = 19800;   // Offset from UTC (in seconds) - India GMT+5:30
const int daylightOffset_sec = 0;   // No daylight offset

// Define relay pin
const int relayPin = 33;  // Pin connected to relay

// Define OLED parameters
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Define NTP and WiFi objects
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec, daylightOffset_sec);

// Create a web server on port 80
WebServer server(80);

// Variables for relay control and timing
int relayOnHour = 16;     // Default ON time in 24HR format
int relayOnMinute = 0;    // Default ON time in minutes
int relayOffHour = 16;    // Default OFF time in 24HR format
int relayOffMinute = 1;   // Default OFF time in minutes
bool relayState = false;  // Initial relay state

// HTML page served to configure times and toggle relay
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Relay Control</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      height: 100vh;
      background-color: #f7f7f7;
    }
    form {
      background: #fff;
      padding: 20px;
      border-radius: 8px;
      box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
      width: 300px;
      margin-bottom: 20px;
    }
    label, input {
      display: block;
      margin-bottom: 10px;
      width: 100%;
    }
    input[type="submit"], .switch {
      display: none;
    }
    .switch {
      position: relative;
      display: inline-block;
      width: 60px;
      height: 34px;
    }
    .switch input {
      opacity: 0;
      width: 0;
      height: 0;
    }
    .slider {
      position: absolute;
      cursor: pointer;
      top: 0;
      left: 0;
      right: 0;
      bottom: 0;
      background-color: #ccc;
      transition: .4s;
      border-radius: 34px;
    }
    .slider:before {
      position: absolute;
      content: "";
      height: 26px;
      width: 26px;
      left: 4px;
      bottom: 4px;
      background-color: white;
      transition: .4s;
      border-radius: 50%;
    }
    input:checked + .slider {
      background-color: #4CAF50; /* Green */
    }
    input:checked + .slider:before {
      transform: translateX(26px);
    }
  </style>
  <script>
    function toggleRelay() {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/toggle", true);
      xhr.onreadystatechange = function() {
        if (xhr.readyState == 4 && xhr.status == 200) {
          var relayState = JSON.parse(xhr.responseText).relayState;
          document.getElementById("toggleSwitch").checked = relayState;
          document.getElementById("toggleSwitch").nextElementSibling.classList.toggle("active", relayState);
        }
      }
      xhr.send();
    }

    function setTimes() {
      var onHour = document.getElementById("onHour").value;
      var onMinute = document.getElementById("onMinute").value;
      var offHour = document.getElementById("offHour").value;
      var offMinute = document.getElementById("offMinute").value;
      
      var xhr = new XMLHttpRequest();
      var url = "/set_time?onHour=" + onHour + "&onMinute=" + onMinute + "&offHour=" + offHour + "&offMinute=" + offMinute;
      xhr.open("GET", url, true);
      xhr.onreadystatechange = function() {
        if (xhr.readyState == 4 && xhr.status == 200) {
          location.reload();  // Refresh page after setting time
        }
      }
      xhr.send();
    }

    window.onload = function() {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/relay_state", true);
      xhr.onreadystatechange = function() {
        if (xhr.readyState == 4 && xhr.status == 200) {
          var relayState = JSON.parse(xhr.responseText).relayState;
          document.getElementById("toggleSwitch").checked = relayState;
          document.getElementById("toggleSwitch").nextElementSibling.classList.toggle("active", relayState);
        }
      }
      xhr.send();
    }
  </script>
</head>
<body>
  <h1>ESP32 Relay Control</h1>
  <form>
    <label for="onHour">ON Time (24HR):</label>
    <input type="number" id="onHour" name="onHour" min="0" max="23" value="16">
    <input type="number" id="onMinute" name="onMinute" min="0" max="59" value="0"><br><br>
    <label for="offHour">OFF Time (24HR):</label>
    <input type="number" id="offHour" name="offHour" min="0" max="23" value="16">
    <input type="number" id="offMinute" name="offMinute" min="0" max="59" value="1"><br><br>
    <input type="button" value="Set Times" onclick="setTimes()">
  </form>
  <label class="switch" onclick="toggleRelay()">
    <input type="checkbox" id="toggleSwitch">
    <span class="slider"></span>
  </label>
</body>
</html>
)rawliteral";

// Handle root URL request
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

// Handle setting ON and OFF times
void handleSetTime() {
  if (server.hasArg("onHour")) relayOnHour = server.arg("onHour").toInt();
  if (server.hasArg("onMinute")) relayOnMinute = server.arg("onMinute").toInt();
  if (server.hasArg("offHour")) relayOffHour = server.arg("offHour").toInt();
  if (server.hasArg("offMinute")) relayOffMinute = server.arg("offMinute").toInt();
  server.send(200, "text/html", "Time updated.<br><a href=\"/\">Go back</a>");
}

// Handle toggling the relay
void handleToggle() {
  relayState = !relayState;
  digitalWrite(relayPin, relayState ? LOW : HIGH);  // Adjust based on your relay configuration
  String response = "{\"relayState\":" + String(relayState ? "true" : "false") + "}";
  server.send(200, "application/json", response);
}

// Handle getting current relay state
void handleRelayState() {
  String response = "{\"relayState\":" + String(relayState ? "true" : "false") + "}";
  server.send(200, "application/json", response);
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Initializing...");
  display.display();
  delay(1000);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize relay pin
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);  // Initial state (adjust based on your relay configuration)

  // Initialize NTP client
  timeClient.begin();

  // Setup web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/set_time", HTTP_GET, handleSetTime);
  server.on("/toggle", HTTP_GET, handleToggle);
  server.on("/relay_state", HTTP_GET, handleRelayState);

  // Start server
  server.begin();
}

void loop() {
  // Update NTP client
  timeClient.update();

  // Display current time and relay status on OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Time: ");
  display.println(timeClient.getFormattedTime());
  display.println("----------------");
  display.print("ON Time: ");
  display.print(relayOnHour);
  display.print(":");
  display.println(relayOnMinute);
  display.print("OFF Time: ");
  display.print(relayOffHour);
  display.print(":");
  display.println(relayOffMinute);
  display.display();

  // Check if it's time to toggle the relay
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();
  
  if (currentHour == relayOnHour && currentMinute == relayOnMinute && !relayState) {
    digitalWrite(relayPin, LOW);  // Activate relay (adjust based on your relay configuration)
    relayState = true;
    Serial.println("Relay ON");
  } else if (currentHour == relayOffHour && currentMinute == relayOffMinute && relayState) {
    digitalWrite(relayPin, HIGH);  // Deactivate relay (adjust based on your relay configuration)
    relayState = false;
    Serial.println("Relay OFF");
  }

  // Handle client requests
  server.handleClient();
}
