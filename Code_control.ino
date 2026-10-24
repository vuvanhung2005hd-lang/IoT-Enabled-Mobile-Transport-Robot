#include <WiFi.h>
#include <WebServer.h>

// --- WIFI CONFIGURATION ---
const char* ssid = "My_car";
const char* password = "12345678";

WebServer server(80);

// --- L298N PINS ---
const int ENA = 33;
const int IN1 = 14;
const int IN2 = 27;
const int IN3 = 26;
const int IN4 = 25;
const int ENB = 32;

// --- HC-SR04 SENSOR PINS ---
const int TRIG_PIN = 4;
const int ECHO_PIN = 18;  
const int SAFE_DISTANCE = 25;

// --- PWM CONFIGURATION ---
const int PWM_FREQ = 5000;
const int PWM_RES  = 8;
int carSpeed = 255;

String carStatus = "STOP";

// --- ULTRASONIC SENSOR VARIABLES ---
volatile long echoStart = 0;
volatile long echoDuration = 0;
volatile bool isMeasurementDone = false;
int currentDistance = 999;

void IRAM_ATTR echoISR() {
  if (digitalRead(ECHO_PIN) == HIGH) {
    echoStart = micros();
  } else {
    echoDuration = micros() - echoStart;
    isMeasurementDone = true;
  }
}

unsigned long lastTrigTime = 0;
void triggerSensor() {
  unsigned long now = millis();
  if (now - lastTrigTime < 60) return; 
  lastTrigTime = now;

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
}

void updateDistance() {
  if (isMeasurementDone) {
    isMeasurementDone = false;
    long duration = echoDuration;
    if (duration == 0 || duration > 25000) {
      currentDistance = 999;
    } else {
      currentDistance = (int)(duration * 0.034f / 2.0f);
    }
    Serial.print("Distance: ");
    Serial.print(currentDistance);
    Serial.println(" cm");
  }
}

// --- WEB INTERFACE (HTML/JS/CSS) ---
const char webInterface[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
  <title>ESP32 Car Control</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; -webkit-tap-highlight-color: transparent; }
    body {
      font-family: Arial, sans-serif;
      background: #1a1a2e;
      color: #ecf0f1;
      display: flex;
      flex-direction: column;
      align-items: center;
      min-height: 100vh;
      padding: 24px 16px;
    }
    .title { font-size: 13px; color: #7f8c8d; letter-spacing: 3px; text-transform: uppercase; margin-bottom: 4px; }
    .subtitle { font-size: 22px; font-weight: bold; margin-bottom: 20px; }
    .status-bar {
      background: #16213e;
      border-radius: 10px;
      padding: 10px 16px;
      width: 100%;
      max-width: 360px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 16px;
    }
    .wifi-dot { width: 10px; height: 10px; border-radius: 50%; background: #2ecc71; display: inline-block; margin-right: 8px; }
    .wifi-label { font-size: 13px; color: #bdc3c7; }
    .badge {
      font-size: 12px; background: #0f3460; color: #4fc3f7;
      padding: 4px 14px; border-radius: 20px; font-weight: bold; transition: all 0.2s;
    }
    .sensor-bar {
      background: #16213e; border-radius: 10px; padding: 10px 16px;
      width: 100%; max-width: 360px;
      display: flex; justify-content: space-between; align-items: center;
      margin-bottom: 16px;
    }
    .sensor-label { font-size: 13px; color: #7f8c8d; }
    .sensor-val { font-size: 16px; font-weight: bold; color: #2ecc71; transition: color 0.3s; }
    .pad {
      display: grid;
      grid-template-columns: repeat(3, 100px);
      grid-template-rows: repeat(3, 100px);
      gap: 12px; margin-bottom: 20px;
    }
    .btn {
      width: 100px; height: 100px; border-radius: 50%;
      border: 3px solid #3498db;
      background: radial-gradient(circle at 35% 35%, #2980b9, #1e3a5f);
      color: white; font-size: 13px; font-weight: bold; cursor: pointer;
      display: flex; flex-direction: column; align-items: center;
      justify-content: center; gap: 4px;
      box-shadow: 0 4px 15px rgba(52,152,219,0.35);
      user-select: none; touch-action: manipulation;
    }
    .btn:active { transform: scale(0.93); box-shadow: 0 2px 6px rgba(52,152,219,0.2); }
    .btn-icon { font-size: 20px; line-height: 1; }
    .btn-stop {
      border-color: #e74c3c;
      background: radial-gradient(circle at 35% 35%, #c0392b, #7b1010);
      box-shadow: 0 4px 15px rgba(231,76,60,0.45);
    }
    .btn-empty { visibility: hidden; }
    .panel { background: #16213e; border-radius: 12px; padding: 14px 18px; width: 100%; max-width: 360px; margin-bottom: 12px; }
    .panel-row { display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px; }
    .panel-label { font-size: 13px; color: #7f8c8d; }
    .panel-val { font-size: 13px; color: #4fc3f7; font-weight: bold; }
    input[type=range] { width: 100%; accent-color: #3498db; }
  </style>
</head>
<body>
  <div class="title">ESP32 CAR CONTROL</div>
  <div class="subtitle">&#128663; Remote Control</div>

  <div class="status-bar">
    <div><span class="wifi-dot"></span><span class="wifi-label">My_car</span></div>
    <div class="badge" id="badge">STOP</div>
  </div>

  <div class="sensor-bar">
    <span class="sensor-label">&#128268; Distance</span>
    <span class="sensor-val" id="dist-val">-- cm</span>
  </div>

  <div class="pad">
    <div class="btn-empty"></div>
    <button class="btn"
      onmousedown="cmd('forward','FORWARD')" onmouseup="cmd('stop','STOP')"
      ontouchstart="cmd('forward','FORWARD')" ontouchend="cmd('stop','STOP')">
      <span class="btn-icon">&#9650;</span>FORWARD
    </button>
    <div class="btn-empty"></div>

    <button class="btn"
      onmousedown="cmd('left','LEFT')" onmouseup="cmd('stop','STOP')"
      ontouchstart="cmd('left','LEFT')" ontouchend="cmd('stop','STOP')">
      <span class="btn-icon">&#9664;</span>LEFT
    </button>
    <button class="btn btn-stop"
      onmousedown="cmd('stop','STOP')" ontouchstart="cmd('stop','STOP')">
      <span class="btn-icon">&#9632;</span>STOP
    </button>
    <button class="btn"
      onmousedown="cmd('right','RIGHT')" onmouseup="cmd('stop','STOP')"
      ontouchstart="cmd('right','RIGHT')" ontouchend="cmd('stop','STOP')">
      <span class="btn-icon">&#9654;</span>RIGHT
    </button>

    <div class="btn-empty"></div>
    <button class="btn"
      onmousedown="cmd('backward','BACKWARD')" onmouseup="cmd('stop','STOP')"
      ontouchstart="cmd('backward','BACKWARD')" ontouchend="cmd('stop','STOP')">
      <span class="btn-icon">&#9660;</span>BACKWARD
    </button>
    <div class="btn-empty"></div>
  </div>

  <div class="panel">
    <div class="panel-row">
      <span class="panel-label">&#128286; Speed</span>
      <span class="panel-val" id="spd-label">100%</span>
    </div>
    <input type="range" min="0" max="255" value="255"
      oninput="document.getElementById('spd-label').textContent=Math.round(this.value/255*100)+'%'; fetch('/speed?v='+this.value)">
  </div>

  <script>
    function cmd(action, label) {
      fetch('/' + action);
      var b = document.getElementById('badge');
      b.textContent = label;
      b.style.color = (action === 'stop') ? '#e74c3c' : '#2ecc71';
    }

    function pollDistance() {
      fetch('/distance')
        .then(r => r.text())
        .then(val => {
          var el = document.getElementById('dist-val');
          var cm = parseInt(val);
          if (cm >= 999) {
            el.textContent = '> 4m';
            el.style.color = '#2ecc71';
          } else {
            el.textContent = cm + ' cm';
            el.style.color = cm < 25 ? '#e74c3c' : cm < 50 ? '#f39c12' : '#2ecc71';
          }
        })
        .catch(() => {});
    }
    setInterval(pollDistance, 300);
  </script>
</body>
</html>
)rawliteral";

// --- MOTOR FUNCTIONS ---
void moveForward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  ledcWrite(ENA, carSpeed);   ledcWrite(ENB, carSpeed);
}
void moveBackward() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  ledcWrite(ENA, carSpeed);   ledcWrite(ENB, carSpeed);
}
void turnLeft() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  ledcWrite(ENA, carSpeed);   ledcWrite(ENB, carSpeed);
}
void turnRight() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  ledcWrite(ENA, carSpeed);   ledcWrite(ENB, carSpeed);
}
void brakeCar() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  ledcWrite(ENA, 0);      ledcWrite(ENB, 0);
}

void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  ledcAttach(ENA, PWM_FREQ, PWM_RES);
  ledcAttach(ENB, PWM_FREQ, PWM_RES);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  attachInterrupt(digitalPinToInterrupt(ECHO_PIN), echoISR, CHANGE);

  brakeCar();

  WiFi.softAP(ssid, password);
  Serial.print("IP: http://"); Serial.println(WiFi.softAPIP());

  server.on("/",         []() { server.send(200, "text/html", webInterface); });
  server.on("/forward",  []() { carStatus = "FORWARD"; server.send(200, "text/plain", "OK"); });
  server.on("/backward", []() { carStatus = "BACKWARD"; server.send(200, "text/plain", "OK"); });
  server.on("/left",     []() { carStatus = "LEFT"; server.send(200, "text/plain", "OK"); });
  server.on("/right",    []() { carStatus = "RIGHT"; server.send(200, "text/plain", "OK"); });
  server.on("/stop",     []() { carStatus = "STOP"; server.send(200, "text/plain", "OK"); });
  server.on("/speed",    []() {
    if (server.hasArg("v")) carSpeed = constrain(server.arg("v").toInt(), 0, 255);
    server.send(200, "text/plain", "OK");
  });
  server.on("/distance", []() {
    server.send(200, "text/plain", String(currentDistance));
  });

  server.begin();
}

void loop() {
  server.handleClient();

  triggerSensor();       
  updateDistance();   

  // Safe distance logic
  if (carStatus == "FORWARD" && currentDistance > 0 && currentDistance < SAFE_DISTANCE) {
    brakeCar();
    carStatus = "STOP";
    Serial.println("OBSTACLE! BRAKING! " + String(currentDistance) + "cm");
  } else {
    if      (carStatus == "FORWARD")  moveForward();
    else if (carStatus == "BACKWARD") moveBackward();
    else if (carStatus == "LEFT")     turnLeft();
    else if (carStatus == "RIGHT")    turnRight();
    else                              brakeCar();
  }
}