# 네트워크 보안 가이드

> 게임 서버의 4가지 보안 메커니즘: AES-128 암호화, HMAC, Rate Limiting, Sequence 검증

---

## 보안 위협과 대응

```
┌─────────────────────────────────────────────────────────────────────────┐
│                      보안 위협 vs 대응 메커니즘                          │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌─────────────────────┐     ┌─────────────────────┐                   │
│  │ 도청 (Eavesdropping)│ ──▶ │ AES-128 암호화       │                   │
│  │ 패킷 내용 엿보기    │     │ 내용을 읽을 수 없게  │                   │
│  └─────────────────────┘     └─────────────────────┘                   │
│                                                                          │
│  ┌─────────────────────┐     ┌─────────────────────┐                   │
│  │ 변조 (Tampering)    │ ──▶ │ HMAC 검증            │                   │
│  │ 패킷 내용 수정      │     │ 무결성 확인          │                   │
│  └─────────────────────┘     └─────────────────────┘                   │
│                                                                          │
│  ┌─────────────────────┐     ┌─────────────────────┐                   │
│  │ DoS 공격            │ ──▶ │ Rate Limiting        │                   │
│  │ 패킷 폭탄           │     │ 초당 요청 제한       │                   │
│  └─────────────────────┘     └─────────────────────┘                   │
│                                                                          │
│  ┌─────────────────────┐     ┌─────────────────────┐                   │
│  │ 리플레이 공격       │ ──▶ │ Sequence 검증        │                   │
│  │ 패킷 재전송         │     │ 중복 패킷 차단       │                   │
│  └─────────────────────┘     └─────────────────────┘                   │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 1. AES-128 암호화

### 개념

```
평문 패킷:
┌────────────────────────────────────────────────────┐
│ [헤더] [패킷ID: 이동] [X: 100] [Y: 200]            │  ← 해커가 읽을 수 있음!
└────────────────────────────────────────────────────┘

암호화 패킷:
┌────────────────────────────────────────────────────┐
│ [헤더] [aK#9xZ!mQ2...암호화된 데이터...]           │  ← 읽을 수 없음!
└────────────────────────────────────────────────────┘
```

### AES-128-CBC 동작

**파일:** `ServerCore/AESCrypto.cpp`

```cpp
// 초기화
bool AESCrypto::Init(const BYTE* key, int32 keyLen)
{
    if (keyLen != 16)  // AES-128은 16바이트(128비트) 키 필요
        return false;

    // AES 알고리즘 열기 (Windows CNG API 사용)
    BCryptOpenAlgorithmProvider(&_hAlgorithm, BCRYPT_AES_ALGORITHM, nullptr, 0);

    // CBC 모드 설정
    BCryptSetProperty(_hAlgorithm, BCRYPT_CHAINING_MODE,
        (PBYTE)BCRYPT_CHAIN_MODE_CBC, sizeof(BCRYPT_CHAIN_MODE_CBC), 0);

    // 대칭 키 생성
    BCryptGenerateSymmetricKey(_hAlgorithm, &_hKey, _keyObject,
        _keyObjectSize, (PBYTE)key, keyLen, 0);

    // IV(초기화 벡터) 설정
    memcpy(_ivBackup, key, 16);

    return true;
}
```

### 패킷 암호화 과정

```
원본 패킷:
┌────────┬────────────────────────────────────┐
│  size  │  id + flags + sequence + payload   │
│ 2bytes │           (평문)                   │
└────────┴────────────────────────────────────┘
    │
    │  AES-128 암호화 (id+flags+seq+payload 부분만)
    ▼
┌────────┬────────────────────────────────────┬──────────┐
│  size  │        암호화된 데이터              │   HMAC   │
│ 2bytes │         (AES-CBC)                  │ 32bytes  │
└────────┴────────────────────────────────────┴──────────┘
```

### 암호화 코드

**파일:** `ServerCore/Session.cpp:327-377`

```cpp
SendBufferRef Session::EncryptBuffer(SendBufferRef sendBuffer)
{
    int32 plainSize = sendBuffer->WriteSize();
    int32 payloadSize = plainSize - sizeof(uint16);  // size 제외

    // 암호화된 크기 계산 (16바이트 블록 패딩)
    int32 encryptedPayloadSize = AESCrypto::GetEncryptedSize(payloadSize);

    // 새 버퍼: [size(2)] + [암호화된 데이터] + [HMAC(32)]
    int32 totalSize = sizeof(uint16) + encryptedPayloadSize + HMAC_SIZE;
    SendBufferRef encryptedBuffer = make_shared<SendBuffer>(totalSize);

    // id+flags+seq+payload 암호화
    _crypto->Encrypt(
        sendBuffer->Buffer() + sizeof(uint16),  // 원본 (size 제외)
        payloadSize,
        bufferPtr + sizeof(uint16),             // 출력 위치
        encryptedPayloadSize
    );

    // HMAC 추가
    _crypto->ComputeHMAC(encryptedData, resultLen, hmacPos);

    return encryptedBuffer;
}
```

### 설정

**파일:** `GameServer/config.json`

```json
{
    "encryptionEnabled": true,
    "encryptionKey": "NamoServerKey123"  // 정확히 16바이트!
}
```

---

## 2. HMAC (Hash-based Message Authentication Code)

### 개념

```
HMAC = 비밀키로 생성한 "패킷 지문"

해커가 패킷 변조 시도:
┌────────────────────────────────────────────────────────────────────────┐
│                                                                         │
│  원본: [암호화된 데이터: 0xAB12...] [HMAC: 0x7F3E...]                   │
│                    │                       │                            │
│                    ▼                       │                            │
│  변조: [암호화된 데이터: 0xAB13...]        │ ← 1바이트 변경             │
│                    │                       │                            │
│                    └───────────────────────┘                            │
│                              │                                          │
│                              ▼                                          │
│  서버: HMAC 다시 계산 → 0x9D21... ≠ 0x7F3E...                          │
│                              │                                          │
│                              ▼                                          │
│                     "변조 감지! 패킷 폐기"                              │
│                                                                         │
└────────────────────────────────────────────────────────────────────────┘
```

### HMAC-SHA256 구현

**파일:** `ServerCore/AESCrypto.cpp:168-223`

```cpp
bool AESCrypto::ComputeHMAC(const BYTE* data, int32 dataLen, BYTE* outHmac)
{
    // HMAC-SHA256 알고리즘으로 해시 생성
    BCryptCreateHash(
        _hHmacAlgorithm,
        &hHash,
        hashObject,
        hashObjectSize,
        (PBYTE)_hmacKey,  // 비밀키 (AES 키와 동일)
        16,
        0
    );

    // 데이터 해시
    BCryptHashData(hHash, (PBYTE)data, dataLen, 0);

    // 결과 추출 (32바이트)
    BCryptFinishHash(hHash, outHmac, HMAC_SIZE, 0);

    return true;
}
```

### HMAC 검증 (타이밍 공격 방지)

```cpp
bool AESCrypto::VerifyHMAC(const BYTE* data, int32 dataLen, const BYTE* expectedHmac)
{
    BYTE calculatedHmac[HMAC_SIZE];
    ComputeHMAC(data, dataLen, calculatedHmac);

    // ⚠️ 상수 시간 비교 (타이밍 공격 방지)
    int32 diff = 0;
    for (int32 i = 0; i < HMAC_SIZE; i++)
    {
        diff |= (calculatedHmac[i] ^ expectedHmac[i]);
    }

    return (diff == 0);
}
```

### 타이밍 공격이란?

```
❌ 취약한 비교:
if (memcmp(calculated, expected, 32) == 0)  // 첫 번째 다른 바이트에서 종료

해커 시도:
  HMAC[0] = 0x00 → 즉시 실패 (빠름)
  HMAC[0] = 0x7F → 다음 바이트 비교 (조금 느림)
  → 시간 차이로 정답 추측 가능!

✅ 안전한 비교 (상수 시간):
for (int i = 0; i < 32; i++)
    diff |= (a[i] ^ b[i]);  // 항상 32번 비교
return (diff == 0);
```

---

## 3. Rate Limiting (속도 제한)

### 개념

```
악성 클라이언트:
"이동 패킷 1초에 1000개 보내서 서버 죽여야지!"
    │
    ▼
┌─────────────────────────────────────────────────────┐
│  Rate Limiter                                        │
│                                                      │
│  규칙: 이동 패킷 = 초당 10개까지                     │
│                                                      │
│  1~10번째: ✅ 통과                                   │
│  11번째~: ❌ 차단 → 연결 끊김                        │
└─────────────────────────────────────────────────────┘
```

### Token Bucket 알고리즘

**파일:** `GameServer/RateLimiter.cpp`

```cpp
class TokenBucket
{
public:
    TokenBucket(int32 maxPerSecond = 10);

    bool TryConsume()
    {
        Refill();  // 1초마다 토큰 리필

        if (_tokens > 0)
        {
            _tokens--;
            return true;   // 통과
        }
        return false;      // 차단
    }

private:
    void Refill()
    {
        uint64 now = ::GetTickCount64();
        uint64 elapsed = now - _lastRefillTime;

        if (elapsed >= 1000)  // 1초 경과
        {
            _tokens = _maxTokens;  // 토큰 리필
            _lastRefillTime = now;
        }
    }

    int32  _tokens;         // 현재 토큰 수
    int32  _maxTokens;      // 최대 토큰 수 (= 초당 허용 횟수)
    uint64 _lastRefillTime; // 마지막 리필 시간
};
```

### 동작 시뮬레이션

```
초당 10개 제한 (maxTokens = 10)

시간     요청    토큰     결과
─────────────────────────────────────
0.0초    -       10       (초기 상태)
0.1초    1번째   9        ✅ 통과
0.2초    2번째   8        ✅ 통과
0.3초    3번째   7        ✅ 통과
...
0.9초    9번째   1        ✅ 통과
0.95초   10번째  0        ✅ 통과
0.96초   11번째  0        ❌ 차단!
─────────────────────────────────────
1.0초    리필    10       (토큰 복구)
1.1초    1번째   9        ✅ 통과
```

### 적용 코드

**파일:** `GameServer/GameSession.cpp`

```cpp
void GameSession::OnConnected()
{
    // 패킷별 Rate Limit 규칙 등록
    _rateLimiter.AddRule(PKT_C2S_MOVE, 10);   // 이동: 초당 10회
    // 필요시 추가
    // _rateLimiter.AddRule(PKT_C2S_SKILL, 5);  // 스킬: 초당 5회
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

    // Rate Limit 체크
    if (_rateLimiter.CheckRateLimit(header->id) == false)
    {
        cout << "[RateLimit] Exceeded! PacketId=" << header->id << endl;
        Disconnect(L"Rate limit exceeded");
        return;
    }

    // 통과하면 정상 처리
    ServerPacketHandler::HandlePacket(session, buffer, len);
}
```

---

## 4. Sequence 검증 (리플레이 공격 방지)

### 리플레이 공격이란?

```
정상 플레이어:
  [스킬 사용, seq=5] ─────────────────────────▶ 서버
                                                 │
해커 (도청):                                     │
  [스킬 사용, seq=5] 캡처!                       ▼
                                            스킬 발동
해커 (5분 후 재전송):
  [스킬 사용, seq=5] ─────────────────────────▶ 서버
                                                 │
                                                 ▼
                                        "seq=5는 이미 처리함!
                                         리플레이 공격이다!"
                                                 │
                                                 ▼
                                              차단!
```

### Sequence 동작 원리

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        Sequence 번호 흐름                                │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  클라이언트                        서버                                  │
│       │                              │                                   │
│  seq=1│ ─── [스킬 사용] ───────────▶ │ _recvSeq=0                       │
│       │                              │ 1 > 0? ✅ OK                      │
│       │                              │ _recvSeq = 1                      │
│       │                              │                                   │
│  seq=2│ ─── [스킬 사용] ───────────▶ │ _recvSeq=1                       │
│       │                              │ 2 > 1? ✅ OK                      │
│       │                              │ _recvSeq = 2                      │
│       │                              │                                   │
│  seq=1│ ─── [리플레이!] ───────────▶ │ _recvSeq=2                       │
│       │                              │ 1 > 2? ❌ 리플레이!              │
│       │                              │                                   │
│  seq=2│ ─── [재전송?] ────────────▶ │ _recvSeq=2                       │
│       │                              │ 2 == 2? ⚠️ 동일 seq              │
│       │                              │ → 캐시된 응답 재전송             │
│       │                              │                                   │
└─────────────────────────────────────────────────────────────────────────┘
```

### 구현 코드

**파일:** `ServerCore/Session.cpp:480-499`

```cpp
// 패킷 수신 시 Sequence 검증
if (header->flags & PKT_FLAG_HAS_SEQUENCE)
{
    if (header->sequence == _recvSeq)
    {
        // 동일 시퀀스 = 재전송 요청
        // → 캐시된 응답 다시 보내기
        if (_lastResponse)
            Send(_lastResponse);
        continue;
    }
    else if (header->sequence < _recvSeq)
    {
        // 이전 시퀀스 = 리플레이 공격!
        cout << "Replay attack detected: seq=" << header->sequence
             << ", lastSeq=" << _recvSeq << endl;
        return -1;  // 연결 끊김
    }

    _recvSeq = header->sequence;  // 시퀀스 갱신
}
```

**파일:** `ServerCore/Session.cpp:34-37` (송신 시)

```cpp
// 패킷 송신 시 Sequence 설정
if (header->flags & PKT_FLAG_HAS_SEQUENCE)
{
    _lastResponse = sendBuffer;    // 재전송용 캐시
    header->sequence = ++_sendSeq; // 시퀀스 증가
}
```

### Sequence 필요 패킷 정의

**파일:** `GameServer/ServerPacketHandler.h:27-36`

```cpp
inline bool NeedsSequence(uint16 packetId)
{
    switch (packetId)
    {
    case PKT_C2S_SKILL:  // 스킬 사용은 Sequence 필요
        return true;
    default:
        return false;
    }
}
```

### 왜 모든 패킷에 Sequence를 안 쓰는가?

| 패킷 종류 | Sequence 필요? | 이유 |
|----------|----------------|------|
| 스킬 사용 | ✅ 필요 | 중복 발동 방지 (아이템 복사 방지) |
| 이동 | ❌ 불필요 | 최신 위치만 중요, 중복되어도 무해 |
| 채팅 | ❌ 불필요 | 중복되어도 큰 문제 없음 |

---

## 보안 계층 전체 흐름

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        패킷 수신 보안 검증 흐름                          │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  클라이언트 패킷 도착                                                    │
│          │                                                               │
│          ▼                                                               │
│  ┌───────────────────┐                                                  │
│  │ 1. HMAC 검증      │  암호화된 데이터 + HMAC                          │
│  │    (무결성)       │                                                   │
│  └─────────┬─────────┘                                                  │
│            │ ✅ 통과                                                     │
│            ▼                                                             │
│  ┌───────────────────┐                                                  │
│  │ 2. AES 복호화     │  암호화 데이터 → 평문                            │
│  │    (기밀성)       │                                                   │
│  └─────────┬─────────┘                                                  │
│            │ ✅ 통과                                                     │
│            ▼                                                             │
│  ┌───────────────────┐                                                  │
│  │ 3. Sequence 검증  │  seq > lastSeq 확인                              │
│  │    (리플레이방지)  │                                                   │
│  └─────────┬─────────┘                                                  │
│            │ ✅ 통과                                                     │
│            ▼                                                             │
│  ┌───────────────────┐                                                  │
│  │ 4. Rate Limiting  │  초당 요청 수 확인                                │
│  │    (DoS 방지)     │                                                   │
│  └─────────┬─────────┘                                                  │
│            │ ✅ 통과                                                     │
│            ▼                                                             │
│  ┌───────────────────┐                                                  │
│  │ 5. 패킷 핸들러    │  정상 처리                                        │
│  │    실행           │                                                   │
│  └───────────────────┘                                                  │
│                                                                          │
│  ❌ 어느 단계든 실패 시 → 연결 끊김 (Disconnect)                         │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 코드 위치 정리

| 보안 기능 | 파일 | 주요 함수 |
|----------|------|----------|
| AES 암호화 | `ServerCore/AESCrypto.cpp` | `Encrypt()`, `Decrypt()` |
| HMAC 검증 | `ServerCore/AESCrypto.cpp` | `ComputeHMAC()`, `VerifyHMAC()` |
| 암호화 적용 | `ServerCore/Session.cpp` | `EncryptBuffer()` |
| 복호화 적용 | `ServerCore/Session.cpp` | `PacketSession::OnRecv()` |
| Rate Limiting | `GameServer/RateLimiter.cpp` | `TryConsume()`, `CheckRateLimit()` |
| Rate Limit 적용 | `GameServer/GameSession.cpp` | `OnRecvPacket()` |
| Sequence 검증 | `ServerCore/Session.cpp` | `PacketSession::OnRecv()` |
| 암호화 설정 | `GameServer/config.json` | `encryptionEnabled`, `encryptionKey` |

---

## 요약

| 보안 기능 | 방어 대상 | 핵심 원리 |
|----------|----------|----------|
| **AES-128** | 도청 | 비밀키로 패킷 내용 암호화 |
| **HMAC** | 변조 | 패킷 지문으로 무결성 확인 |
| **Rate Limiting** | DoS | Token Bucket으로 요청 속도 제한 |
| **Sequence** | 리플레이 | 증가하는 번호로 중복 패킷 차단 |

---

> 문서 작성일: 2026-01-08
> 대상 프로젝트: NamoServer
