/** The MIT License (MIT)

Copyright (c) 2018 David Payne

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "MoonrakerClient.h"

MoonrakerClient::MoonrakerClient(String ApiKey, String server, int port, String user, String pass, boolean psu) {
  updatePrintClient(ApiKey, server, port, user, pass, psu);
}

void MoonrakerClient::updatePrintClient(String ApiKey, String server, int port, String user, String pass, boolean psu) {
  server.toCharArray(myServer, 100);
  myApiKey = ApiKey;
  myPort = port;
  encodedAuth = "";
  if (user != "") {
    String userpass = user + ":" + pass;
    base64 b64;
    encodedAuth = b64.encode(userpass, true);
  }
  pollPsu = psu;
}

boolean MoonrakerClient::validate() {
  boolean rtnValue = false;
  printerData.error = "";
  if (String(myServer) == "") {
    printerData.error += "Server address is required; ";
  }
  if (printerData.error == "") {
    rtnValue = true;
  }
  return rtnValue;
}

WiFiClient MoonrakerClient::getSubmitRequest(String apiGetData) {
  WiFiClient printClient;
  printClient.setTimeout(5000);

  Serial.println("Getting Moonraker Data via GET");
  Serial.println(apiGetData);
  result = "";
  if (printClient.connect(myServer, myPort)) {
    printClient.println(apiGetData);
    printClient.println("Host: " + String(myServer) + ":" + String(myPort));
    if (myApiKey != "") {
      printClient.println("X-Api-Key: " + myApiKey);
    }
    if (encodedAuth != "") {
      printClient.print("Authorization: ");
      printClient.println("Basic " + encodedAuth);
    }
    printClient.println("User-Agent: ArduinoWiFi/1.1");
    printClient.println("Connection: close");
    if (printClient.println() == 0) {
      Serial.println("Connection to " + String(myServer) + ":" + String(myPort) + " failed.");
      Serial.println();
      resetPrintData();
      printerData.error = "Connection to " + String(myServer) + ":" + String(myPort) + " failed.";
      return printClient;
    }
  }
  else {
    Serial.println("Connection to Moonraker failed: " + String(myServer) + ":" + String(myPort));
    Serial.println();
    resetPrintData();
    printerData.error = "Connection to Moonraker failed: " + String(myServer) + ":" + String(myPort);
    return printClient;
  }

  char status[32] = {0};
  printClient.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0 && strcmp(status, "HTTP/1.1 409 CONFLICT") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    printerData.state = "";
    printerData.error = "Response: " + String(status);
    return printClient;
  }

  char endOfHeaders[] = "\r\n\r\n";
  if (!printClient.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    printerData.error = "Invalid response from " + String(myServer) + ":" + String(myPort);
    printerData.state = "";
  }

  return printClient;
}

WiFiClient MoonrakerClient::getPostRequest(String apiPostData, String apiPostBody) {
  WiFiClient printClient;
  printClient.setTimeout(5000);

  Serial.println("Getting Moonraker Data via POST");
  Serial.println(apiPostData + " | " + apiPostBody);
  result = "";
  if (printClient.connect(myServer, myPort)) {
    printClient.println(apiPostData);
    printClient.println("Host: " + String(myServer) + ":" + String(myPort));
    printClient.println("Connection: close");
    if (myApiKey != "") {
      printClient.println("X-Api-Key: " + myApiKey);
    }
    if (encodedAuth != "") {
      printClient.print("Authorization: ");
      printClient.println("Basic " + encodedAuth);
    }
    printClient.println("User-Agent: ArduinoWiFi/1.1");
    printClient.println("Content-Type: application/json");
    printClient.print("Content-Length: ");
    printClient.println(apiPostBody.length());
    printClient.println();
    printClient.println(apiPostBody);
    if (printClient.println() == 0) {
      Serial.println("Connection to " + String(myServer) + ":" + String(myPort) + " failed.");
      Serial.println();
      resetPrintData();
      printerData.error = "Connection to " + String(myServer) + ":" + String(myPort) + " failed.";
      return printClient;
    }
  }
  else {
    Serial.println("Connection to Moonraker failed: " + String(myServer) + ":" + String(myPort));
    Serial.println();
    resetPrintData();
    printerData.error = "Connection to Moonraker failed: " + String(myServer) + ":" + String(myPort);
    return printClient;
  }

  char status[32] = {0};
  printClient.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0 && strcmp(status, "HTTP/1.1 409 CONFLICT") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    printerData.state = "";
    printerData.error = "Response: " + String(status);
    return printClient;
  }

  char endOfHeaders[] = "\r\n\r\n";
  if (!printClient.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    printerData.error = "Invalid response from " + String(myServer) + ":" + String(myPort);
    printerData.state = "";
  }

  return printClient;
}

void MoonrakerClient::getPrinterJobResults() {
  if (!validate()) {
    return;
  }

  String apiGetData = "GET /printer/objects/query?print_stats&virtual_sdcard&extruder&heater_bed HTTP/1.1";
  WiFiClient printClient = getSubmitRequest(apiGetData);
  if (printerData.error != "") {
    return;
  }

  const size_t bufferSize = 4096;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root = jsonBuffer.parseObject(printClient);
  if (!root.success()) {
    Serial.println("Moonraker Data Parsing failed: " + String(myServer) + ":" + String(myPort));
    printerData.error = "Moonraker Data Parsing failed: " + String(myServer) + ":" + String(myPort);
    printerData.state = "";
    printerData.isPrinting = false;
    return;
  }

  JsonObject& status = root["result"]["status"];
  JsonObject& printStats = status["print_stats"];
  JsonObject& virtualSd = status["virtual_sdcard"];
  JsonObject& extruder = status["extruder"];
  JsonObject& bed = status["heater_bed"];

  const char* rawState = printStats["state"];
  String moonrakerState = rawState ? String(rawState) : "";
  moonrakerState.toLowerCase();

  printerData.isPrinting = (moonrakerState == "printing");
  if (printerData.isPrinting) {
    printerData.state = "Printing";
  } else if (moonrakerState == "paused") {
    printerData.state = "Paused";
  } else if (moonrakerState == "error") {
    printerData.state = "Error";
  } else {
    printerData.state = "Operational";
  }

  const char* fileName = printStats["filename"];
  printerData.fileName = fileName ? String(fileName) : "";

  float progress = virtualSd["progress"] | 0.0;
  if (progress < 0.0) {
    progress = 0.0;
  }
  if (progress > 1.0) {
    progress = 1.0;
  }
  float completion = progress * 100.0;
  printerData.progressCompletion = String(completion, 1);

  long filePos = virtualSd["file_position"] | 0;
  printerData.progressFilepos = String(filePos);
  printerData.fileSize = "";

  long printTime = long(printStats["print_duration"] | 0.0);
  printerData.progressPrintTime = String(printTime);

  long printTimeLeft = 0;
  if (progress > 0.0f && printTime > 0) {
    float total = (float)printTime / progress;
    long estimatedLeft = long(total - printTime);
    if (estimatedLeft > 0) {
      printTimeLeft = estimatedLeft;
    }
  }
  printerData.progressPrintTimeLeft = String(printTimeLeft);

  long filamentUsed = long(printStats["filament_used"] | 0.0);
  printerData.filamentLength = String(filamentUsed);

  printerData.averagePrintTime = "";
  printerData.estimatedPrintTime = "";
  printerData.lastPrintTime = "";

  float toolTemp = extruder["temperature"] | 0.0;
  float toolTarget = extruder["target"] | 0.0;
  float bedTemp = bed["temperature"] | 0.0;
  float bedTarget = bed["target"] | 0.0;

  printerData.toolTemp = String(toolTemp, 1);
  printerData.toolTargetTemp = String(toolTarget, 1);
  printerData.bedTemp = String(bedTemp, 1);
  printerData.bedTargetTemp = String(bedTarget, 1);

  if (isOperational()) {
    Serial.println("Status: " + printerData.state);
  } else {
    Serial.println("Printer Not Operational");
  }

  if (isPrinting()) {
    Serial.println("Status: " + printerData.state + " " + printerData.fileName + "(" + getProgressCompletion() + "%)");
  }
}

void MoonrakerClient::getPrinterPsuState() {
  printerData.isPSUoff = false;
}

void MoonrakerClient::resetPrintData() {
  printerData.averagePrintTime = "";
  printerData.estimatedPrintTime = "";
  printerData.fileName = "";
  printerData.fileSize = "";
  printerData.lastPrintTime = "";
  printerData.progressCompletion = "";
  printerData.progressFilepos = "";
  printerData.progressPrintTime = "";
  printerData.progressPrintTimeLeft = "";
  printerData.state = "";
  printerData.toolTemp = "";
  printerData.toolTargetTemp = "";
  printerData.filamentLength = "";
  printerData.bedTemp = "";
  printerData.bedTargetTemp = "";
  printerData.isPrinting = false;
  printerData.isPSUoff = false;
  printerData.error = "";
}

String MoonrakerClient::getAveragePrintTime(){
  return printerData.averagePrintTime;
}

String MoonrakerClient::getEstimatedPrintTime() {
  return printerData.estimatedPrintTime;
}

String MoonrakerClient::getFileName() {
  return printerData.fileName;
}

String MoonrakerClient::getFileSize() {
  return printerData.fileSize;
}

String MoonrakerClient::getLastPrintTime(){
  return printerData.lastPrintTime;
}

String MoonrakerClient::getProgressCompletion() {
  return String(printerData.progressCompletion.toInt());
}

String MoonrakerClient::getProgressFilepos() {
  return printerData.progressFilepos;
}

String MoonrakerClient::getProgressPrintTime() {
  return printerData.progressPrintTime;
}

String MoonrakerClient::getProgressPrintTimeLeft() {
  String rtnValue = printerData.progressPrintTimeLeft;
  if (getProgressCompletion() == "100") {
    rtnValue = "0";
  }
  return rtnValue;
}

String MoonrakerClient::getState() {
  return printerData.state;
}

boolean MoonrakerClient::isPrinting() {
  return printerData.isPrinting;
}

boolean MoonrakerClient::isPSUoff() {
  return printerData.isPSUoff;
}

boolean MoonrakerClient::isOperational() {
  boolean operational = false;
  if (printerData.state == "Operational" || printerData.state == "Paused" || isPrinting()) {
    operational = true;
  }
  return operational;
}

String MoonrakerClient::getTempBedActual() {
  return printerData.bedTemp;
}

String MoonrakerClient::getTempBedTarget() {
  return printerData.bedTargetTemp;
}

String MoonrakerClient::getTempToolActual() {
  return printerData.toolTemp;
}

String MoonrakerClient::getTempToolTarget() {
  return printerData.toolTargetTemp;
}

String MoonrakerClient::getFilamentLength() {
  return printerData.filamentLength;
}

String MoonrakerClient::getError() {
  return printerData.error;
}

String MoonrakerClient::getValueRounded(String value) {
  float f = value.toFloat();
  int rounded = (int)(f + 0.5f);
  return String(rounded);
}

String MoonrakerClient::getPrinterType() {
  return printerType;
}

int MoonrakerClient::getPrinterPort() {
  return myPort;
}

String MoonrakerClient::getPrinterName() {
  return printerData.printerName;
}

void MoonrakerClient::setPrinterName(String printer) {
  printerData.printerName = printer;
}
