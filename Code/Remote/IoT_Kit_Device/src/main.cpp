///////////////////////////////////////////////////////////////////////////
//  @ NAME: G15_IoT_Kit_Device                                           //
//  @ Các thiết bị được điều khiển:                                      //
//  - Đèn phòng bếp (Led 2)                                              //
//  - Còi báo cháy (còi chip)                                            //
///////////////////////////////////////////////////////////////////////////
//_______________________________________________________________ Include Library
  #include <WiFi.h>
  #include <Arduino.h>
  #include <PubSubClient.h>
  #include <WiFiClient.h>

//---------------------------------------------------  
  WiFiClient espClient;
  PubSubClient client(espClient); //<=> client (MQTT)

//_______________________________________________________________ PINOUT ESP32/ESP8266 & Thư viện 
//----- LED 2 & Còi  
  #define ledPin2 26    
  bool led2_sta = false; //Trạng thái led 2
  #define RingPin 14  
//_______________________________________________________________ Khai báo các topic MQTT sử dụng trên thiết bị này
//------ Các topic MQTT:
  #define topic1 "pb/kb/BaoChay"  //dieu khien coi bao, du lieu ve coi bao 
  #define topic2 "pb/tt/Coi"      //dieu khien coi bao, du lieu ve coi bao 
  #define topic3 "pb/dataTemp"    //topic theo doi nhiet do phong bep
  #define topic4 "pk/dataTemp"    //topic theo doi nhiet do phong khach
  #define topic5 "pb/dk/Den2"     //topic để điều khiển đèn led 1
  #define topic6 "pb/tt/Den2" 
  #define topic7 "pb/kb/PIR"
  bool flag_BaoChay = false;
  bool flag_PIR = false;
  String old_BaoChay, old_PIR;    
  String message;  
  char messageBuff[100];
//_______________________________________________________________ Khai báo mạng WiFi sử dụng & IP Broker
//------ SSID & Password WiFi:
  // const char* ssid = "BkStu";
  // const char* password = "1357924680";
  const char* ssid = "TP-Link_0FFA";
  const char* password = "66668888";

//------ Địa chỉ IP broker:
  const char* mqtt_server = "192.168.0.108";
//_______________________________________________________________ Kết nối WiFi & kiểm tra localIP của ESP32
  void setup_WiFi()
{
  delay(3000);
  // Bat dau ket noi voi wifi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Da ket noi WiFi");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
//_______________________________________________________________ reconnect() - Kết nối lại MQTT Broker
  //-------------------------------------------------------------
  //  - Nếu như mất kết nối với MQTT Broker thì hàm này sẽ thực hiện kết nối lại với Broker
  //  - Nếu trên Serial Monitor mà cứ liên tục hiện "Attempting MQTT connection..." thì có thể là:
  //       >> Do tên topic có vấn đề khiến Client (ESP32/ESP8266) 
  //          publish (gửi bản tin) không được
  //       >> Do hàm lấy dữ liệu cảm biến có vấn đề...
  //-------------------------------------------------------------
  void reconnect()
  {
    // Loop until we're reconnected
    while (!client.connected())
    {
      Serial.print("Attempting MQTT connection...");
      // Create a client ID
      String clientId = "ESP8266-2- Phong bep";
      if (client.connect(clientId.c_str())) 
      {
        Serial.println("MQTT connected");
        client.subscribe(topic1);
        //client.subscribe(topic2);
        //client.subscribe(topic3);
        //client.subscribe(topic4);
        client.subscribe(topic5);
        //client.subscribe(topic6);
        client.subscribe(topic7);
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
//_______________________________________________________________ Điều khiển bật/tắt LED & Còi báo hiệu
  void On_Led2()
  {
    digitalWrite(ledPin2, HIGH);
    client.publish("pb/tt/Den2", "ON");
  }
  void Off_Led2()
  {
    digitalWrite(ledPin2, LOW);
    client.publish("pb/tt/Den2", "OFF");
  }
  void On_Coi()
  {
    digitalWrite(RingPin, HIGH);
    client.publish("pb/tt/Coi", "ON");
  }
  void Off_Coi()
  {
    digitalWrite(RingPin, LOW);
    client.publish("pb/tt/Coi", "OFF");
  }
//_______________________________________________________________ callback(): ISR xử lý gói tin nhận được qua giao thức MQTT 
  void callback(char* topic, byte* payload, unsigned int length) 
  {
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
    String TOPIC = String(topic);
    if(TOPIC == "pb/kb/BaoChay")
    {
      flag_BaoChay = true;
      return;
    }
    if(TOPIC == "pb/dk/Den2")
    {
      if(message == "ON")   On_Led2();
      if(message == "OFF")   Off_Led2();
    }
    if(TOPIC == "pb/kb/PIR")
    {
      flag_PIR = true;
      return; 
    }
  } 

//_______________________________________________________________ setup(): Khởi tạo các thông số ban đầu
  void setup() 
{
  // put your setup code here, to run once:
    Serial.begin(115200);
  //Cấu hình cho còi báo là đầu ra:
    pinMode(RingPin, OUTPUT);
    pinMode(ledPin2, OUTPUT);
  //Setup mạng:
    setup_WiFi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
}
//_______________________________________________________________ loop(): Vòng lặp chính
  void loop() 
{
  //Kiểm tra kết nối client-broker
  if (!client.connected()) 
  {
    reconnect();
  }
  if(flag_BaoChay == true)
  {
    if(message == "THigh")   On_Coi();
    if(message == "TLow")   Off_Coi();
    flag_BaoChay = false;
  }
  if(flag_PIR == true)
  {
    if(message == "pbcn") On_Led2();
    if(message == "pb0cn") Off_Led2();
    flag_PIR = false;
  }
  //Duy trì quá trình kết nối client-broker:
    client.loop();
}
