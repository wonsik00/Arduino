#include <SoftwareSerial.h>   //Software Serial Port
#define RxD 2
#define TxD 3

SoftwareSerial blueToothSerial(RxD, TxD); //the software serial port

char recv_str[100];

void setup()
{
  Serial.begin(9600);   //Serial port for debugging
  pinMode(RxD, INPUT);    //UART pin for Bluetooth
  pinMode(TxD, OUTPUT);   //UART pin for Bluetooth
  Serial.println("\r\nPower on!!");
  setupBlueToothConnection(); //initialize Bluetooth
  BTConnection(); //Connect Central&Peri
  //this block is waiting for connection was established.
}

void loop()
{
  
  if (recvMsg(100) == 0)
  {
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

  Serial.println("this is MASTER\r\n");
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
  sendBlueToothCommand("AT+ROLE1");//set to master mode
  sendBlueToothCommand("AT+IMME1");//set to immediately mode
  sendBlueToothCommand("AT+TYPE1");//set to bonding mode
  sendBlueToothCommand("AT+RESTART");//restart module to take effect
  delay(2000);//wait for module restart


  //check if the Bluetooth always exists
  if (sendBlueToothCommand("AT") == 0)
  {
    if (strcmp((char *)recv_str, (char *)"OK") == 0)
    {
      Serial.print("Setup complete\r\n\r\n");
      return 0;
    }
  }

  return -1;
}
int BTConnection()
{
  while (1)
  {
    Serial.print("Discovery slave\n");
    sendBlueToothCommand("AT+DISC?");
    delay(5000);
    recvMsg(100);
    Serial.print("recv: ");
    Serial.print(recv_str);
    Serial.println("\n");
    delay(500);
    Serial.print("Connect slave\n");
    sendBlueToothCommand("AT+CONN0");
    sendBlueToothCommand("AT+CONN1");
    sendBlueToothCommand("AT+CONN2");
    sendBlueToothCommand("AT+CONN3");
    sendBlueToothCommand("AT+CONN4");
    sendBlueToothCommand("AT+CONN5");
    delay(500);
    if (strcmp((char *)recv_str, (char *)"OK+CONNN") == 0)
    {
      Serial.println("Connected complete\r\n");
      break;
    }
  }
  return 0;
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
    delay(100);
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
