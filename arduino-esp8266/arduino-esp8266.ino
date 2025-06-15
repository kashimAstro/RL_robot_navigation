#include <pthread.h>
#include <Hash.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <AccelStepper.h>
#include <ArduinoOTA.h>

const char* ssid = "ESSID_WIFI";
const char* password = "PWD_WIFI";

#define IN1_1 5   // GPIO 5 (D1)
#define IN2_1 4   // GPIO 4 (D2)
#define IN3_1 14  // GPIO 14 (D5)
#define IN4_1 12  // GPIO 12 (D6)

#define IN1_2 0   // GPIO 0 (D3)
#define IN2_2 2   // GPIO 2 (D4)
#define IN3_2 13  // GPIO 13 (D7)
#define IN4_2 15  // GPIO 15 (D8)


// 28BYJ-48 full-step 2048 passi per rivoluzione (per il gear ratio di 1:64)
// 28BYJ-48 half-step 4096 passi per rivoluzione 
// mode half-step con ULN2003 spesso IN1, IN3, IN2, IN4 oppure IN1, IN2, IN3, IN4.
AccelStepper stepper1(AccelStepper::HALF4WIRE, IN1_1, IN3_1, IN2_1, IN4_1);
AccelStepper stepper2(AccelStepper::HALF4WIRE, IN1_2, IN3_2, IN2_2, IN4_2);

ESP8266WebServer server(80);
int steps_per_revolution = 1000; //4096; // 2048 = full-step
// regolato da --step-lin --step-rot ( step per rivoluzione )

const char HTML_PAGE[] PROGMEM = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
  <title>mot</title>
  <style>
    body { font-family: Arial, sans-serif; }
    a{ background:gray; text-decoration:none; color:black; }
  </style>
  </head>
  <body>
  <p>controllo motori</p>
  <a href="#" onclick="fetch('/forward?mot1=1&mot2=1').then(response => response.json()).then(data => console.log(data));">su</a><br>
  <a href="#" onclick="fetch('/backward?mot1=1&mot2=1').then(response => response.json()).then(data => console.log(data));">giu</a><br>
  <a href="#" onclick="fetch('/left?mot1=1&mot2=1').then(response => response.json()).then(data => console.log(data));">sinistra</a><br>
  <a href="#" onclick="fetch('/right?mot1=1&mot2=1').then(response => response.json()).then(data => console.log(data));">destra</a><br>
  <a href="#" onclick="fetch('/stop').then(response => response.json()).then(data => console.log(data));">Ferma il motore</a><br>
  <input type="range" min="1" max="4096" value="1000" oninput="fetch('/stepall?step='+this.value).then(response => response.json()).then(data => console.log(data));;">
  </body>
  </html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.println();
  Serial.print("Connessione a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connesso");
  Serial.println("Indirizzo IP: ");
  Serial.println(WiFi.localIP());

  //ArduinoOTA.setPort(8552);
  ArduinoOTA.setHostname("robot001");
  ArduinoOTA.setPassword("brush1987");

  ArduinoOTA.onStart([]() {
    Serial.println("Inizio aggiornamento OTA...");
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    Serial.println("Aggiornamento: " + type);
    server.stop();
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nAggiornamento OTA completato!");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progresso: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Errore OTA[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
  });

  ArduinoOTA.begin();

  stepper1.setMaxSpeed(1000.0); 
  stepper1.setAcceleration(500); 

  stepper2.setMaxSpeed(1000.0); 
  stepper2.setAcceleration(500); 

  server.on("/", handle_root);
  server.on("/forward", handle_forward);
  server.on("/backward", handle_backward);
  server.on("/left", handle_left);
  server.on("/right", handle_right);
  server.on("/stop", handle_stop);
  server.on("/stepall", handle_step);
  server.begin();
  Serial.println("Server HTTP avviato");
}

void loop() {
  server.handleClient();
  ArduinoOTA.handle();

//  stepper1.run();
//  stepper2.run();
}

void handle_root() {
  server.send(200, "text/html", HTML_PAGE);
}

void handle_step() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  if (server.hasArg("step"))
     steps_per_revolution = server.arg("step").toInt();
 
  server.send(200, "text/plain", "step received: " + String(steps_per_revolution));
}

void handle_left() {
  if (server.hasArg("step"))
     steps_per_revolution = server.arg("step").toInt(); 

  if (server.hasArg("mot1")) 
     stepper1.move(-steps_per_revolution);
  if (server.hasArg("mot2"))
     stepper2.move(-steps_per_revolution);

  while(stepper1.distanceToGo() != 0 && stepper2.distanceToGo() != 0) {
      stepper1.run();
      stepper2.run();
      delay(1);
    }
  
  server.send(200, "text/plain", "left step: " + String(steps_per_revolution));
}

void handle_right() {
  if (server.hasArg("step"))
     steps_per_revolution = server.arg("step").toInt(); 
 
  if (server.hasArg("mot1")) 
     stepper1.move(steps_per_revolution);
  if (server.hasArg("mot2"))
     stepper2.move(steps_per_revolution);
 
  while(stepper1.distanceToGo() != 0 && stepper2.distanceToGo() != 0) {
      stepper1.run();
      stepper2.run();
      delay(1);
    }
 
  server.send(200, "text/plain", "right step: " + String(steps_per_revolution));
}

void handle_forward() {
  if (server.hasArg("step"))
     steps_per_revolution = server.arg("step").toInt(); 
 
  if (server.hasArg("mot1")) 
     stepper1.move(steps_per_revolution);
  if (server.hasArg("mot2"))
     stepper2.move(-steps_per_revolution);
 
  while(stepper1.distanceToGo() != 0 && stepper2.distanceToGo() != 0) {
      stepper1.run();
      stepper2.run();
      delay(1);
    }
 
  server.send(200, "text/plain", "up step: " + String(steps_per_revolution));
}

void handle_backward() {
  if (server.hasArg("step"))
     steps_per_revolution = server.arg("step").toInt(); 
 
  if (server.hasArg("mot1")) 
     stepper1.move(-steps_per_revolution);
  if (server.hasArg("mot2"))
     stepper2.move(steps_per_revolution);

  while(stepper1.distanceToGo() != 0 && stepper2.distanceToGo() != 0) {
      stepper1.run();
      stepper2.run();
      delay(1);
    }
  
  server.send(200, "text/plain", "down step: " + String(steps_per_revolution));
}

void handle_stop() {
  stepper1.stop();
  stepper2.stop();
  server.send(200, "text/plain", "stop all mot");
}
