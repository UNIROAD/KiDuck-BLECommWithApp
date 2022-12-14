#include <SoftwareSerial.h>   //Software Serial Port
#include <ArduinoSTL.h>
#define RxD 7
#define TxD 8

#define MASTER 0    //change this macro to define the Bluetooth as Master or not 

SoftwareSerial blueToothSerial(RxD, TxD); //the software serial port

char recv_str[100];

String kiduckName = "TEST";
int dataCount = 8;
int data[8][3] = { // first index: day, second index: 0-step count 1-drink 2-communication
  {2991, 2000, 6},
  {12444, 2314, 8},
  {7004, 1599, 5},
  {8932, 1211, 2},
  {12322, 1522, 4},
  {4052, 2344, 2},
  {9806, 1112, 9},
  {10000, 1234, 5}
};

int threshold[3] = {10000, 1500, 5};
String password = "abcdefg";
int emergencyAlarm = 0;

void setup()
{
  Serial.begin(9600);   //Serial port for debugging
  pinMode(RxD, INPUT);    //UART pin for Bluetooth
  pinMode(TxD, OUTPUT);   //UART pin for Bluetooth
  Serial.println("\r\nPower on!!");
  setupBlueToothConnection(); //initialize Bluetooth
  //this block is waiting for connection was established.
  while (1)
  {
    if (recvMsg(100) == 0)
    {
      if (strcmp((char *)recv_str, (char *)"OK+CONN") == 0)
      {
        Serial.println("connected\r\n");
        break;
      }
    }
    delay(200);
  }
}

void loop()
{
  delay(200);
  if (recvMsg(1000) == 0)
  {
    if (strcmp((char *)recv_str, (char *)"AppConnected") == 0) {
      if (sendKiDuckInfo() == -1) {
        blueToothSerial.print("Fail to send        ");
      }
    } else if (strcmp((char *)recv_str, (char *)"InitGrowth") == 0) {
      if (sendKiDuckGrowth() == -1) {
        blueToothSerial.print("Fail to send        ");
      }
    } else if (strcmp((char *)recv_str, (char *)"SetGrowth") == 0) {
      blueToothSerial.print("ACK");
      if (recvMsg(1000) == 0) {
        if (parseGrowth() == 0)
          blueToothSerial.print("SUCCESS");
        else
          blueToothSerial.print("FAIL");
      }
    } else if (strcmp((char *)recv_str, (char *)"InitName") == 0) {
      if (sendKiDuckName() == -1) {
        blueToothSerial.print("Fail to send        ");
      }
    } else if (strcmp((char *)recv_str, (char *)"SetName") == 0) {
      blueToothSerial.print("ACK");
      if (recvMsg(1000) == 0) {
        if (parseName() == 0)
          blueToothSerial.print("SUCCESS");
        else
          blueToothSerial.print("FAIL");
      }
    }
    Serial.print("recv: ");
    Serial.print((char *)recv_str);
    Serial.println("");
  }
}

//used for compare two string, return 0 if one equals to each other
int strcmp(char *a, char *b)
{
  unsigned int ptr = 0;
  while (a[ptr] != '\0')
  {
    if (a[ptr] != b[ptr]) return -1;
    ptr++;
  }
  return 0;
}

//configure the Bluetooth through AT commands
int setupBlueToothConnection()
{
  Serial.print("Setting up Bluetooth link\r\n");
  delay(2000);//wait for module restart
  blueToothSerial.begin(9600);

  //wait until Bluetooth was found
  while (1)
  {
    if (sendBlueToothCommand("AT") == 0)
    {
      if (strcmp((char *)recv_str, (char *)"OK") == 0)
      {
        Serial.println("Bluetooth exists\r\n");
        break;
      }
    }
    delay(500);
  }

  //configure the Bluetooth
  sendBlueToothCommand("AT+DEFAULT");//restore factory configurations
  delay(2000);
  sendBlueToothCommand("AT+NAME?");
  sendBlueToothCommand("AT+NAMEKIDUCK");
  sendBlueToothCommand("AT+ROLES");//set to slave mode
  sendBlueToothCommand("AT+RESTART");//restart module to take effect
  delay(2000);//wait for module restart

  //check if the Bluetooth always exists
  if (sendBlueToothCommand("AT") == 0)
  {
    if (strcmp((char *)recv_str, (char *)"OK") == 0)
    {
      Serial.print("Setup complete\r\n\r\n");
      return 0;;
    }
  }

  return -1;
}

//send command to Bluetooth and return if there is a response received
int sendBlueToothCommand(char command[])
{
  Serial.print("send: ");
  Serial.print(command);
  Serial.println("");

  blueToothSerial.print(command);
  delay(300);

  if (recvMsg(200) != 0) return -1;

  Serial.print("recv: ");
  Serial.print(recv_str);
  Serial.println("");
  return 0;
}

//receive message from Bluetooth with time out
int recvMsg(unsigned int timeout)
{
  //wait for feedback
  unsigned int time = 0;
  unsigned char num;
  unsigned char i;

  //waiting for the first character with time out
  i = 0;
  while (1)
  {
    delay(50);
    if (blueToothSerial.available())
    {
      recv_str[i] = char(blueToothSerial.read());
      i++;
      break;
    }
    time++;
    if (time > (timeout / 50)) return -1;
  }

  //read other characters from uart buffer to string
  while (blueToothSerial.available() && (i < 100))
  {
    recv_str[i] = char(blueToothSerial.read());
    i++;
  }
  recv_str[i] = '\0';

  return 0;
}

int sendKiDuckInfo() {
  String sendName = kiduckName;
  if (sendName.length() > 20)
    return -1;

  while (sendName.length() < 20) {
    sendName += ' ';
  }

  String sendDataCount(dataCount);
  if (sendDataCount.length() > 20)
    return -1;

  while (sendDataCount.length() < 20) {
    sendDataCount += ' ';
  }

  std::vector<String> sendData(dataCount);
  for (int i = 0; i < dataCount; i++) {
    sendData[i] = String(data[i][0]) + ' ' + String(data[i][1]) + ' ' + String(data[i][2]);
    if (sendData[i].length() > 20)
      return -1;

    while (sendData[i].length() < 20) {
      sendData[i] += ' ';
    }
  }

  blueToothSerial.print(sendName);
  blueToothSerial.print(sendDataCount);
  for (int i = 0; i < dataCount; i++) {
    blueToothSerial.print(sendData[i]);
  }
  return 0;
}

int sendKiDuckGrowth() {
  String sendThreshold = String(threshold[0]) + ' ' + String(threshold[1]) + ' ' + String(threshold[2]);
  if (sendThreshold.length() > 20)
    return -1;

  while (sendThreshold.length() < 20) {
    sendThreshold += ' ';
  }

  blueToothSerial.print(sendThreshold);
  return 0;
}

int sendKiDuckName() {
  String sendName = kiduckName;
  if (sendName.length() > 20)
    return -1;

  while (sendName.length() < 20) {
    sendName += ' ';
  }

  blueToothSerial.print(sendName);
  return 0;
}

int parseGrowth() {
  String strStepCriteria = "";
  String strDrinkCriteria = "";
  String strCommCriteria = "";

  int curi = 0;
  int type = 0; // 0: step, 1: drink, 2: communication
  while (recv_str[curi] != '\0') {
    if (recv_str[curi] == ' ') {
      type++;
      curi++;
    } else if (recv_str[curi] >= '0' && recv_str[curi] <= '9') {
      if (type == 0) {
        strStepCriteria += recv_str[curi++];
      } else if (type == 1) {
        strDrinkCriteria += recv_str[curi++];
      } else {
        strCommCriteria += recv_str[curi++];
      }
    } else {
      return -1;
    }
  }
  Serial.println("update criteria:");
  Serial.println(strStepCriteria);
  Serial.println(strDrinkCriteria);
  Serial.println(strCommCriteria);

  threshold[0] = strStepCriteria.toInt();
  threshold[1] = strDrinkCriteria.toInt();
  threshold[2] = strCommCriteria.toInt();

  return 0;
}

int parseName() {
  String newName = "";
  newName = String((char *)recv_str);
  Serial.println(newName);
  kiduckName = newName;
  return 0;
}
