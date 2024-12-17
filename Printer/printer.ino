#include "Adafruit_Thermal.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define TX_PIN 17
#define RX_PIN 16
const char* ssid = "Your_SSID";
const char* password = "Your_password";
const char* apiBillUrl = "Your_api";

HardwareSerial mySerial(1);
Adafruit_Thermal printer(&mySerial);

void setup() {
  mySerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  printer.begin();
  Serial.begin(115200);
  printer.println(F(" "));
  printer.println(F(" "));

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  fetchDataFromAPI(apiBillUrl, "bill");
}

void loop() {
  delay(5000);
  fetchDataFromAPI(apiBillUrl, "bill");
  Serial.println("finding bill");
}

void fetchDataFromAPI(const char* url, const String& type) {
  HTTPClient http;
  http.begin(url);
  
  int httpCode = http.GET();

  // Print the response code and headers
  Serial.printf("HTTP GET response code: %d\n", httpCode);
  String headers = http.header("Location");  // Get the Location header
  Serial.println("Location Header: " + headers);  // Debugging the location URL

  if (httpCode == 302) {  // Checking for 302 HTTP redirect status code
    if (headers.length() > 0) {
      Serial.println("Redirecting to: " + headers);
      // Make a new GET request to the redirected URL
      http.end();
      http.begin(headers);
      httpCode = http.GET();
    } else {
      Serial.println("No Location header found.");
    }
  }

  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println("Received JSON Response:");
      Serial.println(payload);  // Print the full response from the API
      parseBillJSON(payload);   // Parse the JSON data
    } else {
      Serial.printf("HTTP request failed with code: %d\n", httpCode);
    }
  } else {
    Serial.printf("HTTP GET request failed, code: %d\n", httpCode);
  }

  http.end();
}

void parseBillJSON(const String& jsonString) {
  // Create a JSON document to hold the parsed data
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, jsonString);
  if (error) {
    Serial.println(F("Failed to parse JSON"));
    return;
  }

  // Extract the main details
  String _id = doc["transaction"]["id"].as<String>();
  String customerName =  doc["transaction"]["customerName"].as<String>();  // You may want to get the customer name if available in the JSON
  float totalAmount = doc["total_selling_price"].as<float>();
  String paymentMethod = doc["payment_method"].as<String>();
  String date =  doc["transaction"]["date"].as<String>();  // You might want to get the actual date from the JSON if available
  String store_name = doc["store"]["store_name"].as<String>();
  String store_address = doc["store"]["store_address"].as<String>();  // You might want to get the actual date from the JSON if available

  // Print transaction information
      Serial.println("id: " + _id);
    Serial.println("Customer Name: " + customerName);
    Serial.println("Amount: " + String(totalAmount));
    Serial.println("Payment Method: " + paymentMethod);
    Serial.println("Date: " + date);


    printer.setSize('M'); 
    printer.justify('C');
    printer.boldOn();    
    printer.println(store_name);
    printer.boldOff();
    printer.setSize('S');
    printer.println(store_address);
    printer.println(F(" "));
    printer.setSize('M');
    printer.boldOn();
    printer.println(F("INVOICE"));
    printer.boldOff();
    printer.justify('C');
    printer.setSize('S');
    printer.println("Date: " + date);
    printer.justify('C');
    printer.setSize('S');
    printer.println("Bill ID: " + _id);
    printer.println("Customer : " + customerName);
    printer.setSize('S');
    printer.println("------------------------------");
    printer.boldOn();
    printer.println("Item        Qty      Amount");
    printer.boldOff();
    Serial.println("Item        Qty      Amount");
    printer.println("------------------------------");

    for (const auto& item : doc["products"].as<JsonArray>()) {
    String itemName = item["name"].as<String>();
    int quantity = item["quantity"].as<int>();
    float price = item["price"].as<float>();

    // Format the product line for printing
    String itemLine = formatLine(itemName, quantity, price);
    Serial.println(itemLine);
    printer.println(itemLine);
  }
    printer.println(F("------------------------------"));
    String totalLine = "              Total: " + String(totalAmount, 2);
    Serial.println("------------------------------");
    printer.boldOn();
    Serial.println(totalLine);
    Serial.println("------------------------------");
    printer.println(totalLine);
    printer.println("------------------------------");
    printer.println(F(" "));
    printer.justify('C');
    Serial.println("Thank you for shopping with us!");
    Serial.println("Visit Again!");
    printer.println("Thank you for shopping with us!");
    printer.println("Visit Again!");
    printer.boldOff();
    printer.justify('L');
    printer.println(F("*Item exchange can be done within 24 hours, Bill is Mandatory."));
    printer.println(F(" "));
    printer.println(F(" "));
    printer.println(F(" "));
    printer.println(F(" "));
}


String formatLine(const String& itemName, int quantity, float price) {
  // Define column widths for 58 mm paper
  const int itemWidth =15 ;   // Adjusted width for Item column
  const int quantityWidth = 5; // Adjusted width for Quantity column   // Adjusted width for Price column
  const int amountWidth = 8;   // Adjusted width for Amount column

  // Create formatted line with padding
  String line = itemName;
  
  // Add padding to item column
  int itemPadding = itemWidth - itemName.length();
  for (int i = 0; i < itemPadding; ++i) {
    line += ' ';
  }
  
  // Add quantity and padding to quantity column
  line += String(quantity);
  int quantityPadding = quantityWidth - String(quantity).length();
  for (int i = 0; i < quantityPadding; ++i) {
    line += ' ';
  }
  
  // Calculate and add total amount with padding
  float amount = price * quantity;
  line += String(amount, 2);
  int amountPadding = amountWidth - String(amount, 2).length();
  for (int i = 0; i < amountPadding; ++i) {
    line += ' ';
  }

  return line;
}