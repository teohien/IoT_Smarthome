//////////////////////////////////////////////////////////////////////////////////
//  Vai trò là 1 thành phần của Gateway:                                        //
//  - Thu thập các gói tin MQTT, chuyển đổi các gói tin này thành dữ liệu       //
//    lưu trữ được trên Cloud Server (Firebase)                                 //
//  - Cập nhật dữ liệu khi phát hiện có sự thay đổi trên Firebase, phân tích dữ //
//    liệu ở Firebase rồi chuyển đổi thành các gói tin điều khiển MQTT tương    //
//    ứng. Mosquitto (Broker) sẽ đưa gói tin tới đúng thiết bị cần điều khiển   //
//////////////////////////////////////////////////////////////////////////////////
//________________________________________________________________ Thu vien
    #include <Arduino.h>
    #include <WiFi.h>
    #include <PubSubClient.h>
    #include "FirebaseESP32.h"
    #include <ArduinoJson.h>
    #include <WiFiClient.h>
    #define FIREBASE_HOST "g15-iot-smarthome-v2-default-rtdb.firebaseio.com/" 
    #define FIREBASE_AUTH "I18uOrjmuKvYndEnYgNYmLi4jzEzXuZ1gdSarFsj"

    WiFiClient espClient;           // <=> client = station Wifi
    PubSubClient client(espClient); //<=> client (MQTT)
//________________________________________________________________ PINOUT ESP32 & Parameter
 
    
//________________________________________________________________ Khai báo các topic MQTT sử dụng trên thiết bị này
  //-------------------------------------------------------------
  // - tt (trạng thái - status): trạng thái thiết bị cần gửi (publish)
  // - data (dữ liệu cảm biến) : dữ liệu thu thập được từ thiết bị cần gửi (publish)
  // - dk (điều khiển - control): lệnh điều khiển thiết bị cần nhận (subcribe)  
  //-------------------------------------------------------------
    #define topic1 "pk/dk/Den1"   
    #define topic2 "pk/tt/Den1"  
    #define topic3 "pk/kb/PIR"     
    #define topic4 "pk/tt/Day"  
    #define topic5 "pk/kb/Gtri"  
    #define topic6 "pk/tt/RGB"
    #define topic7 "pk/dk/Quat"
    #define topic8 "pk/tt/Quat"
    #define topic9 "pk/dk/Rem"
    #define topic10 "pk/tt/Rem"

    #define topic11 "pb/dataTemp"
    #define topic12 "pb/kb/BaoChay"
    #define topic13 "pb/kb/PIR"    
    #define topic14 "pb/tt/Coi"
    #define topic15 "pb/dk/Den2"
    #define topic16 "pb/tt/Den2"
    
//________________________________________________________________
  //------ SSID & Password WiFi:
    // const char* ssid = "Hust_D7";
    // const char* password = "033201003185";
    // const char* ssid = "BkStu";
    // const char* password = "1357924680";
    const char* ssid = "TP-Link_0FFA";
    const char* password = "66668888";

  //------ Địa chỉ IP broker:
    const char* mqtt_server = "192.168.0.108";
    String messageMQTT;

  //------ Ten bien du lieu firebase
    FirebaseData DATA_Mos_2_Fb;     //get data from Mosquitto then send to Firebase
    FirebaseData DATA_Fb_2_Mos;     //get data from Firebase then send to Mosquitto
    FirebaseJson json;
    String path = "/";
    // tên path chính
    String parentPath = "/G15_SmartHome"; 
    // các đường dẫn đến path con (đường dẫn đến từng path con: path nhỏ nhất)
    String childPath[8] = {"/KitchenRoom/Control/Light2", "LivingRoom/Control/Curtain", "LivingRoom/Control/Fan",
                            "LivingRoom/Control/Light1", "LivingRoom/Script/Entertainment"}; 
    bool flag_light2 = false, flag_curtain = false, flag_fan = false, flag_light1 = false, flag_Ettm = false;   
    String App_Request;
//________________________________________________________________ setup_WiFi(): Kết nối WiFi & kiểm tra localIP của ESP32
void connect_WiFi()
  {
    delay(3000);
    // Bat dau ket noi voi wifi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      Serial.print(".");
    }
    {
      Serial.println("");
      Serial.println("Da ket noi WiFi");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
    }
  }
//________________________________________________________________ connect_Firebase(): Kết nối tới cơ sở dữ liệu Firebase
void connect_Firebase()
{
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  //Set database read timeout to 1 minute (max 15 minutes): Wait firebase in 1 minute
  Firebase.setReadTimeout(DATA_Mos_2_Fb, 1000*60);

  if (!Firebase.beginStream(DATA_Mos_2_Fb, path) || !Firebase.beginStream(DATA_Fb_2_Mos, path))
  {
    Serial.print("REASON data 1:");
    Serial.println(DATA_Mos_2_Fb.errorReason());
    Serial.print("REASON data 2:");
    Serial.println(DATA_Fb_2_Mos.errorReason());
    Serial.println();
  }
}

//________________________________________________________________ streamCallback(): Cập nhật dữ liệu mới thay đổi trên Firebase  
void streamCallback(MultiPathStreamData DATA_Fb_2_Mos)
{
  size_t numChild = sizeof(childPath) / sizeof(childPath[0]);

  for (size_t i = 0; i < numChild; i++)
  { 
    // DATA_Fb_2_Mos: biến dùng để lưu dữ liệu từ Firebase về Mosquitto
    // DATA_Fb_2_Mos.get : ứng với từng path (đường dẫn đến từng path con: path nhỏ nhất)
    // Kiểm tra xem có path nào cập nhật thì set cờ báo tương ứng với thiết bị được điều khiển 
    if (DATA_Fb_2_Mos.get(childPath[0]))
    {
      App_Request = DATA_Fb_2_Mos.value.c_str();
      flag_light2 = true;
    } 
    if (DATA_Fb_2_Mos.get(childPath[1]))
    {
      flag_curtain = true;
      App_Request = DATA_Fb_2_Mos.value.c_str();
    }     
    if (DATA_Fb_2_Mos.get(childPath[2]))
    {
      App_Request = DATA_Fb_2_Mos.value.c_str();
      flag_fan = true;
    }
    if (DATA_Fb_2_Mos.get(childPath[3]))
    {
      App_Request = DATA_Fb_2_Mos.value.c_str();
      flag_light1 = true;
    } 
    if (DATA_Fb_2_Mos.get(childPath[4]))
    {
      App_Request = DATA_Fb_2_Mos.value.c_str();
      flag_Ettm = true;
    }
  } 
}

void streamTimeoutCallback(bool timeout)
{
  if (timeout)
    Serial.println("stream timed out, resuming...\n");

  if (!DATA_Fb_2_Mos.httpConnected())
    Serial.printf("error code: %d, reason: %s\n\n", DATA_Fb_2_Mos.httpCode(), DATA_Fb_2_Mos.errorReason().c_str());
}

//________________________________________________________________ reconnect_MQTT() - Kết nối lại MQTT Broker
void reconnect_MQTT()
 {
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a client ID
    String clientId = "ESP32 - Gateway";
    if (client.connect(clientId.c_str())) 
    {
      Serial.println("MQTT connected");
      client.subscribe(topic1);
      client.subscribe(topic2);
      client.subscribe(topic3);
      client.subscribe(topic4);
      client.subscribe(topic5);
      client.subscribe(topic6);
      client.subscribe(topic7);
      client.subscribe(topic8);
      client.subscribe(topic9);
      client.subscribe(topic10);
      client.subscribe(topic11);
      client.subscribe(topic12);
      client.subscribe(topic13);
      client.subscribe(topic14);
      client.subscribe(topic15);
      client.subscribe(topic16);
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

//________________________________________________________________ callbackMQTT() - Xử lý gói tin nhận được qua giao thức MQTT xong đẩy lên Firebase
  void callbackMQTT(char* topic, byte* payload, unsigned int length) 
  {
    char messageBuff[100] = {'\0'};
    int i = 0;
    for (i = 0; i < length; i++) 
    {
      messageBuff[i] = (char)payload[i];
    }
    messageBuff[i] = '\0';
    String messageMQTT = String(messageBuff);
    //Serial monitor check
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.println(messageMQTT);
    String TOPIC = String(topic);

         
    //-------------------------------------------------------------
      if(TOPIC == topic2)
      {
        if(messageMQTT == "ON") Firebase.setString(DATA_Mos_2_Fb, "/G15_SmartHome/LivingRoom/Status/Light1", messageMQTT);
        if(messageMQTT == "OFF") Firebase.setString(DATA_Mos_2_Fb, "/G15_SmartHome/LivingRoom/Status/Light1", messageMQTT);  
        return;
      }
    //[**] HIỂN THỊ Kịch bản >> Phát hiện có người <<-------------------------------------------------------------
      if(TOPIC == topic3)
      {
        if(messageMQTT == "pk0cn") Firebase.setString(DATA_Mos_2_Fb, "/G15_SmartHome/LivingRoom/Script/PIR", "Nobody"); //gui gtri tu esp len fb kieu String
        if(messageMQTT == "pkcn") Firebase.setString(DATA_Mos_2_Fb, "/G15_SmartHome/LivingRoom/Script/PIR", "Active"); 
        return;
      }
    // TRẠNG THÁI ĐÈN RGB -------------------------------------------------------------
      if(TOPIC == topic6)
      {
        if(messageMQTT == "ON")   Firebase.setString(DATA_Mos_2_Fb, "/G15_SmartHome/LivingRoom/Status/RGB", messageMQTT); //gui gtri tu esp len fb kieu String
        if(messageMQTT == "OFF")  Firebase.setString(DATA_Mos_2_Fb, "/G15_SmartHome/LivingRoom/Status/RGB", messageMQTT); 
        return;
      }
    // TRẠNG THÁI TỐC ĐỘ QUẠT-------------------------------------------------------------
      if(TOPIC == topic8) 
      {
        Firebase.setString(DATA_Mos_2_Fb, "/G15_SmartHome/LivingRoom/Status/Fan", messageMQTT);
        return;
      }
    // HIỂN THỊ TRẠNG THÁI RÈM------------------------------------------------
      if( TOPIC == topic10)
      {
        if(messageMQTT == "ON1"){ Firebase.setString(DATA_Mos_2_Fb, "/G15_SmartHome/LivingRoom/Status/Curtain", messageMQTT);}
        if(messageMQTT == "ON2"){ Firebase.setString(DATA_Mos_2_Fb, "/G15_SmartHome/LivingRoom/Status/Curtain", messageMQTT);}
        if(messageMQTT == "OFF"){ Firebase.setString(DATA_Mos_2_Fb, "/G15_SmartHome/LivingRoom/Status/Curtain", messageMQTT);}
        return;
      }
    //-------------------------------------------------------------


    //--------PHÒNG BẾP----------
    
    //[**] HIỂN THỊ:  >> Nhiệt độ <<-------------------------------------------------------------
      if(TOPIC == topic11) 
      {
        Firebase.setString(DATA_Mos_2_Fb, "/G15_SmartHome/KitchenRoom/dataTemp", messageMQTT);
        return;
      }  
    //[**] HIỂN THỊ: >> Báo cháy <<-------------------------------------------------------------
      if(TOPIC == topic12)
      {
        if(messageMQTT == "THigh")  Firebase.setString(DATA_Mos_2_Fb, "/G15_SmartHome/KitchenRoom/Script/FireAlarm", "Warning!");
        if(messageMQTT == "TLow")  Firebase.setString(DATA_Mos_2_Fb, "/G15_SmartHome/KitchenRoom/Script/FireAlarm", "Safe");
        return;
      }
    //-------------------------------------------------------------
      if(TOPIC == topic14)
      {
        if(messageMQTT == "ON" || messageMQTT == "OFF")
         Firebase.setString(DATA_Mos_2_Fb, "/G15_SmartHome/KitchenRoom/Status/Alarm", messageMQTT); 
        return;
      }
    //-------------------------------------------------------------
      if(TOPIC == topic13)
      {
        if(messageMQTT == "pb0cn")
          Firebase.setString(DATA_Mos_2_Fb, "/G15_SmartHome/KitchenRoom/Script/PIR", "Nobody"); 
        if(messageMQTT == "pbcn")
          Firebase.setString(DATA_Mos_2_Fb, "/G15_SmartHome/KitchenRoom/Script/PIR", "Active");
        return; 
      }
    
    // HIỂN THỊ: >> Đèn phòng bếp << -------------------------------------------------------------
      if(TOPIC == topic16)
      {
        if(messageMQTT == "ON") Firebase.setString(DATA_Mos_2_Fb, "/G15_SmartHome/KitchenRoom/Status/Light2", messageMQTT); 
        if(messageMQTT == "OFF") Firebase.setString(DATA_Mos_2_Fb, "/G15_SmartHome/KitchenRoom/Status/Light2", messageMQTT); 
        return;
      }
    //-------------------------------------------------------------
  }

//________________________________________________________________ setup()
  void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  //-----------------------------------------------
  //Setup mạng:
  //Kết nối WiFi
  connect_WiFi();                             
  connect_Firebase();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callbackMQTT);
  //----------------------------------------------- 
   if (!Firebase.beginMultiPathStream(DATA_Fb_2_Mos, parentPath))
   Serial.printf("stream begin error, %s\n\n", DATA_Fb_2_Mos.errorReason().c_str());

  Firebase.setMultiPathStreamCallback(DATA_Fb_2_Mos, streamCallback, streamTimeoutCallback);
  
}

  void loop() 
  {
    //-------------------------------- Kiểm tra kết nối client-broker
      if (!client.connected()) 
      {
        reconnect_MQTT();
      }

    //-------------------------------- Duy trì quá trình kết nối client-broker:
    client.loop();

    //-------------------------------- Kiểm tra có lệnh điều khiển được cập nhật trên Firebase 
    if(flag_light1 == true)
    {
      if (App_Request == "1")  client.publish("pk/dk/Den1", "ON"); 
      if (App_Request == "0")  client.publish("pk/dk/Den1", "OFF");
      flag_light1 = false;
    }
    if(flag_curtain == true)
    {
      if (App_Request == "0") client.publish("pk/dk/Rem", "OFF");
      if (App_Request == "1") client.publish("pk/dk/Rem", "ON1");
      if (App_Request == "2") client.publish("pk/dk/Rem", "ON2");
      flag_curtain = false;
    }
    if(flag_Ettm == true)
    {
      if (App_Request == "1")  client.publish("pk/kb/Gtri", "film");              
      if (App_Request == "0")  client.publish("pk/kb/Gtri", "0film");
      flag_Ettm = false;
    }
    if(flag_light2 == true)
    {
      if (App_Request == "1")  client.publish("pb/dk/Den2", "ON");              
      if (App_Request == "0")  client.publish("pb/dk/Den2", "OFF");
      flag_light2 = false;
    }
    if(flag_fan == true)
    {
      char speedChar[8];
      int sp = App_Request.toInt();
      dtostrf(sp,2,0,speedChar);
      client.publish("pk/dk/Quat", speedChar);  
      flag_fan = false;
    }
  }
  