#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Servo.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>


// Replace with your network credentials
const char* ssid = "dgubo";
const char* password = "123456789";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

Servo servo;
int angle = 10;
int greenLED =5;
int redLED = 0;
int inputPin = 4;               // choose the input pin (for PIR sensor)
int pirState = LOW;             // we start, assuming no motion detected
int val = 0;
int count = 0;
int dhtPin = 14;
DHT dht(dhtPin, DHT11);
float temp=0.0;
unsigned long previousMillis = 0; 
const long interval = 10000;



const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
<title>Nasa mala web-klinika</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" href="data:,">
</head>
<body>
  <div class="topnav">
    <img src="https:i.imgur.com/wEjPdeO.jpeg" alt="">
    <h1>Nasa mala web-klinika</h1>
  </div>
  <div class="content">
    <div class="card">
      <h2>Kontrolne poruke</h2>
      <div class="state" id="table">
      </div>
    </div>
  </div>
<script>
  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  window.addEventListener('load', onLoad);
  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; // <-- add this line
  }
  function onOpen(event) {
    console.log('Connection opened');
  }
  function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
  }
  function onMessage(event) {
    var state;
    let paragraph = document.createElement("p");
    if (event.data == "HITNO"){
      state=event.data;
      paragraph.style.color = "red";
    }
    
    function time(){
                const today = new Date();
                const date = today.getFullYear()+'-'+(today.getMonth()+1)+'-'+today.getDate();
                const time = today.getHours() + ":" + today.getMinutes() + ":" + today.getSeconds();
                const dateTime = '[ '+ date +' '+ time + ' ]';
                return dateTime;
            }
    
    paragraph.innerHTML=time()+" "+event.data;
    document.getElementById('table').prepend(paragraph);
  }
  function onLoad(event) {
    initWebSocket();
  }
</script>
<style>
    html {
      font-family: Arial, Helvetica, sans-serif;
      text-align: center;
    }
    h1 {
      font-size: 1.8rem;
      color: white;
    }
    h2{
      font-size: 1.5rem;
      font-weight: bold;
      color: #143642;
    }
    .topnav {
      overflow: hidden;
      background-color: #143642;
    }
    .topnav img{
        margin-top: 2rem;
        border-radius: 2.5rem;
        width: 11.5rem;
     .state {
       font-size: 1.05rem;
       color:#8c8c8c;
       font-weight: medium;
     }
    </style>
</body>
</html>
)rawliteral";


void notifyClients() {
  ws.textAll(String(1)); 
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
   //if (strcmp((char*)data, "VRACA") == 0 || strcmp((char*)data, "IZLAZI") == 0 ) {
      //ledState = !ledState;
      //notifyClients();
    //}
  }
}

// Client to Server side
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
    switch (type) {
      case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
      case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
      case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
      case WS_EVT_PONG:
      case WS_EVT_ERROR:
        break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

String processor(const String& var){

  if(var == "STATE"){
    if (val){
      return "1";
    }
    else{
      return "0";
    }
  }return String();
}

void setup(){
  pinMode(12, INPUT);
  servo.attach(2);
  servo.write(angle);
  pinMode(inputPin, INPUT);     // declare sensor as input
  pinMode(greenLED,OUTPUT);
  pinMode(redLED,OUTPUT);
  
  // Serial port for debugging purposes
  Serial.begin(115200);
  dht.begin();
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  initWebSocket();

  // Route for root / web page
 server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Start server
  server.begin();
}

void loop() {
  ws.cleanupClients();
  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you updated the DHT values
    previousMillis = currentMillis;
    // Read temperature as Celsius (the default)
    float newT = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    //float newT = dht.readTemperature(true);
    // if temperature read failed, don't change t value
    if (isnan(newT)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      temp = newT;
      Serial.println(temp);
      ws.textAll("Temperatura pacijenta je: "+String(temp)+" °C");
    }
  }

  // Button press - higher priority
  if(digitalRead(12)){
    ws.textAll("HITNO");
  }
  val = digitalRead(inputPin);
    if (val == HIGH)  // check if the input is HIGH
  {            
    
    digitalWrite(greenLED, HIGH);
    digitalWrite(redLED, LOW);
    if (pirState == LOW) 
  {
       
      for(angle = 10; angle < 180; angle++)  
      {                                  
        servo.write(angle);               
        delay(15);                   
      } 
      Serial.println("Motion detected!"); // print on output change
      pirState = HIGH;
      if(count==0){
          ws.textAll("Korisnik je izašao iz sobe");
          count=1;
      } 
      else if(count==1){
        ws.textAll("Korisnik se vratio u sobu");
        count=0;
      } 
      
      
    }
     digitalWrite(greenLED, LOW);
     delay(2000);
  } 
  else 
  {
    digitalWrite(redLED, HIGH);
    digitalWrite(greenLED, LOW);
    if (pirState == HIGH)
  {
    
    for(angle = 180; angle > 10; angle--)    
    {                                
      servo.write(angle);           
      delay(15);       
    } 
      Serial.println("Motion ended!");  // print on output change
      pirState = LOW;
    }
     digitalWrite(redLED, LOW);
  }
 // scan from 0 to 180 degrees
  //delay(2000);
  // now scan back from 180 to 0 degrees
  
}
