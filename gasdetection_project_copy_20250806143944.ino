#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include "DHT.h"

// WiFi Credentials 
const char* ssid = "Test";
const char* password = "laurah24@GMT";

// === Hardware Pins ===
#define MQ2_PIN     34
#define BUZZER      23
#define LED_RED     25
#define LED_GREEN   26
#define DHT_PIN     15
#define DHT_TYPE    DHT11

// Sensor Objects 
DHT dht(DHT_PIN, DHT_TYPE);

//  Globals 
WebServer server(80);
int gasLevel = 0;
String gasLabel = "Flammable Gas & Smoke";
String airStatus = "Initializing...";
String timeStr = "";
String dateStr = "";
float temperature = 0.0;
float humidity = 0.0;

// Dashboard HTML (stored in PROGMEM to save RAM)
const char htmlContent[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>Smart Gas Monitoring</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    body {
      background: linear-gradient(135deg, #2b2d42, #1a1a2e);
      font-family: 'Segoe UI', sans-serif;
      color: #f0f0f0;
      margin: 0;
      padding: 0;
      animation: fadeIn 1.5s ease;
    }

    @keyframes fadeIn {
      from {opacity: 0;}
      to {opacity: 1;}
    }

    .container {
      max-width: 800px;
      margin: 50px auto;
      padding: 30px;
      background-color: rgba(255, 255, 255, 0.05);
      border-radius: 20px;
      box-shadow: 0 0 20px rgba(128, 0, 128, 0.6);
      backdrop-filter: blur(8px);
      transition: all 0.3s ease-in-out;
    }

    .container:hover {
      transform: scale(1.01);
      box-shadow: 0 0 25px rgba(255, 20, 147, 0.6);
    }

    h1 {
      text-align: center;
      color: #bb86fc;
      margin-bottom: 10px;
    }

    .greeting {
      text-align: center;
      font-size: 1.2em;
      color: #8be9fd;
      margin-bottom: 30px;
    }

    .reading {
      font-size: 1.4em;
      padding: 15px;
      border-left: 5px solid #00b894;
      background-color: rgba(0, 255, 0, 0.05);
      margin: 15px 0;
      border-radius: 10px;
      transition: 0.4s ease;
    }

    .reading:hover {
      background-color: rgba(0, 255, 0, 0.12);
      transform: scale(1.02);
    }

    .danger {
      color: #ff5252;
      border-left: 5px solid #ff1744;
      background-color: rgba(255, 82, 82, 0.15);
    }

    .danger:hover {
      background-color: rgba(255, 82, 82, 0.25);
    }

    .label {
      font-weight: bold;
      color: #ffffff;
    }

    .gauge {
      position: relative;
      width: 200px;
      height: 100px;
      background: #222;
      border-top-left-radius: 100px;
      border-top-right-radius: 100px;
      margin: 40px auto;
      border: 4px solid #5e60ce;
      overflow: hidden;
    }

    .needle {
      position: absolute;
      width: 6px;
      height: 100px;
      background: #ff1744;
      left: 50%;
      bottom: 0;
      transform-origin: bottom center;
      transform: rotate(0deg);
      transition: transform 0.8s ease;
    }

    #dangerMessage {
      display: block;
      text-align: center;
      margin-top: 10px;
      font-weight: bold;
    }

    .danger #dangerMessage {
      animation: blink 1s infinite;
    }

    @keyframes blink {
      0%, 100% { opacity: 1; }
      50% { opacity: 0.2; }
    }

    .chart-container {
      margin-top: 30px;
      background: rgba(255, 255, 255, 0.03);
      padding: 20px;
      border-radius: 15px;
      height: 300px;
      width: 100%;
      position: relative;
      border: 1px solid rgba(255, 255, 255, 0.1);
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>Smart Indutrial and Home Gas Monitor</h1>
    <div class="greeting">Hello, stay safe! Monitoring your air quality...</div>

    <p class="reading"><span class="label">Temperature:</span> <span id="temp">%TEMPERATURE%</span> °C</p>
    <p class="reading"><span class="label">Humidity:</span> <span id="hum">%HUMIDITY%</span> %%</p>
    <p class="reading"><span class="label">Gas Level:</span> <span id="gas">%GAS%</span> ppm</p>
    <p class="reading"><span class="label">Gas Type:</span> <span id="type">%GASTYPE%</span></p>
    <p class="reading"><span class="label">Time:</span> <span id="time">%TIME%</span></p>
    <p class="reading"><span class="label">Date:</span> <span id="date">%DATE%</span></p>

    <div class="gauge">
      <div class="needle" id="needle"></div>
    </div>

    <div class="reading %STATUS_CLASS%" id="dangerAlert">
      <span class="label">Status:</span> <span id="status">%STATUS%</span>
      <span id="dangerMessage"></span>
    </div>

    <div class="chart-container">
      <canvas id="gasChart" width="400" height="300"></canvas>
    </div>
  </div>

<script>
function updateDashboard(data) {
  document.getElementById("temp").innerText = isNaN(data.temperature) ? "N/A" : data.temperature;
  document.getElementById("hum").innerText = isNaN(data.humidity) ? "N/A" : data.humidity;
  document.getElementById("gas").innerText = data.gas;
  document.getElementById("type").innerText = data.gasType;
  document.getElementById("time").innerText = data.time;
  document.getElementById("date").innerText = data.date;
  document.getElementById("status").innerText = data.status;

  let angle = Math.min(Math.max(data.gas, 0), 3000) / 3000 * 180;
  document.getElementById("needle").style.transform = "rotate(" + angle + "deg)";

  if (data.status.startsWith("DANGER")) {
    document.getElementById("dangerAlert").classList.add("danger");
    document.getElementById("dangerMessage").innerHTML = "<br><strong style='color:#ff1744; font-size:18px;'>Dangerous Gas Detected!</strong>";
  } else {
    document.getElementById("dangerAlert").classList.remove("danger");
    document.getElementById("dangerMessage").innerHTML = "";
  }

  if (gasChart) {
    gasChart.data.datasets[0].data = [data.temperature, data.humidity, data.gas];
    gasChart.update();
  }
}

function fetchData() {
  fetch('/data')
    .then(res => res.json())
    .then(data => updateDashboard(data));
}

let gasChart;
function drawChart() {
  const ctx = document.getElementById('gasChart').getContext('2d');
  gasChart = new Chart(ctx, {
    type: 'bar',
    data: {
      labels: ['Temperature (°C)', 'Humidity (%)', 'Gas (ppm)'],
      datasets: [{
        label: 'Sensor Readings',
        data: [0, 0, 0],
        backgroundColor: [
          '#f9c74f', // Yellow for Temperature
          '#90be6d', // Green for Humidity
          '#f94144'  // Red for Gas
        ],
        borderColor: '#fff',
        borderWidth: 1
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      scales: {
        y: {
          beginAtZero: true,
          grid: {
            display: true,
            color: 'rgba(255, 255, 255, 0.1)',
            drawBorder: true,
            borderColor: 'rgba(255, 255, 255, 0.3)'
          },
          ticks: {
            color: '#fff',
            stepSize: 20
          }
        },
        x: {
          grid: {
            display: true,
            color: 'rgba(255, 255, 255, 0.1)',
            drawBorder: true,
            borderColor: 'rgba(255, 255, 255, 0.3)'
          },
          ticks: {
            color: '#fff'
          }
        }
      },
      plugins: {
        legend: {
          labels: {
            color: '#fff',
            font: {
              size: 12
            }
          }
        }
      }
    }
  });
}

window.onload = function() {
  drawChart();
  fetchData();
  setInterval(fetchData, 2000);
};
</script>
</body>
</html>
)rawliteral";

// Replace placeholders with actual values
String htmlPage() {
  String page = FPSTR(htmlContent);
  page.replace("%TEMPERATURE%", String(temperature, 1));
  page.replace("%HUMIDITY%", String(humidity, 1));
  page.replace("%GAS%", String(gasLevel));
  page.replace("%GASTYPE%", gasLabel);
  page.replace("%TIME%", timeStr);
  page.replace("%DATE%", dateStr);
  page.replace("%STATUS%", airStatus);
  page.replace("%STATUS_CLASS%", airStatus.startsWith("DANGER") ? "danger" : "");
  return page;
}

// HTTP Handlers 
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleData() {
  String json = "{";
  json += "\"temperature\":" + String(temperature, 1) + ",";
  json += "\"humidity\":" + String(humidity, 1) + ",";
  json += "\"gas\":" + String(gasLevel) + ",";
  json += "\"gasType\":\"" + gasLabel + "\",";
  json += "\"status\":\"" + airStatus + "\",";
  json += "\"time\":\"" + timeStr + "\",";
  json += "\"date\":\"" + dateStr + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void updateTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    timeStr = "N/A";
    dateStr = "N/A";
    return;
  }
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
  timeStr = String(buffer);
  strftime(buffer, sizeof(buffer), "%d-%m-%Y", &timeinfo);
  dateStr = String(buffer);
}

void waitForTimeSync() {
  Serial.print("Waiting for NTP");
  struct tm timeinfo;
  unsigned long startTime = millis();
  
  while (!getLocalTime(&timeinfo)) {
    if (millis() - startTime > 1000) { // 10 second timeout
      Serial.println(" Failed! Using default time.");
      timeStr = "12:00:00";
      dateStr = "01-01-2023";
      return;
    }
    Serial.print(".");
    delay(500);
  }
  Serial.println(" Synced!");
}

void setup() {
  Serial.begin(115200);
  
  // Initialize hardware
  pinMode(MQ2_PIN, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(BUZZER, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
  
  dht.begin();
  
  // Connect to WiFi with timeout
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  unsigned long wifiStartTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStartTime < 15000) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(" Failed to connect!");
    while(1) delay(1000); // Halt if no WiFi
  }
  
  Serial.println("\nConnected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Configure time
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");
  waitForTimeSync();
  
  // Start web server
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  
  static unsigned long lastTimeUpdate = 0;
  if (millis() - lastTimeUpdate >= 1000) {
    lastTimeUpdate = millis();
    updateTime();
  }

  static unsigned long lastSensorRead = 0;
  if (millis() - lastSensorRead >= 200) {
    lastSensorRead = millis();

    // Read sensors with calibration
    int raw_adc = analogRead(MQ2_PIN);
    
    // Add calibration offset (adjust based on your sensor)
    int calibrated_adc = raw_adc - 300; // Example calibration offset
    
    // Ensure value stays within bounds
    calibrated_adc = constrain(calibrated_adc, 0, 4095);
    
    // Map to ppm with more reasonable range
    gasLevel = map(calibrated_adc, 0, 4095, 0, 1000); 

    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t)) temperature = t;
    if (!isnan(h)) humidity = h;

    // === Thresholds ===
    const int GAS_THRESHOLD = 300;
    const float NORMAL_TEMP_LIMIT = 30.0;
    const float FIRE_TEMP_THRESHOLD = 60.0;

    // === Danger Logic ===
    if (temperature >= FIRE_TEMP_THRESHOLD) {
      airStatus = "DANGER: Possible Fire!";
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_RED, HIGH);
      digitalWrite(BUZZER, HIGH);
    }
    else if (temperature > NORMAL_TEMP_LIMIT) {
      airStatus = "WARNING: High Temperature";
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_RED, HIGH);
      digitalWrite(BUZZER, HIGH);
    }
    else if (gasLevel >= GAS_THRESHOLD) {
      airStatus = "DANGER: High Gas";
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_RED, HIGH);
      digitalWrite(BUZZER, HIGH);
    }
    else {
      airStatus = "NORMAL";
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_RED, LOW);
      digitalWrite(BUZZER, LOW);
    }


    // Serial output for debugging
    Serial.print("Raw ADC: "); Serial.print(raw_adc);
    Serial.print(" | Calibrated: "); Serial.print(calibrated_adc);
    Serial.print(" | Gas: "); Serial.print(gasLevel); Serial.print(" ppm");
    Serial.print(" | Temp: "); Serial.print(temperature); Serial.print("°C");
    Serial.print(" | Hum: "); Serial.print(humidity); Serial.print("%");
    Serial.print(" | Status: "); Serial.println(airStatus);
  }
  
  delay(1);
}