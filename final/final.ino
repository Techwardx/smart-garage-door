// bot token: 8343027359:AAFxhpiXDij08O2-qfY6C7mgwKMbfmmZiCg
// my own chat ID: 8230113959
// group chat ID: -1003519840912
// https://api.telegram.org/bot8343027359:AAFxhpiXDij08O2-qfY6C7mgwKMbfmmZiCg/sendMessage?chat_id=-1003519840912&text=测试静默&disable_notification=true

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <time.h>
#include <ArduinoOTA.h>
// #include "soc/soc.h"
// #include "soc/rtc_cntl_reg.h"

// WiFi credentials
const char* ssid = "Horus@HOME";
const char* password = "dhzy1308";

// Telegram Bot Token and Chat ID
#define BOT_TOKEN "8343027359:AAFxhpiXDij08O2-qfY6C7mgwKMbfmmZiCg"
#define CHAT_ID "-1003519840912"

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// Sensor configuration
const int SENSOR_PIN = 5;
bool sensorState = false;
bool lastSensorState = false;

// RTC memory to preserve state across deep sleep
RTC_DATA_ATTR bool rtc_lastSensorState = false;
RTC_DATA_ATTR unsigned long rtc_lastAlertTime = 0;
RTC_DATA_ATTR int rtc_lastMessageReceived = 0;

// Timing
unsigned long lastCheckTime = 0;
unsigned long lastBotCheckTime = 0;
const long checkInterval = 1000;
const long botCheckInterval = 1000;

// Alert interval for 9PM-midnight period
const long alertInterval = 600000; // 10 minutes

// Time config
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -28800;
const int daylightOffset_sec = 0;

// Time ranges
const int sleepStartHour = 0;   // Midnight
const int sleepEndHour = 19;    // 7 PM
const int activeStartHour = 18; // 7 PM
const int alertStartHour = 20;  // 8 PM
const int alertEndHour = 24;    // Midnight

// Deep sleep duration
const uint64_t sleepDuration = 3600000000; // 1 hour in microseconds

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Configure wake-up timer
  esp_sleep_enable_timer_wakeup(sleepDuration);
  
  // Setup sensor pin
  pinMode(SENSOR_PIN, INPUT);
  
  // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
    delay(500);
    Serial.print(".");
    wifiAttempts++;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi failed, going to sleep...");
    goToSleep();
    return;
  }
  
  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Signal strength (RSSI): ");
  Serial.println(WiFi.RSSI());
  Serial.println("");

  // Set OTA
  ArduinoOTA.setHostname("ESP32-Sensor");
  ArduinoOTA.setPassword("1234");
  
  ArduinoOTA.onStart([]() {
    Serial.println("OTA Update Starting...");
  });

  ArduinoOTA.onEnd([](){
    Serial.println("\nOTA Update Complete!");
  });

  ArduinoOTA.onError([](ota_error_t error){
    Serial.printf("OTA Error[%u]: ", error);
  });

  ArduinoOTA.begin();

  // Configure time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Waiting for time sync...");
  
  // Time sync with retry
  int retries = 0;
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo) && retries < 10) {
    delay(1000);
    Serial.print(".");
    retries++;
  }
  
  if (retries < 10) {
    Serial.println("\nTime synced!");
    printLocalTime();
  } else {
    Serial.println("\nTime sync failed, continuing...");
  }
  
  // Configure SSL
  client.setInsecure();
  
  // Auto-skip old messages on first boot (when RTC memory is 0)
  if (rtc_lastMessageReceived == 0) {
    bot.getUpdates(bot.last_message_received + 1);
    rtc_lastMessageReceived = bot.last_message_received;
    Serial.println("First boot - skipped all old messages");
  }

  // Restore last message offset from RTC memory
  bot.last_message_received = rtc_lastMessageReceived;

  // Restore last state from RTC memory
  lastSensorState = rtc_lastSensorState;
  
  // Check if it should be in deep sleep
  if (shouldBeAsleep()) {
    Serial.println("Currently in sleep hours (midnight-7PM)");
    
    // Check messages and process them
    checkMessages();
    delay(5000);
    
    goToSleep(); // Send wake time message
    return;
  }
  
  // Send startup message (only during active hours)
  bot.sendMessage(CHAT_ID, "🟢 ESP32 Hall Sensor is online!", "");
  
  // Read initial sensor state
  sensorState = digitalRead(SENSOR_PIN);
  lastSensorState = sensorState;
  rtc_lastSensorState = sensorState;
  
  bot.sendMessage(CHAT_ID, "Current state: " + String(sensorState ? "🚪 CLOSED" : "🚪❗ OPEN"), "");
  Serial.println("Startup complete!");
}

void loop() {
  unsigned long currentTime = millis();
  
  ArduinoOTA.handle();

  // Check if it should go to sleep
  if (shouldBeAsleep()) {
    Serial.println("Entering sleep hours...");
    bot.sendMessage(CHAT_ID, "Entering sleep hours...", "");
    goToSleep();
    return;
  }
  
  // Check for incoming Telegram messages
  if (currentTime - lastBotCheckTime >= botCheckInterval) {
    lastBotCheckTime = currentTime;
    checkMessages();
  }

  // Check sensor
  if (currentTime - lastCheckTime >= checkInterval) {
    lastCheckTime = currentTime;
    sensorState = digitalRead(SENSOR_PIN);
    
    // Check if state changed
    if (sensorState != lastSensorState) {
      String message = "🚨 Door Status Changed!\n\n";
      message += "New State: ";
      message += sensorState ? "🚪 CLOSED" : "🚪❗ OPEN";
      message += "\nTime: ";
      message += getTimeString();
      
      if (bot.sendMessage(CHAT_ID, message, "")) {
        Serial.println("Change notification sent");
      }
      
      lastSensorState = sensorState;
      rtc_lastSensorState = sensorState;
      rtc_lastAlertTime = currentTime; // Reset alert timer
    }
    
    // Continuous alerts during 9PM-midnight if door is open
    if (isInAlertPeriod() && sensorState == false) {
      if (currentTime - rtc_lastAlertTime >= alertInterval) {
        String alertMsg = "⚠️ REMINDER: Door is still OPEN!\n";
        alertMsg += "Time: " + getTimeString();
        alertMsg += "\nDuration: " + String((currentTime - rtc_lastAlertTime) / 60000) + " min";
        
        if (bot.sendMessage(CHAT_ID, alertMsg, "")) {
          Serial.println("Reminder sent - door still open");
        }
        rtc_lastAlertTime = currentTime;
      }
    }
  }
}

// Check if in alert period (8PM-midnight)
bool isInAlertPeriod() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return false;
  }
  
  int currentHour = timeinfo.tm_hour;
  return (currentHour >= alertStartHour && currentHour < alertEndHour);
}

// Check if should be sleeping (midnight-7PM)
bool shouldBeAsleep() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return false; // If can't get time, stay awake
  }
  
  int currentHour = timeinfo.tm_hour;
  // Sleep from midnight to 7PM
  return (currentHour >= sleepStartHour && currentHour < sleepEndHour);
}

// Enter deep sleep
void goToSleep() {
  // Calculate next wake time
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    // Add 1 hour to current time
    time_t now = mktime(&timeinfo);
    now += 3600; // Add 1 hour (3600 seconds)
    struct tm* wakeTime = localtime(&now);
    
    char wakeStr[32];
    strftime(wakeStr, sizeof(wakeStr), "%I:%M %p", wakeTime);
    
    String sleepMsg = "😴 Going to sleep...\n";
    sleepMsg += "Next wake: ";
    sleepMsg += String(wakeStr);
    
    // Send notification
    bot.sendMessage(CHAT_ID, sleepMsg, "");
  } else {
    // If can't get time, send simple message
    bot.sendMessage(CHAT_ID, "😴 Going to sleep for 1 hour...", "");
  }
  
  Serial.println("Going to deep sleep for 1 hour...");
  delay(2000); // Give time for message to send
  Serial.flush();
  esp_deep_sleep_start();
}

void checkMessages() {
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  
  // Save message offset to RTC memory after fetching
  rtc_lastMessageReceived = bot.last_message_received;

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    
    Serial.println("Received: " + text);
    
    if (chat_id == CHAT_ID) {
      
      if (text == "status" || text == "/status" || text == "Status") {
        String response = "📊 Current Door Status\n\n";
        response += "State: ";
        response += sensorState ? "🚪 CLOSED" : "🚪❗ OPEN";
        response += "\nTime: ";
        response += getTimeString();
        response += "\n\nActive: 7PM-Midnight";
        response += "\nAlerts: 9PM-Midnight (every 10 min if open)";
        response += "\nSleep: Midnight-7PM (checks messages hourly)";
        
        bot.sendMessage(chat_id, response, "");
      }
      else if (text == "/start" || text == "start" || text == "Start" || text == "help" || text == "/help" || text == "Help") {
        String welcome = "👋 ESP32 Hall Sensor Bot\n\n";
        welcome += "Commands:\n";
        welcome += "• status - Check door state\n";
        welcome += "• start/help - Show this instruction\n\n";
        welcome += "🕐 Active: 7PM-Midnight\n";
        welcome += "😴 Sleep: Midnight-7PM (tip: please send only ONE message per hour)\n";
        welcome += "⚠️ Extra alerts: 9PM-Midnight\n";
        
        bot.sendMessage(chat_id, welcome, "");
      }
      else {
        bot.sendMessage(chat_id, "❓ Unknown command. Try 'status' or 'help' for instructions", "");
      }
    }
  }
}

String getTimeString() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Time unavailable";
  }
  
  char timeStr[64];
  strftime(timeStr, sizeof(timeStr), "%I:%M:%S %p", &timeinfo);
  return String(timeStr);
}

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  
  Serial.print("Current time: ");
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}