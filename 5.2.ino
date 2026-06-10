#include <Wire.h>
#include <arduinoFFT.h> 

// ==========================================
// THÔNG SỐ CẤU HÌNH
// ==========================================
const int MPU_ADDR = 0x68;             
const uint16_t samples = 128;          
const double samplingFrequency = 500;  

unsigned int sampling_period_us;
unsigned long microseconds;

double vReal[samples];
double vImag[samples];

// Khởi tạo đối tượng FFT phiên bản v2.x
ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, samples, samplingFrequency);

// --- BỘ LỌC LOW-PASS FILTER (LPF) ---
// false = Xem phổ tín hiệu GỐC bị nhiễu
// true  = Xem phổ tín hiệu ĐÃ QUA LỌC thông thấp
bool USE_LOW_PASS_FILTER = false; 

float alpha = 0.2; 
float filtered_Z = 0;

// ==========================================
// HÀM SETUP
// ==========================================
void setup() {
  Serial.begin(115200); 
  Wire.begin();
  
  // Khởi động cảm biến MPU6500
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); 
  Wire.write(0);    
  Wire.endTransmission(true);

  // Cấu hình tầm đo gia tốc +/- 2g
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C); 
  Wire.write(0x00); 
  Wire.endTransmission(true);

  // Tính chu kỳ lấy mẫu tương ứng với 500Hz
  sampling_period_us = round(1000000 * (1.0 / samplingFrequency));
}

// ==========================================
// HÀM LOOP CHÍNH
// ==========================================
void loop() {
  // 1. TIẾN HÀNH LẤY MẪU DỮ LIỆU TỪ MPU6500
  for (int i = 0; i < samples; i++) {
    microseconds = micros();    

    // Đọc dữ liệu thô trục Z
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3F); 
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 2, true); 
    
    int16_t accel_Z_raw = (Wire.read() << 8 | Wire.read());
    float accel_Z = accel_Z_raw / 16384.0; 

    // Kiểm tra bộ lọc Low-Pass Filter
    if (USE_LOW_PASS_FILTER) {
      filtered_Z = (alpha * accel_Z) + ((1.0 - alpha) * filtered_Z);
      vReal[i] = filtered_Z;
    } else {
      vReal[i] = accel_Z; 
    }
    
    vImag[i] = 0.0; 

    // Đảm bảo tần số lấy mẫu chuẩn xác ở 500Hz
    while (micros() - microseconds < sampling_period_us) {
      // Đợi cho đúng thời gian chu kỳ
    }
  }

  // 2. THỰC HIỆN BIẾN ĐỔI FFT
  FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD); 
  FFT.compute(FFT_FORWARD);                        
  FFT.complexToMagnitude();                        

  // 3. XUẤT ĐẦU RA CHO ARDUINO SERIAL PLOTTER VẼ ĐỒ THỊ
  for (int i = 2; i < (samples / 2); i++) { 
    // In kèm nhãn dán để đồ thị hiện tên chuẩn xác
    Serial.print("Bien_do_FFT:"); 
    Serial.println(vReal[i]); 
  }

  // Dừng 3 giây để kịp nhìn biểu đồ và chụp ảnh làm báo cáo
  delay(100); 
}