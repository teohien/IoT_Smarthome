/////////////////////////////////////////////////////////////////////////
//  @ NAME: G15_IoT_Liv_Device2                                        //
//   Các thiết bị được điều khiển:                                     //
//    - Quat (Động cơ DC 9V)                                           //
//    - Rèm  (Servo SG90)                                              //
/////////////////////////////////////////////////////////////////////////
//________________________________________________________________ Include Library
  #include <WiFi.h>
  #include <Arduino.h>
  #include <PubSubClient.h>
  #include <ESP32Servo.h>
  #include <WiFiClient.h>
    
//________________________________________________________________ PINOUT ESP32 & Parameter
//3. Quat - DC motor
  #define MOTOR_PIN 14            //ENA - L298N: cấp xung pwm điều khiển tốc độ động cơ
  #define IN1 19
  #define IN2 21
  const uint8_t PwmChannelA3 = 3; //Chon kenh pwm3
  const uint8_t resolution = 8;   //chọn độ phân giải cho pwm là 8 <=> range (0-255)
  const uint16_t freq = 5000;     //tần số pwm: f = 5000Hz  
//4. Rèm - Servo SG90
  #define Servo_PIN 15
  Servo servoRem;
  //nâu :GND
  //đỏ: Vin (5V)
  //cam: chân tín hiệu

//________________________________________________________________ Khai báo các topic MQTT sử dụng trên thiết bị này
//-------------------------------------------------------------
// - tt (trạng thái): trạng thái thiết bị cần gửi (publish)
// - data : dữ liệu thu thập được từ thiết bị cần gửi (publish)
// - dk (điều khiển): lệnh điều khiển thiết bị cần nhận (subcribe)
//-------------------------------------------------------------
  #define topic1 "pk/dk/Den1"   
  #define topic2 "pk/tt/Den1"   
  #define topic3 "pk/tt/Quat"
  #define topic4 "pk/dk/Quat"
  //#define topic6 "pk/dataTemp"
  #define topic5 "pk/kb/Gtri"
  #define topic7 "pk/tt/RGB"
  #define topic8 "pk/tt/Rem"
  #define topic9 "pk/dk/Rem"

  #define topic10 "pk/tt/Day"
  #define topic11 "pk/dataTemp"
  #define topic12 "pk/kb/PIR"
  bool flag_Rem = false; 
  bool flag_Quat = false;
  bool flag_Gtri = false;
  String message; //Lưu bản tin MQTT 
  String TOPIC;   //TOPIC mà MCU nhận

//________________________________________________________________ Khai báo mạng WiFi sử dụng & IP Broker
  //------ SSID & Password WiFi:
    // const char* ssid = "Hust_D7";
    // const char* password = "033201003185";
    // const char* ssid = "BkStu";
    // const char* password = "1357924680";
    const char* ssid = "TP-Link_0FFA";
    const char* password = "66668888";

//------ Địa chỉ IP broker:
    const char* mqtt_server = "192.168.0.108";
    
    WiFiClient espClient;           
    PubSubClient client(espClient); //<=> client (MQTT)
//________________________________________________________________ Kết nối WiFi & kiểm tra localIP của ESP32
void setup_WiFi()
{
  delay(3000);
  WiFi.begin(ssid, password);
  static unsigned long time_loss_WiFi = millis();
    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("Da ket noi WiFi 1");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());    
}

//________________________________________________________________ reconnect(): Kết nối lại MQTT Broker
void reconnect()
 {
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a client ID
    String clientId = "ESP32-02 - Phong khach";
    if (client.connect(clientId.c_str())) 
    {
      Serial.println("MQTT connected");
      client.subscribe(topic4);   //topic: pk/dk/Quat 
      client.subscribe(topic5);   //topic: pk/kb/Gtri
      client.subscribe(topic9);   //topic: pk/dk/Rem
    }
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
 }
//________________________________________________________________ Hàm điều khiển quạt, rèm
//--------------- RÈM (Servo SG90)
void Goc_Rem(int pos)   //pos (góc): 0-180*
{
  servoRem.write(pos);
  if(pos == 180) client.publish("pk/tt/Rem", "ON2");
  if(pos == 135) client.publish("pk/tt/Rem", "ON1");
  if(pos == 90) client.publish("pk/tt/Rem", "OFF");
  flag_Rem = false;
}

//---------------- Quạt (DC motor)
void Tocdo_Quat(int speed)
{
  //speed = [0-100] <=> [0-255] duty cycle 
  int duty = int(speed*28.3);
  ledcWrite(PwmChannelA3, duty);
  char speedChar[8];
  dtostrf(speed, 2, 0, speedChar);
  client.publish("pk/tt/Quat",speedChar);
}

//________________________________________________________________ ISR: Xử lý gói tin nhận được qua giao thức MQTT 
void callback(char* topic, byte* payload, unsigned int length) 
{
  char messageBuff[20] = {'\0'};
  int i = 0;
    for (i = 0; i < length; i++) 
    {
      messageBuff[i] = (char)payload[i];
    }
    messageBuff[i] = '\0';
    message = String(messageBuff);
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.println(message);
    TOPIC = String(topic);
    //-------------------------------------------------------------
    if(TOPIC == "pk/dk/Quat")
    { 
      flag_Quat = true; //Bật cờ báo có hiệu lệnh điều khiển quạt
      return;
    }
    //-------------------------------------------------------------
    if(TOPIC == "pk/dk/Rem")
    {
      flag_Rem = true; //Bật cờ báo có hiệu lệnh điều khiển rèm
      return;
    }
    //-------------------------------------------------------------
    if(TOPIC == "pk/kb/Gtri")
    {
      flag_Gtri = true; //Bật cờ báo có kịch bản Giải trí
      return;
    }
}

//________________________________________________________________ 
void setup() 
{
  // put your setup code here, to run once:
    Serial.begin(115200);
  //-----------------------------------------------
  //*Setup mạng:
    setup_WiFi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
    
  //*Cấu hình và setup các tham số cho động cơ 
    ledcSetup(PwmChannelA3, freq, resolution);
    ledcAttachPin(MOTOR_PIN, PwmChannelA3); //ENA
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    servoRem.attach(Servo_PIN);
} 

void loop() 
{
  //-----------------------------------------------
  //Kiểm tra kết nối client-broker
  if (!client.connected()) 
  {
    reconnect();
  }
  //-----------------------------------------------
  //-----------------------------------------------
  // Dieu khien rem
  if(flag_Rem == true) 
  {
    if(message == "ON2")
      Goc_Rem(180);
    if(message == "ON1")
      Goc_Rem(135);
    if(message == "OFF") 
      Goc_Rem(90);
    flag_Rem = false;
  }
  //--------------------------------------------
  //Dieu khien toc do quat
  if(flag_Quat == true)
  {
    if(message.toInt() <= 9 && message.toInt() >=0)
    {
      Tocdo_Quat(message.toInt());
    }
    flag_Quat = false;
  }  
  //-------------------------------------------- Kich ban Giai tri
  //Quat se dat o muc trung binh
  if(flag_Gtri == true)
  {
    Tocdo_Quat(5);
    flag_Gtri = false;
  }
  //-----------------------------------------------
  //* Duy trì quá trình kết nối client-broker:
    client.loop();
}
