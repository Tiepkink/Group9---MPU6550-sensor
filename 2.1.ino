#include <Wire.h>

const int MPU_ADDR = 0x68; 

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(100000); // Giữ tốc độ an toàn 100kHz chong nhieu

  // Đánh thức cảm biến
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); 
  Wire.write(0x00);    
  Wire.endTransmission(); 

  Serial.println("\n==========================================");
  Serial.println("  HIEU CHINH GIA TOC KE (CHU DONG BAM ENTER)");
  Serial.println("==========================================");
  Serial.println("Huong dan: Kiem tra hinh tren Serial, dat cam bien cho chuan.");
  Serial.println("Sau do go 1 phim bat ky (vd: a) roi bam Enter de may lay so lieu.\n");

  // Bắt đầu chuỗi 6 vị trí (1: trục X, 2: trục Y, 3: trục Z)
  float z_up   = measureAxis("1. DAT PHANG TREN BAN (Truc Z huong LEN: +1g)", 3);
  float z_down = measureAxis("2. LAT UP CAM BIEN (Truc Z huong XUONG: -1g)", 3);
  
  float x_up   = measureAxis("3. DUNG DUNG CAM BIEN (Truc X huong LEN: +1g)", 1);
  float x_down = measureAxis("4. DAO CHIEU CAM BIEN (Truc X huong XUONG: -1g)", 1);
  
  float y_up   = measureAxis("5. NGHIENG SANG CANH (Truc Y huong LEN: +1g)", 2);
  float y_down = measureAxis("6. DAO CHIEU CANH (Truc Y huong XUONG: -1g)", 2);

  // Tính toán kết quả
  Serial.println("\n==========================================");
  Serial.println("--- DA DO XONG! DANG TINH TOAN OFFSET ---");
  
  float offset_x = (x_up + x_down) / 2.0; 
  float offset_y = (y_up + y_down) / 2.0; 
  float offset_z = (z_up + z_down) / 2.0; 

  Serial.println("=> KET QUA OFFSET GIA TOC (Don vi: g):");
  Serial.print("Offset Ax = "); Serial.println(offset_x, 5);
  Serial.print("Offset Ay = "); Serial.println(offset_y, 5);
  Serial.print("Offset Az = "); Serial.println(offset_z, 5);
  Serial.println("==========================================");
  Serial.println("Hoan thanh! Hay chep lai 3 con so nay vao bao cao.");
}

void loop() {
  // Không làm gì thêm sau khi tính xong
}

// Hàm hỗ trợ đợi lệnh và đo lường
float measureAxis(String instruction, int axis) {
  Serial.println("\n------------------------------------------");
  Serial.println(instruction);
  Serial.println(">> Xin moi vao vi tri. Go 1 phim bat ky roi bam Enter de do...");

  // Chờ cho đến khi người dùng nhập dữ liệu vào Serial Monitor
  while(Serial.available() == 0) {
    delay(10);
  }
  // Xóa sạch bộ đệm (tránh bị nhảy đúp xuống bước sau nếu nhập nhiều ký tự)
  while(Serial.available() > 0) {
    Serial.read();
  }
  
  Serial.println("[DANG DO DU LIEU - VUI LONG GIU YEN 1 GIAY!]");

  long sum_ax = 0, sum_ay = 0, sum_az = 0;
  int valid_samples = 0;

  // Lấy 200 mẫu an toàn (bọc thép chống treo I2C)
  while (valid_samples < 200) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B); 
    Wire.endTransmission(); 
    Wire.requestFrom(MPU_ADDR, 6);

    if(Wire.available() >= 6) {
      sum_ax += (Wire.read() << 8 | Wire.read());
      sum_ay += (Wire.read() << 8 | Wire.read());
      sum_az += (Wire.read() << 8 | Wire.read());
      valid_samples++; // Chỉ tăng biến đếm nếu thực sự đọc được dữ liệu
    }
    delay(5);
  }

  // Đổi ra đơn vị g
  float avg_ax = (sum_ax / 200.0) / 16384.0;
  float avg_ay = (sum_ay / 200.0) / 16384.0;
  float avg_az = (sum_az / 200.0) / 16384.0;

  Serial.print("=> Ket qua do: X: "); Serial.print(avg_ax, 4);
  Serial.print("g | Y: "); Serial.print(avg_ay, 4);
  Serial.print("g | Z: "); Serial.println(avg_az, 4);

  // Trả về trục cần thiết
  if (axis == 1) return avg_ax;
  if (axis == 2) return avg_ay;
  return avg_az; // axis == 3
}