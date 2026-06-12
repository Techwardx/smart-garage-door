const int digitalPin = 5;  // 连接到 D0
// const int analogPin = 6;   // 连接到 A0

void setup() {
  Serial.begin(115200);
  pinMode(digitalPin, INPUT); // 设置数字引脚为输入
  Serial.println("Sensor: ON");
}

void loop() {
  // 1. 读取数字信号：有磁场时通常输出低电平(LOW)
  int digitalVal = digitalRead(digitalPin);
  
  // 2. 读取模拟信号：磁场越强，数值变化越大
  // int analogVal = analogRead(analogPin);

  // 打印到串口监视器
  Serial.print("Ditigal Status: ");
  Serial.println(digitalVal == LOW ? "Magnet: YES" : "Maget: NO");
  // Serial.print("Amalog: ");
  // Serial.println(analogVal);

  delay(200); // 每 0.2 秒刷新一次
}