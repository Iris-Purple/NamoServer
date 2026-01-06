# Example4

### RAII  (Resource Acquisition Is Initialization)

RAII란 무엇인가? RAII의 약자 Resource Acquisition Is Initialization를 직역하면 자원의 획득은 초기화라는 뜻이다. 그러나 RAII의 원칙은 더 엄밀히 따지면 자원의 initialization, 초기화보다는 destruction, 파괴에 초점을 맞추고 있다.

RAII의 핵심은 **프로그래머가 직접 자원을 획득하고 관리하는 것이 아니라, 자원의 생성, 파괴, 관리를 모두 객체에 위임하는 것을 의미한다.**

자세한 내용은  블로그 참고    :     [https://nx006.tistory.com/40](https://nx006.tistory.com/40)

**🎯 왜 RAII가 중요한가?**

### 문제 상황 (RAII 없이)

cpp

```cpp
void DangerousFunction() {
    char* buffer = new char[1024];  *// 메모리 할당*
    FILE* file = fopen("data.txt", "r");  *// 파일 열기*
    HANDLE mutex = CreateMutex();  *// 뮤텍스 생성*
    
    if (error1) {
        *// 😱 메모리 누수! file, mutex도 해제 안 됨*
        return;
    }
    
    if (error2) {
        delete[] buffer;  *// 일부만 정리// 😱 file, mutex는 누수!*
        return;
    }
    
    *// 복잡한 정리 코드 필요*
    delete[] buffer;
    fclose(file);
    CloseHandle(mutex);
}
```

## C++  std::lock_guard

std::lock_guard의 핵심 장점들

**1. 🛡️ 완벽한 예외 안전성 (Exception Safety)**

```cpp
*// ❌ 수동 관리 - 위험!*
void ManualLocking() {
    m.lock();
    
    ProcessData();      *// 예외 발생하면?*
    ValidateData();     *// 여기서 throw하면?*
    SaveToDatabase();   *// DB 에러 발생하면?*
    
    m.unlock();  *// 예외 시 실행 안 됨! 💥*
}

*// ✅ lock_guard - 완벽하게 안전!*
void SafeLocking() {
    std::lock_guard<std::mutex> lock(m);
    
    ProcessData();      *// 예외 발생해도*
    ValidateData();     *// 어디서 throw 해도*
    SaveToDatabase();   *// 언제나 unlock 보장!*
    
}  *// 예외가 발생해도 소멸자는 항상 호출됨*
```