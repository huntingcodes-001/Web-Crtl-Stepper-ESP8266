// Load Wi-Fi library
#include <ESP8266WiFi.h> // Use ESP8266WiFi.h instead of WiFi.h
#include <ESPAsyncWebServer.h>
#include "A4988.h"

const char* ssid = "you_ssid"; // Replace with your SSID
const char* password = "you_password"; // Replace with your Wi-Fi password

AsyncWebServer server(80);

int Step = 14;  // GPIO14---D5 of NodeMCU--Step of stepper motor driver
int Dire = 2;   // GPIO2---D4 of NodeMCU--Direction of stepper motor driver
int Sleep = 12; // GPIO12---D6 of NodeMCU--Control Sleep Mode on A4988
int MS1 = 13;   // GPIO13---D7 of NodeMCU--MS1 for A4988
int MS2 = 16;   // GPIO16---D0 of NodeMCU--MS2 for A4988
int MS3 = 15;   // GPIO15---D8 of NodeMCU--MS3 for A4988

// Motor Specs
const int spr = 200; // Steps per revolution
int RPM = 100;       // Motor Speed in revolutions per minute
int Microsteps = 1;  // Step size (1 for full steps, 2 for half steps, 4 for quarter steps, etc)

// Providing parameters for motor control
A4988 stepper(spr, Dire, Step, MS1, MS2, MS3);

// State variables
volatile bool isMoving = false;
volatile int currentDirection = 0; // 1 for clockwise, -1 for counterclockwise, 0 for stop

void handleRoot(AsyncWebServerRequest *request) {
  String html = R"=====(
    <!DOCTYPE html>
    <html>
    <head>
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <style>
        body {
          font-family: Arial, sans-serif;
          text-align: center;
          margin-top: 50px;
          background-color: #f4f4f4;
        }
        h1 {
          color: #333;
        }
        button {
          height: 50px;
          font-size: 20px;
          margin: 10px;
          background-color: #4CAF50;
          color: white;
          border: none;
          border-radius: 5px;
          cursor: pointer;
          transition: background-color 0.3s ease;
          box-sizing: border-box;
        }
        button:hover {
          background-color: #45a049;
        }
        #btnCW.active, #btnCCW.active {
          background-color: #f44336;
        }
      </style>
    </head>
    <body>
      <h1>Stepper Motor Control</h1>
      <button id="btnCW" onclick="sendRequest('/clockwise')">Clockwise</button>
      <button id="btnCCW" onclick="sendRequest('/counterclockwise')">Counterclockwise</button>
      <script>
        function sendRequest(endpoint) {
          var xhr = new XMLHttpRequest();
          xhr.open('GET', endpoint, true);
          xhr.send();
          
          if (endpoint === '/clockwise') {
            var btnCW = document.getElementById('btnCW');
            btnCW.innerHTML = 'Stop';
            btnCW.classList.add('active');
            btnCW.setAttribute('onclick', "sendRequest('/stop')");
            
            var btnCCW = document.getElementById('btnCCW');
            btnCCW.innerHTML = 'Counterclockwise';
            btnCCW.classList.remove('active');
            btnCCW.setAttribute('onclick', "sendRequest('/counterclockwise')");
          }
          
          if (endpoint === '/counterclockwise') {
            var btnCCW = document.getElementById('btnCCW');
            btnCCW.innerHTML = 'Stop';
            btnCCW.classList.add('active');
            btnCCW.setAttribute('onclick', "sendRequest('/stop')");
            
            var btnCW = document.getElementById('btnCW');
            btnCW.innerHTML = 'Clockwise';
            btnCW.classList.remove('active');
            btnCW.setAttribute('onclick', "sendRequest('/clockwise')");
          }
          
          if (endpoint === '/stop') {
            var btnCW = document.getElementById('btnCW');
            btnCW.innerHTML = 'Clockwise';
            btnCW.classList.remove('active');
            btnCW.setAttribute('onclick', "sendRequest('/clockwise')");
            
            var btnCCW = document.getElementById('btnCCW');
            btnCCW.innerHTML = 'Counterclockwise';
            btnCCW.classList.remove('active');
            btnCCW.setAttribute('onclick', "sendRequest('/counterclockwise')");
          }
        }
      </script>
    </body>
    </html>
  )=====";
  
  request->send(200, "text/html", html);
}

void handleClockwise(AsyncWebServerRequest *request) {
  if (currentDirection != 1) {
    currentDirection = 1;
    isMoving = true;
    digitalWrite(Dire, HIGH);
    Serial.println("Motor is moving clockwise");
  }
  request->send(200, "text/plain", "Motor set to clockwise");
}

void handleCounterclockwise(AsyncWebServerRequest *request) {
  if (currentDirection != -1) {
    currentDirection = -1;
    isMoving = true;
    digitalWrite(Dire, LOW);
    Serial.println("Motor is moving counterclockwise");
  }
  request->send(200, "text/plain", "Motor set to counterclockwise");
}

void handleStop(AsyncWebServerRequest *request) {
  isMoving = false;
  currentDirection = 0;
  Serial.println("Motor stopped");
  request->send(200, "text/plain", "Motor stopped");
}

void setup() {
  Serial.begin(9600);
  pinMode(Step, OUTPUT); // Step pin as output
  pinMode(Dire, OUTPUT); // Direction pin as output
  pinMode(Sleep, OUTPUT); // Set Sleep OUTPUT Control button as output
  digitalWrite(Step, LOW); // Currently no stepper motor movement
  digitalWrite(Dire, LOW);
  digitalWrite(Sleep, LOW); // Keep motor in sleep initially
  
  // Set target motor RPM and microstepping setting
  stepper.begin(RPM, Microsteps);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  // Start the web server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/clockwise", HTTP_GET, handleClockwise);
  server.on("/counterclockwise", HTTP_GET, handleCounterclockwise);
  server.on("/stop", HTTP_GET, handleStop);
  server.begin();
  Serial.println("Web server started");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (isMoving) {
    digitalWrite(Sleep, HIGH); // Wake up the motor driver
    stepper.move(currentDirection * spr); // Move one full revolution in the set direction
  } else {
    digitalWrite(Sleep, LOW); // Put the motor driver to sleep
  }
}
