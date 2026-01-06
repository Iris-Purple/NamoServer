# Example11

** DFS를 활용한 데드락 탐지 시스템 구현하기**

## 1. 데드락이란?

### 개념 이해

데드락(Deadlock)은 두 개 이상의 스레드가 서로가 보유한 자원을 기다리며 무한정 대기하는 상황입니다.

cpp

`*// 클래식한 데드락 시나리오*
Thread1: Lock(A) → Lock(B)  *// A를 잡고 B를 기다림*
Thread2: Lock(B) → Lock(A)  *// B를 잡고 A를 기다림*
```

### 데드락 발생 조건 (Coffman Conditions)
1. **상호 배제(Mutual Exclusion)**: 자원은 한 번에 한 스레드만 사용
2. **점유 대기(Hold and Wait)**: 자원을 보유한 채 다른 자원 대기
3. **비선점(No Preemption)**: 강제로 자원을 빼앗을 수 없음
4. **순환 대기(Circular Wait)**: 자원 대기 그래프에 사이클 존재

우리는 **순환 대기**를 DFS로 탐지하여 데드락을 예방합니다!

---

## 2. DFS로 사이클 탐지하기

### 락 의존성 그래프
락 획득 순서를 방향 그래프로 표현합니다:
```
A → B: "A를 잡은 상태에서 B를 획득"

Thread1: A → B
Thread2: B → C  
Thread3: C → A  *// 사이클 발생! A → B → C → A*`

### DFS 사이클 탐지 알고리즘

```cpp
*// 의사 코드*
function hasCycle(graph):
    visited = {}      *// 전체 방문 기록*
    recStack = {}     *// 현재 DFS 경로 스택*
    
    for each node in graph:
        if dfs(node, visited, recStack):
            return true  *// 사이클 발견!*
    return false

function dfs(node, visited, recStack):
    visited[node] = true
    recStack[node] = true  *// 현재 경로에 추가*
    
    for each neighbor in graph[node]:
        if not visited[neighbor]:
            if dfs(neighbor, visited, recStack):
                return true
        else if recStack[neighbor]:  *// 현재 경로에 있는 노드를 다시 방문*
            return true  *// 사이클!*
    
    recStack[node] = false  *// 현재 경로에서 제거*
    return false
```

### 클래스 구조

```cpp
class DeadLockProfiler {
private:
    *// 락 이름 ↔ ID 매핑*
    unordered_map<const char*, int32> _nameToId;  *// "MutexA" → 0*
    unordered_map<int32, const char*> _idToName;  *// 0 → "MutexA"*
    
    *// 현재 스레드가 보유한 락 스택*
    stack<int32> _lockStack;  *// [0, 1] = MutexA → MutexB*
    
    *// 락 획득 히스토리 (방향 그래프)*
    map<int32, set<int32>> _lockHistory;  *// 0 → {1, 2} = A → B, A → C*
    
    *// DFS 탐색용 변수*
    vector<int32> _discoveredOrder;  *// 노드 발견 순서*
    vector<bool> _finished;          *// DFS 완료 여부*
    vector<int32> _parent;           *// 부모 노드 추적*
};
```

---

## 4. 핵심 알고리즘 구현

### 락 획득 추적 (PushLock)

```cpp
void DeadLockProfiler::PushLock(const char* name) {
    lock_guard<mutex> guard(_lock);
    
    *// 1️⃣ 락 이름을 ID로 변환 (없으면 생성)*
    int32 lockId = 0;
    auto findIt = _nameToId.find(name);
    if (findIt == _nameToId.end()) {
        lockId = static_cast<int32>(_nameToId.size());
        _nameToId[name] = lockId;
        _idToName[lockId] = name;
    } else {
        lockId = findIt->second;
    }
    
    *// 2️⃣ 현재 보유한 락이 있다면 의존성 기록*
    if (!_lockStack.empty()) {
        const int32 prevId = _lockStack.top();
        if (lockId != prevId) {
            set<int32>& history = _lockHistory[prevId];
            
            *// 3️⃣ 새로운 엣지가 추가되면 사이클 체크!*
            if (history.find(lockId) == history.end()) {
                history.insert(lockId);  *// prevId → lockId 엣지 추가*
                CheckCycle();  *// 🔍 데드락 검사*
            }
        }
    }
    
    _lockStack.push(lockId);
}
```

### DFS 사이클 탐지 (핵심!)

```cpp
void DeadLockProfiler::Dfs(int32 here) {
    *// 이미 방문했으면 스킵*
    if (_discoveredOrder[here] != -1)
        return;
    
    *// 현재 노드 방문 처리*
    _discoveredOrder[here] = _discoveredCount++;
    
    *// 인접 노드들 탐색*
    auto findIt = _lockHistory.find(here);
    if (findIt == _lockHistory.end()) {
        _finished[here] = true;
        return;
    }
    
    set<int32>& nextSet = findIt->second;
    for (int32 there : nextSet) {
        *// 케이스 1: 아직 방문 안 한 노드*
        if (_discoveredOrder[there] == -1) {
            _parent[there] = here;
            Dfs(there);
            continue;
        }
        
        *// 케이스 2: 이미 방문했고, here의 후손인 경우*
        if (_discoveredOrder[here] < _discoveredOrder[there])
            continue;  *// 정상적인 순방향 간선*
        
        *// 케이스 3: 역방향 간선 발견! (사이클)*
        if (_finished[there] == false) {
            *// 데드락 발견!*
            PrintCyclePath(here, there);
            throw std::runtime_error("DEADLOCK_DETECTED");
        }
    }
    
    _finished[here] = true;
}
```

### 사이클 경로 출력

```cpp
void PrintCyclePath(int32 here, int32 there) {
    printf(" Cycle detected: %s -> %s\n", 
           _idToName[here], _idToName[there]);
    
    *// 사이클 경로 역추적*
    int32 now = here;
    while (true) {
        printf("  %s -> %s\n", 
               _idToName[_parent[now]], _idToName[now]);
        now = _parent[now];
        if (now == there)
            break;
    }
}
```

---

## 5. 실전 사용 예제

### 데드락 시나리오 1: Classic A↔B

```cpp
*// Thread 1*
void Thread1_Scenario2() {
    LockGuard lock1(mutexA, "MutexA");  *// A 획득*
    std::this_thread::sleep_for(10ms);
    LockGuard lock2(mutexB, "MutexB");  *// B 획득 시도*
}

*// Thread 2*
void Thread2_Scenario2() {
    LockGuard lock1(mutexB, "MutexB");  *// B 획득*
    std::this_thread::sleep_for(10ms);
    LockGuard lock2(mutexA, "MutexA");  *// A 획득 시도 →  데드락!*
}

*// 실행 흐름:// 1. Thread1: PushLock("MutexA") → _lockHistory[A] = {}// 2. Thread2: PushLock("MutexB") → _lockHistory[B] = {}// 3. Thread1: PushLock("MutexB") → _lockHistory[A] = {B} ✅// 4. Thread2: PushLock("MutexA") → _lockHistory[B] = {A} //    → CheckCycle() → 사이클 발견! A→B→A*
```

### 데드락 시나리오 2: 3-way 순환

```cpp
*// Thread 1: A → B// Thread 2: B → C// Thread 3: C → A  // 사이클 완성!// 그래프 구조://     A//    ↙ ↘//   C ← B*
```