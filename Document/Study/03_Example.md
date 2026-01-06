## 03_Example


## Vector 2개 스레드 Push 이슈

스레드 2개를 생성하여   vector push_back()  호출하면  예외 메시지 발생된다

```cpp
void example3::VectorException()
{
	for (int i = 0; i < 2; i++)
	{
		GThreadManager->Launch([]()
			{
				Push();
			});
	}

	GThreadManager->Join();
	cout << v.size() << endl;
}
```

## 왜 예외가 발생하는가?

### vector의 내부 구조

```cpp
class vector {
    T* data;        *// 실제 데이터 배열*
    size_t size;    *// 현재 원소 개수*
    size_t capacity; *// 할당된 메모리 크기*
};
```

### push_back()의 내부 동작

```cpp
void push_back(const T& value) {
    if (size == capacity) {
        *// 1. 새로운 더 큰 메모리 할당*
        T* new_data = new T[capacity * 2];
        *// 2. 기존 데이터 복사*
        copy(data, data + size, new_data);
        *// 3. 기존 메모리 해제*
        delete[] data;
        *// 4. 포인터 교체*
        data = new_data;
        capacity *= 2;
    }
    *// 5. 새 원소 추가*
    data[size++] = value;
}
```

## 동시 실행 시 발생하는 문제들

```cpp
### 문제 1: 메모리 재할당 충돌

Thread1: [capacity 체크] → [새 메모리 할당] → [복사] → [기존 메모리 해제]
Thread2:                      [capacity 체크] → [이미 해제된 메모리에 접근!] 💥`

### 문제 2: size 업데이트 경쟁

`*// 두 스레드가 동시에 같은 위치에 쓰기*
Thread1: data[10] = value1; size = 11;
Thread2: data[10] = value2; size = 11;  *// 하나의 원소가 덮어써짐!*`

### 문제 3: 댕글링 포인터

`Thread1: 재할당 중 → 기존 메모리 delete
Thread2: 이미 delete된 메모리에 push_back 시도 → 크래시!`
```

## vector 를  미리 예약 공간을 할당하면 해결될까?

v.reserve(20000);

vector 공간을 예약하면  실행시  에러는 발생 안하지만  push 데이터 사이즈가 이상하게 출력

### reserve()가 해결한 것

```cpp
v.reserve(20000);  *// capacity = 20000으로 미리 할당*
```

- ✅ 메모리 재할당 방지 → 크래시는 안 남
- ❌ 하지만 `size` 업데이트는 여전히 문제!

### push_back()의 실제 동작

```cpp
void push_back(const T& value) {
    *// reserve로 이 부분은 실행 안 됨// if (size == capacity) { ... }*
    
    *// 하지만 이 부분은 여전히 실행됨!*
    data[size] = value;  *// 1단계: 값 쓰기*
    size++;              *// 2단계: size 증가*
}
```

## 발생하는 Race Condition

### 시나리오 1: size 증가 손실

```cpp
시간 →
Thread1: [size(100) 읽기] → [data[100]에 쓰기] → [size=101로 변경]
Thread2:      [size(100) 읽기] → [data[100]에 쓰기] → [size=101로 변경]

결과: 두 스레드가 같은 위치에 덮어씀, size는 102가 아닌 101
```

### 시나리오 2: 중복 인덱스 문제

```cpp
*// 현재 size = 500일 때*
Thread1: data[500] = 1234;
Thread2: data[500] = 5678;  *// 같은 위치!*
Thread1: size = 501;
Thread2: size = 501;  *// size도 잘못됨*
```

### MUTEX  LOCK  사용하자

MutexExample  매서드 참조

## **표준화된 인터페이스**

```cpp
*// 어떤 플랫폼에서도 동일하게 동작*
std::mutex m;
m.lock();
m.unlock();
*// Windows, Linux, Mac 모두 동일한 코드*
```

unlock을 안 하면 발생하는 심각한 문제들!

### 즉각적인 문제: 데드락(Deadlock)

```cpp
std::mutex m;

void Thread1() {
    m.lock();
    *// 작업...// unlock 깜빡! 🚫*
}

void Thread2() {
    m.lock();  *// 영원히 대기... ⏳// 이 코드는 절대 실행 안 됨!*
}
```

**결과**: Thread2는 **영원히 블록**되어 프로그램이 멈춤!

### 예외 발생 시 더 위험!

```cpp
void DangerousFunction() {
    m.lock();
    
    ProcessData();  *// 여기서 예외 발생하면?*
    
    m.unlock();  *// 이 코드는 실행 안 됨!*
}

*// 예외 발생 후:// - mutex는 영원히 잠긴 상태// - 다른 모든 스레드 블록// - 프로그램 복구 불가능*
```