/*
  MyCarRemote
  A custom web-based remote control for the QD001 car:
  - drive (mecanum move/strafe/rotate) with adjustable speed, mouse/touch or keyboard
  - left/right lights: off / solid / breathing
  - buzzer presets: chirp / honk / alarm / victory
  - head servo, manual or as a sweeping radar with the ultrasonic sensor
  - autonomous modes: obstacle avoidance and line following
  - auto-stop if the browser stops sending heartbeats mid-drive

  Connect your computer's WiFi to the "MyCarRemote" hotspot below,
  then open http://192.168.4.1 in a browser.
*/

#include <ACB_SmartCar_V2.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ultrasonic.h>

const char* ssid = "MyCarRemote";
const char* password = "12345678";  // must be 8+ characters

const int BuzzerPin = 33;
const int HeadPin = 25;
const int LeftLedPin = 12;
const int RightLedPin = 2;
const int UltrasonicTrigPin = 13;
const int UltrasonicEchoPin = 14;
const int LeftTrackPin = 35;
const int MiddleTrackPin = 36;
const int RightTrackPin = 39;

const unsigned long DriveTimeoutMs = 800;    // auto-stop if no heartbeat within this window
const unsigned long DistancePollMs = 150;
const unsigned long BreathePeriodMs = 2000;
const unsigned long ObstacleTurnMs = 1200;
const unsigned long LineFollowStepMs = 30;
const int ObstacleDistanceCm = 25;
const int BlackLineThreshold = 2000;

ACB_SmartCar_V2 ACB_SmartCar;
WebServer server(80);
Servo headServo;
ultrasonic myUltrasonic;

int headAngle = 90;
int driveSpeed = 255;

// light mode: 0 = off, 1 = solid on, 2 = breathing
int leftLightMode = 0;
int rightLightMode = 0;

bool isDriving = false;
unsigned long lastMoveTime = 0;

int currentDistance = 0;
unsigned long lastDistanceRead = 0;

// radar sweep (reuses the head servo + ultrasonic sensor)
bool radarMode = false;
int radarAngle = 0;
int radarDir = 1;
int radarDistances[19];  // one entry per 10 degrees, 0..180

// autonomous drive: 0 = manual, 1 = obstacle avoid, 2 = line follow
int autoMode = 0;
bool oaTurning = false;
unsigned long oaTurnStart = 0;
unsigned long lastLineFollowStep = 0;

void beepChirp() {
  tone(BuzzerPin, 880);
  delay(120);
  noTone(BuzzerPin);
  delay(40);
  tone(BuzzerPin, 1175);
  delay(150);
  noTone(BuzzerPin);
}

void beepHonk() {
  tone(BuzzerPin, 220);
  delay(150);
  noTone(BuzzerPin);
  delay(60);
  tone(BuzzerPin, 220);
  delay(150);
  noTone(BuzzerPin);
}

void beepAlarm() {
  for (int i = 0; i < 4; i++) {
    tone(BuzzerPin, 1000);
    delay(100);
    noTone(BuzzerPin);
    tone(BuzzerPin, 700);
    delay(100);
    noTone(BuzzerPin);
  }
}

void beepVictory() {
  int notes[] = { 262, 330, 392, 523 };
  for (int i = 0; i < 4; i++) {
    tone(BuzzerPin, notes[i]);
    delay(130);
    noTone(BuzzerPin);
    delay(20);
  }
}

void updateLight(int pin, int mode, unsigned long now) {
  if (mode == 0) {
    ledcWrite(pin, 0);
  } else if (mode == 1) {
    ledcWrite(pin, 255);
  } else {
    unsigned long t = now % BreathePeriodMs;
    unsigned long half = BreathePeriodMs / 2;
    int duty = (t < half) ? map(t, 0, half, 0, 255) : map(t, half, BreathePeriodMs, 255, 0);
    ledcWrite(pin, duty);
  }
}

void runObstacleAvoid(unsigned long now) {
  if (oaTurning) {
    if (now - oaTurnStart > ObstacleTurnMs) {
      ACB_SmartCar.Move(Stop, 0);
      oaTurning = false;
    }
  } else if (currentDistance > 0 && currentDistance <= ObstacleDistanceCm) {
    ACB_SmartCar.Move(Contrarotate, 180);
    oaTurning = true;
    oaTurnStart = now;
  } else {
    ACB_SmartCar.Move(Forward, driveSpeed);
  }
}

void runLineFollow(unsigned long now) {
  if (now - lastLineFollowStep < LineFollowStepMs) return;
  lastLineFollowStep = now;

  int l = analogRead(LeftTrackPin);
  int m = analogRead(MiddleTrackPin);
  int r = analogRead(RightTrackPin);
  int turnSpeed = constrain((int)(driveSpeed * 0.85), 60, 255);

  if (l < BlackLineThreshold && m >= BlackLineThreshold && r < BlackLineThreshold) {
    ACB_SmartCar.Move(Forward, driveSpeed);
  } else if (l < BlackLineThreshold && m >= BlackLineThreshold && r >= BlackLineThreshold) {
    ACB_SmartCar.Move(Forward, driveSpeed);
  } else if (l >= BlackLineThreshold && m >= BlackLineThreshold && r < BlackLineThreshold) {
    ACB_SmartCar.Move(Forward, driveSpeed);
  } else if (l >= BlackLineThreshold && m < BlackLineThreshold && r < BlackLineThreshold) {
    ACB_SmartCar.Move(Contrarotate, turnSpeed);
  } else if (l < BlackLineThreshold && m < BlackLineThreshold && r >= BlackLineThreshold) {
    ACB_SmartCar.Move(Clockwise, turnSpeed);
  } else if (l >= BlackLineThreshold && m >= BlackLineThreshold && r >= BlackLineThreshold) {
    ACB_SmartCar.Move(Forward, driveSpeed);
  }
}

void handleRoot() {
  String page = R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  :root {
    --bg: #14141a;
    --panel: #1e1e27;
    --panel-2: #262633;
    --accent: #6d7bff;
    --accent-2: #ff9d3d;
    --text: #eee;
    --text-dim: #9a9aab;
  }
  * { box-sizing: border-box; }
  body {
    margin: 0;
    padding: 20px 14px 40px;
    font-family: -apple-system, Segoe UI, Roboto, sans-serif;
    background: radial-gradient(circle at 50% 0%, #23233066, var(--bg) 60%);
    color: var(--text);
    text-align: center;
    user-select: none;
  }
  h1 {
    font-size: 1.4em;
    letter-spacing: 0.5px;
    margin: 4px 0 20px;
    background: linear-gradient(90deg, var(--accent), var(--accent-2));
    -webkit-background-clip: text;
    background-clip: text;
    color: transparent;
    font-weight: 800;
  }
  .card {
    background: var(--panel);
    border: 1px solid #2e2e3a;
    border-radius: 16px;
    padding: 18px;
    max-width: 340px;
    margin: 0 auto 16px;
    box-shadow: 0 6px 20px rgba(0,0,0,0.35);
  }
  .card h2 {
    font-size: 0.75em;
    text-transform: uppercase;
    letter-spacing: 1.5px;
    color: var(--text-dim);
    margin: 0 0 14px;
  }
  .pad {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 10px;
  }
  .pad button {
    aspect-ratio: 1;
    font-size: 1.4em;
    border-radius: 12px;
    border: none;
    background: var(--panel-2);
    color: var(--text);
    box-shadow: inset 0 0 0 1px #34344233;
    transition: background .12s, transform .06s;
  }
  .pad button:active, .pad button.held {
    background: var(--accent);
    transform: scale(0.94);
  }
  .pad.disabled button { opacity: 0.3; pointer-events: none; }
  .pad .label { font-size: 0.45em; text-transform: uppercase; letter-spacing: 1px; color: var(--text-dim); display:block; margin-top:2px; }
  .hint { font-size: 0.7em; color: var(--text-dim); margin-top: 12px; }

  .row { display: flex; align-items: center; justify-content: space-between; gap: 10px; }
  .row + .row { margin-top: 14px; }

  input[type=range] {
    -webkit-appearance: none;
    flex: 1;
    height: 6px;
    border-radius: 3px;
    background: var(--panel-2);
    outline: none;
  }
  input[type=range]::-webkit-slider-thumb {
    -webkit-appearance: none;
    width: 20px; height: 20px;
    border-radius: 50%;
    background: var(--accent);
    border: 2px solid #fff2;
    cursor: pointer;
  }
  input[type=range]:disabled { opacity: 0.3; }
  .valuePill {
    min-width: 50px;
    font-size: 0.85em;
    color: var(--text-dim);
    font-variant-numeric: tabular-nums;
  }

  .lightLabel { font-size: 0.95em; }
  .segmented {
    display: flex;
    background: var(--panel-2);
    border-radius: 10px;
    padding: 3px;
    gap: 3px;
  }
  .segmented button {
    border: none;
    background: transparent;
    color: var(--text-dim);
    font-size: 0.75em;
    padding: 7px 10px;
    border-radius: 8px;
  }
  .segmented button.active {
    background: var(--accent-2);
    color: #1b1b1f;
    font-weight: 700;
  }

  .beepGrid {
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 10px;
  }
  .beepGrid button {
    padding: 14px 0;
    border-radius: 12px;
    border: none;
    background: linear-gradient(135deg, var(--accent-2), #ffb85e);
    color: #1b1b1f;
    font-weight: 800;
    font-size: 0.85em;
    letter-spacing: 0.5px;
    text-transform: uppercase;
  }
  .beepGrid button:active { transform: scale(0.96); }

  #radarBtn {
    width: 100%;
    margin-top: 10px;
    padding: 12px;
    border-radius: 10px;
    border: none;
    background: var(--panel-2);
    color: var(--text);
    font-weight: 700;
    font-size: 0.85em;
  }
  #radarBtn.on { background: var(--accent); }
</style>
</head>
<body>
<h1>My Car Remote</h1>

<div class="card">
  <h2>Drive</h2>
  <div class="pad" id="drivePad">
    <button id="btn-tl" ontouchstart="startMove('tl')" ontouchend="stopMove()" onmousedown="startMove('tl')" onmouseup="stopMove()" onmouseleave="stopMove()">&#8634;<span class="label">Turn L</span></button>
    <button id="btn-f"  ontouchstart="startMove('f')"  ontouchend="stopMove()" onmousedown="startMove('f')"  onmouseup="stopMove()" onmouseleave="stopMove()">&#8593;<span class="label">Fwd</span></button>
    <button id="btn-tr" ontouchstart="startMove('tr')" ontouchend="stopMove()" onmousedown="startMove('tr')" onmouseup="stopMove()" onmouseleave="stopMove()">&#8635;<span class="label">Turn R</span></button>

    <button id="btn-l"  ontouchstart="startMove('l')"  ontouchend="stopMove()" onmousedown="startMove('l')"  onmouseup="stopMove()" onmouseleave="stopMove()">&#8592;<span class="label">Left</span></button>
    <button id="btn-b"  ontouchstart="startMove('b')"  ontouchend="stopMove()" onmousedown="startMove('b')"  onmouseup="stopMove()" onmouseleave="stopMove()">&#8595;<span class="label">Back</span></button>
    <button id="btn-r"  ontouchstart="startMove('r')"  ontouchend="stopMove()" onmousedown="startMove('r')"  onmouseup="stopMove()" onmouseleave="stopMove()">&#8594;<span class="label">Right</span></button>
  </div>
  <div class="hint">Keyboard: W A S D to drive &middot; Q E to strafe</div>
</div>

<div class="card">
  <h2>Auto Drive</h2>
  <div class="segmented" id="autoSeg">
    <button class="active" data-v="manual" onclick="setAutoMode('manual')" style="flex:1;">Manual</button>
    <button data-v="avoid" onclick="setAutoMode('avoid')" style="flex:1;">Avoid</button>
    <button data-v="line" onclick="setAutoMode('line')" style="flex:1;">Line</button>
  </div>
  <div class="hint">Avoid = dodges obstacles with the ultrasonic sensor &middot; Line = follows a dark line under the car</div>
</div>

<div class="card">
  <h2>Sensor</h2>
  <div class="row">
    <span class="lightLabel">Distance ahead</span>
    <span class="valuePill" id="distanceLabel">-- cm</span>
  </div>
</div>

<div class="card">
  <h2>Radar</h2>
  <svg id="radarSvg" viewBox="0 0 300 170" style="width:100%;max-width:300px;">
    <path d="M10,160 A140,140 0 0 1 290,160" fill="none" stroke="#33333f" stroke-width="1"/>
    <path d="M55,160 A95,95 0 0 1 245,160" fill="none" stroke="#2a2a35" stroke-width="1"/>
    <path d="M100,160 A50,50 0 0 1 200,160" fill="none" stroke="#2a2a35" stroke-width="1"/>
    <line x1="150" y1="160" x2="150" y2="20" stroke="#2a2a35" stroke-width="1"/>
    <g id="radarDots"></g>
  </svg>
  <button id="radarBtn" onclick="toggleRadar()">Start Sweep</button>
</div>

<div class="card">
  <h2>Speed</h2>
  <div class="row">
    <input type="range" min="60" max="255" value="255" id="speedSlider"
           oninput="document.getElementById('speedLabel').innerText = Math.round(this.value/255*100) + '%'"
           onchange="fetch('/speed?value=' + this.value)">
    <span class="valuePill" id="speedLabel">100%</span>
  </div>
</div>

<div class="card">
  <h2>Lights</h2>
  <div class="row">
    <span class="lightLabel">Left</span>
    <div class="segmented" id="leftSeg">
      <button class="active" data-v="off" onclick="setLight('left','off')">Off</button>
      <button data-v="on" onclick="setLight('left','on')">On</button>
      <button data-v="breathe" onclick="setLight('left','breathe')">Breathe</button>
    </div>
  </div>
  <div class="row">
    <span class="lightLabel">Right</span>
    <div class="segmented" id="rightSeg">
      <button class="active" data-v="off" onclick="setLight('right','off')">Off</button>
      <button data-v="on" onclick="setLight('right','on')">On</button>
      <button data-v="breathe" onclick="setLight('right','breathe')">Breathe</button>
    </div>
  </div>
</div>

<div class="card">
  <h2>Head</h2>
  <div class="row">
    <input type="range" min="0" max="180" value="90" id="headSlider"
           oninput="moveHead(this.value)"
           onchange="fetch('/head?angle=' + this.value)">
    <span class="valuePill" id="angleLabel">90&deg;</span>
  </div>
</div>

<div class="card">
  <h2>Beep</h2>
  <div class="beepGrid">
    <button onclick="fetch('/beep?type=chirp')">Chirp</button>
    <button onclick="fetch('/beep?type=honk')">Honk</button>
    <button onclick="fetch('/beep?type=alarm')">Alarm</button>
    <button onclick="fetch('/beep?type=victory')">Victory</button>
  </div>
</div>

<script>
var activeDir = null;
var heartbeat = null;
var autoModeActive = false;
var dirButtonId = { f:'btn-f', b:'btn-b', l:'btn-l', r:'btn-r', tl:'btn-tl', tr:'btn-tr' };

function startMove(dir) {
  if (autoModeActive || activeDir === dir) return;
  stopMove();
  activeDir = dir;
  fetch('/car?move=' + dir);
  heartbeat = setInterval(function () { fetch('/car?move=' + dir); }, 300);
  var btn = document.getElementById(dirButtonId[dir]);
  if (btn) btn.classList.add('held');
}

function stopMove() {
  if (heartbeat) { clearInterval(heartbeat); heartbeat = null; }
  if (activeDir) {
    var btn = document.getElementById(dirButtonId[activeDir]);
    if (btn) btn.classList.remove('held');
    fetch('/car?move=s');
  }
  activeDir = null;
}

var keyDirMap = {
  'w': 'f', 'arrowup': 'f',
  's': 'b', 'arrowdown': 'b',
  'a': 'tl', 'd': 'tr',
  'q': 'l', 'e': 'r',
  'arrowleft': 'l', 'arrowright': 'r'
};

document.addEventListener('keydown', function (e) {
  if (e.repeat || e.target.tagName === 'INPUT' || autoModeActive) return;
  var dir = keyDirMap[e.key.toLowerCase()];
  if (!dir) return;
  e.preventDefault();
  startMove(dir);
});

document.addEventListener('keyup', function (e) {
  var dir = keyDirMap[e.key.toLowerCase()];
  if (dir && dir === activeDir) stopMove();
});

var lastHeadSend = 0;
function moveHead(value) {
  document.getElementById('angleLabel').innerText = value + '°';
  var now = Date.now();
  if (now - lastHeadSend > 60) {
    lastHeadSend = now;
    fetch('/head?angle=' + value);
  }
}

function setLight(side, mode) {
  var seg = document.getElementById(side + 'Seg');
  Array.prototype.forEach.call(seg.children, function (btn) {
    btn.classList.toggle('active', btn.dataset.v === mode);
  });
  fetch('/light?side=' + side + '&mode=' + mode);
}

function setAutoMode(mode) {
  fetch('/automode?mode=' + mode);
}

var radarOn = false;
function toggleRadar() {
  radarOn = !radarOn;
  fetch('/radarmode?state=' + (radarOn ? 'on' : 'off'));
}

var maxRadarDist = 150;
function polarToXY(angle, dist) {
  var r = Math.min(dist, maxRadarDist) / maxRadarDist * 140;
  var theta = (180 - angle) * Math.PI / 180;
  return { x: 150 + r * Math.cos(theta), y: 160 - r * Math.sin(theta) };
}

function refreshRadar() {
  fetch('/radar').then(function (r) { return r.text(); }).then(function (txt) {
    var dots = document.getElementById('radarDots');
    dots.innerHTML = '';
    txt.split(',').forEach(function (p) {
      var parts = p.split(':');
      var angle = parseInt(parts[0]);
      var dist = parseInt(parts[1]);
      if (dist < 0) return;
      var pt = polarToXY(angle, dist);
      var c = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
      c.setAttribute('cx', pt.x);
      c.setAttribute('cy', pt.y);
      c.setAttribute('r', 4);
      c.setAttribute('fill', dist <= 25 ? '#ff5d5d' : '#6d7bff');
      dots.appendChild(c);
    });
  });
}

function applyStatus(radar, auto) {
  radarOn = radar === 1;
  var radarBtn = document.getElementById('radarBtn');
  radarBtn.innerText = radarOn ? 'Stop Sweep' : 'Start Sweep';
  radarBtn.classList.toggle('on', radarOn);
  document.getElementById('headSlider').disabled = radarOn;

  autoModeActive = auto !== 0;
  var modes = ['manual', 'avoid', 'line'];
  var seg = document.getElementById('autoSeg');
  Array.prototype.forEach.call(seg.children, function (btn) {
    btn.classList.toggle('active', btn.dataset.v === modes[auto]);
  });
  document.getElementById('drivePad').classList.toggle('disabled', autoModeActive);
}

setInterval(function () {
  fetch('/status').then(function (r) { return r.text(); }).then(function (t) {
    var parts = t.split(',');
    applyStatus(parseInt(parts[0]), parseInt(parts[1]));
  });
}, 1000);

setInterval(function () {
  if (radarOn) refreshRadar();
}, 300);

setInterval(function () {
  fetch('/distance').then(function (r) { return r.text(); }).then(function (d) {
    document.getElementById('distanceLabel').innerText = d + ' cm';
  });
}, 400);
</script>
</body>
</html>
)HTML";
  server.send(200, "text/html", page);
}

void handleMove() {
  if (autoMode != 0) {
    server.send(200, "text/plain", "IGNORED");
    return;
  }
  if (server.hasArg("move")) {
    String move = server.arg("move");
    lastMoveTime = millis();
    if (move == "f") { ACB_SmartCar.Move(Forward, driveSpeed); isDriving = true; }
    else if (move == "b") { ACB_SmartCar.Move(Backward, driveSpeed); isDriving = true; }
    else if (move == "l") { ACB_SmartCar.Move(Move_Right, driveSpeed); isDriving = true; }
    else if (move == "r") { ACB_SmartCar.Move(Move_Left, driveSpeed); isDriving = true; }
    else if (move == "tl") { ACB_SmartCar.Move(Contrarotate, driveSpeed); isDriving = true; }
    else if (move == "tr") { ACB_SmartCar.Move(Clockwise, driveSpeed); isDriving = true; }
    else if (move == "s") { ACB_SmartCar.Move(Stop, 0); isDriving = false; }
  }
  server.send(200, "text/plain", "OK");
}

void handleSpeed() {
  if (server.hasArg("value")) {
    driveSpeed = constrain(server.arg("value").toInt(), 0, 255);
  }
  server.send(200, "text/plain", "OK");
}

void handleLight() {
  if (server.hasArg("side") && server.hasArg("mode")) {
    String modeStr = server.arg("mode");
    int mode = 0;
    if (modeStr == "on") mode = 1;
    else if (modeStr == "breathe") mode = 2;

    if (server.arg("side") == "left") leftLightMode = mode;
    else if (server.arg("side") == "right") rightLightMode = mode;
  }
  server.send(200, "text/plain", "OK");
}

void handleBeep() {
  String type = server.hasArg("type") ? server.arg("type") : "chirp";
  if (type == "honk") beepHonk();
  else if (type == "alarm") beepAlarm();
  else if (type == "victory") beepVictory();
  else beepChirp();
  server.send(200, "text/plain", "OK");
}

void handleHead() {
  if (server.hasArg("angle")) {
    headAngle = constrain(server.arg("angle").toInt(), 0, 180);
    headServo.write(headAngle);
  }
  server.send(200, "text/plain", "OK");
}

void handleDistance() {
  server.send(200, "text/plain", String(currentDistance));
}

void handleRadarMode() {
  if (server.hasArg("state")) {
    bool on = server.arg("state") == "on";
    if (on) {
      autoMode = 0;
      radarMode = true;
      radarAngle = 0;
      radarDir = 1;
      for (int i = 0; i < 19; i++) radarDistances[i] = -1;
    } else {
      radarMode = false;
      headServo.write(headAngle);
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleRadarData() {
  String out = "";
  for (int i = 0; i < 19; i++) {
    if (i > 0) out += ",";
    out += String(i * 10) + ":" + String(radarDistances[i]);
  }
  server.send(200, "text/plain", out);
}

void handleAutoMode() {
  if (server.hasArg("mode")) {
    String m = server.arg("mode");
    if (m == "manual") autoMode = 0;
    else if (m == "avoid") autoMode = 1;
    else if (m == "line") autoMode = 2;

    if (autoMode != 0) {
      radarMode = false;
      headServo.write(90);
      oaTurning = false;
    } else {
      ACB_SmartCar.Move(Stop, 0);
      isDriving = false;
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleStatus() {
  server.send(200, "text/plain", String(radarMode ? 1 : 0) + "," + String(autoMode));
}

void setup() {
  Serial.begin(115200);

  ACB_SmartCar.Init();
  headServo.attach(HeadPin);
  headServo.write(headAngle);
  myUltrasonic.Init(UltrasonicTrigPin, UltrasonicEchoPin);

  pinMode(LeftTrackPin, INPUT);
  pinMode(MiddleTrackPin, INPUT);
  pinMode(RightTrackPin, INPUT);

  for (int i = 0; i < 19; i++) radarDistances[i] = -1;

  ledcAttach(LeftLedPin, 5000, 8);
  ledcAttach(RightLedPin, 5000, 8);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.print("Car ready. Connect to WiFi \"");
  Serial.print(ssid);
  Serial.println("\" then open http://192.168.4.1");

  server.on("/", handleRoot);
  server.on("/car", handleMove);
  server.on("/speed", handleSpeed);
  server.on("/light", handleLight);
  server.on("/beep", handleBeep);
  server.on("/head", handleHead);
  server.on("/distance", handleDistance);
  server.on("/radarmode", handleRadarMode);
  server.on("/radar", handleRadarData);
  server.on("/automode", handleAutoMode);
  server.on("/status", handleStatus);
  server.begin();
}

void loop() {
  server.handleClient();

  unsigned long now = millis();

  if (now - lastDistanceRead > DistancePollMs) {
    lastDistanceRead = now;
    int d = myUltrasonic.Ranging();
    currentDistance = d;
    if (radarMode) {
      radarDistances[radarAngle / 10] = d;
      radarAngle += radarDir * 10;
      if (radarAngle >= 180) { radarAngle = 180; radarDir = -1; }
      else if (radarAngle <= 0) { radarAngle = 0; radarDir = 1; }
      headServo.write(radarAngle);
    }
  }

  updateLight(LeftLedPin, leftLightMode, now);
  updateLight(RightLedPin, rightLightMode, now);

  if (autoMode == 1) {
    runObstacleAvoid(now);
  } else if (autoMode == 2) {
    runLineFollow(now);
  } else if (isDriving && now - lastMoveTime > DriveTimeoutMs) {
    ACB_SmartCar.Move(Stop, 0);
    isDriving = false;
  }
}
