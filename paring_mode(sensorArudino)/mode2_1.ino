#include <SoftwareSerial.h>   //Software Serial Port
#include <DHT11.h>
#define RxD 2
#define TxD 3

SoftwareSerial blueToothSerial(RxD, TxD); //the software serial port
DHT11 dht11(10);

char recv_str[100];
int cds = A0; // 조도
char data[50];
char ard_id = '1';
char led;
float temp, humi;
int t,h;
int err;

void setup()
{
  Serial.begin(9600);   //Serial port for debugging
  pinMode(11, OUTPUT);
  pinMode(RxD, INPUT);    //UART pin for Bluetooth
  pinMode(TxD, OUTPUT);   //UART pin for Bluetooth
  Serial.println("\r\nPower on!!");
  setupBlueToothConnection(); //initialize Bluetooth
  //this block is waiting for connection was established.
}
void loop()
{
  digitalWrite(11, HIGH);
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
  delay(1000);

  if ( (err = dht11.read(humi, temp)) == 0 ) // 온습도 센서 값 받기
  {
    t = temp;
    h = humi;
  }
  int light = get_cds(cds); // 조도 센서 값 받기
  if ( digitalRead(11) == LOW) // LED 상태 값 받기
    led = '0';
  else
    led = '1';
  // sprintf(data, "%c:%d:%s:%s:%c@", ard_id, light, temp, humid, led);
  sprintf(data, "%c%d%d%d%c@", ard_id, light, t, h, led);
  Serial.println(data);
  blueToothSerial.print(data);
  //blueToothSerial.print("17315301");
  //blueToothSerial.print("@");
  //Serial.println("17315301@");
  while (1)
  {
    if (recvMsg(100) == 0)
    {
      if (strcmp((char *)recv_str, (char *)"OK+LOST") == 0)
      {
        Serial.println("closed\r\n");
        break;
      }
    }
    delay(200);
  }
  //sendBlueToothCommand("AT+ROLE1");
  delay(1000);
  //sendBlueToothCommand("AT+ROLE0");
}

int get_cds(int i)
{
  int val = analogRead(i);
  if (val / 10 > 100) val = 1000;
  val = 100 - val / 10;
  return val;
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
  Serial.println("this is SLAVE1\r\n");
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
  sendBlueToothCommand("AT+SHOW0");
  sendBlueToothCommand("AT+ROLE0");
  sendBlueToothCommand("AT+NOTI1");
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



