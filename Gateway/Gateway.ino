#include <SPI.h>
#include <WiFi.h>
#include <SoftwareSerial.h>
#define RxD 2
#define TxD 3
#define Number 2
SoftwareSerial blueToothSerial(RxD, TxD); // SoftwareSerial(RX, TX)

int k=0;
char recv_str[100];
char data_t[20];
char data_h[20];
char ard_id;
char light[20];
char led;
char ssid[] = "000740CDBF38";      //  your network SSID (name)
int keyIndex = 0;            // your network key Index number (needed only for WEP)
int check = 0;
char mac[Number][50];

int status = WL_IDLE_STATUS;
// Initialize the Wifi client library
WiFiClient client;

// server address:

IPAddress server(164, 125, 234, 97);
IPAddress ip(164, 125, 234, 99);

unsigned long lastConnectionTime = 0;            // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 10L * 300L; // delay between updates, in milliseconds

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  pinMode(RxD, INPUT);    //UART pin for Bluetooth
  pinMode(TxD, OUTPUT);   //UART pin for Bluetooth
  Serial.println("\r\nPower on!!");
  setupBlueToothConnection(); //initialize Bluetooth
  setupBlueToothMac();
  k = 0; // 라운드로빈 (0번부터 시작)

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv != "1.1.0") {
    Serial.println("Please upgrade the firmware");
  }

  WiFi.config(ip);
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid);

    // wait 10 seconds for connection:
    delay(1000);
  }
  // you're connected now, so print out the status:
  printWifiStatus();
}

void loop() {

  // GET BLE data
  BTConnection();

  if (recvMsg(-1) == 0)
  {
    parsing_data(); // BLE 페이로드 파싱
    check = 0;
  }
  else
    check = -1;

  closeConnection();

  // Send BLE data by WIFI
  if ( check != -1) {
    if (millis() - lastConnectionTime > postingInterval) {
      httpRequest();
    }
    while (client.available()) {
      char c = client.read();
      Serial.write(c);
    }
  }
  delay(2000);
}

// this method makes a HTTP connection to the server:
void httpRequest() {
  char server_url[128];
  sprintf(server_url, "GET /set?ard_id=%c&light=%s&temp=%s&humidity=%s&led=%c HTTP/1.0", ard_id, light, data_t, data_h, led);
  Serial.println(server_url);
  // close any connection before send a new request.
  // This will free the socket on the WiFi shield
  client.stop();

  // if there's a successful connection:
  if (client.connect(server, 3000)) {
    Serial.println();
    Serial.println("connecting...");
    // send the HTTP PUT request:
    client.println(server_url);
    client.println("Connection:close");
    client.println();

    // note the time that the connection was made:
    lastConnectionTime = millis();
  } else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
  }
}


void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
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
  sendBlueToothCommand("AT+ROLE1");//set to master mode
  sendBlueToothCommand("AT+IMME1");//set to master mode
  sendBlueToothCommand("AT+TYPE0");//set to bonding mode
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
  char cmd[50];
  while (1)
  {
    delay(500);
    Serial.print("Connect slave\n");
    sprintf(cmd, "AT+CON%s", mac[k]);
    //라운드로빈 방식
    k++;
    if ( k >= Number )
      k = k % Number;
    sendBlueToothCommand(cmd);
    if ( strcmp((char *)recv_str, (char *)"OK+CONNAOK+CONN") == 0) // 연결되면
    {
      Serial.println("Connected complete\r\n");
      break;
    }
    else
      sendBlueToothCommand("AT");
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
int recvMsg(int timeout)
{
  //wait for feedback
  unsigned int time = 0;
  unsigned char num;
  unsigned char i = 0;

  if ( timeout < 0 ) {
    while (1)
    {
      while (blueToothSerial.available())
      {
        recv_str[i] = char(blueToothSerial.read());
        Serial.print(recv_str[i]);
        if ( recv_str[i] == '@') {
          i++;
          recv_str[i] = '\0';
          return 0;
        }
        else if ( strstr((char *)recv_str, (char *)"LOST") != 0 )
          return -1;
        i++;
      }
    }
  }

  else {
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

}

int closeConnection()
{
  while (1)
  {
    if (sendBlueToothCommand("AT") == 0)
    {
      if (strcmp((char *)recv_str, (char *)"OK+LOST") == 0)
      {
        Serial.println("Connection close\r\n");
        break;
      }
    }
    delay(100);
  }
}
int str_chr(char *p, char search)
{
  int count = 0;
  while (1)
  {
    if (*p == 0)
      return -1;
    if (*p == search)
      return count;
    *p++;
    count++;
  }
}
void setupBlueToothMac() {
  Serial.print("Discovery slave\n");
  sendBlueToothCommand("AT+DISC?");
  delay(3000);
  recvMsg(100);
  Serial.print("recv: ");
  Serial.print(recv_str);
  Serial.println("\n");
  while ( strstr((char *)recv_str, (char *)"DIS0") != 0 )
  {
    int a = str_chr(recv_str, ':') + 1;
    strcpy(recv_str, &recv_str[a]);
    a = str_chr(recv_str, '+');
    strncpy(mac[k], recv_str, a);
    mac[k++][a - 2] = '\0';
    //  Serial.println(mac[k++]);
  }
}
void parsing_data()
{
  //  char *p;
  Serial.print("recv: ");
  Serial.print((char *)recv_str);
  Serial.println("");

  ard_id = recv_str[0];
  Serial.println(ard_id);
  strncpy(light, &recv_str[1], 2);
  Serial.println(light);
  strncpy(data_t, &recv_str[3], 2);
  Serial.println(data_t);
  strncpy(data_h, &recv_str[5], 2);
  Serial.println(data_h);
  led = recv_str[7];
  Serial.println(led);
}
