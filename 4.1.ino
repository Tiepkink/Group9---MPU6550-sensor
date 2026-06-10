#include <Wire.h>

const int MPU_addr = 0x68; 
int16_t AcX, AcY, AcZ, GyX, GyY, GyZ;

unsigned long timer;
float dt;
float roll_accel;

// Khai báo 3 biến góc cho 3 cấu hình alpha khác nhau
float roll_comp_90 = 0; 
float roll_comp_95 = 0; 
float roll_comp_98 = 0; 

// Định nghĩa 3 giá trị alpha theo tài liệu
const float alpha_1 = 0.90;
const float alpha_2 = 0.95;
const float alpha_3 = 0.98;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B); 
  Wire.write(0);    
  Wire.endTransmission(true);
  timer = micros();
}

void loop() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B); 
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 14, true); 
  
  AcX = Wire.read()<<8|Wire.read();  
  AcY = Wire.read()<<8|Wire.read();  
  AcZ = Wire.read()<<8|Wire.read();  
  Wire.read()<<8|Wire.read(); 
  GyX = Wire.read()<<8|Wire.read();  
  GyY = Wire.read()<<8|Wire.read();  
  GyZ = Wire.read()<<8|Wire.read();  

  dt = (float)(micros() - timer) / 1000000.0;
  timer = micros();

  float gyroX_dps = (float)GyX / 131.0; 
  roll_accel = atan2((float)AcY, (float)AcZ) * 180.0 / PI;

  // --- TÍNH TOÁN ĐỒNG THỜI 3 BỘ LỌC BÙ KHÁC ALPHA ---
  
  // 1. Alpha = 0.90 (Tin Accel 10% - Hợp cho thiết bị rung nhiều, chuyển động chậm)
  roll_comp_90 = alpha_1 * (roll_comp_90 + gyroX_dps * dt) + (1.0 - alpha_1) * roll_accel;

  // 2. Alpha = 0.95 (Cân bằng - Mặc định cho hầu hết ứng dụng)
  roll_comp_95 = alpha_2 * (roll_comp_95 + gyroX_dps * dt) + (1.0 - alpha_2) * roll_accel;

  // 3. Alpha = 0.98 (Tin Gyro 98% - Hợp cho thiết bị chuyển động nhanh, ít rung)
  roll_comp_98 = alpha_3 * (roll_comp_98 + gyroX_dps * dt) + (1.0 - alpha_3) * roll_accel;


  // --- XUẤT DỮ LIỆU ĐỂ SO SÁNH TRÊN SERIAL PLOTTER ---
  Serial.print("Accel_Raw:");   Serial.print(roll_accel);    Serial.print(",");
  Serial.print("Alpha_0.90:");  Serial.print(roll_comp_90);  Serial.print(",");
  Serial.print("Alpha_0.95:");  Serial.print(roll_comp_95);  Serial.print(",");
  Serial.print("Alpha_0.98:");  Serial.println(roll_comp_98);

  delay(10); 
}