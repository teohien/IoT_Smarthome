/*
*** Local
- Cơ chế hoạt động khi không có internet : 
+ Điều khiển qua Web: Người dùng sẽ truy cập vào trình duyệt theo địa chỉ IP 
của ESP32 để chạy Web Client, điều khiển thiết bị thông qua trình duyệt, Web Server 
sẽ chạy trên ESP32. Trình duyệt hiển thị nhiệt độ, độ ẩm đo được từ cảm biến DHT11, 
có nút nhấn để điều khiển đèn, có thanh trượt để điều khiển tốc độ quạt. 
+ Điều khiển cứng: có nút nhấn điều khiển trực tiếp đèn. Trạng thái đèn và quạt được 
cập nhật đồng bộ, hiển thị trên trình duyệt: nhấn nút nhấn cứng thì đèn sáng, trên Web 
hiển thị trạng thái đèn đang bật.
=> Đồng bộ trạng thái đèn khi điều khiển bằng web hoặc nút nhấn: nếu nhấn nút thì đèn sáng, 
trạng thái đèn trên web - state: ON, nhấn nút lần nữa thì đèn tắt, trạng thái đèn trên 
web - state: OFF.

//Chân đọc dữ liệu DHT11
#define DHTPIN 23     
// Chân điều khiển quạt
const int output = 14;
#define IN1 19
#define IN2 21

// Chân Toggle Led
const int output1 = 13; 

// Chân nút nhấn
const int buttonPin = 32;

*/


#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

// Khai báo Wifi
const char* ssid = "BkStu";
const char* password = "1357924680";
//Chân đọc dữ liệu DHT11
#define DHTPIN 23     
// DHT 11
#define DHTTYPE    DHT11     
const char* PARAM_INPUT_1 = "state";
// Chân điều khiển quạt
const int output = 14;  // PWM
#define IN1 19
#define IN2 21
// Chân Toggle Led
const int output1 = 13; 
// Chân nút nhấn
const int buttonPin = 32;
String sliderValue = "0";
int data = 0;
// Biến nút nhấn
int ledState = LOW;          // the current state of the output pin
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
// Khởi tạo PWM để điều khiển quạt
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;
// Biến đếm thời gian nút nhấn
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers
//Biến tìm kiếm giá trị của thanh trượt khi di chuyển
const char* PARAM_INPUT = "value";
// Khai báo cảm biến
DHT dht(DHTPIN, DHTTYPE);

// Tạo Web Server cổng 80
AsyncWebServer server(80);
// Đọc dữ liệu từ DHT11
String readDHTTemperature() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  //float t = dht.readTemperature(true);
  // Check if any reads failed and exit early (to try again).
  if (isnan(t)) {    
    Serial.println("Failed to read from DHT sensor!");
    return "--";
  }
  else {
    Serial.println(t);
    return String(t);
  }
}

String readDHTHumidity() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  if (isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    return "--";
  }
  else {
    Serial.println(h);
    return String(h);
  }
}

// Tạo giao diện cho Web Server
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>


  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <title>IoT LivingRoom 1</title>
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 2.3rem;}
    p {font-size: 1.9rem;}
    body {max-width: 400px; margin:0px auto; padding-bottom: 25px;}
    .slider2 { -webkit-appearance: none; margin: 14px; width: 360px; height: 25px; background: #FFD65C;
     outline: none; -webkit-transition: .2s; transition: opacity .2s;}
    .slider2::-webkit-slider-thumb {-webkit-appearance: none; appearance: none; width: 35px; height: 35px; background: #003249; cursor: pointer;}
    .slider2::-moz-range-thumb { width: 35px; height: 35px; background: #003249; cursor: pointer; } 

    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
    
	.units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
	
	
  </style>
</head>
<body>

  <h2>IoT LivingRoom </h2>
<!-- Hiển thị nhiệt độ và độ ẩm -->
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temperature</span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">Humidity</span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units">&percnt;</sup>
  </p>
<!-- Hiển thị thanh trượt điều khiển quạt -->
  <h3>FAN SPEED</h3>
  <p><span id="textSliderValue">%SLIDERVALUE%</span></p>
  <p><input type="range" onchange="updateSliderPWM(this)" id="pwmSlider" min="0" max="9" value="%SLIDERVALUE%" step="1" class="slider2"></p>
<!-- Khi thanh ghi thay đổi thì lấy giá trị và hiển thị lên Web bằng 
cách tạo yêu cầu get -->
  <script>
  function updateSliderPWM(element) {
  var sliderValue = document.getElementById("pwmSlider").value;
  document.getElementById("textSliderValue").innerHTML = sliderValue;
  console.log(sliderValue);
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/slider?value="+sliderValue, true);
  xhr.send();
}
</script>
<!-- Hiển thị nút nhấn điều khiển đèn, cập nhật trạng thái đèn vào biến state mỗi 
     giây bằng yêu cầu get -->  
    %BUTTONPLACEHOLDER%
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?state=1", true); }
  else { xhr.open("GET", "/update?state=0", true); }
  xhr.send();
}


<!-- Hàm cập nhật trạng thái đèn sau 1s -->
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var inputChecked;
      var outputStateM;
      if( this.responseText == 1){ 
        inputChecked = true;
        outputStateM = "On";
      }
      else { 
        inputChecked = false;
        outputStateM = "Off";
      }
      document.getElementById("output").checked = inputChecked;
      document.getElementById("outputState").innerHTML = outputStateM;
    }
  };
  xhttp.open("GET", "/state", true);
  xhttp.send();
}, 1000 ) ;
</script>

</body>
<!-- Hàm cập nhật nhiệt độ, độ ẩm sau 10s -->
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 5000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 5000 ) ;
</script>

</html>
)rawliteral";

// Đọc trạng thái đèn để hiện thị lên Web Server
String outputState(){
  if(digitalRead(output1)){
    return "checked";
  }
  else {
    return "";
  }
  return "";
}

// Thay đổi giá trị các khoảng trắng
String processor(const String& var){
// Thanh ghi
  if (var == "SLIDERVALUE"){
    return sliderValue;
  }
// Nút nhấn
  if(var == "BUTTONPLACEHOLDER"){
    String buttons ="";
    String outputStateValue = outputState();
    buttons+= "<h4>LIGHT BULB - State <span id=\"outputState\"></span></h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + outputStateValue + "><span class=\"slider\"></span></label>";
    return buttons;
  }
// Nhiệt độ, độ ẩm 
  if(var == "TEMPERATURE"){
    return readDHTTemperature();
  }
  else if(var == "HUMIDITY"){
    return readDHTHumidity();
  }
  
  return String();
}


void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  //khởi tạo DHT11
  dht.begin();
  // Khởi tạo các chân 
  pinMode(output1, OUTPUT);
  digitalWrite(output1, LOW);
  pinMode(buttonPin, INPUT_PULLUP);
  
  // configure LED PWM functionalitites
  ledcSetup(ledChannel, freq, resolution);
  
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(output, ledChannel);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  
  ledcWrite(ledChannel, sliderValue.toInt());

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

// Khởi tạo Web Server
// Khi nhận được yêu cầu trên thư mục gốc / URL, Web Server sẽ gửi trang HTML cũng 
// như bộ xử lý processor để cập nhật dữ liệu hiển thị lên Web.
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

// Ví dụ như nhận được hàm get từ /URL: /slider, ESP32 sẽ lấy giá trị thanh trượt 
// để điều khiển quạt
  // Send a GET request to <ESP_IP>/slider?value=<inputMessage>
  server.on("/slider", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      sliderValue = inputMessage;
	  data = map(sliderValue.toInt(),0,9,0,255);
      ledcWrite(ledChannel,data);
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });

  // Send a GET request to <ESP_IP>/update?state=<inputMessage>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/update?state=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
	  
      digitalWrite(output1,inputMessage.toInt());
      ledState = !ledState;
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });
// Đọc trạng thái đèn và hiển thị lên web
  // Send a GET request to <ESP_IP>/state
  server.on("/state", HTTP_GET, [] (AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(digitalRead(output1)).c_str());
  });
  
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readDHTTemperature().c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readDHTHumidity().c_str());
  });

  // Start server
  server.begin();
}
  
void loop() {
// Nút nhấn cứng và đọc trạng thái nút nhấn
  int reading = digitalRead(buttonPin);
// Hàm xử lý chống dội phím
  if (reading != lastButtonState) {
// bắt đầu đếm 50ms (debounceDelay = 50)
    lastDebounceTime = millis();
  }
/* Khi đủ 50ms thì kiểm tra lại, nếu trạng thái nút nhấn vẫn thay đổi so với trạng 
 thái trước đó (buttonState) thì mới xác định có nhấn phím và đổi trạng thái đèn (ledState) */
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;
      // only toggle the LED if the new button state is HIGH
      if (buttonState == HIGH) {
        ledState = !ledState;
      }
    }
  }
// Điều khiển nút nhấn
  digitalWrite(output1, ledState);
// Lưu lại giá trị nút nhấn hiện tại
  lastButtonState = reading;
}
