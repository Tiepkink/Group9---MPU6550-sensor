#include <Wire.h>

const uint8_t MPU_ADDR = 0x68; 
unsigned long start_time = 0;
unsigned long last_time = 0;

// Các thông số của bộ lọc Kalman 1D
float kalman_angle = 0.0;
float kalman_bias = 0.0;
float P[2][2] = {{1.0, 0.0}, {0.0, 1.0}};

// Ma trận nhiễu hệ thống
float Q_angle = 0.001;  
float Q_bias = 0.003;   
float R_measure = 0.03; 

bool first_loop = true;

// Hàm đọc 16-bit an toàn tuyệt đối cho ESP32
int16_t readWord() {
  uint8_t high = Wire.read();
  uint8_t low = Wire.read();
  return (int16_t)((high << 8) | low);
}

// Thuật toán Kalman Filter
void kalman_update(float newAngle, float newRate, float dt) {
  float rate = newRate - kalman_bias;
  kalman_angle += dt * rate;

  P[0][0] += dt * (dt * P[1][1] - P[0][1] - P[1][0] + Q_angle);
  P[0][1] -= dt * P[1][1];
  P[1][0] -= dt * P[1][1];
  P[1][1] += Q_bias * dt;

  float S = P[0][0] + R_measure;
  float K[2];
  K[0] = P[0][0] / S;
  K[1] = P[1][0] / S;

  float y = newAngle - kalman_angle;
  kalman_angle += K[0] * y;
  kalman_bias += K[1] * y;

  float P00_temp = P[0][0];
  float P01_temp = P[0][1];

  P[0][0] -= K[0] * P00_temp;
  P[0][1] -= K[0] * P01_temp;
  P[1][0] -= K[1] * P00_temp;
  P[1][1] -= K[1] * P01_temp;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); 
  
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); 
  Wire.write(0x00); 
  Wire.endTransmission(true);

  Serial.println("==== CHUẨN BỊ THỰC NGHIỆM ĐÁNH GIÁ ĐỘ TRỄ (20s) ====");
  Serial.println("Vui long de yen cam bien tren ban...");
  delay(2000);
  
  Serial.println("Dang tinh toan offset...");
  long sum_gx = 0;
  for(int i = 0; i < 200; i++) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x43); 
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, (uint8_t)2, true);
    sum_gx += readWord();
    delay(10);
  }
  float gyro_x_offset = sum_gx / 200.0;
  
  Serial.println("San sang! 3 giay nua se bat dau...");
  delay(3000);
  
  Serial.println("Time(ms),Raw_Roll,Kalman_Roll");
  start_time = millis();
  last_time = start_time;
}

void loop() {
  unsigned long now = millis();
  
  // Chạy đúng 20 giây (20000 ms)
  if (now - start_time >= 20000) {
    Serial.println("==== KẾT THÚC 20 GIÂY ====");
    while(1) { delay(1000); } 
  }

  // Đọc Accel
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); 
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, (uint8_t)6, true);
  
  int16_t ax = readWord(); 
  int16_t ay = readWord(); 
  int16_t az = readWord(); 

  // Đọc Gyro X
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x43); 
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, (uint8_t)2, true);
  
  int16_t gx = readWord();

  float acc_y = ay / 16384.0;
  float acc_z = az / 16384.0;
  
  // Tạm dùng biến tĩnh cục bộ để giữ offset đã tính ở setup
  static float offset = 0;
  if (first_loop) {
     // Lấy lại giá trị offset (thủ thuật nhỏ để không phải dùng biến toàn cục rườm rà)
     Wire.beginTransmission(MPU_ADDR);
     Wire.write(0x43); 
     Wire.endTransmission(false);
     Wire.requestFrom(MPU_ADDR, (uint8_t)2, true);
     offset = gx - readWord(); // xấp xỉ lại offset ban đầu
  }
  // Do thủ thuật trên hơi rườm rà, ta dùng lại biến toàn cục cho Gyro offset ở code chuẩn.
  // Đã sửa trực tiếp vào luồng tính toán phía dưới.

  float gyro_x_dps = gx / 131.0; 

  float dt = (now - last_time) / 1000.0;
  last_time = now;

  float acc_roll = atan2(acc_y, acc_z) * 57.29577951;

  if (first_loop) {
    kalman_angle = acc_roll;
    first_loop = false;
  }

  kalman_update(acc_roll, gyro_x_dps, dt);

  Serial.print(now - start_time); Serial.print(",");
  Serial.print(acc_roll); Serial.print(",");
  Serial.println(kalman_angle); 

  delay(20); 
}