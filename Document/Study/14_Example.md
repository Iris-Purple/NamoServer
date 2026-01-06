# Exampel14

## 1. 메모리 풀이란?

메모리 풀(Memory Pool)은 **미리 할당해둔 메모리 블록들의 저장소**입니다.

`일반 할당 방식:              메모리 풀 방식:
필요할 때마다 OS에 요청 →     미리 준비된 메모리 사용
(느림)                       (빠름)

[프로그램] → [OS] → [메모리]   [프로그램] → [메모리 풀] → [재사용]`

### 비유로 이해하기

`일반 메모리 할당 = 매번 마트에 가서 물 사오기
메모리 풀      = 냉장고에 물 미리 채워두고 쓰기`

---

## 2. 왜 메모리 풀이 필요한가?

### 게임 서버의 특징

`게임 서버는 초당 수천~수만 개의 객체를 생성/삭제합니다:
- 플레이어 이동 패킷
- 몬스터 생성/제거
- 스킬 이펙트
- 아이템 드랍`

### 일반 할당의 문제점

### **성능 문제**

```cpp
*// 나쁜 예: 매 프레임마다 수천 번 실행*
for(int i = 0; i < 1000; i++) {
    Monster* monster = new Monster();  *// OS에 메모리 요청 (느림!)// ... 처리 ...*
    delete monster;  *// OS에 메모리 반환 (느림!)*
}
```

### 2️⃣ **메모리 단편화**

`시간이 지날수록 메모리가 조각나서 비효율적:
[사용][빈공간][사용][빈공간][사용] ← 낭비되는 공간`

### ✅ 메모리 풀의 해결책

```cpp
*// 좋은 예: 메모리 풀 사용*
for(int i = 0; i < 1000; i++) {
    Monster* monster = xnew<Monster>();  *// 풀에서 즉시 가져옴 (빠름!)// ... 처리 ...*
    xdelete(monster);  *// 풀에 반환 (재사용 가능!)*
}
```

---

## 3. 시스템 구조 이해하기

### 전체 아키텍처

`┌─────────────────────────────────────┐
│         사용자 코드 (main)           │
│   xnew<Knight>() / xdelete()        │
└────────────┬────────────────────────┘
             │
┌────────────▼────────────────────────┐
│      Allocator (인터페이스)          │
│   xalloc() / xrelease()             │
└────────────┬────────────────────────┘
             │
┌────────────▼────────────────────────┐
│      Memory (관리자)                 │
│   여러 크기의 메모리 풀 관리          │
└────────────┬────────────────────────┘
             │
┌────────────▼────────────────────────┐
│      MemoryPool (실제 풀)           │
│   32바이트 풀, 64바이트 풀...        │
└─────────────────────────────────────┘`

### 메모리 풀 크기 전략

`크기별 풀 구성:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
32~1024 바이트:  32 단위로 증가 (세밀한 관리)
   [32][64][96][128]...[1024]
   
1025~2048 바이트: 128 단위로 증가 (중간 크기)
   [1152][1280][1408]...[2048]
   
2049~4096 바이트: 256 단위로 증가 (큰 크기)
   [2304][2560][2816]...[4096]
   
4096 초과: 일반 할당 사용
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━`

---

## 4. 핵심 컴포넌트 분석

### 1. MemoryHeader - 메모리 블록의 메타데이터

```cpp
struct MemoryHeader : public SLIST_ENTRY {
    int32 allocSize;  *// 할당된 크기 정보*
    
    *// 실제 메모리 구조:// [MemoryHeader][실제 데이터]//      ↑            ↑//   메타데이터    사용자가 쓰는 부분*
};
```

**동작 원리:**

`할당 시: [Header 붙이기] → [데이터 영역 반환]
해제 시: [Header 찾기] → [전체 블록 풀에 반환]`

### 2. MemoryPool - 단일 크기 메모리 풀

```cpp
class MemoryPool {
    SLIST_HEADER _header;     *// Windows의 Lock-Free 리스트*
    int32 _allocSize;         *// 이 풀이 관리하는 블록 크기*
    atomic<int32> _allocCount; *// 할당된 블록 수 (디버깅용)*
};
```

**Lock-Free 리스트란?**

`여러 스레드가 동시에 접근해도 안전한 자료구조
멀티스레드 게임 서버에 필수!

Thread 1: Push() ─┐
Thread 2: Pop()  ─┼→ 충돌 없이 동시 처리!
Thread 3: Push() ─┘`

### 3. Memory - 전체 메모리 관리자

```cpp
class Memory {
    vector<MemoryPool*> _pools;           *// 모든 풀 목록*
    MemoryPool* _poolTable[MAX_ALLOC_SIZE + 1]; *// 빠른 검색 테이블*
};
```

**빠른 검색 테이블의 비밀:**

`요청 크기 → 즉시 적절한 풀 찾기 (O(1))

_poolTable[100] → 100바이트용 풀
_poolTable[256] → 256바이트용 풀
...`

---

## 5. 실제 동작 과정

### 메모리 할당 과정 (xnew)

```cpp
Knight* knight = xnew<Knight>();
```

**단계별 분석:**

`1. sizeof(Knight) 계산 → 예: 100바이트
   ↓
2. Memory::Allocate(100) 호출
   ↓
3. 실제 필요 크기 = 100 + sizeof(MemoryHeader)
   ↓
4. _poolTable[실제크기]로 적절한 풀 찾기
   ↓
5. Pool에서 Pop() → 재사용 가능한 블록 있으면 반환
                  → 없으면 새로 생성
   ↓
6. MemoryHeader 붙이기
   ↓
7. placement new로 Knight 생성자 호출
   ↓
8. Knight 포인터 반환`

### 메모리 해제 과정 (xdelete)

```cpp
xdelete(knight);
```

**단계별 분석:**

`1. Knight 소멸자 호출
   ↓
2. Memory::Release() 호출
   ↓
3. MemoryHeader 찾기 (포인터 - sizeof(Header))
   ↓
4. Header에서 allocSize 읽기
   ↓
5. 적절한 Pool 찾기
   ↓
6. Pool에 Push() → 재사용을 위해 보관`

---

## 6. 코드 흐름 따라가기

### 실전 예제: Knight 생성부터 삭제까지

```cpp
*// GameServer.cpp*
int main() {
    *// 1. 프로그램 시작 시 자동 초기화// CoreGlobal 생성자 → Memory 객체 생성 → 모든 풀 준비*
    
    *// 2. Knight 객체 생성*
    Knight* knight = xnew<Knight>();
    *// xnew 매크로 → xalloc → Memory::Allocate → Pool::Pop*
    
    *// 3. 사용*
    cout << knight->_hp << endl;
    
    *// 4. Knight 객체 삭제*
    xdelete(knight);
    *// xdelete 매크로 → 소멸자 → xrelease → Memory::Release → Pool::Push*
}
```

### 디버깅 포인트

```cpp
*// MemoryPool::Pop()*
if (memory == nullptr) {
    *// 풀이 비어있음 → 새로 생성*
    memory = reinterpret_cast<MemoryHeader*>(
        ::_aligned_malloc(_allocSize, SLIST_ALIGNMENT)
    );
} else {
    *// 재사용 가능한 블록 있음*
    ASSERT(memory->allocSize == 0); *// 안전 체크*
}
```

---

## 7. 성능 최적화 포인트

### 1. Lock-Free 자료구조

```cpp
*// Windows의 SList 사용 - 원자적 연산*
::InterlockedPushEntrySList(&_header, ptr);  *// 스레드 안전*
::InterlockedPopEntrySList(&_header);        *// 락 없이 빠름*
```

### 2. 메모리 정렬

```cpp
DECLSPEC_ALIGN(SLIST_ALIGNMENT)  *// 16바이트 정렬// CPU 캐시 라인에 최적화 → 더 빠른 메모리 접근*
```

### 3. O(1) 검색

```cpp
*// 배열 인덱싱으로 즉시 풀 찾기*
MemoryPool* pool = _poolTable[size];  *// 상수 시간!*
```

### 4. 메모리 재사용

`새 할당: OS 요청 → 페이지 폴트 가능 → 느림
재사용: 이미 있는 메모리 → 캐시 히트 → 빠름`

---

## 성능 비교

### 벤치마크 결과 (예상)

`1000개 객체 생성/삭제 반복 (1000회):

일반 new/delete:     ~500ms
메모리 풀:           ~50ms
성능 향상:           10배!`

### 메모리 사용량

`초기: 메모리 풀이 더 많이 사용 (미리 할당)
장기: 단편화 없어서 더 효율적`