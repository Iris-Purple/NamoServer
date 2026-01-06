# Example6

### SPIN LOCK   VS  Mutex

스핀락과 뮤텍스는 둘 다 공유 자원에 대한 동시 접근을 막는 동기화 기법이지만, 
락을 얻지 못한 스레드에 대한 처리 방식이 다릅니다. 
**스핀락은 대기하는 동안 CPU를 계속 사용하며(스핀) 락을 확인하고, 뮤텍스는 락이 풀릴 때까지 대기하며 운영체제에 CPU를 양보하고 sleep 상태가 됩니다**. 
따라서 락이 짧은 시간만 유지될 경우 스핀락이 더 효율적일 수 있지만, 락이 길게 유지될 경우 뮤텍스가 CPU 자원을 낭비하지 않아 더 효율적입니다. 

## example6  예제 코드 참고
compare_exchange_strong 이해하기

```cpp
cppbool expected = false;  // "잠금이 풀려있길 기대"
bool desired = true;    // "잠금을 걸고 싶다"

// compare_exchange_strong(expected, desired)의 동작:
if (_locked == expected) {  // 현재 값이 expected와 같으면
    _locked = desired;      // desired로 바꾸고
    return true;           // 성공!
} else {
    expected = _locked;    // 실패하면 expected를 현재 값으로 업데이트
    return false;         // 실패!
}
```

```cpp
int32 sum = 0;
spinLock spLock;

void Add()
{
	for (int32 i = 0; i < 100000; i++)
	{
		lock_guard<spinLock> guard(spLock);
		sum++;
	}
}
void Sub()
{
	for (int32 i = 0; i < 100000; i++)
	{
		lock_guard<spinLock> guard(spLock);
		sum--;
	}
}

```

### **SpinLock의 장단점**

```cpp
*// ✅ 장점: 짧은 Critical Section에 매우 빠름*
void GoodUseCase() {
    lock_guard<SpinLock> guard(spinLock);
    counter++;  *// 아주 짧은 작업 - SpinLock 적합!*
}

*// ❌ 단점: 긴 작업에는 CPU 낭비*
void BadUseCase() {
    lock_guard<SpinLock> guard(spinLock);
    Sleep(100);  *// 긴 작업 - SpinLock 부적합!// 다른 스레드가 100ms 동안 CPU 100% 사용!*
}
```

### **CPU 사용률 차이**

```cpp
`*// SpinLock 대기 중*
void SpinLockWaiting() {
    while (못 얻음) {
        *// CPU: 100% // 계속 확인, 확인, 확인...*
    }
}

*// Mutex 대기 중*
void MutexWaiting() {
    if (못 얻음) {
        *// CPU: 0% // OS가 스레드를 재움*
    }
}`
```