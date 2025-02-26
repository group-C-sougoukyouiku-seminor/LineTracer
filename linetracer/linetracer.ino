//loopで渡し続ける変数
struct DoubleResult{
  int position;
  int lightCount;
};

//ピン設定
const int AIN1 = 4;
const int AIN2 = 2;
const int BIN1 = 27;
const int BIN2 = 26;
const int STBY = 25;  
int previousDirection = 0;

int step = 0; 
float integral = 0;  // 積分初期値
float lastError = 0; // 1ステップ前の初期値
float Kp = 90; // 比例定数
float Ki = 1; // 積分定数
float Kd = 5; // 微分定数
float PIDvalue = 0.2; //PID制御の倍率
float baseSpeed = 120; //モーターの初期速度
int threshold = 2000; //フォトリフレクタ反応しきい値
float maxSpeed = 255;
float minSpeed = 0;

//モーターの設定クラス
class DRV8833 {
  private:
    const int freq = 10000;  // 周波数
    const int resolution = 8;

  public:
    DRV8833(int AIN1, int AIN2, int BIN1, int BIN2, int STBY) {
      pinMode(STBY, OUTPUT);
      digitalWrite(STBY, HIGH); 

      ledcAttach(AIN1, freq, resolution);
      ledcAttach(AIN2, freq, resolution);
      ledcAttach(BIN1, freq, resolution);
      ledcAttach(BIN2, freq, resolution);
    }

    void motorDRV8833_L(int speed) {
      if (speed > 0 && speed < 256) {
        ledcWrite(AIN1, 255 - speed);
        ledcWrite(AIN2, 255);
      } else if (speed == 0) {
        ledcWrite(AIN1, 255);
        ledcWrite(AIN2, 255);
      } else if (speed > -256 && speed < 0) {
        ledcWrite(AIN1, 255);
        ledcWrite(AIN2, 255 + speed);
      } else {
        ledcWrite(AIN1, 255);
        ledcWrite(AIN2, 255);
      }
    }

    void motorDRV8833_R(int speed) {
      if (speed > 0 && speed < 256) {
        ledcWrite(BIN2, 255 - speed);
        ledcWrite(BIN1, 255);
      } else if (speed == 0) {
        ledcWrite(BIN2, 255);
        ledcWrite(BIN1, 255);
      } else if (speed > -256 && speed < 0) {
        ledcWrite(BIN2, 255);
        ledcWrite(BIN1, 255 + speed);
      } else {
        ledcWrite(BIN2, 255);
        ledcWrite(BIN1, 255);
      }
    }
};

DRV8833 motor(AIN1, AIN2, BIN1, BIN2, STBY);

void setup() {
  Serial.begin(115200);
}

//PIDの補正値計算する関数
float PIDControl(float error){
  // P成分
  float proportional = Kp * error;

  // I成分
  integral += error;
  float integralCorrection = Ki * integral;

  // D成分
  float derivative = Kd * (error - lastError);
  lastError = error;

  // 最終補正量
  float correction = (proportional + integralCorrection + derivative) * PIDvalue;
  
  return correction;
}

DoubleResult photoReflector(int s1, int s2, int s3, int s4, int s5){
  DoubleResult PhotoReflector;
  // フォトリフレクタの値読み取り
  // ライン上(黒)→ 値0
  // ライン外(白) → 値4095
  PhotoReflector.position = (s1 < threshold ? -3 : 0) +
                            (s2 < threshold ? -1 : 0) +
                            (s3 < threshold ? 0 : 0) +
                            (s4 < threshold ? 1 : 0) +
                            (s5 < threshold ? 3 : 0);

  PhotoReflector.lightCount = (s1 < threshold ? 1 : 0) + 
                              (s2 < threshold ? 1 : 0) + 
                              (s3 < threshold ?  1 : 0) + 
                              (s4 < threshold ?  1 : 0) + 
                              (s5 < threshold ?  1 : 0);

  return PhotoReflector;
}

//時間計測
unsigned long previousTime = 0;
const unsigned long interval = 10; //PID制御を回す周期
float correction;

void loop(){
  int s1 = analogRead(14);
  int s2 = analogRead(33);
  int s3 = analogRead(32);
  int s4 = analogRead(34);
  int s5 = analogRead(35);

  DoubleResult PhotoReflector = photoReflector(s1, s2, s3, s4, s5);

  unsigned long currentTime = millis();
  if (currentTime - previousTime >= interval) {
    previousTime = currentTime;

    //ズレ
    float error = (PhotoReflector.lightCount != 0) ? 
               (PhotoReflector.position / PhotoReflector.lightCount) : 0;  

    //補正値
    float correction = PIDControl(error);

    float RmotorSpeed = baseSpeed - correction;
    float LmotorSpeed = baseSpeed + correction;


    if (RmotorSpeed >= maxSpeed){
      RmotorSpeed = maxSpeed;
    } 
    if (LmotorSpeed >= maxSpeed){
      LmotorSpeed = maxSpeed;
    } 
    if (RmotorSpeed <= minSpeed){
      RmotorSpeed = minSpeed;
    } 
    if (LmotorSpeed <= minSpeed){
      LmotorSpeed = minSpeed;
    } 
  
    //モーターの回転数に補正値を反映
    motor.motorDRV8833_R(RmotorSpeed);
    motor.motorDRV8833_L(LmotorSpeed);


  }

  if (s1 > threshold && s2 > threshold && s3 > threshold && s4 > threshold && s5 > threshold) {
    Serial.println("ライン外れ");
    motor.motorDRV8833_R(0);
    motor.motorDRV8833_L(0);
  }
}