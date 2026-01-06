# Example10

# Read-Write Lock 구현하기: 게임 서버의 성능 최적화

## 📚 학습 목표

- Read-Write Lock의 필요성과 동작 원리 이해
- 비트 플래그를 활용한 효율적인 락 상태 관리
- 스핀락과 적응형 대기 전략 구현

## 1. 왜 Read-Write Lock이 필요한가?

게임 서버에서는 데이터 읽기가 쓰기보다 훨씬 빈번하게 발생합니다:

- 플레이어 상태 조회 (HP, 위치, 인벤토리)
- 맵 정보 읽기
- 퀘스트 진행 상황 확인

**일반 뮤텍스의 문제점:**

```cpp
*// 일반 뮤텍스 사용 시*
mutex m;
{
    lock_guard<mutex> lock(m);
    *// 단순 읽기인데도 다른 읽기 스레드를 블록함*
    return player.GetHP();  
}
```

**Read-Write Lock의 장점:**

- 여러 스레드가 동시에 읽기 가능
- 쓰기는 독점적으로 수행
- 읽기가 많은 환경에서 처리량 대폭 향상

## 2. 핵심 설계: 32비트 플래그 구조

```cpp
*/**
    32비트 atomic 변수 하나로 모든 상태 관리
    [31-16 비트: Write Thread ID] [15-0 비트: Read Count]
    
    예시:
    0x00000000 = 아무도 사용 안 함
    0x00050003 = Thread ID 5가 Write Lock, Read Count 3
    0x00000007 = 7개 스레드가 Read Lock
**/*

enum : uint32
{
    WRITE_THREAD_MASK = 0xFFFF'0000,  *// 상위 16비트 마스크*
    READ_COUNT_MASK   = 0x0000'FFFF,  *// 하위 16비트 마스크*
    EMPTY_FLAG        = 0x0000'0000   *// 초기 상태*
};
```

### 비트 연산의 장점

- 원자적 연산으로 두 정보를 동시 관리
- 캐시 친화적 (4바이트만 사용)
- CAS 연산 한 번으로 상태 변경

## 3. Write Lock 구현 분석

### 3.1 재귀적 락 지원

```cpp
void Lock::WriteLock()
{
    *// 현재 락을 소유한 스레드 ID 추출*
    const uint32 lockThreadId = (_lockFlag.load() & WRITE_THREAD_MASK) >> 16;
    
    *// 같은 스레드면 재진입 허용*
    if (LThreadId == lockThreadId)
    {
        _writeCount++;  *// 재귀 카운트 증가*
        return;
    }
    *// ...*
}
```

**재귀적 락이 필요한 경우:**

```cpp
void Player::TakeDamage(int damage)
{
    WriteLockGuard lock(_lock);
    _hp -= damage;
    
    if (_hp <= 0)
        Die();  *// Die()도 WriteLock을 요구할 수 있음*
}
```

### 3.2 스핀 후 양보 전략

```cpp
while (true)
{
    *// 1단계: 적극적 스핀 (CPU 사용)*
    for (uint32 spinCount = 0; spinCount < MAX_SPIN_COUNT; spinCount++)
    {
        uint32 expected = EMPTY_FLAG;
        if (_lockFlag.compare_exchange_strong(expected, desired))
        {
            _writeCount++;
            return;
        }
    }
    
    *// 2단계: 타임아웃 체크*
    if (::GetTickCount64() - beginTick >= ACQUIRE_TIMEOUT_TICK)
        throw std::runtime_error("LOCK_TIMEOUT");
    
    *// 3단계: CPU 양보*
    this_thread::yield();
}
```

## 4. Read Lock의 특별한 처리

### 4.1 Write Lock 소유자의 Read Lock

```cpp
void Lock::ReadLock()
{
    *// Write Lock을 가진 스레드는 Read Lock도 가능*
    const uint32 lockThreadId = (_lockFlag.load() & WRITE_THREAD_MASK) >> 16;
    if (LThreadId == lockThreadId)
    {
        _lockFlag.fetch_add(1);  *// Read Count만 증가*
        return;
    }
    *// ...*
}
```

**이유:** Write Lock 보유 = 독점적 접근권 = Read도 당연히 가능

### 4.2 경합 상황 처리

```cpp
*// Read Lock 획득 시도*
uint32 expected = (_lockFlag.load() & READ_COUNT_MASK);
if (_lockFlag.compare_exchange_strong(expected, expected + 1))
    return;  *// 성공*
```

**주의:** Write Lock이 대기 중이면 실패 → Write Starvation 방지 필요

## 5. RAII 패턴으로 안전성 보장

```cpp
class ReadLockGuard
{
public:
    ReadLockGuard(Lock& lock) : _lock(lock) 
    { 
        _lock.ReadLock(); 
    }
    
    ~ReadLockGuard() 
    { 
        _lock.ReadUnlock();  *// 예외 발생해도 자동 해제*
    }
    
private:
    Lock& _lock;
};
```

### 사용 예시

```cpp
void GameRoom::BroadcastPlayerList()
{
    ReadLockGuard lock(_playersLock);  *// 자동 획득*
    
    for (auto& player : _players)
    {
        *// 안전하게 읽기*
        SendPlayerInfo(player);
    }
    
}  *// 스코프 종료 시 자동 해제*
```