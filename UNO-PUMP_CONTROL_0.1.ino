/*
 Basic MQTT example

 This sketch demonstrates the basic capabilities of the library.
 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic"
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.
 
*/

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <EEPROM.h> 
#include <SoftwareSerial.h>

#define SSerialRX        8
#define SSerialTX        9 

#define SSerialTxControl 3   //RS485 Direction control
#define RS485Transmit    HIGH
#define RS485Receive     LOW



// Update these with values suitable for your network.
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 41);;
IPAddress server(192, 168, 1, 21);;

char message_buff[100];

SoftwareSerial RS485Serial(SSerialRX, SSerialTX); // RX, TX

unsigned long lastConnectionTime = 0;          // last time "heatbeat" was send to the pump, in milliseconds
const unsigned long postingInterval = 2000;

unsigned char bye[]  = {
  0xFF,0x00,0xFF,0xA5,0x00,0x60,0x10,0x04,0x01,0x00,0x01,0x1A}; //Release pump's Display, enables local control
unsigned char att[]  = {
  0xFF,0x00,0xff,0xA5,0x00,0x60,0x10,0x04,0x01,0xff,0x02,0x19}; // Attention (Disable pumps display, enables remote control)
unsigned char sp1[]  = {
  0xFF,0x00,0xFF,0xA5,0x00,0x60,0x10,0x01,0x04,0x03,0x21,0x00,0x08,0x01,0x46}; // Run speed one
unsigned char sp2[]  = {
  0xFF,0x00,0xFF,0xA5,0x00,0x60,0x10,0x01,0x04,0x03,0x21,0x00,0x10,0x01,0x4E}; // Run Speed Two
unsigned char sp3[]  = {
  0xFF,0x00,0xFF,0xA5,0x00,0x60,0x10,0x01,0x04,0x03,0x21,0x00,0x18,0x01,0x56}; // Run Speed Three
unsigned char sp4[]  = {
  0xFF,0x00,0xFF,0xA5,0x00,0x60,0x10,0x01,0x04,0x03,0x21,0x00,0x20,0x01,0x5E}; // Run Speed Four
unsigned char Stop[] = {
  0xFF,0x00,0xFF,0xA5,0x00,0x60,0x10,0x01,0x04,0x03,0x21,0x00,0x00,0x01,0x3E}; // Stops PUMP

unsigned char PStat[]  = {
  0xFF,0x00,0xFF,0xA5,0x00,0x60,0x10,0x07,0x00,0x01,0x1C}; // Request Pump Status NOT USED HERE (only works when remote)
int pSpeed =  EEPROM.read(30);
  

void callback(char* topic, byte* payload, unsigned int length) {
  int i = 0;

  Serial.println("Message arrived:  topic: " + String(topic));
  Serial.println("Length: " + String(length,DEC));
  

//   int pSpeed =  EEPROM.read(30);
//   pSpeed =constrain(pSpeed,1,4);

    for(i=0; i<length; i++) {
      message_buff[i] = payload[i];
    }
    message_buff[i] = '\0';
  
    String msgString = String(message_buff);
  
    Serial.println("Payload: " + msgString);
    
    if(msgString == "on"){
      Serial.println("PoolPump on");
      //int pSpeed =  EEPROM.read(30);
      //RS485Serial.write (att , HEX);
      digitalWrite(2, HIGH);
      digitalWrite(4, HIGH);  
    }
    if(msgString == "off"){
      Serial.println("PoolPump off");
      digitalWrite(2, LOW);      
      digitalWrite(4, LOW);
      digitalWrite(5, LOW);
      digitalWrite(6, LOW);
      digitalWrite(7, LOW);
        RS485Serial.write ( Stop, HEX);
        delay(60);
        RS485Serial.write ( Stop, HEX);    
    }
    
     if(msgString == "0") {
      Serial.println("PoolPump off");
      digitalWrite(2, LOW);      
      digitalWrite(4, LOW);
      digitalWrite(5, LOW);
      digitalWrite(6, LOW);
      digitalWrite(7, LOW);
        RS485Serial.write ( Stop, HEX);
        delay(60);
        RS485Serial.write ( Stop, HEX); 
     }
    if((msgString >= "1") && (msgString <= "25")){
      Serial.println("PumpSpeed 25%");
      RS485Serial.write ( sp1, HEX);
      digitalWrite(2, HIGH);
       EEPROM.write(30, 1);
       digitalWrite(4, HIGH);
    }
    if((msgString >= "26") && (msgString <= "50")){
      Serial.println("PumpSpeed 50%");
       RS485Serial.write ( sp2, HEX);
      digitalWrite(2, HIGH);
       EEPROM.write(30, 2);
       digitalWrite(5, HIGH);
    }  
     if((msgString >= "51") && (msgString <= "75")){
      Serial.println("PumpSpeed 75%");
      RS485Serial.write ( sp3, HEX);
      digitalWrite(2, HIGH);
       EEPROM.write(30, 3);
       digitalWrite(6, HIGH);
     }
     if((msgString >= "76") && (msgString <= "99")){
      Serial.println("PumpSpeed 100%");
      RS485Serial.write ( sp4, HEX);
      digitalWrite(2, HIGH);
      digitalWrite(4, HIGH);
       EEPROM.write(30, 4);
       digitalWrite(7, HIGH);
       EEPROM.write(30, 4);
//       digitalWrite(4, HIGH);
//       digitalWrite(7, HIGH);
    }  
       

  

}

EthernetClient ethClient;
PubSubClient client(ethClient);

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("iPoolClient")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("PoolPump","hello world");
      // ... and resubscribe
      client.subscribe("PoolPump");
      client.subscribe("Pumpspeed");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(9600);

  client.setServer(server, 1883);
  client.setCallback(callback);

  Ethernet.begin(mac, ip);
  // Allow the hardware to sort itself out
  delay(1500);
  Serial.println("Pentair VS PUMP RS485 Remote"); 
  pinMode(2,OUTPUT);
  digitalWrite(2,LOW);
  pinMode(4,OUTPUT);   // Speed 1
  digitalWrite(2,LOW);
  pinMode(5,OUTPUT);  // Speed 2
  digitalWrite(5,LOW);
  pinMode(6,OUTPUT);  // Speed 3
  digitalWrite(6,LOW);
  pinMode(7,OUTPUT);  // Speed 4
  digitalWrite(7,LOW);
   RS485Serial.begin(9600);



}

void loop()
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  serialControl();
  if  (digitalRead(4) == LOW && (millis() - lastConnectionTime > postingInterval))
  {
    
    RS485Serial.write ( att, HEX);
    delay(20); 
    Serial.print ("PUMP OFF   :");
    Serial.println (millis() /1000);
    lastConnectionTime = millis(); 
  }

  

  while (digitalRead(4) == HIGH && (millis() - lastConnectionTime > postingInterval))
  {
    
    if (EEPROM.read(30)==1) 
    {
        
        RS485Serial.write ( sp1, HEX);
        Serial.print ("Speed 1 at  :");
        Serial.println (millis() /1000);
    }
    if (EEPROM.read(30)==2) 
    {
        digitalWrite(5, HIGH);
        RS485Serial.write ( sp2, HEX);
        Serial.print ("Speed 2 at  :");
        Serial.println (millis() /1000);
    }
    if (EEPROM.read(30)==3) 
    {
        digitalWrite(6, HIGH);
        RS485Serial.write ( sp3, HEX);
        Serial.print ("Speed 3 at  :");
        Serial.println (millis() /1000);
    }
    if (EEPROM.read(30)==4) 
    {
        digitalWrite(7, HIGH);
        RS485Serial.write ( sp4, HEX);
        Serial.print ("Speed 4 at  :");
        Serial.println (millis() /1000);
    }
        
        lastConnectionTime = millis();        

  }
  
}
