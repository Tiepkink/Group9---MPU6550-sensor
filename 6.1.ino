#include <Wire.h>
#include <math.h>

const int MPU_ADDR = 0x68; 

int16_t raw_accel_x, raw_accel_y, raw_accel_z;
float ax, ay, az;

// Biến lưu trữ Offset của góc
float roll_offset = 0;
float pitch_offset = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  // Đánh thức MPU6050
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); 
  Wire.write(0);    
  Wire.endTransmission(true);

  // --- TỰ ĐỘNG TÍNH OFFSET (ZERO HÓA) ---
  int num_samples = 500;
  float sum_roll = 0;
  float sum_pitch = 0;

  // Bỏ qua 100 mẫu đầu để module ổn định
  for(int i = 0; i < 100; i++) {
    readAccelData();
    delay(3);
  }

  // Thu thập 500 mẫu ở trạng thái tĩnh
  for(int i = 0; i < num_samples; i++) {
    readAccelData();
    float roll_raw = atan2(ay, az) * 180.0 / PI;
    float pitch_raw = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI;
    
    sum_roll += roll_raw;
    sum_pitch += pitch_raw;
    delay(3);
  }

  roll_offset = sum_roll / num_samples;
  pitch_offset = sum_pitch / num_samples;
  
  // Lưu ý: Không in text (chữ) ra Serial nữa để Serial Plotter không bị lỗi đồ thị
}

// Hàm đọc dữ liệu gia tốc thô và quy đổi sang g
void readAccelData() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); 
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);

  raw_accel_x = Wire.read() << 8 | Wire.read();
  raw_accel_y = Wire.read() << 8 | Wire.read();
  raw_accel_z = Wire.read() << 8 | Wire.read();

  ax = raw_accel_x / 16384.0;
  ay = raw_accel_y / 16384.0;
  az = raw_accel_z / 16384.0;
}

void loop() {
  readAccelData();

  // Tính góc thô từ Accelerometer
  float roll_raw = atan2(ay, az) * 180.0 / PI;
  float pitch_raw = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI;

  // TRỪ ĐI OFFSET (ÉP VỀ 0.00 ĐỘ)
  float roll_cal = roll_raw - roll_offset;
  float pitch_cal = pitch_raw - pitch_offset;

  // Xuất dữ liệu liên tục theo định dạng của Serial Plotter
  Serial.print("Roll_Noise:"); Serial.print(roll_cal); Serial.print(",");
  Serial.print("Pitch_Noise:"); Serial.println(pitch_cal);
  
  delay(20); // Lấy mẫu với tần số 50Hz để vẽ đồ thị mượt mà
}