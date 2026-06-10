#include <Wire.h>

const int MPU_addr = 0x68; 
int16_t AcX, AcY, AcZ;
float ax, ay, az, amag;

unsigned long lastTime = 0;
const unsigned long interval = 2000; // Tần số 500Hz (2000 microseconds)

const float THRESHOLD = 2.0; 

// Biến toàn cục phục vụ thuật toán tự động khóa đồ thị
bool da_chup_xung = false;
int dem_mau = 0;

void setup() {
  Serial.begin(115200); 
  Wire.begin();
  Wire.setClock(400000); // Chạy I2C tốc độ cao 400kHz

  // Khởi động MPU
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B); 
  Wire.write(0);     
  Wire.endTransmission(true);

  // Cấu hình dải đo gia tốc lên +/- 8g
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x1C); 
  Wire.write(0x10); 
  Wire.endTransmission(true);
}

void loop() {
  unsigned long currentTime = micros();
  
  if (currentTime - lastTime >= interval) {
    lastTime = currentTime;
    
    // NẾU ĐÃ KHÓA: Giữ đồ thị đứng im ở trạng thái tĩnh để thong thả chụp màn hình
    if (da_chup_xung) {
      Serial.print("Gia_toc_tong_hop:"); Serial.print(1.0); Serial.print(",");
      Serial.print("Trang_thai_tinh:");  Serial.print(1.0); Serial.print(",");
      Serial.print("Nguong_tren:");      Serial.print(1.0 + THRESHOLD); Serial.print(",");
      Serial.print("Nguong_duoi:");     Serial.println(1.0 - THRESHOLD);
      return; 
    }

    // ĐỌC DỮ LIỆU CẢM BIẾN MPU6500/6050
    Wire.beginTransmission(MPU_addr);
    Wire.write(0x3B);  
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_addr, 6, true);  
    
    AcX = (Wire.read() << 8 | Wire.read());  
    AcY = (Wire.read() << 8 | Wire.read());  
    AcZ = (Wire.read() << 8 | Wire.read());  
    
    // Đổi sang đơn vị g (chia cho 4096.0 ứng với dải +/- 8g)
    ax = (float)AcX / 4096.0;
    ay = (float)AcY / 4096.0;
    az = (float)AcZ / 4096.0;
    
    // Tính độ lớn gia tốc tổng hợp |a|
    amag = sqrt(ax*ax + ay*ay + az*az);

    // THUẬT TOÁN TRIGGER: Kiểm tra nếu biến động gia tốc vượt ngưỡng va chạm
    if (abs(amag - 1.0) > THRESHOLD && dem_mau == 0) {
      dem_mau = 1; // Bắt đầu kích hoạt đếm mẫu
    }

    // Đếm số mẫu sau khi va chạm để bắt trọn dao động tắt dần
    if (dem_mau > 0) {
      dem_mau++;
      if (dem_mau > 300) { // Đọc thêm 300 mẫu (~0.6 giây), khóa đồ thị lại
        da_chup_xung = true; 
      }
    }

    // XUẤT DỮ LIỆU ĐỊNH DẠNG ĐỔI TÊN VALUE TRÊN SERIAL PLOTTER
    Serial.print("Gia_toc_tong_hop:"); Serial.print(amag); Serial.print(",");
    Serial.print("Trang_thai_tinh:");  Serial.print(1.0); Serial.print(",");
    Serial.print("Nguong_tren:");      Serial.print(1.0 + THRESHOLD); Serial.print(",");
    Serial.print("Nguong_duoi:");     Serial.println(1.0 - THRESHOLD);  
  }
}