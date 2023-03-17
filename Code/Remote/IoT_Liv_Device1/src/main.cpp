////////////////////////////////////////////////////////////////////////////
//  @ NAME: G15_IoT_Liv_Device1                                           //
//  @ Các thiết bị được điều khiển:                                       //  
//    - Đèn phòng khách  (Led1)                                           //  
//    - Đèn giải trí thay đổi màu tùy kịch bản giải trí (Led RGB)         //  
////////////////////////////////////////////////////////////////////////////
//________________________________________________________________ Include Library
  #include <WiFi.h>
  #include <PubSubClient.h>
  #include <WiFiClient.h>
  #include <Arduino.h>
//________________________________________________________________ PINOUT ESP32 & Parameter
//1. LED & Button:
  #define button 32     
  #define ledPin1 14 
  bool led1_sta = true; //Trạng thái led 1
//---------------------------------------------------

//2. LED RGB - Catode chung (GND chung):
  #define RED_PIN 27    //led đỏ
  #define GREEN_PIN 26  //led xanh lá
  #define BLUE_PIN 25   //led xanh dương
  //ESP32 có 16 kenh pwm
  const uint8_t PwmChannelA0 = 0; //Chon kenh pwm0
  const uint8_t PwmChannelA1 = 1; //Chon kenh pwm1
  const uint8_t PwmChannelA2 = 2; //Chon kenh pwm2
  const uint8_t resolution = 8;   //chọn độ phân giải cho pwm là 8 <=> range (0-255)
  const uint16_t freq = 5000;     //tần số pwm: f = 5000Hz
  bool RGB_sta = 0;
  unsigned long time_Random_RGB = millis();

//________________________________________________________________ Khai báo các topic MQTT sử dụng trên thiết bị này
  //-------------------------------------------------------------
  // - tt (trạng thái): trạng thái thiết bị cần gửi (publish)
  // - data : dữ liệu thu thập được từ thiết bị cần gửi (publish)
  // - dk (điều khiển): lệnh điều khiển thiết bị cần nhận (subcribe)
  //-------------------------------------------------------------  
    #define topic1 "pk/dk/Den1"   //topic để điều khiển đèn led 1
    #define topic2 "pk/tt/Den1"   //Trạng thái Đèn 1
    #define topic3 "pk/tt/Quat"
    #define topic4 "pk/dk/Quat"  //topic điều khiển quạt
    //#define topic6 "pk/dataTemp"
    #define topic5 "pk/kb/Gtri"
    #define topic7 "pk/tt/RGB"
    #define topic8 "pk/tt/Rem"
    #define topic9 "pk/dk/Rem"
    #define topic10 "pk/tt/Day"
    #define topic11 "pk/dataTemp"
    #define topic12 "pk/kb/PIR"
  
    bool flag_showSpec = false;
    bool flag_Gtri = false;
    bool flag_PIR = false;
    String message;
    String TOPIC;
//________________________________________________________________ Khai báo mạng WiFi sử dụng & IP Broker
//------ SSID & Password WiFi:
  // const char* ssid = "BkStu";
  // const char* password = "1357924680";
  const char* ssid = "TP-Link_0FFA";
  const char* password = "66668888";

//------ Địa chỉ IP broker:
  const char* mqtt_server = "192.168.0.108";
  WiFiClient espClient; // <=> client = station Wifi
  PubSubClient client(espClient); //<=> client (MQTT)

//________________________________________________________________ setup_WiFi(): Kết nối WiFi & kiểm tra localIP của ESP32
void setup_WiFi()
{
  delay(3000);
  int idwifi=1;
  // Bat dau ket noi voi wifi
  WiFi.begin(ssid, password);
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
    String clientId = "ESP32-01 - Phong khach";
    if (client.connect(clientId.c_str())) 
    {
      Serial.println("MQTT connected");
      client.subscribe(topic1);   // topic: pk/dk/Den1
      client.subscribe(topic5);   // topic: pk/kb/Gtri
      client.subscribe(topic12);  // topic: pk/kb/PIR
    }
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 3 seconds");
      // Wait 3 seconds before retrying
      delay(3000);
    }
  }
 }

//________________________________________________________________ Processing_Button1(): Xử lý button để bật/tắt Led 1
void Processing_Button1()
{
  bool btn_sta = digitalRead(button);
  if(btn_sta == LOW)
  {
    delay(20); //tránh dội phím
	  if(btn_sta == LOW)
	  {
      if(led1_sta == LOW)
      {
        digitalWrite(ledPin1, HIGH);
        led1_sta = HIGH; //update lại trạng thái led
        client.publish("pk/tt/Den1", "ON");
      }
      else
      {
        digitalWrite(ledPin1, LOW);
        led1_sta = LOW;
        client.publish("pk/tt/Den1", "OFF");
      }
	  }
	  while(digitalRead(button) == LOW);
  }
}

//________________________________________________________________ Điều khiển bật/tắt Đèn và Đèn RGB
//LED1:
void On_Led1()
{
  digitalWrite(ledPin1, HIGH);
  led1_sta = 1;
  client.publish("pk/tt/Den1", "ON");
}
void Off_Led1()
{
  digitalWrite(ledPin1, LOW);
  led1_sta = 0;
  client.publish("pk/tt/Den1", "OFF");
}
//LED RGB:
void Off_LedRGB()       //Tắt RGB
{
  ledcWrite(PwmChannelA0, 0);
	ledcWrite(PwmChannelA1, 0);
  ledcWrite(PwmChannelA2, 0);
  RGB_sta = 0;
  client.publish("pk/tt/RGB", "OFF");
}
void showSpectrum()     //Bật đèn giải trí với hiệu ứng đổi màu liên tục
{
  if(millis()- time_Random_RGB > 200)
  {
    time_Random_RGB = millis();
    ledcWrite(PwmChannelA0, uint32_t(random(0,255))); //Trả ngẫu nhiên giá trị {0,1} cho RED_PIN
    ledcWrite(PwmChannelA1, uint32_t(random(0,255)));
    ledcWrite(PwmChannelA2, uint32_t(random(0,255)));
    if(RGB_sta == 0)
    {
      client.publish("pk/tt/RGB", "ON");
      RGB_sta = 1;
    }
  }   
}

//________________________________________________________________ callback(): ISR xử lý gói tin nhận được qua giao thức MQTT 
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
    // Message từ topic "pk/dk/Den1"
    if(TOPIC == "pk/dk/Den1")
    {  
      if (message == "ON")  On_Led1();
      if (message == "OFF") Off_Led1();
      return;
    } 
    //-------------------------------------------------------------
    
    //-------------------------------------------------------------
    if(TOPIC == "pk/kb/Gtri")
    {
      flag_Gtri = true;
      return;
    }
    //-------------------------------------------------------------

    //-------------------------------------------------------------
    // Message từ topic "pk/kbPIR"
    if(TOPIC == "pk/kb/PIR")
    {
      flag_PIR = true;
      return;
    }
    //-------------------------------------------------------------
}

//________________________________________________________________ setup(): Khởi tạo các thông số ban đầu
void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  //-----------------------------------------------
  //*Setup mạng:
  setup_WiFi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  //-----------------------------------------------
  pinMode(ledPin1, OUTPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(button, INPUT_PULLUP);
  //-----------------------------------------------
  //*Cấu hình và setup các tham số cho đèn RGB
  ledcSetup(PwmChannelA0, freq, resolution);
  ledcSetup(PwmChannelA1, freq, resolution);
  ledcSetup(PwmChannelA2, freq, resolution);
  ledcAttachPin(RED_PIN, PwmChannelA0);
  ledcAttachPin(GREEN_PIN, PwmChannelA1);
  ledcAttachPin(BLUE_PIN, PwmChannelA2);
}
//________________________________________________________________ loop(): Vòng lặp chính
void loop() 
{
  //::: Kiểm tra kết nối client-broker
  if (!client.connected()) 
  {
    reconnect();
  }
  
  //:::  Điều khiển bật/tắt đèn bằng nút bấm
  Processing_Button1();
  if(flag_showSpec == true) showSpectrum();

  //::: Kiểm tra kịch bản:
  //------ Kịch bản Giải trí:
  if(flag_Gtri == true)
  {
    if(message == "film")
    {
      Off_Led1();
      flag_showSpec = true;
      flag_Gtri = false;
    }
    if(message == "0film")
    {
      On_Led1();
      Off_LedRGB();
      flag_Gtri = false;
      flag_showSpec = false;
    }
  }
  //------ Kịch bản phát hiện có người - tự động bật đèn
  if((flag_PIR == true) && (flag_Gtri == false))
  {
    if(message == "pkcn") On_Led1();
    if(message == "pk0cn") Off_Led1();
    flag_PIR = false;
  }
  //::: Duy trì quá trình kết nối client-broker:
  client.loop();
}
