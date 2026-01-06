# Packet Handling

## 개요

네트워크 패킷의 직렬화/역직렬화 및 처리 방법을 다룹니다.

## 주요 내용

### 패킷 구조

```
+----------------+----------------+----------------+
|  Header (4B)   |    Size (2B)   |   Payload      |
+----------------+----------------+----------------+
```

### 직렬화 (Serialization)

데이터를 바이트 스트림으로 변환

```cpp
class SendBuffer
{
public:
    template<typename T>
    void Write(T data)
    {
        memcpy(_buffer + _writePos, &data, sizeof(T));
        _writePos += sizeof(T);
    }
};
```

### 역직렬화 (Deserialization)

바이트 스트림을 데이터로 복원

```cpp
class RecvBuffer
{
public:
    template<typename T>
    T Read()
    {
        T data;
        memcpy(&data, _buffer + _readPos, sizeof(T));
        _readPos += sizeof(T);
        return data;
    }
};
```

### 패킷 핸들러

```cpp
using PacketHandler = std::function<void(Session*, Packet*)>;
std::map<uint16, PacketHandler> _handlers;
```

## 관련 예제

- Study 디렉토리 예제 참조

## 참고 자료

- Protocol Buffers, FlatBuffers
