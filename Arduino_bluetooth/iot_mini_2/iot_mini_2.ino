#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

/* ====== HW 설정 ====== */
LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial BT(2, 3); // (RX, TX)  HC-06: TXD->D2, RXD->D3(분압)


// FULL 점멸용(논블로킹)
bool fullBlinkOn = false;
unsigned long lastBlinkMs = 0;
const unsigned long BLINK_INTERVAL_MS = 350; // 깜박 주기(원하면 200~600 조절)
bool isFullNow = false; // 현재 상태가 FULL인지 기억

/* RGB 핀 (PWM 권장) */
const int PIN_R = 9;
const int PIN_G = 10;
const int PIN_B = 11;

/* ====== 쓰레기통 기준 ====== */
const float BIN_EMPTY_CM_MAX   = 25.0; // 빈 상태(바닥까지 거리) 최대치
const float FULL_THRESHOLD_CM  = 4.0;  // avg < 4이면 FULL

/* 튀는 값(스파이크) 컷 */
const int SPIKE_CUT_CM = 100;          // 100 이상은 말도 안 되는 값으로 무시

/* ====== 수신 버퍼 ====== */
#define CMD_SIZE 80
char recvBuf[CMD_SIZE];

/* ====== 유틸 ====== */
void setRGB(int r, int g, int b) {
  analogWrite(PIN_R, r);
  analogWrite(PIN_G, g);
  analogWrite(PIN_B, b);
}

void updateFullBlink() {
  if (!isFullNow) return;

  unsigned long now = millis();
  if (now - lastBlinkMs >= BLINK_INTERVAL_MS) {
    lastBlinkMs = now;
    fullBlinkOn = !fullBlinkOn;

    // FULL일 때는 빨강 ON/OFF 점멸
    if (fullBlinkOn) setRGB(255, 0, 0);
    else             setRGB(0, 0, 0);
  }
}

// "TRASH@d1@d2" 파싱
bool parseTrashLine(const char* line, int &d1, int &d2) {
  const char* p = strstr(line, "TRASH@");
  if (!p) return false;
  return (sscanf(p, "TRASH@%d@%d", &d1, &d2) == 2);
}

// 0~BIN_EMPTY_CM_MAX로 클립
int clipTrash(int v) {
  if (v < 0) return 0;
  if (v > (int)BIN_EMPTY_CM_MAX) return (int)BIN_EMPTY_CM_MAX;
  return v;
}

// 퍼센트 계산: avg<3 => 100, 3~25 => 99~0
int calcPercent(float avg) {
  if (avg < FULL_THRESHOLD_CM) return 100;

  float a = avg;
  if (a > BIN_EMPTY_CM_MAX) a = BIN_EMPTY_CM_MAX;

  float ratio = (BIN_EMPTY_CM_MAX - a) / (BIN_EMPTY_CM_MAX - FULL_THRESHOLD_CM); // 25->0, 3->1
  int pct = (int)(ratio * 99.0 + 0.5);  // 0~99
  if (pct < 0) pct = 0;
  if (pct > 99) pct = 99;
  return pct;
}

// FULL / HIGH / MED / LOW
const char* calcLevel(float avg, int pct) {
  if (avg < FULL_THRESHOLD_CM || pct >= 99) return "FULL";

  // (3, 25] 3등분
  float range = (BIN_EMPTY_CM_MAX - FULL_THRESHOLD_CM) / 3.0; // (25-3)/3=7.333..
  float b1 = FULL_THRESHOLD_CM + range;       // 10.33
  float b2 = FULL_THRESHOLD_CM + 2 * range;   // 17.66

  // avg가 작을수록 더 가득 참
  if (avg <= b1) return "HIGH";       // 3~10.33
  else if (avg <= b2) return "MED";   // 10.33~17.66
  else return "LOW";                  // 17.66~25
}

// LCD/LED 업데이트 (clear 최소화 버전: 깜빡임/지연 줄임)
void applyUI(float avg, int pct, const char* level) {
  // 현재 FULL 상태 저장
  isFullNow = (!strcmp(level, "FULL"));

  // RGB 처리
  if (isFullNow) {
    // FULL은 updateFullBlink()가 점멸 처리하므로 여기서는 건드리지 않음
  } else {
    // FULL이 아니면 점멸 상태 초기화 + 고정색
    fullBlinkOn = false;
    lastBlinkMs = millis();

    if (!strcmp(level, "LOW"))      setRGB(0, 255, 0);   // Green
    else if (!strcmp(level, "MED")) setRGB(160, 160, 0); // Yellow
    else                            setRGB(255, 0, 0);   // RED
  }

  // LCD 출력
  lcd.setCursor(0, 0);
  char line1[17];
  snprintf(line1, sizeof(line1), "Fill:%3d%% %-4s", pct, level);
  lcd.print(line1);

  lcd.setCursor(0, 1);
  char line2[17];
  int avg10 = (int)(avg * 10 + 0.5);
  int avg_i = avg10 / 10;
  int avg_f = avg10 % 10;
  snprintf(line2, sizeof(line2), "Avg:%2d.%1dcm       ", avg_i, avg_f);
  lcd.print(line2);
}


void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart TrashCan ");
  lcd.setCursor(0, 1);
  lcd.print("Waiting...     ");

  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  setRGB(0, 0, 0);

  BT.begin(9600); // HC-06 기본
  Serial.println("Start. Waiting BT lines...");
}

void loop() {
  updateFullBlink();
  
  if (!BT.available()) return;

  int len = BT.readBytesUntil('\n', recvBuf, CMD_SIZE - 1);
  recvBuf[len] = '\0';

  // \r 제거
  for (int i = 0; i < len; i++) {
    if (recvBuf[i] == '\r') recvBuf[i] = '\0';
  }

  Serial.print("RX: ");
  Serial.println(recvBuf);

  int d1 = 0, d2 = 0;
  if (!parseTrashLine(recvBuf, d1, d2)) return;

  // 스파이크(100 이상)면 무시
  if (d1 >= SPIKE_CUT_CM || d2 >= SPIKE_CUT_CM) {
    return;
  }

  // 범위 클립
  d1 = clipTrash(d1);
  d2 = clipTrash(d2);

  // 평균
  float avg = (d1 + d2) / 2.0;

  // 즉시 UI 반영
  int pct = calcPercent(avg);
  const char* level = calcLevel(avg, pct);
  applyUI(avg, pct, level);
}
