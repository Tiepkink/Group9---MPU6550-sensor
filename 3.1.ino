#include <Wire.h>
#include <math.h>

const int MPU_addr = 0x68;
const int numSamples = 200; // Đo đúng 200 mẫu theo yêu cầu tài liệu
const float LSB_to_g = 16384.0; // Hệ số quy đổi sang g ở tầm đo mặc định (+-2g)

void setup() {
  Wire.begin();
  Serial.begin(115200);

  // Đánh thức MPU6050
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B); // Thanh ghi PWR_MGMT_1
  Wire.write(0);    // Ghi 0 để đánh thức
  Wire.endTransmission(true);

  Serial.println("==================================================");
  Serial.println("THÍ NGHIỆM KHẢO SÁT ẢNH HƯỞNG GIA TỐC TRỌNG TRƯỜNG");
  Serial.println("==================================================");
  Serial.println("Hướng dẫn: Đặt cố định cảm biến ở góc cần đo.");
  Serial.println("Gõ chữ 'm' vào ô Chat phía trên và nhấn Send để ĐO.");
}

void loop() {
  // Đợi người dùng gõ lệnh 'm' từ Serial Monitor để bắt đầu đo
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'm' || c == 'M') {
      tienHanhDo200Mau();
    }
  }
}

void tienHanhDo200Mau() {
  long sumX = 0, sumY = 0, sumZ = 0;

  Serial.print("\n-> Đang thu thập 200 mẫu... ");
  for (int i = 0; i < numSamples; i++) {
    Wire.beginTransmission(MPU_addr);
    Wire.write(0x3B); // Bắt đầu từ thanh ghi ACCEL_XOUT_H
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_addr, 6, true); // Đọc 6 byte (X, Y, Z gia tốc)

    int16_t rawX = Wire.read() << 8 | Wire.read();
    int16_t rawY = Wire.read() << 8 | Wire.read();
    int16_t rawZ = Wire.read() << 8 | Wire.read();

    sumX += rawX;
    sumY += rawY;
    sumZ += rawZ;
    delay(10); // Khoảng cách giữa các mẫu là 10ms (tổng thời gian đo ~2 giây)
  }
  Serial.println("Xong!");

  // 1. Tính giá trị trung bình thô (Raw Mean)
  float meanRawX = (float)sumX / numSamples;
  float meanRawY = (float)sumY / numSamples;
  float meanRawZ = (float)sumZ / numSamples;

  // 2. Quy đổi dữ liệu thô sang giá trị vật lý (đơn vị g)
  float Ax = meanRawX / LSB_to_g;
  float Ay = meanRawY / LSB_to_g;
  float Az = meanRawZ / LSB_to_g;

  // 3. Kiểm tra toàn diện hệ thống: Độ dài vector g tổng hợp
  float totalG = sqrt(Ax * Ax + Ay * Ay + Az * Az);
  float saiSoPercent = abs(totalG - 1.0) * 100.0;

  // 4. In kết quả hiển thị lên màn hình
  Serial.println("--------------------------------------------------");
  Serial.print("Gia tốc trục X (Ax thực tế): "); Serial.print(Ax, 3); Serial.println(" g");
  Serial.print("Gia tốc trục Y (Ay thực tế): "); Serial.print(Ay, 3); Serial.println(" g");
  Serial.print("Gia tốc trục Z (Az thực tế): "); Serial.print(Az, 3); Serial.println(" g");
  Serial.println("- - - - - - - - - - - - - - - - - - - - - - - - - ");
  Serial.print("Kiểm tra tổng bình phương vector: "); Serial.print(totalG, 4); Serial.println(" g");
  Serial.print("Sai lệch so với 1.0g lý thuyết: "); Serial.print(saiSoPercent, 2); Serial.println("%");

  if (saiSoPercent > 3.0) {
    Serial.println("=> ⚠️ SAI SỐ > 3%: Cần hiệu chỉnh phần cứng hoặc giữ yên mạch khi đo.");
  } else {
    Serial.println("=> ✅ ĐẠT YÊU CẦU: Thỏa mãn điều kiện bài khảo sát.");
  }
  Serial.println("--------------------------------------------------");
  Serial.println("Sẵn sàng cho góc tiếp theo. Gõ 'm' để tiếp tục đo.");
}