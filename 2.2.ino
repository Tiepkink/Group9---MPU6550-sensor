#include <Wire.h>

const int MPU_ADDR = 0x68; 
const float THRESHOLD_STD = 0.1; 

const int SAMPLES_OFFSET = 500; 
const int SAMPLES_STD = 500;    

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(100000); 

  // 1. ĐÁNH THỨC CẢM BIẾN
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); // Thanh ghi Power Management
  Wire.write(0x00);    
  Wire.endTransmission(); 
  delay(10);

  // 2. BẬT BỘ LỌC CHỐNG NHIỄU DLPF (ĐÂY LÀ CHÌA KHÓA!)
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1A); // Thanh ghi CONFIG
  Wire.write(0x05); // Set DLPF_CFG = 5 (Lọc bỏ mọi nhiễu > 10Hz)
  Wire.endTransmission();
  delay(10);

  Serial.println("\n=============================================");
  Serial.println("  DO OFFSET & STD (DA BAT BO LOC DLPF)");
  Serial.println("=============================================");
  Serial.println(">> Vui long giu yen cam bien. Bat dau sau 2 giay...");
  delay(2000); 
  
  // --------------------------------------------------------
  // GIAI ĐOẠN 1: ĐỌC 500 MẪU ĐỂ TÍNH OFFSET 
  // --------------------------------------------------------
  Serial.print("[1/2] Dang doc "); Serial.print(SAMPLES_OFFSET); Serial.print(" mau Offset");
  float sum_gx = 0, sum_gy = 0, sum_gz = 0;
  int valid_samples_1 = 0;

  while (valid_samples_1 < SAMPLES_OFFSET) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x43); 
    Wire.endTransmission();
    Wire.requestFrom(MPU_ADDR, 6);

    if (Wire.available() >= 6) {
      // Ép kiểu tường minh, an toàn tuyệt đối cho Arduino 8-bit
      int16_t raw_x = (Wire.read() << 8) | Wire.read();
      int16_t raw_y = (Wire.read() << 8) | Wire.read();
      int16_t raw_z = (Wire.read() << 8) | Wire.read();

      sum_gx += (raw_x / 131.0);
      sum_gy += (raw_y / 131.0);
      sum_gz += (raw_z / 131.0);
      
      valid_samples_1++;
      if (valid_samples_1 % (SAMPLES_OFFSET / 10) == 0) Serial.print("."); 
    }
    delay(10); 
  }
  float offset_x = sum_gx / SAMPLES_OFFSET;
  float offset_y = sum_gy / SAMPLES_OFFSET;
  float offset_z = sum_gz / SAMPLES_OFFSET;
  Serial.println(" Xong!");

  // --------------------------------------------------------
  // GIAI ĐOẠN 2: THUẬT TOÁN WELFORD
  // --------------------------------------------------------
  Serial.print("[2/2] Dang doc tiep "); Serial.print(SAMPLES_STD); Serial.print(" mau STD");
  
  float mean_x = 0, M2_x = 0;
  float mean_y = 0, M2_y = 0;
  float mean_z = 0, M2_z = 0;
  int valid_samples_2 = 0;

  while (valid_samples_2 < SAMPLES_STD) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x43); 
    Wire.endTransmission();
    Wire.requestFrom(MPU_ADDR, 6);

    if (Wire.available() >= 6) {
      int16_t raw_x = (Wire.read() << 8) | Wire.read();
      int16_t raw_y = (Wire.read() << 8) | Wire.read();
      int16_t raw_z = (Wire.read() << 8) | Wire.read();

      float gx_new = raw_x / 131.0;
      float gy_new = raw_y / 131.0;
      float gz_new = raw_z / 131.0;

      valid_samples_2++;
      float n = (float)valid_samples_2;

      // Cập nhật trục X
      float delta_x = gx_new - mean_x;
      mean_x += delta_x / n;
      M2_x += delta_x * (gx_new - mean_x);

      // Cập nhật trục Y
      float delta_y = gy_new - mean_y;
      mean_y += delta_y / n;
      M2_y += delta_y * (gy_new - mean_y);

      // Cập nhật trục Z
      float delta_z = gz_new - mean_z;
      mean_z += delta_z / n;
      M2_z += delta_z * (gz_new - mean_z);
      
      if (valid_samples_2 % (SAMPLES_STD / 10) == 0) Serial.print("."); 
    }
    delay(10); 
  }
  Serial.println(" Xong!\n");

  // Tính STD cuối cùng
  float std_x = sqrt(M2_x / SAMPLES_STD);
  float std_y = sqrt(M2_y / SAMPLES_STD);
  float std_z = sqrt(M2_z / SAMPLES_STD);

  // --------------------------------------------------------
  // IN KẾT QUẢ CUỐI CÙNG
  // --------------------------------------------------------
  Serial.println("--- KET QUA HIEN THI ---");
  Serial.print("Offset X: "); Serial.print(offset_x, 4); Serial.print(" | STD X: "); Serial.println(std_x, 4);
  Serial.print("Offset Y: "); Serial.print(offset_y, 4); Serial.print(" | STD Y: "); Serial.println(std_y, 4);
  Serial.print("Offset Z: "); Serial.print(offset_z, 4); Serial.print(" | STD Z: "); Serial.println(std_z, 4);

  if (std_x > THRESHOLD_STD || std_y > THRESHOLD_STD || std_z > THRESHOLD_STD) {
    Serial.println("\n[!] LOI: STD > 0.1 do/s. Mach da bi rung lac!");
  } else {
    Serial.println("\n[V] DAT CHUAN: Mach hoan toan dung im, so lieu nay qua dep!");
  }
}

void loop() {}