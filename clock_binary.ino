#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESP8266WebServer.h>

const char* ssid = "YOUR-WIFI-SSID";
const char* password = "YOUR-WIFI-PASSWORD";

// Set GPIO pins for LEDs
const int hour_led_pins[] = { D6, D7, D8 };             
const int minute_led_pins[] = { D5, D4, D3, D2, D1, D0 }; 

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pk.pool.ntp.org");

ESP8266WebServer server(80);

void display_binary(int number, const int* led_pins, int num_leds, bool blink_zeros) {
  String binary_str = String(number, BIN);
  int leading_zeros = num_leds - binary_str.length();
  for (int i = 0; i < leading_zeros; i++) {
    if (blink_zeros || binary_str.charAt(i) == '1') {
      digitalWrite(led_pins[i], LOW);  // Turn off leading zeros if blink_zeros is true or it's '1'
    }
  }
  for (int i = leading_zeros; i < num_leds; i++) {
    digitalWrite(led_pins[i], binary_str.charAt(i - leading_zeros) == '1' ? HIGH : LOW);
  }
}

void turnOffHourLEDs() {
  for (int i = 0; i < sizeof(hour_led_pins) / sizeof(hour_led_pins[0]); i++) {
    digitalWrite(hour_led_pins[i], LOW);
  }
}

void print_time_to_serial() {
  Serial.print("Current Time: ");
  Serial.print(timeClient.getHours());
  Serial.print(":");
  Serial.print(timeClient.getMinutes());
  Serial.print(":");
  Serial.println(timeClient.getSeconds());
}

void handleRoot() {
  int hours = timeClient.getHours();
  int minutes = timeClient.getMinutes();

  int result_12h = (hours + 12) % 12;
  if (result_12h == 0) {
    result_12h = 12;
  }

  // Get the current time
  String currentTime = timeClient.getFormattedTime();

  // Generate the HTML response with CSS styles
  String html = "<html><head><title>ESP8266 Clock</title>";
  html += "<style>";
  html += "body {background-color: rgb(0, 0, 0);}";
  html += "h1 { color: rgb(255, 217, 1); text-align: center; font-family: 'Gill Sans', 'Gill Sans MT', Calibri, 'Trebuchet MS', sans-serif; }";
  html += "p { font-size: 18px; text-decoration: none; color: yellow; font-family: 'Lucida Sans', 'Lucida Sans Regular', 'Lucida Grande', 'Lucida Sans Unicode', Geneva, Verdana, sans-serif; }";
  html += "a { text-decoration: none; color: silver; font-family: cursive; font: bold; font-weight: bolder; font-size: larger; padding-top: 40px; margin-top: 20px; margin-bottom: 20px; text-align: left; }";
  html += "</style>";
  html += "</head><body>";
  html += "<h1>ESP8266 Clock</h1>";
  html += "<p>The current time is: " + currentTime + "</p>";
  html += "<p>12-Hour Format: " + String(result_12h) + "</p>";
  html += "<p>Minutes: " + String(minutes) + "</p>";
  html += "<p><a href=\"/restart\">Restart</a></p>";
  html += "</body></html>";

  // Send the HTML response to the client
  server.send(200, "text/html", html);
}




void handleRestart() {
  server.send(200, "text/plain", "Restarting...");
  ESP.restart();
}

void setup() {
  Serial.begin(115200);
  delay(100);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected to WiFi, IP address: ");
  Serial.println(WiFi.localIP());

  for (int pin : hour_led_pins) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }

  for (int pin : minute_led_pins) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }

  timeClient.begin();
  timeClient.setTimeOffset(-7 * 3600); // Set this according to your time zone currently set for pakistan GMT+5 time zone you.. you can see what time its showing using serial monitor
  timeClient.update();

  server.on("/", handleRoot);
  server.on("/restart", handleRestart);
  server.begin();
}

void loop() {
  timeClient.update();

  int hours = timeClient.getHours();
  int minutes = timeClient.getMinutes();

  int result_12h = (hours + 12) % 12; //logic for converting 24 hour format into 12 hour format
  if (result_12h == 0) {
    result_12h = 12;
  }

  if (result_12h > 7) { // logic for blinking when hour  is greater than 7
    result_12h -= 7;
    for (int i = 0; i < 5; i++) {
      display_binary(result_12h, hour_led_pins, sizeof(hour_led_pins) / sizeof(hour_led_pins[0]), true);
      display_binary(minutes, minute_led_pins, sizeof(minute_led_pins) / sizeof(minute_led_pins[0]), false);
      delay(50);
      display_binary(0, hour_led_pins, sizeof(hour_led_pins) / sizeof(hour_led_pins[0]), true);
    }
  } else {
    display_binary(result_12h, hour_led_pins, sizeof(hour_led_pins) / sizeof(hour_led_pins[0]), false);
    display_binary(minutes, minute_led_pins, sizeof(minute_led_pins) / sizeof(minute_led_pins[0]), false);
  }

  print_time_to_serial();

  server.handleClient();
  delay(600);
}
