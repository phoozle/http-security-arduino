#include <Bridge.h>
#include <BridgeServer.h>
#include <BridgeClient.h>

// Listen to the default port 5555, the Yún webserver
// will forward there all the HTTP requests you send
BridgeServer server;

const int ALARM_THREASHOLD = 500;
const int ALARM_TIME = 30000;

int sensorPin = 12;
int alarmPin = 13;

unsigned long sensorOkMillis = 0;

unsigned long alarmedAt = 0;

bool securityActivated = true;

void setup() {
  Bridge.begin();

  pinMode(sensorPin, INPUT);
  pinMode(alarmPin, OUTPUT);

  // Listen for incoming connection only from localhost
  // (no one from the external network could connect)
  server.listenOnLocalhost();
  server.begin();
}

String securityStatus(bool currentState) {
  //"0": stay armed
  //"1": away armed
  //"2": night armed
  //"3": disarmed
  //"4": alarm has been triggered

  // when alarm is activated 
  if (currentState && digitalRead(alarmPin) == HIGH) {
    return "4";
  }

  return securityActivated ? "0" : "3";
}

void process(BridgeClient client) {
  // read the command
  String command = client.readStringUntil('\r');

  if (command == "enable") {
    securityActivated = true;
    client.print("Enabling security...");
    client.print("Status:" + securityStatus(false));
  } else if (command == "disable") {
    securityActivated = false;
    client.print("Disabling security...");
    client.print("Status:" + securityStatus(false));
  } else if (command == "status") {
    client.print(securityStatus(true));
  } else if (command == "targetState") {
    client.print(securityStatus(false));
  }
}

void enableSensor() {
  pinMode(sensorPin, INPUT);
}

void disableSensor() {
  pinMode(sensorPin, OUTPUT);
  digitalWrite(sensorPin, HIGH);
}

void securityCheck() {
  if (digitalRead(alarmPin) == HIGH && millis() - alarmedAt < ALARM_TIME) {
    // require alarm to go for X seconds  
    digitalWrite(alarmPin, HIGH);
  } else if (millis() - sensorOkMillis > ALARM_THREASHOLD) {
    alarmedAt = millis();
    digitalWrite(alarmPin, HIGH);
  } else {
    digitalWrite(alarmPin, LOW);
  }
}

void loop() {
  // Get clients coming from server
  BridgeClient client = server.accept();

  // There is a new client?
  if (client) {
    // Process request
    process(client);

    // Close connection and free resources.
    client.stop();
  }

  if (securityActivated) {
    enableSensor();
    delay(50); // 50ms
    
    pinMode(sensorPin, INPUT);
    int state = digitalRead(sensorPin);

    if (state == HIGH) {
     sensorOkMillis = millis();
    }

    disableSensor(); // flash effect

    securityCheck();
  } else {
    digitalWrite(alarmPin, LOW);
    disableSensor();
  }

  delay(50); // Poll every 50ms
}
