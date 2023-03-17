///////////////////////////////////////////////////////////////////////////
//  @ NAME: G15_IoT_Kit_Sensor                                           //  
//  Các cảm biến sử dụng                                                 //
//  - Cảm biến hồng ngoại chuyển động PIR                                //
//  - Cảm biến nhiệt độ - độ ẩm DHT11                                    //
///////////////////////////////////////////////////////////////////////////
//_______________________________________________________________ Include Library
  #include <WiFi.h>
  #include <Arduino.h>
  #include <WiFiClient.h>
  #include <PubSubClient.h>
  #include <DHT.h>
//_______________________________________________________________ PINOUT ESP32 & parameters
//----- Loại cảm biến nhiệt độ sử dụng:
  #define DHTPIN 5        // D1
  #define DHTTYPE DHT11   // DHT 11
  DHT dht(DHTPIN, DHTTYPE);
  unsigned long pre_milisTemp = millis();

//----- PIR - SR501:
  #define cb_out 19 
  int var_pir_now, var_pir_last = 1;
  unsigned long time_detect_false;
  bool detect;
  unsigned long pre_millisPIR = millis();
//_______________________________________________________________ Khai báo các topic MQTT sử dụng trên thiết bị này
//------ Các topic MQTT:
  #define topic1   "pb/kb/BaoChay"   //dieu khien coi bao, du lieu ve coi bao 
  #define topic2 "pb/tt/Coi"         //dieu khien coi bao, du lieu ve coi bao 
  #define topic3 "pb/dataTemp"       //topic theo doi nhiet do phong bep
  #define topic4 "pk/dataTemp"       //topic theo doi nhiet do phong khach
  #define topic5 "pb/dk/Den2"        //topic để điều khiển đèn led 2
  #define topic6 "pb/tt/Den2" 
  #define topic7 "pb/kb/PIR"
//_______________________________________________________________ Khai báo mạng sử dụng & IP Broker
  //------ SSID & Password WiFi:
    // const char* ssid = "BkStu";
    // const char* password = "1357924680";
    const char* ssid = "TP-Link_0FFA";
    const char* password = "66668888";

  //------ Địa chỉ IP broker:
    const char* mqtt_server = "192.168.0.108";
    WiFiClient espClient;           //<=> client = station Wifi
    PubSubClient client(espClient); //<=> client (MQTT)
    char messageBuff[100];
//_______________________________________________________________ setup_WiFi(): Kết nối WiFi & kiểm tra localIP của ESP32
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
  //  - Nếu như mất kết nối với MQTT Broker thì hàm này sẽ thực 
  //    hiện kết nối lại với Broker
  //  - Nếu trên Serial Monitor mà cứ liên tục hiện "Attempting 
  //    MQTT connection..." thì có thể là:
  //       >> Do tên topic có vấn đề khiến Client (ESP32/ESP8266) 
  //          publish (gửi bản tin) không được
  //       >> Do hàm lấy dữ liệu cảm biến có vấn đề...
  //       >> Do mất kết nối mạng
void reconnect()
  {
    // Loop until we're reconnected
    while (!client.connected())
    {
      Serial.print("Attempting MQTT connection...");
      // Create a client ID
      String clientId = "ESP8266-02 - Phong bep";
      if (client.connect(clientId.c_str())) 
      {
        Serial.println("MQTT connected");
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

//_______________________________________________________________ read_temp() - Đọc nhiệt độ cảm biến DHT11
  //-------------------------------------------------------------
  // - Sử dụng thư viện DHT của Adafruit
  // - read_temp() có chu kỳ thực hiện là 5s. Có 2 task chính:
  //   >> Đọc nhiệt độ từ cảm biến DHT11 sau đó ép kiểu float -> String để publish lên Mosquitto (Broker). 
  //   >> Kiểm tra nhiệt độ đo có đang ở mức ngưỡng báo không (đang là 29*C) nếu có thì sẽ publish gói tin
  //      kịch bản cảnh báo còi.
  //-------------------------------------------------------------
void read_temp()
{
  static int t_to = 0, t_be = 0; //Biến lưu lại nhiệt độ lần đo trước đã cao/thấp hơn ngưỡng rồi để tránh publish bản tin lặp lại
  //------------------------------------------------------------
  //Sau 2s mới đọc nhiệt độ 1 lần
  if(millis() - pre_milisTemp >= 5000)
  {
    pre_milisTemp = millis(); 
    float t = dht.readTemperature();
    // Check if any reads failed and exit early (to try again).
    if (isnan(t))
    {
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    }
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.println(F("*C ")); 
  //-------------------------------------------------------------
  //publish du lieu nhiet do de theo doi
    char dataTemp[8];
    dtostrf(t, 4, 2, dataTemp);
    client.publish( "pb/dataTemp", dataTemp);
    client.publish( "pk/dataTemp", dataTemp); 
  //-------------------------------------------------------------
  //publish du lieu cho topic dieu khien coi bao phòng bếp
    if(t >= 29 && t_to == 0)
    {
      t_to = 1;
      t_be = 0;
      client.publish("pb/kb/BaoChay", "THigh");
    }
    if(t < 29 && t_be == 0)
    {
      t_to = 0;
      t_be = 1;
      client.publish("pb/kb/BaoChay", "TLow");
    } 
  }
  else return;
}

//________________________________________________________________ checkPIR() - Kiểm tra trạng thái cảm biến chuyển động PIR
  //-------------------------------------------------------------
  // - checkPIR() có chu kỳ đọc trạng thái cảm biến là 1s. Có 1 task chính:
  //   >> Kiểm tra trạng thái của cảm biến:
  //     + Nếu phát hiện có người <=> có chuyển động thì gửi bản tin "pkcn" 
  //      tới topic "pk/kbPIR", "pb/kbPIR"
  //     + Nếu không có người trong 5p <=> không có chuyển động trong 5p 
  //      thì gửi bản tin "pk0cn" tới topic "pk/kbPIR", "pb/kbPIR"  
  //-------------------------------------------------------------
void checkPIR()
{
  static int pir_1 = 0, pir_0 = 0;
  //Sau 1s mới kiểm tra trạng thái cảm biến chuyển động 1 lần
  if(millis() - pre_millisPIR >= 1000)
  {
    pre_millisPIR = millis(); //Cập nhật lại thời điểm đọc cảm biến
    var_pir_now = digitalRead(cb_out);
    Serial.print("Status of PIR SC501 sensor: ");
    Serial.println(var_pir_now);
    //--------------------------------------------------------------------
    //Phát hiện có sự thay đổi trạng thái cảm biến PIR
    if(var_pir_now != var_pir_last) 
    {
      if(var_pir_now == 0 ) //Cảm biến không phát hiện chuyển động
      {
        var_pir_last = var_pir_now; //Cập nhật giá tị
        detect = false;             //Đang không có người
        time_detect_false = millis();
      }
      else              //Cảm biến phát hiện có chuyển động
      {
        var_pir_last = var_pir_now; //Cập nhật giá trị
        detect = true;
        if(pir_1 == 0)
        {
          pir_1 = 1;
          pir_0 = 0;
          client.publish("pk/kb/PIR", "pkcn");
          client.publish("pb/kb/PIR", "pbcn");
        }    
      }
    }
    //--------------------------------------------------------------------
    //Phát hiện Không có người trong 10s, gửi bản tín báo
    if( (detect == false) && (millis()-time_detect_false >= 10000)) //Xác nhận 10s không có người chuyển động
    {
      if(pir_0 == 0)
      {
        pir_0 = 1;
        pir_1 = 0;
        client.publish("pk/kb/PIR","pk0cn");
        client.publish("pb/kb/PIR","pb0cn");
      }        
    }
    //--------------------------------------------------------------------
    //Phát hiện Có người, gửi bản tin báo 
    if((detect == true) || (var_pir_now == 1) )
    {
      if(pir_1 == 0)
      {
        pir_1 = 1;
        pir_0 = 0;
        client.publish("pk/kb/PIR", "pkcn");
        client.publish("pb/kb/PIR", "pbcn");
      }
    }
  }
  else return;
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
  String message = String(messageBuff);
} 

//_______________________________________________________________ setup(): Khởi tạo các thông số ban đầu
void setup() 
{
  // put your setup code here, to run once:
    Serial.begin(115200);
  //Cấu hình chân ra cảm biến PIR là đầu vào ESP32/ESP8266
    pinMode(cb_out, INPUT);
  //Bắt đầu sử dụng DHT11:
    dht.begin();
  //Setup mạng:
    setup_WiFi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
}

//_______________________________________________________________ loop() - Vòng lặp chính
void loop() 
{
  //Kiểm tra kết nối client-broker
    if (!client.connected()) 
    {
      reconnect();
    }
  //Giám sát dữ liệu cảm biến:
    read_temp();
    checkPIR();
  //Duy trì quá trình kết nối client-broker:
    client.loop();
}
