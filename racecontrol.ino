class RaceControl {
  public:
    //Is race heat or practice session started
    boolean sessionStarted;
  
    RaceControl();
    void parse(char c);
    void process();
    void updateTimes();
  private:
    //Supporting up to 8 lane tracks
    //Each element will represent lane pin, if it's 0 - it's not active
    int lanes[22];
    int relayPin;
    //Keep track of last time a car passed the sensing part
    //May cause a bug after 50 days of constant arduino operation
    unsigned long lastTimeCarPassed[22];

    //Minimal time it should take for a lap in milliseconds
    unsigned long minTime;

    //Don't do constant updating to serial port
    int lastReadSensorValues[22];

    //Request storage
    char request[8];
    int index;
    char cmdStr[3];
    byte cmd;
    char laneStr[3];
    byte lane;
    char pinStr[3];
    byte pin;
    char timeStr[5];

    //Value and response storage
    boolean readValue;
    char response[18];

    //API methods
    void startSession();
    void pauseSession();
    void stopSession();
    void setLane();
    void setMinTime();
    void setPowerRelay();

    //Internal methods
    void writeResponse();
    void reset();
};

RaceControl::RaceControl() {
  reset();
}

void RaceControl::parse(char c) {
  if (c == '!') index = 0;        //Reset request
  else if (c == '.') process();   //End request and process
  else request[index++] = c;      //Append to request
}

void RaceControl::process() {
  response[0] = '\0';

  //Parse the command (further info is only needed if we set the lanes or min time)
  strncpy(cmdStr, request, 2);  cmdStr[2] =  '\0';
  cmd = atoi(cmdStr);

  switch(cmd) {
    //!000801.
    case 0: setLane       (); break;
    //!012900.
    case 1: setMinTime    (); break;
    //!02.
    case 2: startSession  (); break;
    //!03.
    case 3: pauseSession  (); break;
    //!04.
    case 4: stopSession   (); break;
    //!0507.
    case 5: setPowerRelay (); break;
    default:                  break;
  }

  if (response[0] != '\0') {
    writeResponse();
  }
}

void RaceControl::updateTimes() {
  unsigned long carPassedAt = millis();
  for (int i = 0; i < 22; i++) {
    readValue = digitalRead(lanes[i]);
    if (lanes[i] > 0 && sessionStarted && lastReadSensorValues[i] != readValue) {
      unsigned long timePassed = carPassedAt - lastTimeCarPassed[i];
      if (timePassed > minTime) {
        sprintf(response, "%02d:%lu.", i, timePassed);
        lastTimeCarPassed[i] = carPassedAt;
        writeResponse();
      }
    }
    lastReadSensorValues[i] = readValue;
  }
}

void RaceControl::startSession() {
  unsigned long startTime = millis();
  //Will need to power up the track with relay too
  sessionStarted = true;
  for(int i = 0; i < 22; i++) {
    lastTimeCarPassed[i] = startTime;
    lastReadSensorValues[i] = 0;
  }
  digitalWrite(relayPin, HIGH);
  sprintf(response, "SS:%lu.", startTime);
}

void RaceControl::setPowerRelay() {
  strncpy(pinStr, request + 2, 2);   pinStr[2] = '\0';
  relayPin = atol(pinStr);
  digitalWrite(relayPin, LOW);
  sprintf(response, "PR:%02d.", relayPin);
}

void RaceControl::pauseSession() {
  sessionStarted = false;
  digitalWrite(relayPin, LOW);
}

void RaceControl::stopSession() {
  sessionStarted = false;
  //Will need to power off the track with relay too
  for(int i = 0; i < 22; i++) {
    lastTimeCarPassed[i] = 0;
    lastReadSensorValues[i] = 0;
  }
  digitalWrite(relayPin, LOW);
}

void RaceControl::setLane() {
  strncpy(pinStr, request + 2, 2);    pinStr[2] = '\0';
  strncpy(laneStr, request + 4, 3);   laneStr[2] = '\0';
  pin = atoi(pinStr);
  lane = atoi(laneStr);
  lanes[lane] = pin;
  sprintf(response, "L:%02dP:%02d.", lane, pin);
}

void RaceControl::setMinTime() {
  strncpy(timeStr, request + 2, 4);   timeStr[4] = '\0';
  minTime = atol(timeStr);
  sprintf(response, "MT:%04lu.", minTime);
}

void RaceControl::writeResponse() {
  Serial.print(response);
  Serial.print("\n");
}

void RaceControl::reset() {
  sessionStarted = false;
  minTime = 3000;
  for (int i = 0; i < 22; i++) {
    lanes[i] = 0;
    lastTimeCarPassed[i] = 0;
    lastReadSensorValues[i] = 0;
  }
}

RaceControl raceControl;

void setup() {
  Serial.begin(9600);
}

void loop() {
  while(Serial.available() > 0) raceControl.parse(Serial.read());

  if (raceControl.sessionStarted) {
    raceControl.updateTimes();    
  }
  Serial.flush();
}

