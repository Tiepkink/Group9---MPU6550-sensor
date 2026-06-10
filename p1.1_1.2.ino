#include <Wire.h>

const int MPU_ADDR = 0x68; // Địa chỉ I2C của MPU6500

// Hệ số Scale Factor cho dải cấu hình mặc định (+-2g và +-250 deg/s)
const float ACCEL_SCALE = 16384.0;
const float GYRO_SCALE = 131.0;

// Cấu trúc (Struct) ứng dụng Thuật toán Welford để tính Mean & STD an toàn
struct StatCalc {
  long n = 0;
  float mean = 0.0;
  float M2 = 0.0;

  // Cập nhật từng mẫu raw (int16_t) vào bộ tính toán
  void update(int16_t x) {
    n++;
    float delta = x - mean;
    mean += delta / n;
    float delta2 = x - mean;
    M2 += delta * delta2;
  }

  float getMeanRaw() { return mean; }
  float getStdRaw() { return (n > 1) ? sqrt(M2 / (n - 1)) : 0.0; }
};

// Khai báo 6 bộ tính toán độc lập cho 6 trục
StatCalc ax_stat, ay_stat, az_stat;
StatCalc gx_stat, gy_stat, gz_stat;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000); // Ép xung I2C lên Fast Mode 400kHz

  // 1. KIỂM TRA PHẦN CỨNG MPU6500
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x75); // Thanh ghi WHO_AM_I
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 1, true);
  
  if (Wire.read() != 0x70) {
    Serial.println("Loi: Khong phai MPU6500 (Ma 0x70)! Kiem tra lai ket noi.");
    while(1);
  }

  // 2. ĐÁNH THỨC CẢM BIẾN
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); // Thanh ghi PWR_MGMT_1
  Wire.write(0x00); // Bật cảm biến
  Wire.endTransmission(true);
  delay(100);

  Serial.println("\n==================================================");
  Serial.println(" BAT DAU THU THAP 500 MAU - DUNG CHAM VAO MACH!");
  Serial.println("==================================================");
  delay(2000); // Trễ 2 giây để triệt tiêu rung động từ tay

  // 3. THỰC THI NHIỆM VỤ 1.1 VÀ 1.2
  runTask_1_1_and_1_2();
}

void loop() {
  // Hoàn thành nhiệm vụ ở setup(), loop() để trống tránh xả rác màn hình.
}

// -------------------------------------------------------------
// HÀM LÕI
// -------------------------------------------------------------

void fetchRawData() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // Bắt đầu từ thanh ghi ACCEL_XOUT_H
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 14, true);

  if (Wire.available() == 14) {
    // Đọc, ghép bit và nạp thẳng số nguyên vào hàm thống kê
    ax_stat.update(Wire.read() << 8 | Wire.read());
    ay_stat.update(Wire.read() << 8 | Wire.read());
    az_stat.update(Wire.read() << 8 | Wire.read());
    
    Wire.read(); Wire.read(); // Bỏ qua 2 byte nhiệt độ
    
    gx_stat.update(Wire.read() << 8 | Wire.read());
    gy_stat.update(Wire.read() << 8 | Wire.read());
    gz_stat.update(Wire.read() << 8 | Wire.read());
  }
}

void runTask_1_1_and_1_2() {
  const int NUM_SAMPLES = 500;
  Serial.println("Dang quet du lieu...");

  // BƯỚC 1: LẤY 500 MẪU RAW (Đáp ứng phần 1.1)
  for (int i = 0; i < NUM_SAMPLES; i++) {
    fetchRawData();
    delay(2); // Giả lập tần số lấy mẫu ~500Hz
  }

  // BƯỚC 2: QUY ĐỔI RA ĐƠN VỊ VẬT LÝ TỪ KẾT QUẢ RAW (Đáp ứng phần 1.2)
  // Lấy Mean & STD của Raw chia cho Scale Factor
  float ax_mean_phys = ax_stat.getMeanRaw() / ACCEL_SCALE;
  float ax_std_phys  = ax_stat.getStdRaw() / ACCEL_SCALE;
  float ay_mean_phys = ay_stat.getMeanRaw() / ACCEL_SCALE;
  float ay_std_phys  = ay_stat.getStdRaw() / ACCEL_SCALE;
  float az_mean_phys = az_stat.getMeanRaw() / ACCEL_SCALE;
  float az_std_phys  = az_stat.getStdRaw() / ACCEL_SCALE;

  float gx_mean_phys = gx_stat.getMeanRaw() / GYRO_SCALE;
  float gx_std_phys  = gx_stat.getStdRaw() / GYRO_SCALE;
  float gy_mean_phys = gy_stat.getMeanRaw() / GYRO_SCALE;
  float gy_std_phys  = gy_stat.getStdRaw() / GYRO_SCALE;
  float gz_mean_phys = gz_stat.getMeanRaw() / GYRO_SCALE;
  float gz_std_phys  = gz_stat.getStdRaw() / GYRO_SCALE;

  // BƯỚC 3: IN BÁO CÁO TOÀN DIỆN RA SERIAL
  Serial.println("\n[ KET QUA PHAN 1.1: DU LIEU THO (RAW LSB) ]");
  Serial.println("Truc \t Mean Raw \t STD Raw (Nhieu)");
  Serial.println("--------------------------------------------------");
  Serial.print(" Ax \t "); Serial.print(ax_stat.getMeanRaw(), 2); Serial.print(" \t\t "); Serial.println(ax_stat.getStdRaw(), 2);
  Serial.print(" Ay \t "); Serial.print(ay_stat.getMeanRaw(), 2); Serial.print(" \t\t "); Serial.println(ay_stat.getStdRaw(), 2);
  Serial.print(" Az \t "); Serial.print(az_stat.getMeanRaw(), 2); Serial.print(" \t\t "); Serial.println(az_stat.getStdRaw(), 2);
  Serial.print(" Gx \t "); Serial.print(gx_stat.getMeanRaw(), 2); Serial.print(" \t\t "); Serial.println(gx_stat.getStdRaw(), 2);
  Serial.print(" Gy \t "); Serial.print(gy_stat.getMeanRaw(), 2); Serial.print(" \t\t "); Serial.println(gy_stat.getStdRaw(), 2);
  Serial.print(" Gz \t "); Serial.print(gz_stat.getMeanRaw(), 2); Serial.print(" \t\t "); Serial.println(gz_stat.getStdRaw(), 2);

  Serial.println("\n[ KET QUA PHAN 1.2: DON VI VAT LY (g & deg/s) ]");
  Serial.println("Truc \t Mean (Zero-Offset) \t STD (Noise Level)");
  Serial.println("--------------------------------------------------");
  Serial.print(" Ax \t "); Serial.print(ax_mean_phys, 5); Serial.print(" g \t\t "); Serial.print(ax_std_phys, 5); Serial.println(" g");
  Serial.print(" Ay \t "); Serial.print(ay_mean_phys, 5); Serial.print(" g \t\t "); Serial.print(ay_std_phys, 5); Serial.println(" g");
  Serial.print(" Az \t "); Serial.print(az_mean_phys, 5); Serial.print(" g \t\t "); Serial.print(az_std_phys, 5); Serial.println(" g");
  Serial.println("   *(Truc Az ly tuong nen gan bang 1.00000 g)*");
  
  Serial.print(" Gx \t "); Serial.print(gx_mean_phys, 5); Serial.print(" d/s \t\t "); Serial.print(gx_std_phys, 5); Serial.println(" d/s");
  Serial.print(" Gy \t "); Serial.print(gy_mean_phys, 5); Serial.print(" d/s \t\t "); Serial.print(gy_std_phys, 5); Serial.println(" d/s");
  Serial.print(" Gz \t "); Serial.print(gz_mean_phys, 5); Serial.print(" d/s \t\t "); Serial.print(gz_std_phys, 5); Serial.println(" d/s");
  Serial.println("==================================================");
  Serial.println("Nhiem vu 1.1 va 1.2 hoan tat. San sang cho bo loc.");
}