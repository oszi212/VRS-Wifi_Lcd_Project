#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h> 
#include <SoftwareSerial.h>
#include <FS.h>   // Include the SPIFFS library
#include <EasyButton.h>

ESP8266WiFiMulti wifiMulti;       // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'
ESP8266WebServer server(80);      // Create a webserver object that listens for HTTP request on port 80
SoftwareSerial usart(D6, D5);     // Enabling USART communication on RX, TX pinouts


String nodeJs_IP = "10.0.0.139";  // this will be updated if client connected

String imgBuffer="";      // stores img buffer, string of each pixels included their rgb values
uint16_t index_ = 0;      // iterator for pixel from buffer

String getContentType(String filename); // convert the file extension to the MIME type
bool handleFileRead(String path);       // send the right file to the client (if it exists)

bool imgReceived = false; 
uint32_t counter = 0;

EasyButton button(0);       // attaching flash button

void setup() {
 // Serial.setDebugOutput(true);
 
  Serial.begin(115200);         // Start the Serial communication between board and the computer
  delay(10);
  usart.begin(9600);            // communication with another board, through USART
  delay(10);

  wifiConfig();
  serverConfig();
}

void wifiConfig(){
  // add Wi-Fi networks you want to connect to
  wifiMulti.addAP("motorolaone4879", "aaaabbbb");
  wifiMulti.addAP("DESKTOP-5TRECRQ", "aaaabbbb");
  wifiMulti.addAP("HOMENET", "*bogi*csalad*");   

  Serial.println("Connecting to WiFi network ...");
 
  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(250);
    Serial.print('.');
  }
  Serial.println('\n');
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());              // Tell us what network we're connected to
  Serial.print("IP address:\t");
  Serial.print(WiFi.localIP());             // Send the IP address of the ESP8266 to the computer
  Serial.println(" ... [OK]");

  if (MDNS.begin("esp8266")) {              // Start the mDNS responder for esp8266.local
    Serial.println("mDNS responder started ... [OK]");
  } 
  else {
    Serial.println("Error setting up MDNS responder! ... [ERRROR]");
  }
}

void handleNotFound()
{
    if (server.method() == HTTP_OPTIONS)
    {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Access-Control-Max-Age", "10000");
        server.sendHeader("Access-Control-Allow-Methods", "PUT,POST,GET,OPTIONS");
        server.sendHeader("Access-Control-Allow-Headers", "*");
        server.send(204);
    }
    else
    {
        server.send(404, "text/plain", "");
    }
}

void serverConfig(){
  SPIFFS.begin();         // Start the SPI Flash File System

  server.on("/espIP", HTTP_POST, [](){
      server.sendHeader("Access-Control-Allow-Origin", "*");
      if(server.hasArg("plain")){
        nodeJs_IP = server.arg("plain");
        nodeJs_IP.replace("\"data\":", "");
        nodeJs_IP.replace("{","");
        nodeJs_IP.replace("}","");
        nodeJs_IP.replace("\"","");
    
        Serial.print("NodeJS IP set: ");
        Serial.println(nodeJs_IP);
        server.send(200,"text/html","NodeJS_IP set [esp]");
      }
      else{
        server.send(200,"text/html","NodeJS_IP not set [esp]");
      }
  });
  server.on("/displayImg", HTTP_POST, displayScreenshot);
  server.on("/displayImgPart", HTTP_POST, displayScreenshotPart);
  
  server.onNotFound(handleNotFound);
  
  server.begin();                           // Actually start the server
  Serial.println("HTTP server started ... [OK]");
}

void displayNextPixel(){   // to lcd
  
  if(index_ < imgBuffer.length()){
    uint8_t count = 0;
    String dataTx="";
    // one pixel has format R;G;B;
    while(count != 3){    // counting delimeters ';'
        dataTx +=imgBuffer[index_];
        
        if(imgBuffer[index_] == ';'){
          count++;
        }
        if(count == 3){
          int j=0;
          while(j < dataTx.length()){
            usart.write(dataTx[j]);
            delay(1);
            j++;
          }
          index_++;      // for next calling
          return;
       }   
       index_++;
    }
  }
}
void displayScreenshot(){
  Serial.println("Getting screenshot ...");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  
  if(server.hasArg("plain")){
    imgBuffer = server.arg("plain");
    imgBuffer.replace("\"data\"", "");
    imgBuffer.replace("{","");
    imgBuffer.replace("}","");
    imgBuffer.replace(":","");
    imgBuffer.replace(",","");
    imgBuffer.replace("[","");
    imgBuffer.replace("]","");
    imgBuffer.replace("\"","");

    Serial.println("Got "+String(imgBuffer.length())+" characters.");
    index_ = 0;
    usart.write('!');
    delay(5000);
    usart.write('>');
    imgReceived = true;
    
    Serial.println("Displaying screenshot ...");
    server.send(200,"text/plain", "Displaying screenshot ...");
  }
  else{
    Serial.println("Empty buffer got");
    server.send(200,"text/plain", "Empty buffer got");
  }
}

void displayScreenshotPart(){
  Serial.println("Displaying next part ...");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  if(server.hasArg("plain")){
    imgBuffer = server.arg("plain");

    imgBuffer.replace("\"data\"", "");
    imgBuffer.replace("{","");
    imgBuffer.replace("}","");
    imgBuffer.replace(":","");
    imgBuffer.replace(",","");
    imgBuffer.replace("[","");
    imgBuffer.replace("]","");
    imgBuffer.replace("\"","");

    Serial.println("Got "+String(imgBuffer.length())+" characters.");
    index_ = 0;
  //  delay(50);
    imgReceived = true;  
    Serial.println("Imagepart got!");
    server.send(200,"text/plain", "Imagepart got!");
  }
  else{
    Serial.println("Empty buffer got");
    server.send(200,"text/plain", "Empty buffer got");
  }
}

void requestNextPart(){
  Serial.println("Requesting next part ...");
  Serial.println(counter);
  counter= counter+1;

  HTTPClient http;  //Declare an object of class HTTPClient 
  http.begin("http://10.0.0.139:3000/sendNextPartOfImg");  //Specify request destination
  int httpCode = http.GET();                                                                  //Send the request
   
  if (httpCode > 0) { //Check the returning code
    String payload = http.getString();   //Get the request response payload
    Serial.println(payload);                     //Print the response payload
    Serial.println("Request send");
  }
  http.end();   //Close connection
}

void loop(void) {
 // webSocket.loop();
  server.handleClient();
  delay(20);  //10

  if(imgReceived == true){
    if(index_ < imgBuffer.length()){
      displayNextPixel();
      if(index_ == imgBuffer.length()){
          Serial.println("Pixels drawn");
          imgReceived = false;
          requestNextPart(); 
      }
    }
  }
  if(button.read() == 1){   // if flash button pressed
    Serial.println("Flash button pressed");
    usart.write('!');              // set command to STM32 for clearing display 
  }
}
