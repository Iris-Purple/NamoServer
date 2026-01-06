# Example7

# C++ Condition Variable을 이용한 동기화

## 1) 조건변수 (Condition Variable)

### 개념

조건변수는 **특정 조건이 만족될 때까지 스레드를 효율적으로 대기**시키는 동기화 메커니즘입니다. mutex와 함께 사용되며, 조건이 만족되지 않으면 lock을 해제하고 대기 상태로 들어갑니다.

### 주요 특징

- **User-Level Object**: 커널 오브젝트가 아니므로 커널 모드 전환 오버헤드가 없어 Event보다 빠릅니다
- **크로스 플랫폼**: C++ 표준 라이브러리이므로 Windows, Linux, macOS 등 모든 플랫폼에서 동작합니다
- **효율적인 대기**: 조건이 만족되지 않으면 CPU를 소비하지 않고 대기 상태로 전환됩니다
- **mutex와 결합**: 반드시 unique_lock<mutex>와 함께 사용되어 공유 자원을 보호합니다

### 핵심 메서드

- **wait(lock, predicate)**: 조건(predicate)이 true가 될 때까지 대기합니다
    - 내부에서 lock을 해제하고 대기
    - notify를 받으면 깨어나서 lock을 다시 획득
    - 조건을 재확인하여 true면 진행, false면 다시 대기
- **notify_one()**: 대기 중인 스레드 하나를 깨웁니다
- **notify_all()**: 대기 중인 모든 스레드를 깨웁니다

### Spurious Wakeup (가짜 깨어남)

조건변수는 notify 없이도 **운영체제나 시스템 신호에 의해 임의로 깨어날 수 있습니다**. 이를 Spurious Wakeup이라 하며, 이 때문에 wait에 조건식(predicate)을 반드시 사용해야 합니다.

```cpp
*// ❌ 잘못된 사용 - Spurious Wakeup 대응 불가*
cv.wait(lock);
if (q.empty() == false) { ... }

*// ✅ 올바른 사용 - 깨어날 때마다 조건 재확인*
cv.wait(lock, []() { return q.empty() == false; });
```

notify_one()을 했어도 조건식이 필요한 이유:

1. **Spurious Wakeup**: 시스템이 임의로 깨울 수 있음
2. **다중 Consumer**: 여러 Consumer가 있다면 먼저 깨어난 스레드가 데이터를 가져갈 수 있음
3. **안정성**: 깨어났을 때 조건이 여전히 유효한지 확인

## 2) 코드 설명

### Producer 스레드

```cpp
void Producer()
{
    while (true)
    {
        *// 1) Lock을 잡고 공유 변수(큐) 수정*
        {
            unique_lock<mutex> lock(m);
            q.push(100);
        }  *// 2) 스코프 종료로 Lock 자동 해제*
        
        *// 3) 조건변수로 대기 중인 Consumer에게 통지*
        cv.notify_one();  *// 대기 중인 스레드 1개를 깨움*
        
        this_thread::sleep_for(chrono::milliseconds(10));
    }
}
```

**중요 포인트**: notify_one()은 lock 밖에서 호출하는 것이 좋습니다. lock 안에서 호출하면 깨어난 스레드가 즉시 lock을 얻지 못해 불필요한 컨텍스트 스위칭이 발생할 수 있습니다.

### Consumer 스레드

```cpp
void Consumer()
{
    while (true)
    {
        unique_lock<mutex> lock(m);  *// 1) Lock 획득*
        
        *// 2) 조건 확인 후 대기*
        cv.wait(lock, []() { return q.empty() == false; });
        
        *// wait 내부 동작:// - 조건식(람다)이 false면: lock을 해제하고 대기 상태// - notify를 받으면: 깨어나서 lock 재획득, 조건식 재확인// - 조건식이 true면: wait를 빠져나와 아래 코드 실행*
        
        *// 3) 조건이 만족되어 빠져나왔으므로 데이터 소비*
        int data = q.front();
        q.pop();
        cout << data << endl;
        
    }  *// 4) 스코프 종료로 lock 자동 해제*
}
```

### cv.wait() 내부 동작 흐름
```
1. lock을 잡은 상태로 진입
2. 조건식 확인: q.empty() == false?
   - true → 즉시 반환 (데이터가 있음)
   - false → 3번으로
3. lock을 해제하고 대기 큐에 진입
4. notify_one() 호출 또는 Spurious Wakeup 발생
5. 깨어나서 lock 재획득 시도
6. lock 획득 성공 후 조건식 재확인 (2번으로)
```

### 동작 시나리오

**정상 케이스**:

1. Consumer가 `cv.wait()`에서 대기 (큐가 비어있음)
2. Producer가 데이터를 큐에 추가하고 `notify_one()` 호출
3. Consumer가 깨어나서 조건식 확인 → `q.empty() == false` (true)
4. Consumer가 데이터를 소비

**Spurious Wakeup 케이스**:

1. Consumer가 대기 중
2. 시스템 신호로 임의로 깨어남 (notify 없이)
3. 조건식 확인 → `q.empty() == false` (false)
4. 다시 대기 상태로 진입

이처럼 조건식을 통해 **안전하게** 조건이 만족될 때만 진행할 수 있습니다.