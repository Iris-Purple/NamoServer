# Example12

### Stomp Allocator 학습

```cpp
class Knight
{
public:
	Knight() { cout << "Knight()" << endl; }
	~Knight() { cout << "~Knight()" << endl; }

	int _hp;
};

int main()
{
	
	Knight* test = new Knight;
	cout << "삭제 전 HP: " << test->_hp << endl;  // 100

	delete test;

	// Use After Free (UAF) 버그!
	cout << "삭제 후 HP: " << test->_hp << endl;  // 100 또는 쓰레기값
	test->_hp = 200;
	cout << "수정 후 HP: " << test->_hp << endl;  // 200 또는 크래시
}	
```

## delete 후 메모리 상태

```cpp
Knight* test = new Knight;  *// Heap에 메모리 할당*
delete test;                *// 메모리 해제 (OS에 반환)*
test->_hp = 200;            *// 해제된 메모리에 접근 (위험!)*
```

## 왜 크래시가 안 나는가?

### 1. **메모리가 즉시 회수되지 않음**

```cpp
delete test;  *// 메모리를 "사용 가능" 상태로 표시만 함// 실제 메모리 페이지는 아직 프로세스가 소유*
```

OS는 효율성을 위해 메모리를 즉시 회수하지 않고, 해당 메모리 블록을 "재사용 가능"으로 표시만 합니다.

# StompAllocator: UAF와 메모리 오버플로우 버그 잡기

## 📚 강의 개요

게임 서버 개발에서 가장 찾기 어려운 버그인 **Use After Free(UAF)**와 **메모리 오버플로우**를 즉시 탐지하는 StompAllocator를 구현해봅니다.

## 🎯 학습 목표

1. UAF 버그를 즉시 크래시로 전환하는 방법 이해
2. 메모리 오버플로우 감지 메커니즘 구현
3. Windows Virtual Memory API 활용법 학습

## 1. StompAllocator란?

**"버그를 숨기지 말고 즉시 드러내자!"**

일반 할당자는 메모리를 재사용하여 UAF 버그를 숨깁니다. StompAllocator는 **각 할당마다 새로운 메모리 페이지**를 사용하여 버그를 즉시 크래시로 만듭니다.

### 핵심 아이디어

```cpp
*// 일반 할당자: 메모리 재사용*
[사용중][해제됨(재사용가능)][사용중]

*// StompAllocator: 각 할당마다 독립된 페이지*
[페이지1: Object][페이지2: Object][페이지3: Object]
         ↓ Release
[접근 금지 페이지]  *// 접근 시 즉시 크래시!*
```

---

## 2. 구현 코드 분석

### StompAllocator 헤더

```cpp
class StompAllocator
{
    enum { PAGE_SIZE = 0x1000 };  *// 4KB (Windows 페이지 크기)*
public:
    static void* Alloc(int32 size);
    static void Release(void* ptr);
};
```

### 핵심 구현: Alloc 함수

```cpp
void* StompAllocator::Alloc(int32 size)
{
    *// 1. 필요한 페이지 개수 계산 (올림)*
    const int64 pageCount = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    
    *// 2. 오버플로우 감지를 위해 끝에 배치*
    const int64 dataOffset = pageCount * PAGE_SIZE - size;
    
    *// 3. 가상 메모리 할당*
    void* baseAddress = ::VirtualAlloc(
        NULL,                           *// 시스템이 주소 선택*
        pageCount * PAGE_SIZE,         *// 할당 크기*
        MEM_RESERVE | MEM_COMMIT,      *// 예약 + 커밋*
        PAGE_READWRITE                 *// 읽기/쓰기 가능*
    );
    
    *// 4. 데이터를 페이지 끝에 배치 (오버플로우 시 다음 페이지 접근 → 크래시)*
    return static_cast<void*>(static_cast<int8*>(baseAddress) + dataOffset);
}
```

### 메모리 레이아웃 시각화
```
size = 10 bytes 요청 시:
[=========== 4KB 페이지 ===========]
[   빈 공간 (4086 bytes)   ][데이터(10)]
                             ↑
                         반환되는 주소
                         
오버플로우 시도하면 → 다음 페이지(할당안됨) 접근 → 크래시!
```

### Release 함수 구현

```cpp
void StompAllocator::Release(void* ptr)
{
    *// 1. 포인터 주소 가져오기*
    const int64 address = reinterpret_cast<int64>(ptr);
    
    *// 2. 페이지 시작 주소 계산 (페이지 정렬)*
    const int64 baseAddress = address - (address % PAGE_SIZE);
    
    *// 3. 페이지 완전 해제 (접근 권한 제거)*
    ::VirtualFree(
        reinterpret_cast<void*>(baseAddress), 
        0,              *// 전체 해제*
        MEM_RELEASE     *// 메모리 반환*
    );
}
```

---

## 3. 버그 감지 실전 예제

### 🐛 UAF 버그 즉시 감지

```cpp
*// UAF 버그 테스트*
Knight* knight = static_cast<Knight*>(StompAllocator::Alloc(sizeof(Knight)));
new(knight) Knight();  *// placement new로 생성자 호출*

StompAllocator::Release(knight);  *// 메모리 해제 → 페이지 접근 금지*

knight->_hp = 200;  *// 💥 즉시 크래시!// Access Violation: 접근 금지된 메모리에 쓰기 시도*
```

**일반 할당자 vs StompAllocator:**

```cpp
*// 일반 new/delete*
Knight* k = new Knight();
delete k;
k->_hp = 200;  *// 😱 동작할 수도 있음 (위험!)// StompAllocator*
Knight* k = static_cast<Knight*>(StompAllocator::Alloc(sizeof(Knight)));
StompAllocator::Release(k);
k->_hp = 200;  *// 💥 100% 크래시 보장!*
```

### 🐛 메모리 오버플로우 감지

```cpp
*// 잘못된 크기로 할당 (Player 크기로 할당, Knight로 사용)*
Knight* knight = static_cast<Knight*>(
    StompAllocator::Alloc(sizeof(Player))  *// Player는 Knight보다 작음*
);

knight->_hp = 200;  *// Knight의 _hp는 Player 크기를 벗어남// 💥 크래시! (다음 페이지 접근 시도)*
```

**메모리 레이아웃:**
```
Player size = 1 byte, Knight size = 8 bytes

[======= 페이지 1 =======][======= 페이지 2 (할당안됨) =======]
[      빈공간      ][P]    [접근 금지 영역]
                    ↑
                Player 크기만큼만 할당
                
knight->_hp 접근 시 → 페이지2 접근 → 크래시!
```

### 🐛 표준 new와 비교

```cpp
*// 위험한 코드 (표준 new 사용)*
Knight* knight = (Knight*)(new Player);
knight->_hp = 20;  *// 😱 메모리 침범! 하지만 크래시 안날 수도...// 다른 객체 오염 가능*
Player* p1 = new Player();
Player* p2 = new Player();  *// p1 바로 다음에 할당*
Knight* k = (Knight*)p1;
k->_hp = 100;  *// p2 메모리 오염!*
```

---

## 4. StompAllocator 장단점

### ✅ 장점

1. **UAF 100% 감지**: 해제된 메모리 접근 시 즉시 크래시
2. **오버플로우 감지**: 할당 크기 초과 시 즉시 크래시
3. **디버깅 용이**: 크래시 덤프로 정확한 위치 파악
4. **숨겨진 버그 제거**: 우연히 동작하는 버그 방지

### ❌ 단점

1. **메모리 낭비**: 1바이트 할당도 4KB 페이지 사용
2. **성능 저하**: VirtualAlloc은 느림
3. **개발/디버그 전용**: 릴리즈에서는 사용 불가

## 5. 실전 활용 방법

### 조건부 컴파일로 활용

```cpp
#ifdef _DEBUG
    #define ALLOC(size)    StompAllocator::Alloc(size)
    #define RELEASE(ptr)   StompAllocator::Release(ptr)
#else
    #define ALLOC(size)    malloc(size)
    #define RELEASE(ptr)   free(ptr)
#endif

*// 사용 예시*
Knight* knight = static_cast<Knight*>(ALLOC(sizeof(Knight)));
new(knight) Knight();  *// placement new*
knight->~Knight();      *// 소멸자 호출*
RELEASE(knight);
```