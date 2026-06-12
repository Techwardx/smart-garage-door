
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <time.h>

// WiFi credentials
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// Telegram Bot Token and Chat ID
#define BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"
#define CHAT_ID "YOUR_CHAT_ID"

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// Sensor configuration
const int SENSOR_PIN = 5;
bool sensorState = false;
bool lastSensorState = false;

// Timing
unsigned long lastCheckTime = 0;
unsigned long lastBotCheckTime = 0;
const long checkInterval = 1000; // Check sensor every second
const long botCheckInterval = 1000; // Check for message every second

// Time config
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -28800; // Set time zone
const int daylightOffset_sec = 0;

// Notification time range
const int notifyStartHour = 19; // 7pm
const int notifyEndHour = 24; // 0pm

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Setup sensor pin
  pinMode(SENSOR_PIN, INPUT);
  
  // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Signal strength (RSSI): ");
  Serial.println(WiFi.RSSI());

  // Configure time from NTP server
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Waiting for time sync...");
  delay(8000);
  printLocalTime(); // Show current time
  
  // Configure SSL for Telegram
  client.setInsecure(); // For testing - see note below
  
  // Send startup message
  bot.sendMessage(CHAT_ID, "ESP32 Hall Sensor is online and ready!", "");
  bot.sendMessage(CHAT_ID, "Now sensor state is: " + String(sensorState ? "ON" : "OFF"), "");
  Serial.println("Startup message sent!");
  
  // Read initial sensor state
  lastSensorState = digitalRead(SENSOR_PIN);
  Serial.print("Sensor state: ");
  Serial.println(lastSensorState ? "ON" : "OFF");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Check for incoming Telegram messages
  if (currentTime - lastBotCheckTime >= botCheckInterval) {
    lastBotCheckTime = currentTime;
    checkMessages();
  }

  // Check sensor at regular intervals
  if (currentTime - lastCheckTime >= checkInterval) {
    lastCheckTime = currentTime;
    
    // Read sensor
    sensorState = digitalRead(SENSOR_PIN);
    
    // Check if state changed
    if (sensorState != lastSensorState) {
      if (isWithinNotificationTime()) {
        String message = "🚨 Sensor Status Changed!\n\n";
        message += "New State: ";
        message += sensorState ? "✅ ON" : "❌ OFF";
        message += "\nTime: "; // Include timestamp
        message += getTimeString(); 
      
        // Send notification
        if (bot.sendMessage(CHAT_ID, message, "")) {
          Serial.println("Notification sent: " + message);
        } else {
          Serial.println("Failed to send notification");
        }
      }
      // Update last state
      lastSensorState = sensorState;
    }
  }
}

// Check for incoming Telegram messages and responds to commands
void checkMessages() {
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    
    Serial.println("Received: " + text);
    
    // Check if message is from authorized user
    if (chat_id == CHAT_ID) {
      text.toLowerCase(); // Make command case-insensitive
      
      if (text == "status" || text == "/status") { // Handle "status" command
        // Send current sensor status
        String response = "📊 Current Sensor Status\n\n";
        response += "State: ";
        response += sensorState ? "✅ ON" : "❌ OFF";
        response += "\nTime: ";
        response += getTimeString();
        response += "\n\nNotification Hours: 7 PM - Midnight";
        
        bot.sendMessage(chat_id, response, "");
      }
      else if (text == "/start" || text == "start") { // Handle "/start" command
        String welcome = "👋 Welcome to ESP32 Sensor Bot!\n\n";
        welcome += "Commands:\n";
        welcome += "• status - Check current sensor state\n";
        welcome += "• start - Show this help message\n\n";
        welcome += "Notifications are sent from 7 PM to midnight.";
        
        bot.sendMessage(chat_id, welcome, "");
      }
      else { // Handle unknown commands
        bot.sendMessage(chat_id, "❓ Unknown command. Send 'status' to check sensor.", "");
      }
    }
  }
}

// Check if current time is within notification hours
bool isWithinNotificationTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return true; // If can't get time, send notifications anyway
  }
  
  int currentHour = timeinfo.tm_hour;
  
  // Check if between 7 PM (19:00) and midnight (24:00)
  return (currentHour >= notifyStartHour && currentHour < notifyEndHour);
}

// Get formatted time string
String getTimeString() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Time unavailable";
  }
  
  char timeStr[64];
  strftime(timeStr, sizeof(timeStr), "%I:%M:%S %p", &timeinfo);
  return String(timeStr);
}

// Print current time to Serial Monitor for debugging
void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  
  Serial.print("Current time: ");
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}
