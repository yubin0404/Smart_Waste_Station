# 🗑️ Smart_Waste_Station

## 프로젝트 개요
현장의 스마트 쓰레기통(STM32)과 관리자용 관제실 대시보드(Arduino)를 분리하고, 이를 서버(MariaDB)로 연결한 **통합 IoT 폐기물 관리 시스템**입니다. 

## 기술 스택
* **MCU / Hardware:** STM32, Arduino UNO
* **Hardware & Sensors:** touch sensor, servo motor, LCD display, state check LED(red, yellow, green, RGB), Wifi module, bluetooth module, ultrasonic sensor
* **Data DB:** IoT Server, MariaDB
* **Languages:** C/C++, mariaDB

## 시스템 구조 및 역할

### 1. [현장: 스마트 쓰레기통] STM32 & Wifi
* **역할:** STM32 기반 스마트 쓰레기통
* **기능:** - 터치 센서 입력을 통한 서보모터 제어 (뚜껑 개폐)
            - LED를 통한 현재 적재량 상태 시각화
            - 2개의 초음파 센서를 통해 내부 적재량을 측정하고 Wifi 모듈을 통해 서버로 데이터 전송
### 2. [서버: 데이터 저장] IoT_Server & DataBase_MariaDB
* **역할:** 데이터 수집 및 저장
* **기능:**
        - STM32로부터 전송된 각 쓰레기통의 적재량 데이터를 MariaDB에 저장
        - 관제실에서 적재량 데이터를 확인할 수 있도록 표와 그래프 구성
  
### 3. [관제실: 모니터링 시스템] Arduino & bluetooth
* **역할:** 관제실의 관리자용 원격 대시보드
* **기능:**
        - 서버로부터 계산된 적재량 평균 데이터를 수신하여 LCD 패널에 출력
        - 특정 적재량(Full) 도달 시 관리자에게 LED 점멸을 통해 알림
   
## 트러블 슈팅
* **문제 상황:** 초음파 센서 값이 100cm 이상으로 튀는 이상치가 발생
* **원인 분석:** 다수의 센서 연결과 아날로그 신호 하드웨어 자체의 불안정성에 의한 센서 노이즈로 판단
* **한계 및 개선 계획:** 프로젝트 마감 기한으로 인해 점프선의 외부 충격 간섭을 최소한으로하는 배치로 수정하여 임시 조치
                        -> 추후 코드를 업데이트한다면 칼만 필터나 이동 평균 필터를 적용하여 튀는 값을 소프트웨어적으로 무시하고자함
