#include <Wire.h>

const int MPU_ADDR = 0x69; // Đúng địa chỉ của bạn
const int SAMPLES = 500; 

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  // VŨ KHÍ CHỐNG TREO MÁY: Báo lỗi và đi tiếp nếu dây bị lỏng quá 3 giây
  Wire.setWireTimeout(3000, true); 
  
  Wire.setClock(100000); 

  // 1. Đánh thức cảm biến
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); 
  Wire.write(0x00); 
  Wire.endTransmission(); 
  delay(10);

  // 2. BẬT BỘ LỌC LOW-PASS FILTER (DLPF) 10Hz
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1A); 
  Wire.write(0x05); 
  Wire.endTransmission();
  delay(100);

  Serial.println("\n--- DO ZERO-OFFSET (BIAS) VA NOISE (STD) CO DLPF ---");
  Serial.println(">> Vui long dat cam bien nam yen tren mat phang...");
  delay(2000); 

  float mean_ax = 0, M2_ax = 0;
  float mean_ay = 0, M2_ay = 0;
  float mean_az = 0, M2_az = 0;
  float mean_gx = 0, M2_gx = 0;
  float mean_gy = 0, M2_gy = 0;
  float mean_gz = 0, M2_gz = 0;

  Serial.print("Dang lay 500 mau");

  for (int i = 1; i <= SAMPLES; i++) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B); 
    Wire.endTransmission();
    Wire.requestFrom(MPU_ADDR, 14); 

    if (Wire.available() >= 14) {
      int16_t raw_ax = (Wire.read() << 8) | Wire.read();
      int16_t raw_ay = (Wire.read() << 8) | Wire.read();
      int16_t raw_az = (Wire.read() << 8) | Wire.read();
      Wire.read(); Wire.read(); 
      int16_t raw_gx = (Wire.read() << 8) | Wire.read();
      int16_t raw_gy = (Wire.read() << 8) | Wire.read();
      int16_t raw_gz = (Wire.read() << 8) | Wire.read();

      float n = (float)i;

      float delta = raw_ax - mean_ax;
      mean_ax += delta / n;
      M2_ax += delta * (raw_ax - mean_ax);

      delta = raw_ay - mean_ay;
      mean_ay += delta / n;
      M2_ay += delta * (raw_ay - mean_ay);

      delta = raw_az - mean_az;
      mean_az += delta / n;
      M2_az += delta * (raw_az - mean_az);

      delta = raw_gx - mean_gx;
      mean_gx += delta / n;
      M2_gx += delta * (raw_gx - mean_gx);

      delta = raw_gy - mean_gy;
      mean_gy += delta / n;
      M2_gy += delta * (raw_gy - mean_gy);

      delta = raw_gz - mean_gz;
      mean_gz += delta / n;
      M2_gz += delta * (raw_gz - mean_gz);
      
      // In dấu chấm mỗi 50 mẫu để thấy mạch vẫn đang sống
      if (i % 50 == 0) {
        Serial.print("."); 
      }
    }
    delay(5); 
  }

  // Kiểm tra xem I2C có bị ngắt kết nối giữa chừng không
  if (Wire.getWireTimeoutFlag()) {
    Serial.println("\n[!] LOI: Day dien bi long! I2C da bi treo va duoc reset. Vui long cam chat day va an nut Reset tren bo Arduino.");
    Wire.clearWireTimeoutFlag();
    return; // Dừng chương trình
  }

  Serial.println(" Xong!");

  float std_ax = sqrt(M2_ax / SAMPLES);
  float std_ay = sqrt(M2_ay / SAMPLES);
  float std_az = sqrt(M2_az / SAMPLES);
  float std_gx = sqrt(M2_gx / SAMPLES);
  float std_gy = sqrt(M2_gy / SAMPLES);
  float std_gz = sqrt(M2_gz / SAMPLES);

  Serial.println("\n[ Bieu bang 1.1: DU LIEU THO (RAW DATA) - DA LOC DLPF ]");
  Serial.println("Truc\tMean Raw (Offset)\tSTD Raw (Noise)");
  Serial.println("-------------------------------------------------");
  Serial.print("Ax\t"); Serial.print(mean_ax, 2); Serial.print("\t\t"); Serial.println(std_ax, 2);
  Serial.print("Ay\t"); Serial.print(mean_ay, 2); Serial.print("\t\t"); Serial.println(std_ay, 2);
  Serial.print("Az\t"); Serial.print(mean_az, 2); Serial.print("\t\t"); Serial.println(std_az, 2);
  Serial.print("Gx\t"); Serial.print(mean_gx, 2); Serial.print("\t\t"); Serial.println(std_gx, 2);
  Serial.print("Gy\t"); Serial.print(mean_gy, 2); Serial.print("\t\t"); Serial.println(std_gy, 2);
  Serial.print("Gz\t"); Serial.print(mean_gz, 2); Serial.print("\t\t"); Serial.println(std_gz, 2);

  Serial.println("\n[ Bieu bang 1.2: DON VI VAT LY (g & deg/s) - DA LOC DLPF ]");
  Serial.println("Truc\tZero-offset (Bias)\tNoise (STD)");
  Serial.println("-------------------------------------------------");
  
  Serial.print("Ax\t"); Serial.print(mean_ax / 16384.0, 5); Serial.print(" g\t\t"); Serial.print(std_ax / 16384.0, 5); Serial.println(" g");
  Serial.print("Ay\t"); Serial.print(mean_ay / 16384.0, 5); Serial.print(" g\t\t"); Serial.print(std_ay / 16384.0, 5); Serial.println(" g");
  Serial.print("Az\t"); Serial.print(mean_az / 16384.0, 5); Serial.print(" g\t\t"); Serial.print(std_az / 16384.0, 5); Serial.println(" g");
  
  Serial.print("Gx\t"); Serial.print(mean_gx / 131.0, 5); Serial.print(" d/s\t\t"); Serial.print(std_gx / 131.0, 5); Serial.println(" d/s");
  Serial.print("Gy\t"); Serial.print(mean_gy / 131.0, 5); Serial.print(" d/s\t\t"); Serial.print(std_gy / 131.0, 5); Serial.println(" d/s");
  Serial.print("Gz\t"); Serial.print(mean_gz / 131.0, 5); Serial.print(" d/s\t\t"); Serial.print(std_gz / 131.0, 5); Serial.println(" d/s");
}

void loop() {}