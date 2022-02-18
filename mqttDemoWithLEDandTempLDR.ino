
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <SparkFun_APDS9960.h>
#define LM73_ADDR 0x4D
#define Light 36  

int analog_value = 0;
double temp=0;
#define LED_ON   LOW
#define LED_OFF  HIGH
int sensorValue;
SparkFun_APDS9960 apds = SparkFun_APDS9960();
int isr_flag = 0;

const char* ssid     = "Buzzlightyear"; // Change to yours
const char* password = "@Top12345"; // Change to yours

const char* mqtt_server = "192.168.0.164"; // Change to yours

int cnt;

/* create an instance of PubSubClient client */
WiFiClient espClient;
PubSubClient client(espClient);

/*LED GPIO pin*/
const char led1 = 2;
const char led2 = 12;

/* topics */
#define TEMP_TOPIC    "room1/temp"
#define LDR_TOPIC    "room1/ldr"
#define LED_TOPIC1     "room1/led1" /* 1=on, 0=off */
#define LED_TOPIC2     "room1/led2" /* 1=on, 0=off */


long lastMsg = 0;
char msg[20];
char tempC[20];
char LDRC[20];


float readTemperature() {
  Wire1.beginTransmission(LM73_ADDR);
  Wire1.write(0x00); // Temperature Data Register
  Wire1.endTransmission();
 
  uint8_t count = Wire1.requestFrom(LM73_ADDR, 2);
  float temp = 0.0;
  if (count == 2) {
    byte buff[2];
    buff[0] = Wire1.read();
    buff[1] = Wire1.read();
    temp += (int)(buff[0]<<1);
    if (buff[1]&0b10000000) temp += 1.0;
    if (buff[1]&0b01000000) temp += 0.5;
    if (buff[1]&0b00100000) temp += 0.25;
    if (buff[0]&0b10000000) temp *= -1.0;
  }
  return temp;
}
void receivedCallback(char* topic, byte* payload, unsigned int length) {

  Serial.print("Message received: ");
  Serial.println(topic);
  if (String(topic) == "room1/led1") {
    Serial.print("payload: ");
    for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
    }
    Serial.println();

    if ((char)payload[0] == '1') {
      Serial.println("Your LED is on");
      digitalWrite(led1, LED_ON);  /* got '1' -> on state */
    } else {
      Serial.println("Your LED is off");
      digitalWrite(led1, LED_OFF);
    } /* we got '0' -> off state */

  }
  else if (String(topic) == "room1/led2") {
    Serial.print("payload: ");
    for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
    }
    Serial.println();

    if ((char)payload[0] == '1') {
      Serial.println("Your LED is on");
      digitalWrite(led2, LED_ON);  /* got '1' -> on state */
    } else {
      Serial.println("Your LED is off");
      digitalWrite(led2, LED_OFF);
    } /* we got '0' -> off state */

  }

}

void mqttconnect() {
  /* Loop until reconnected */
  while (!client.connected()) {
    Serial.print("MQTT connecting ...");
    /* client ID */
    String clientId = "ESP32Client";
    /* connect now */
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      /* subscribe topic with default QoS 0*/
      client.subscribe(LED_TOPIC1);
      client.subscribe(LED_TOPIC2);
    } else {
      Serial.print("failed, status code =");
      Serial.print(client.state());
      Serial.println("try again in 5 seconds");
      /* Wait 5 seconds before retrying */
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  Wire1.begin(4, 5);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  /* set led as output to control led on-off */
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  digitalWrite(led1, LED_OFF);
  digitalWrite(led2, LED_OFF);
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, 1883); /* configure the MQTT server with IPaddress and port */
  client.setCallback(receivedCallback); /* client received subscribed topic */

}

void loop() {

  if (!client.connected()) { /* try to reconnect */
    mqttconnect();
  }

  client.loop(); /* listen for incomming subscribed receivedCallback */

  /* every 3 secs count increament for a published message */
  long now = millis();
  if (now - lastMsg > 3000) {
    lastMsg = now;
    temp = readTemperature();

    
    int val = analogRead(Light);    
    Serial.print("Temp:");
    Serial.println(temp);
    Serial.println(val);
    snprintf (tempC, 20, "%.2f", temp);
    snprintf (LDRC, 20, "%d", val);
    client.publish(TEMP_TOPIC, tempC); /* publish the message */
    client.publish(LDR_TOPIC, LDRC); /* publish the message */
  }
}
