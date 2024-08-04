#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <WiegandNG.h>
#include "wiegandOutput.h"
#include <ArduinoJson.h>


#define D0_RECEIVE 23
#define D0_SEND 18
#define D1_RECEIVE 22
#define D1_SEND 19

#define BUTTON 21

WiegandNG wg;
WiegandOut wiegandOut(D0_SEND,D1_SEND,true);

int data = 0;
int buttonData = 0;


const char* ssid = "IOT";
const char* password = "risk3sixty";


WebServer server(80);


void setup() {
  
  Serial.begin(115200);
  
  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());


  // Setup server routes
  server.on("/", HTTP_GET, []() {
    File file = SPIFFS.open("/index.html", "r");
    server.streamFile(file, "text/html");
    file.close();
  });

  // Serve files from SPIFFS
  server.on("/spiffs", HTTP_GET, []() {
    File file = SPIFFS.open("/index.html", "r");
    server.streamFile(file, "text/html");
    file.close();
  });

  server.on("/style.css", HTTP_GET, []() {
    File file = SPIFFS.open("/style.css", "r");
    server.streamFile(file, "text/css");
    file.close();
  });

  server.on("/script.js", HTTP_GET, []() {
    File file = SPIFFS.open("/script.js", "r");
    server.streamFile(file, "text/javascript");
    file.close();
  });

  server.on("/data/data.txt", HTTP_GET, []() {
    File file = SPIFFS.open("/data/data.txt", "r");
    server.streamFile(file, "text/plain");
    file.close();
  });

  server.on("/api/setbutton", HTTP_POST, []() {
    if (server.hasArg("plain")) {
      String json = server.arg("plain");
      
      // Parse JSON
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, json);
      
      // Extract "data" field
      if (doc.containsKey("data")) {
        buttonData = doc["data"]; // Automatically converted to int
        
        Serial.print("Received data: ");
        Serial.println(buttonData);
        
        server.send(200, "text/plain", "JSON Data received");
      } else {
        server.send(400, "text/plain", "JSON does not contain 'data'");
      }
    } else {
      server.send(400, "text/plain", "Body not received");
    }
  });

  server.on("/api/replay", HTTP_POST, []() {
    if (server.hasArg("plain")) {
      String json = server.arg("plain");
      
      // Parse JSON
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, json);
      
      // Extract "data" field
      if (doc.containsKey("data")) {
        replayDataBinary(doc["data"]);
    
        server.send(200, "text/plain", "JSON Data received");
      } else {
        server.send(400, "text/plain", "JSON does not contain 'data'");
      }
    } else {
      server.send(400, "text/plain", "Body not received");
    }
  });

  // Start server
  server.begin();

  unsigned int wiegandbits = 48;
	unsigned int packetGap = 15;			// 25 ms between packet

  pinMode(BUTTON, INPUT_PULLUP); // Use internal pull-up resistor

	
	if(!wg.begin(D0_RECEIVE, D1_RECEIVE, wiegandbits, packetGap)) {
		Serial.println("Out of memory!");
	}
	Serial.println("Ready...");

  clearFile("/data/data.txt");

}


void loop() {
  server.handleClient();

  if(wg.available()) {
    if(wg.getBitCounted() < 8) {
      wg.clear();
    }
    else {
		wg.pause();			// pause Wiegand pin interrupts
		Serial.print("Bits=");
		Serial.println(wg.getBitCounted()); // display the number of bits counted
		Serial.print("RAW Binary=");
		PrintBinary(wg); // display raw data in binary form, raw data inclusive of PARITY
    data = GetWiegandData(wg);
    remove_msb_lsb(&data);
		wg.clear();	// compulsory to call clear() to enable interrupts for subsequent data
    appendLog();
	  }
  }

  if(digitalRead(BUTTON) == LOW) {
    // Debounce the button
    delay(500); // Simple debounce, adjust as needed
    if(digitalRead(BUTTON) == LOW) { // Check again to confirm the button is still pressed
      // Pause Wiegand to prevent detection of replay
      wg.pause();

      // Replay Wiegand data here
      replayData(buttonData); // Implement this function to replay your data
    
      // After replay, clear Wiegand to resume normal operation
      wg.clear();

    }
  }
}
void replayDataBinary(const char* binaryString) {
  int tempData = (int)strtol(binaryString, NULL, 2);

  wiegandOut.send(tempData,26,true); // Send 26 bits with facility code

}
void clearFile(const char* path) {
  // Delete the file
  if (!SPIFFS.remove(path)) {
    Serial.println("Failed to delete file");
    return;
  }

  // Recreate the file
  File file = SPIFFS.open(path, "w");
  if (!file) {
    Serial.println("Failed to create file");
    return;
  }

  // Close the file
  file.close();
  Serial.println("File cleared");
}

void appendLog() {
  File file = SPIFFS.open("/data/data.txt", "a");
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }

  // Convert decimal data to a string
  String dataStr = String(data);

  // Convert data to binary string
  String binaryStr = "";
  for (int i = sizeof(data) * 8 - 1; i >= 0; i--) {
    binaryStr += (data & (1 << i)) ? "1" : "0";
  }
  // Optionally, trim leading zeros from the binary representation if desired
  //binaryStr.remove(0, binaryStr.indexOf('1'));

  // Calculate the number of bits in the binary representation
  int bits = binaryStr.length();

  // Append data in CSV format: decimal, binary, number of bits
  file.print(dataStr);
  file.print(",");
  file.print(binaryStr);
  file.print(",");
  file.println(bits);

  // Close the file
  file.close();
}


unsigned long GetWiegandData(WiegandNG &tempwg) {
    volatile unsigned char *buffer = tempwg.getRawData();
    unsigned int bufferSize = tempwg.getBufferSize();
    unsigned int countedBits = tempwg.getBitCounted();
    
    unsigned long result = 0; // To accumulate the bits into an unsigned long

    unsigned int countedBytes = (countedBits / 8);
    if ((countedBits % 8) > 0) countedBytes++;
    
    for (unsigned int i = bufferSize - countedBytes; i < bufferSize; i++) {
        unsigned char bufByte = buffer[i];
        for (int x = 0; x < 8; x++) {
            if ((((bufferSize - i) * 8) - x) <= countedBits) {
                result <<= 1; // Shift result to the left by 1 bit to make room for the new bit
                if ((bufByte & 0x80)) { // Check the most significant bit
                    result |= 1; // Set the least significant bit in result
                }
            }
            bufByte <<= 1; // Shift bufByte to get the next bit on the next iteration
        }
    }
    
    return result; // Return the accumulated binary data as an unsigned long
}

void PrintBinary(WiegandNG &tempwg) {
	volatile unsigned char *buffer=tempwg.getRawData();
	unsigned int bufferSize = tempwg.getBufferSize();
	unsigned int countedBits = tempwg.getBitCounted();

	unsigned int countedBytes = (countedBits/8);
	if ((countedBits % 8)>0) countedBytes++;
	
	for (unsigned int i=bufferSize-countedBytes; i< bufferSize;i++) {
		unsigned char bufByte=buffer[i];
		for(int x=0; x<8;x++) {
			if ( (((bufferSize-i) *8)-x) <= countedBits) {
				if((bufByte & 0x80)) {
					Serial.print("1");
				}
				else {
					Serial.print("0");
				}
			}
			bufByte<<=1;
		}
	}
	Serial.println();
}

void remove_msb_lsb(int *data) {
    unsigned long int mask = ~0UL; // All bits set
    mask ^= (1UL << (sizeof(unsigned long int) * 8 - 1)); // Clear MSB
    *data &= mask; // Apply mask to clear MSB in data
    *data >>= 1; // Remove LSB by shifting right
}

void replayData(int tempData) {
  wiegandOut.send(tempData,26,true); // Send 26 bits with facility code
}
