# Memory Management

## 개요

게임 서버에서 효율적인 메모리 관리 방법을 다룹니다.

## 주요 내용

### 왜 메모리 관리가 중요한가?

- 빈번한 할당/해제로 인한 성능 저하
- 메모리 단편화 방지
- 메모리 누수 방지

### Memory Pool

미리 메모리를 할당해두고 재사용하는 방식

```cpp
class MemoryPool
{
public:
    void* Allocate();
    void Release(void* ptr);

private:
    std::queue<void*> _pool;
};
```

### Pool Allocator

STL 컨테이너와 함께 사용할 수 있는 커스텀 할당자

```cpp
template<typename T>
class PoolAllocator
{
public:
    T* allocate(size_t n);
    void deallocate(T* p, size_t n);
};
```

## 관련 예제

- [Memory](../Study/Memory.cpp)
- [MemoryPool](../Study/MemoryPool.cpp)
- [PoolAllocator](../Study/PoolAllocator.cpp)

## 참고 자료

- C++ Custom Allocator
