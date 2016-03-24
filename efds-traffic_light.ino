#include <EEPROM.h>
#include <SoftwareSerial.h>

#define ELCB1  3
#define ELCB2  4
#define MAIN   5
#define AMBER  6
#define VOLTAGEIN 2

SoftwareSerial gsmSerial(7, 8);


bool tnbfault = false;
bool elcbfault = false;
bool amberfault = false;
int addr = 1;
byte lastStatus = 0;

void setup() {
  
  pinMode(ELCB1, OUTPUT);
  pinMode(ELCB2, OUTPUT);
  pinMode(MAIN, OUTPUT);
  pinMode(AMBER, OUTPUT);
  pinMode(VOLTAGEIN, INPUT);

  Serial.begin(115200);
  gsmSerial.begin(115200); 
  delay(10000);

  Serial.println("TLCFDM starting...");

  if(EEPROM.read(addr)<5){
    SetLastStatus();
  }else{
    EEPROM.write(addr, 0);
  }
}

void(* resetDevice) (void) = 0; //declare reset function @ address 0

void loop() {

  Serial.println("Starting loop...");
  
  if(!tnbfault){
    if(valueRelay(MAIN)>0){
      delay(5000);

      if(!elcbfault){
        if(valueRelay(ELCB1, ELCB2)>0){
          delay(5000);
          
          if(!amberfault){
            
            if(valueTimeRelay(AMBER, 50)>0){
              SendMessage("TLCFDM:AMBER=FLASH;");
            }
            
          }else{
            if(valueFullTimeRelay(AMBER, 50)<1){
              SendMessage("TLCFDM:AMBER=OK;");
            }
          }
        }else{
          elcbfault = true;
          SendMessage("TLCFDM:ELCB=TRIP;");
        }
      }else{
        if(valueRelay(ELCB1, ELCB2)>0){
          elcbfault = false;
          SendMessage("TLCFDM:ELCB=OK;");
        }
      }
    }else{
      tnbfault = true;
      SendMessage("TLCFDM:TNB=FAIL;"); 
    }
  }else{
    if(valueRelay(MAIN)>0){
      tnbfault = false;
      SendMessage("TLCFDM:TNB=OK;");
    }
  }
  
  SetEEPROM();
  delay(900000);

  // restart device every 15 minutes to avoid RAM problems
  if((millis()/1000)>900){
    Serial.println("Resetting TLCFDM...");
    gsmSerial.println("AT+CFUN=1,1");
    delay(500);
    resetDevice();
  }
}

int valueRelay(int buf){
  digitalWrite(buf,HIGH);
  delay(2500);
  int sensorValue = digitalRead(VOLTAGEIN);
  digitalWrite(buf,LOW);
  delay(2500);
  Serial.print("Checking relay: ");
  Serial.println(buf);
  Serial.print("Value: ");
  Serial.println(sensorValue);

  return sensorValue;
}

int valueRelay(int buf1, int buf2){
  digitalWrite(buf1,HIGH);
  digitalWrite(buf2,HIGH);
  delay(2500);
  int sensorValue = digitalRead(VOLTAGEIN);
  digitalWrite(buf1,LOW);
  digitalWrite(buf2,LOW);
  delay(2500);
  Serial.print("Checking relay: ");
  Serial.print(buf1);
  Serial.print(" & ");
  Serial.println(buf2);
  Serial.print("Value: ");
  Serial.println(sensorValue);

  return sensorValue;
}

int valueTimeRelay(int buf, int tbuf){ // 1 tbuf = 200 mili second
  int transitioncounter = 0;
  int ambercounter = 0;
  int oldval = 0;
  int sensorValue = 0;

  digitalWrite(buf,HIGH);
  delay(1000);
  Serial.print("Checking time relay: ");
  Serial.println(buf);
  
  while(ambercounter<tbuf){
    delay(200);
    sensorValue = digitalRead(VOLTAGEIN);
    Serial.print("Value: ");
    Serial.println(sensorValue);
    
    if(ambercounter<1){
      oldval = sensorValue;
    }

    ambercounter++;

    if(oldval != sensorValue){
      transitioncounter++;
      oldval = sensorValue;
    }

    if(transitioncounter>4){
      amberfault = true;
      return amberfault;
      
      digitalWrite(buf,LOW);
      delay(500);
    }
  }
  digitalWrite(buf,LOW);
  delay(500);
  return 0;
}

int valueFullTimeRelay(int buf, int tbuf){
  int transitioncounter = 0;
  int ambercounter = 0;
  int oldval = 0;
  int sensorValue = 0;

  digitalWrite(buf,HIGH);
  delay(2500);
  while(ambercounter<50){
    delay(200);
    sensorValue = digitalRead(VOLTAGEIN);
    
    if(ambercounter<1){
      oldval = sensorValue;
    }

    ambercounter++;

    if(oldval != sensorValue){
      transitioncounter++;
      oldval = sensorValue;
    }
  }

  digitalWrite(buf,LOW);
  delay(2500);

  if(transitioncounter<4){
    amberfault = false;
    return amberfault;
  }else{
    return 1;
  }
}

 void SendMessage(String buf)
{ 
  Serial.print("Status: ");
  Serial.println(buf);
  gsmSerial.println("AT+CMGF=1");    //Sets the GSM Module in Text Mode
  delay(1000);
  gsmSerial.println("AT+CMGS=\"+60172522746\"\r"); // SMS Gateway number
  delay(1000);
  gsmSerial.println(buf); // The traffic light status code
  delay(100);
  gsmSerial.println((char)26);
  delay(5000);
}

void SetEEPROM(){
  if(tnbfault){
    EEPROM.write(addr, 1);
  }else if(elcbfault){
    EEPROM.write(addr, 2);
  }else if(amberfault){
    EEPROM.write(addr, 3);
  }else{
    EEPROM.write(addr, 0);
  }

  delay(500);
}

void SetLastStatus(){
  lastStatus = EEPROM.read(addr);
  
  if(lastStatus == 1){
    tnbfault = true;
  }else if(lastStatus == 2){
    elcbfault = true;
  }else if(lastStatus == 3){
    amberfault = true;
  }
  delay(500);

  Serial.print("Last status: ");
  Serial.println(lastStatus);
}

void testRelay(int buf){
  Serial.print("Test on relay: ");
  Serial.println(buf);
  digitalWrite(buf,HIGH);
  delay(1000);
  digitalWrite(buf,LOW);
  delay(1000);
}

void testRelayHold(int buf){
  Serial.print("Test on relay: ");
  Serial.println(buf);
  digitalWrite(buf,HIGH);
  delay(1000);
  int sensorValue = digitalRead(VOLTAGEIN);
  Serial.print("Value: ");
  Serial.println(sensorValue);
}

void testRelayHold(int buf1, int buf2){
  Serial.print("Test on relay: ");
  Serial.println(buf1);
  digitalWrite(buf1,HIGH);
  digitalWrite(buf2,HIGH);
  delay(1000);
  int sensorValue = digitalRead(VOLTAGEIN);
  Serial.print("Value: ");
  Serial.println(sensorValue);
}
