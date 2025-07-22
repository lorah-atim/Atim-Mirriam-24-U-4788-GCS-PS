#include <WiFi.h>
#include <WebServer.h>

// WiFi credentials
const char* ssid = "personalSSID/networkName";
const char* password = "personalWifiPASSWORD";

// Pin definitions
const int ldrPin = 19;
const int motionPin = 23;
const int mosfetPin = 12;             // Security Light via MOSFET
const int livingRoomLightPin = 27;    // Indoor Light
const int bedRoomLightPin = 26;       // Bedroom Light

// State variables
bool overwrite = false;
int ldrThreshold = 1500;

WebServer server(80);

// HTML and JS dashboard
const char* html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Smart Home Dashboard</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { margin: 0; font-family: Arial; background: #f4f4f4; display: flex; }
    .sidebar { width: 220px; background: rgb(0,44,110); color: white; height: 100vh; padding: 20px 15px; }
    .sidebar h2 { font-size: 20px; margin-bottom: 20px; border-bottom: 1px solid #ccc; padding-bottom: 10px; }
    .sidebar ul { list-style-type: none; padding: 0; }
    .sidebar ul li { margin: 15px 0; }
    .sidebar ul li a { color: white; text-decoration: none; }
    .main { flex: 1; display: flex; flex-direction: column; }
    header { background: rgb(0,44,110); color: #fff; padding: 20px; text-align: center; }
    .container { padding: 20px; }
    h1 { font-size: 22px; }
    .profile { text-align: center; margin-bottom: 20px; }
    .profile img { width: 80px; height: 80px; border-radius: 50%; border: 2px solid #ccc; }
    .section { background: white; padding: 15px; margin-bottom: 20px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
    .controls label { display: block; margin: 10px 0 5px; }
    button { padding: 10px 15px; margin-top: 5px; background: rgb(34,179,21); border: none; color: white; border-radius: 4px; cursor: pointer; }
    button:hover { background: rgb(28,139,17); }
    .footer { text-align: center; padding: 15px; background: rgb(0,44,110); color: white; font-size: 14px; margin-top: auto; }
  </style>
  <script>
    function greet() {
      const hour = new Date().getHours();
      let msg = "Hello";
      if (hour < 12) msg = "Good morning";
      else if (hour < 18) msg = "Good afternoon";
      else msg = "Good evening";
      document.getElementById('greeting').innerText = msg + ", welcome to Bit Creator Systems!";
    }

    function toggleLight(type) {
      let endpoint = '';
      switch (type) {
        case 'livingRoom': endpoint = '/toggleIndoor'; break;
        case 'bedroom': endpoint = '/toggleBedroom'; break;
        case 'security': endpoint = '/toggleSecurity'; break;
        default: return alert("Unknown light type");
      }

      fetch(endpoint)
        .then(res => res.text())
        .then(alert)
        .catch(err => alert("Failed to toggle light"));
    }

    function applyThreshold() {
      const value = document.getElementById("thresholdInput").value;
      fetch('/setThreshold?value=' + value)
        .then(res => res.text())
        .then(alert);
    }

    function showSection(id) {
      const sections = document.querySelectorAll(".section");
      sections.forEach(sec => sec.style.display = "none");
      document.getElementById(id).style.display = "block";
    }

    window.onload = () => {
      greet();
      showSection('lighting');
    }
  </script>
</head>
<body>
  <div class="sidebar">
    <h2>Dashboard</h2>
    <ul>
      <li><a href="#" onclick="showSection('lighting')">Lighting</a></li>
      <li><a href="#" onclick="showSection('settings')">Settings</a></li>
      <li><a href="#" onclick="showSection('security')">Security</a></li>
    </ul>
  </div>

  <div class="main">
    <header>
      <h1 id="greeting">Welcome</h1>
    </header>

    <div class="container">
      <div class="profile">
        <img src="/profile.jpg" alt="Profile Picture">
      </div>

      <div id="lighting" class="section controls">
        <h2>Lighting Control</h2>
        <label>Living Room Light</label>
        <button onclick="toggleLight('livingRoom')">Toggle Living Room Light</button>

        <label>Bedroom Light</label>
        <button onclick="toggleLight('bedroom')">Toggle Bedroom Light</button>

        <label>Security Light</label>
        <button onclick="toggleLight('security')">Toggle Security Light</button>
      </div>

      <div id="settings" class="section" style="display:none">
        <h2>Settings</h2>
        <label>Threshold (LDR):</label>
        <input type="number" id="thresholdInput" value="1500">
        <button onclick="applyThreshold()">Apply</button>
      </div>

      <div id="security" class="section" style="display:none">
        <h2>Security</h2>
        <p>Motion detection and automation</p>
        <button onclick="fetch('/disableOverwrite').then(r => r.text()).then(alert)">Enable Automation</button>
      </div>
    </div>

    <div class="footer">
      Bit Creator Systems © 2025 - Developed by Your Team<br>
      ATIM MIRRIAM
    </div>
  </div>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Pin modes
  pinMode(ldrPin, INPUT);
  pinMode(motionPin, INPUT);
  pinMode(mosfetPin, OUTPUT);
  pinMode(livingRoomLightPin, OUTPUT);
  pinMode(bedRoomLightPin, OUTPUT);

  // Initialize lights to OFF
  digitalWrite(mosfetPin, LOW);
  digitalWrite(livingRoomLightPin, LOW);
  digitalWrite(bedRoomLightPin, LOW);

  // Routes
  server.on("/", []() {
    server.send(200, "text/html", html);
  });

  server.on("/toggleIndoor", []() {
    bool state = digitalRead(livingRoomLightPin);
    digitalWrite(livingRoomLightPin, !state);
    server.send(200, "text/plain", !state ? "Living Room Light ON" : "Living Room Light OFF");
  });

  server.on("/toggleBedroom", []() {
    bool state = digitalRead(bedRoomLightPin);
    digitalWrite(bedRoomLightPin, !state);
    server.send(200, "text/plain", !state ? "Bedroom Light ON" : "Bedroom Light OFF");
  });

  server.on("/toggleSecurity", []() {
    overwrite = true;
    bool state = digitalRead(mosfetPin);
    digitalWrite(mosfetPin, !state);
    server.send(200, "text/plain", !state ? "Security Light ON (manual)" : "Security Light OFF (manual)");
  });

  server.on("/disableOverwrite", []() {
    overwrite = false;
    server.send(200, "text/plain", "Automation re-enabled");
  });

  server.on("/setThreshold", []() {
    if (server.hasArg("value")) {
      ldrThreshold = server.arg("value").toInt();
      Serial.print("Threshold set to: ");
      Serial.println(ldrThreshold);
      server.send(200, "text/plain", "Threshold updated to " + String(ldrThreshold));
    } else {
      server.send(400, "text/plain", "Missing 'value' parameter");
    }
  });

  server.begin();
}

void loop() {
  server.handleClient();

  int l
