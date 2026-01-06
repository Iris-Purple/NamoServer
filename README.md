# NamoServer

C++ 기반 게임 서버 프로젝트입니다.

## 게임서버에 필요한 기술

게임 서버 개발에 필요한 핵심 기술 문서입니다.

| 순서 | 주제 | 문서 |
|:----:|------|------|
| 1 | 네트워크 기초 | [Network](Document/01_Network.md) |
| 2 | IOCP | [IOCP](Document/02_IOCP.md) |
| 3 | 멀티스레딩 | [MultiThreading](Document/03_MultiThreading.md) |
| 4 | Lock & 동기화 | [Lock](Document/04_Lock.md) |
| 5 | 메모리 관리 | [MemoryManagement](Document/05_MemoryManagement.md) |
| 6 | 데드락 | [DeadLock](Document/06_DeadLock.md) |
| 7 | 패킷 처리 | [PacketHandling](Document/07_PacketHandling.md) |
| 8 | 세션 관리 | [SessionManagement](Document/08_SessionManagement.md) |

## Study 디렉토리

서버 개발을 위한 학습 예제들이 포함되어 있습니다.

### 예제 목록

| 파일 | 설명 |
|------|------|
| example1 ~ example15 | 기초 학습 예제 |
| exampleServer | TCP 소켓 기반 서버 예제 |
| spinLock | SpinLock 구현 |
| example9Queue | Queue 자료구조 예제 |
| example10Lock | Lock 관련 예제 |
| Memory / MemoryPool | 메모리 관리 및 풀 구현 |
| PoolAllocator | Pool Allocator 구현 |
| DeadLockProfiler | 데드락 탐지 프로파일러 |
