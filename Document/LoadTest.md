# 서버 부하 테스트

## 개요

이 문서는 게임 서버의 부하 테스트 방법과 결과 해석 방법을 설명합니다.

---

## 서버 상태 모니터링

서버는 **5초 주기**로 Job 처리 상태를 출력합니다.

```
[Jobs] Wait: 1ms (max: 5ms) | Process: 2ms (max: 10ms) | Count: 5000
```

### 지표 설명

| 지표 | 설명 |
|------|------|
| **Wait** | 평균 대기 시간 (Job 생성 ~ 실행 시작) |
| **Wait max** | 측정 기간 중 가장 오래 대기한 Job |
| **Process** | 평균 처리 시간 (Execute 시작 ~ 종료) |
| **Process max** | 측정 기간 중 가장 오래 걸린 Job |
| **Count** | 측정 기간 동안 처리된 총 Job 개수 |

---

## 지표 상세 설명

### 1. Wait (대기 시간)

```
Job 생성 시점 ──────────────────────────▶ Job 실행 시작 시점
      │                                         │
      │◄─────────── Wait Time ─────────────────▶│
      │                                         │
   DoAsync() 호출                        Execute() 시작
```

- **의미**: Job이 큐에서 대기한 시간
- **높을 경우**: Worker Thread가 부족하거나 Job이 과도하게 많음

### 2. Process (처리 시간)

```
Job 실행 시작 ──────────────────────────▶ Job 실행 완료
      │                                         │
      │◄─────────── Process Time ──────────────▶│
      │                                         │
   Execute() 시작                        Execute() 종료
```

- **의미**: Job 하나를 처리하는 데 걸린 실제 시간
- **높을 경우**: Job 내부 로직이 무겁거나 I/O 블로킹 발생

### 3. Count (처리 횟수)

- **의미**: 측정 기간(5초) 동안 처리된 총 Job 개수
- **서버 처리량(Throughput)** 을 나타내는 핵심 지표

---

## 지표 읽는 법

```
[Jobs] Wait: 2ms (max: 15ms) | Process: 1ms (max: 8ms) | Count: 1523
         │          │              │          │              │
         │          │              │          │              └── 1523개 Job 처리됨
         │          │              │          │
         │          │              │          └── 가장 오래 걸린 Job: 8ms
         │          │              │
         │          │              └── 평균 처리 시간: 1ms
         │          │
         │          └── 가장 오래 대기한 Job: 15ms
         │
         └── 평균 대기 시간: 2ms
```

---

## 부하 테스트 조건

### 클라이언트 설정
- **동시 접속자**: 500명
- **행동 주기**: 0.5초마다 이동 또는 공격

---

## 테스트 결과

### TEST CASE 1: 암호화 OFF

| 설정 항목 | 값 |
|-----------|-----|
| encryptionEnabled | `false` |
| workerThread | 20 |
| roomCount | 5 |
| monsterCount | 0 |

**결과**:
![Image](https://github.com/user-attachments/assets/f9dcf084-fa9a-4111-87ae-aae2339d7481)

| 항목 | 결과 |
|------|------|
| 클라이언트 상태 | 정상 (탈락 0명) |
| 평균 대기 시간 | 2288ms |
| 최대 대기 시간 | 12828ms |
| 평균 처리 시간 | 7ms |
| 최대 처리 시간 | 2016ms |
| 처리량 | 3538 jobs/5sec |

---

### TEST CASE 2: 암호화 ON

| 설정 항목 | 값 |
|-----------|-----|
| encryptionEnabled | `true` |
| workerThread | 20 |
| roomCount | 5 |
| monsterCount | 0 |

**결과**:
![Image](https://github.com/user-attachments/assets/118fc6c4-53d4-4fa8-9913-7f610c7bc333)

| 항목 | 결과 |
|------|------|
| 클라이언트 상태 | **4명 탈락** |
| 평균 대기 시간 | 3063ms |
| 최대 대기 시간 | 13234ms |
| 평균 처리 시간 | 8ms |
| 최대 처리 시간 | 1890ms |
| 처리량 | 2989 jobs/5sec |

---

### TEST CASE 3: 암호화 ON + 몬스터

| 설정 항목 | 값 |
|-----------|-----|
| encryptionEnabled | `true` |
| workerThread | 20 |
| roomCount | 5 |
| monsterCount | 3 |

**결과**:
![Image](https://github.com/user-attachments/assets/b1a4ce62-d05e-41bd-aaf3-b36d4d09b23a)

| 항목 | 결과 |
|------|------|
| 클라이언트 상태 | **4명 탈락** |
| 평균 대기 시간 | 2330ms |
| 최대 대기 시간 | 13000ms |
| 평균 처리 시간 | 7ms |
| 최대 처리 시간 | 1610ms |
| 처리량 | 3289 jobs/5sec |

---

## 테스트 결과 비교

| 항목 | CASE 1 | CASE 2 | CASE 3 |
|------|--------|--------|--------|
| 암호화 | OFF | ON | ON |
| 몬스터 | 0 | 0 | 3 |
| 탈락자 | 0명 | 4명 | 4명 |
| Wait 평균 | 2288ms | 3063ms | 2330ms |
| Wait 최대 | 12828ms | 13234ms | 13000ms |
| Process 평균 | 7ms | 8ms | 7ms |
| Count | 3538 | 2989 | 3289 |

