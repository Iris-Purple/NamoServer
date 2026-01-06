# Example13

지난번에 만든 Stomp Allocator 를 이용한  custom STL Allocator  생성 및 사용

STL 컨테이너의 메모리 버그를 즉시 감지하기 위해 StompAllocator를 기반으로 한 **커스텀 STL Allocator**를 구현합니다. 이를 통해 vector, list, map 등의 메모리 오류를 개발 단계에서 완벽하게 잡아냅니다.

## 🎯 학습 목표

1. STL Allocator의 동작 원리 이해
2. 커스텀 Allocator 구현 방법 학습
3. Template Alias를 활용한 편리한 인터페이스 설계
4. STL 컨테이너의 메모리 버그 디버깅 기법 습득

---

## 1. STL Allocator란?

STL 컨테이너는 메모리 할당/해제를 직접 하지 않고 **Allocator**에게 위임합니다.

### 기본 STL 동작

```cpp
*// 기본 allocator 사용*
vector<int> v;  *// std::allocator<int> 사용*
v.push_back(10); *// allocator가 메모리 할당// 커스텀 allocator 사용*
vector<int, MyAllocator<int>> v;  *// MyAllocator 사용*
```

### Allocator가 필요한 이유

```cpp
*// STL 컨테이너 내부 동작*
template<typename T, typename Alloc = std::allocator<T>>
class vector {
    Alloc _allocator;
    T* _data;
    
    void push_back(const T& val) {
        *// new T가 아닌 allocator 사용*
        T* newData = _allocator.allocate(_capacity * 2);
        *// ...*
    }
};
```

---

## 2. StlAllocator 구현 분석

### 핵심 구현 코드

```cpp
template<typename T>
class StlAllocator
{
public:
    *// STL이 요구하는 타입 정의*
    using value_type = T;
    
    *// 기본 생성자*
    StlAllocator() {}
    
    *// 템플릿 복사 생성자 (rebind 지원)*
    template<typename Other>
    StlAllocator(const StlAllocator<Other>&) {}
    
    *// 메모리 할당 (객체 count개)*
    T* allocate(size_t count)
    {
        const int32 size = static_cast<int32>(count * sizeof(T));
        return static_cast<T*>(StompAllocator::Alloc(size));
    }
    
    *// 메모리 해제*
    void deallocate(T* ptr, size_t count)
    {
        StompAllocator::Release(ptr);  *// count는 무시 (StompAllocator가 크기 추적)*
    }
};
```

### 필수 요구사항 설명

### 1. value_type 정의

```cpp
using value_type = T;  *// STL이 타입 정보를 얻는 방법*
```

### 2. Rebind 지원 (템플릿 복사 생성자)

```cpp
template<typename Other>
StlAllocator(const StlAllocator<Other>&) {}

*// 왜 필요한가?// map<int, string>은 내부적으로 Node<pair<int,string>> 할당// StlAllocator<pair<int,string>> → StlAllocator<Node> 변환 필요*
```

### 3. allocate/deallocate 인터페이스

```cpp
*// STL 표준 인터페이스*
T* allocate(size_t count);        *// count개 객체 메모리 할당*
void deallocate(T* ptr, size_t);  *// 메모리 해제*
```

---

## 3. Template Alias로 사용성 개선

### 문제: 복잡한 템플릿 문법

```cpp
*// 😰 매번 이렇게 쓰기는 너무 복잡!*
vector<Knight, StlAllocator<Knight>> knights;
map<int, Knight, less<int>, StlAllocator<pair<const int, Knight>>> knightMap;
```

### 해결: Template Alias 사용

```cpp
*// Vector 타입 정의*
template<typename Type>
using Vector = vector<Type, StlAllocator<Type>>;

*// List 타입 정의*
template<typename Type>
using List = list<Type, StlAllocator<Type>>;

*// Map 타입 정의 (pair에 주의!)*
template<typename Key, typename Type, typename Pred = less<Key>>
using Map = map<Key, Type, Pred, StlAllocator<pair<const Key, Type>>>;

*// Set 타입 정의*
template<typename Key, typename Pred = less<Key>>
using Set = set<Key, Pred, StlAllocator<Key>>;
```

### 사용 비교

```cpp
*// Before: 복잡한 템플릿 문법*
vector<Knight, StlAllocator<Knight>> v1;
map<int, Knight, less<int>, StlAllocator<pair<const int, Knight>>> m1;

*// After: 깔끔한 사용*
Vector<Knight> v2;
Map<int, Knight> m2;
```

---

## 4. 실전 사용 예제

### 기본 사용

```cpp
class Knight
{
public:
    Knight() { cout << "Knight()" << endl; }
    ~Knight() { cout << "~Knight()" << endl; }
    int _hp = 10;
};

int main()
{
    *// 100개 Knight 객체를 담는 벡터*
    Vector<Knight> knights(100);
    
    *// 각 Knight마다 독립된 페이지에 할당됨*
    knights[0]._hp = 100;
    knights[99]._hp = 200;
}
```

### 메모리 레이아웃
```
일반 vector<Knight>:
[Knight0][Knight1][Knight2]...[Knight99]  *// 연속된 메모리*

Vector<Knight> (StompAllocator):
[Page1: Knight0][Page2: Knight1]...[Page100: Knight99]
각 객체가 독립된 페이지에!
```

## 🎓 핵심 정리

커스텀 STL Allocator는 **STL 컨테이너의 메모리 버그를 즉시 감지**하는 강력한 도구입니다.

### 장점

- ✅ STL 컨테이너의 UAF 버그 즉시 감지
- ✅ Iterator 무효화 버그 감지
- ✅ 범위 초과 접근 감지
- ✅ 기존 코드 최소 수정 (타입만 변경)

### 활용 팁

1. **개발 초기**부터 사용하여 버그 예방
2. **Unit Test**에서 필수 사용
3. **스트레스 테스트**로 숨은 버그 발견
4. **Release 빌드**에서는 반드시 비활성화

게임 서버처럼 안정성이 중요한 시스템에서는 개발 중 StlAllocator를 사용하여 메모리 관련 버그를 사전에 제거하는 것이 필수입니다!