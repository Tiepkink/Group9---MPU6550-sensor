#include <Wire.h>

const int MPU_addr = 0x68; 
int16_t AcX, AcY, AcZ, GyX, GyY, GyZ;

unsigned long timer;
float dt;
float roll_accel;

// --- CÁC BIẾN VÀ THAM SỐ KALMAN (4.2) ---
float kalman_roll = 0.0;      // Góc tối ưu trả về (Trạng thái ước lượng 1)
float kalman_bias_roll = 0.0; // Bias của Gyro (Trạng thái ước lượng 2)
float P[2][2] = {{0, 0}, {0, 0}}; // Ma trận hiệp phương sai sai số P

// Các tham số cấu hình hệ thống trích từ tài liệu ảnh 2 của bạn:
const float Q_angle = 0.001;   // Nhiễu mô hình góc
const float Q_bias = 0.003;    // Nhiễu tốc độ thay đổi bias
const float R_measure = 0.03;  // Nhiễu đo lường thực tế từ Accel

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
  // Đọc dữ liệu thô từ cảm biến
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

  // Tính thời gian chu kỳ dt
  dt = (float)(micros() - timer) / 1000000.0;
  timer = micros();

  // Quy đổi đơn vị cảm biến
  float gyroX_dps = (float)GyX / 131.0; 
  roll_accel = atan2((float)AcY, (float)AcZ) * 180.0 / PI;

  // --- THUẬT TOÁN BỘ LỌC KALMAN 2 BƯỚC (Theo ảnh 1 & 2) ---
  
  // BƯỚC 1: PREDICT (DỰ BÁO)
  // angle_pred = angle_prev + (gyro_dps - bias_prev) * dt
  float roll_pred = kalman_roll + dt * (gyroX_dps - kalman_bias_roll);
  
  // Tính ma trận P_pred = A @ P @ A.T + Q (Đại số hóa ma trận 2x2)
  P[0][0] += dt * (dt * P[1][1] - P[0][1] - P[1][0] + Q_angle);
  P[0][1] -= dt * P[1][1];
  P[1][0] -= dt * P[1][1];
  P[1][1] += Q_bias * dt;

  // BƯỚC 2: UPDATE (CẬP NHẬT TỪ ACCEL)
  // Tính toán độ lợi Kalman K: K = P_pred @ H.T @ inv(H @ P_pred @ H.T + R)
  float S = P[0][0] + R_measure; 
  float K[2]; 
  K[0] = P[0][0] / S;
  K[1] = P[1][0] / S;

  // Cập nhật trạng thái góc và bias mới dựa trên sai lệch y
  float y = roll_accel - roll_pred; 
  kalman_roll = roll_pred + K[0] * y;
  kalman_bias_roll += K[1] * y;

  // Cập nhật lại ma trận hiệp phương sai P = (I - K @ H) @ P cho vòng lặp tới
  float P00_temp = P[0][0];
  float P01_temp = P[0][1];
  P[0][0] -= K[0] * P00_temp;
  P[0][1] -= K[0] * P01_temp;
  P[1][0] -= K[1] * P00_temp;
  P[1][1] -= K[1] * P01_temp;

  // --- XUẤT DỮ LIỆU ĐỂ VẼ ĐỒ THỊ ---
  Serial.print("Accel_Raw:");     Serial.print(roll_accel);  Serial.print(",");
  Serial.print("Bo_Loc_Kalman:"); Serial.println(kalman_roll);

  delay(10); 
}