// incorporate the shoutouts and stuff in an include
#include <PID_v1.h>
#include <Adafruit_MAX31855.h>
#define thermoDO 3
#define thermoCS 4
#define thermoCLK 5
Adafruit_MAX31855 tc1(thermoCLK, thermoCS, thermoDO);

// pins for scr control
#define relayTop 12
#define relayBot 11
// relay on, led on
#define ledPin   13
// relay pwm cycle time in millis
#define winSize  5000

int botOn    = 0;
int topOn    = 0;

int stepNum   = 0;  // first step
int lastStep  = -1; // needs to be less than first step

unsigned long winStartTime;

int startTemp, endTemp, currTemp, stepTemp, rampTemp, lastTemp;
double Setpoint, Input, Output;
PID myPID(&Setpoint, &Output, &Input,2,5,1, DIRECT);

unsigned long loopTime = 6000; // step cycle time in millis (one minute, 60,000, unless you want to change the time scale)
unsigned long now, lastTime, nextTime, stepTime, startTime, rampStart, rampTime;
unsigned long loopLast, loopStart;

boolean runOK = false; // default to disabled
boolean holding = false;
int rampCnt, cntErrors;
int maxErrors = 5;

// startTemp, endTemp, rampTime (minutes)
String rampSets[] = {	
						"15,22,2",
						"22,25,2",
						"25,32,1",
						"32,32,10",
						"32,24,3"
					};

void setup() {
	Serial.begin(9600);
	delay(5000);
	Serial.println("starting now");
	loopStart = millis();
	loopLast  = loopStart;
	
	myPID.SetOutputLimits(0,winSize);
	myPID.SetMode(AUTOMATIC);
	myPID.SetControllerDirection(DIRECT);
	
	pinMode(relayTop, OUTPUT);
	pinMode(relayBot, OUTPUT);
	pinMode(ledPin, OUTPUT);

	// make sure the relays are off to start
	digitalWrite(relayTop, LOW);
	digitalWrite(relayBot, LOW);
	digitalWrite(ledPin, LOW);
        
	rampCnt = sizeof(rampSets) / 6;
	runOK = true;
	rampTemp = readTemp();
}

void loop() {
	if (runOK) {
		runKiln();
	}
	else {
		stopKiln();
	}		
}

int runKiln () {
	now = millis(); 	
	currTemp = readTemp(); 
	
	if (stepNum < rampCnt ) { // not past last ramp?
		if (lastStep < stepNum) { // time to get next ramp settings
			getStep();
			lastStep = stepNum;
		}
		if (now >= (loopLast + loopTime) ) { // time to run the ramp increment
			loopLast = now;
			if (!holding) { //start and end temp are different
		
				if (startTemp < endTemp) { // going up
					if ( currTemp >= rampTemp) { // wait for currTemp to reach rampTemp
						rampTemp = rampTemp + stepTemp; // add next ramp step
						if (rampTemp > endTemp) { // keep rampTemp from growing past endTemp
							rampTemp = endTemp;
						}
					}			
					if (currTemp >= endTemp) {
						Serial.print("Step #");
						Serial.print(stepNum);
						Serial.println(" ramp up done");
						stepNum++;
					}
				}
				else { // going down
					if ( currTemp <= rampTemp) { // wait for currTemp to reach rampTemp
						rampTemp = rampTemp + stepTemp; // add next ramp step
						if (rampTemp < endTemp) { // keep rampTemp from growing past endTemp
							rampTemp = endTemp;
						}
					}			
					if (currTemp <= endTemp) {
						Serial.print("Step #");
						Serial.print(stepNum);
						Serial.println(" ramp down done");
						stepNum++;
					}						
				}
			}
			else { // start and end temp are the same, hold that temp for ramp time
				if (now >= (rampStart + rampTime) ) { // get new parameters
					Serial.print("Step #");
					Serial.print(stepNum);
					Serial.println(" done");
					stepNum++;
				}
			}
		}		
			displayData();
		
		Input = (double) currTemp; // pid wants doubles
		Setpoint = (double) rampTemp; 
		now = millis(); // get the time again
		myPID.Compute(); 
		if (now - winStartTime > winSize) { //time to shift the Relay Window
			winStartTime += winSize;
		}
		if (Output > now - winStartTime) {
			highHeat();
		}
		else {
			noHeat();
		}
		
	}
	else {
		runOK = false;
	}


}

void getStep () {
	String rampLine = rampSets[stepNum];
	String startTempStr = getValue(rampLine, ',', 0);
	String endTempStr   = getValue(rampLine, ',', 1);
	String rampTimeStr  = getValue(rampLine, ',', 2);

	startTemp = startTempStr.toInt();
	endTemp   = endTempStr.toInt();
	long rampTimeMinutes = rampTimeStr.toInt();
	
	rampTime  = rampTimeMinutes * loopTime;
	if (startTemp != endTemp) {
		holding = false;
		stepTemp  = (endTemp - currTemp) / rampTimeMinutes;
	}
	else {
		holding = true;
		stepTemp = 0;
	}
}
	
String getValue(String data, char separator, int index) {
	int found = 0;
	int strIndex[] = {0, -1 };
	int maxIndex = data.length()-1;
	for(int i=0; i<=maxIndex && found<=index; i++) {
		if(data.charAt(i)==separator || i==maxIndex) {
			found++;
			strIndex[0] = strIndex[1]+1;
			strIndex[1] = (i == maxIndex) ? i+1 : i;
		}
	}
	return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

int readTemp() {
	double c = tc1.readCelsius();
	if (isnan(c)) {
		cntErrors += 1;
		if (cntErrors > maxErrors) {
			runOK = false;
			return 0;
		}
		else {
			c = lastTemp;
		}
	} else {
		lastTemp = c;
		cntErrors = 0;
	}
	int t = int(c);
	return t;
	//return rampTemp;
}

void displayData () { // func for display to allow replacement for lcd, etc...
	Serial.print(stepNum);
	Serial.print(" , ");
	Serial.print(startTemp);
	Serial.print(" , ");
	Serial.print(currTemp);
	Serial.print(" , ");
	Serial.print(rampTemp);
	Serial.print(" , ");
	Serial.print(endTemp);
	Serial.print(" , ");
	Serial.println(Output);
}

void highHeat() {
	digitalWrite(relayTop, HIGH);
	topOn = 1;
	digitalWrite(relayBot, HIGH);
	botOn = 1;
	digitalWrite(ledPin, HIGH);
}

void noHeat () {
  botOn = 0;
  topOn = 0;
  digitalWrite(relayTop, LOW);
  digitalWrite(relayBot, LOW);
  digitalWrite(ledPin, LOW);
}

void stopKiln() {
	Serial.println("idle");
	noHeat();
	delay(1000);
}

