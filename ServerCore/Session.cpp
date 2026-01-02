#include "pch.h"
#include "Session.h"
#include "SocketUtils.h"
#include "Service.h"
#include "AESCrypto.h"

/*--------------
	Session
---------------*/

Session::Session() : _recvBuffer(BUFFER_SIZE)
{
	_socket = SocketUtils::CreateSocket();
}

Session::~Session()
{
	SocketUtils::Close(_socket);

	if (_crypto)
	{
		delete _crypto;
		_crypto = nullptr;
	}
}

void Session::Send(SendBufferRef sendBuffer)
{
	if (IsConnected() == false)
		return;

	// 응답 캐시 (재전송용)
	if (_cacheNextResponse)
	{
		_lastResponse = sendBuffer;
		_cacheNextResponse = false;
	}

	// Sequence 설정 (암호화 전에)
	PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
	if (header->flags & PKT_FLAG_HAS_SEQUENCE)
	{
		header->sequence = ++_sendSeq;
	}

	// 암호화 ON이고 crypto 초기화됨
	if (GEncryptionEnabled && _crypto)
	{
		sendBuffer = EncryptBuffer(sendBuffer);
		if (sendBuffer == nullptr)
			return;  // 암호화 실패
	}

	bool registerSend = false;

	{
		WRITE_LOCK;

		_sendQueue.push(sendBuffer);

		if (_sendRegistered.exchange(true) == false)
			registerSend = true;

		if (registerSend)
			RegisterSend();
	}
}

bool Session::Connect()
{
	return RegisterConnect();
}

void Session::Disconnect(const WCHAR* cause)
{
	if (_connected.exchange(false) == false)
		return;

	// TEMP
	wcout << "Disconnect : " << cause << endl;

	RegisterDisconnect();
}

HANDLE Session::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

void Session::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes)
{
	switch (iocpEvent->eventType)
	{
	case EventType::Connect:
		ProcessConnect();
		break;
	case EventType::Disconnect:
		ProcessDisconnect();
		break;
	case EventType::Recv:
		ProcessRecv(numOfBytes);
		break;
	case EventType::Send:
		ProcessSend(numOfBytes);
		break;
	default:
		break;
	}
}

bool Session::RegisterConnect()
{
	if (IsConnected())
		return false;

	if (GetService()->GetServiceType() != ServiceType::Client)
		return false;

	if (SocketUtils::SetReuseAddress(_socket, true) == false)
		return false;

	if (SocketUtils::BindAnyAddress(_socket, 0/*���°�*/) == false)
		return false;

	_connectEvent.Init();
	_connectEvent.owner = shared_from_this(); // ADD_REF

	DWORD numOfBytes = 0;
	SOCKADDR_IN sockAddr = GetService()->GetNetAddress().GetSockAddr();
	if (false == SocketUtils::ConnectEx(_socket, reinterpret_cast<SOCKADDR*>(&sockAddr), sizeof(sockAddr), nullptr, 0, &numOfBytes, &_connectEvent))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			_connectEvent.owner = nullptr; // RELEASE_REF
			return false;
		}
	}

	return true;
}

bool Session::RegisterDisconnect()
{
	_disconnectEvent.Init();
	_disconnectEvent.owner = shared_from_this(); // ADD_REF

	if (false == SocketUtils::DisconnectEx(_socket, &_disconnectEvent, TF_REUSE_SOCKET, 0))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			_disconnectEvent.owner = nullptr; // RELEASE_REF
			return false;
		}
	}

	return true;
}

void Session::RegisterRecv()
{
	if (IsConnected() == false)
		return;

	_recvEvent.Init();
	_recvEvent.owner = shared_from_this(); // ADD_REF

	WSABUF wsaBuf;
	wsaBuf.buf = reinterpret_cast<char*>(_recvBuffer.WritePos());
	wsaBuf.len = _recvBuffer.FreeSize();

	DWORD numOfBytes = 0;
	DWORD flags = 0;
	if (SOCKET_ERROR == ::WSARecv(_socket, &wsaBuf, 1, OUT &numOfBytes, OUT &flags, &_recvEvent, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			_recvEvent.owner = nullptr; // RELEASE_REF
		}
	}
}

void Session::RegisterSend()
{
	if (IsConnected() == false)
		return;

	_sendEvent.Init();
	_sendEvent.owner = shared_from_this(); // ADD_REF

	{
		int32 writeSize = 0;
		while (_sendQueue.empty() == false)
		{
			SendBufferRef sendBuffer = _sendQueue.front();

			writeSize += sendBuffer->WriteSize();
			_sendQueue.pop();
			_sendEvent.sendBuffers.push_back(sendBuffer);
		}
	}

	vector<WSABUF> wsaBufs;
	wsaBufs.reserve(_sendEvent.sendBuffers.size());
	for (SendBufferRef sendBuffer : _sendEvent.sendBuffers)
	{
		WSABUF wsaBuf;
		wsaBuf.buf = reinterpret_cast<char*>(sendBuffer->Buffer());
		wsaBuf.len = static_cast<LONG>(sendBuffer->WriteSize());
		wsaBufs.push_back(wsaBuf);
	}

	DWORD numOfBytes = 0;
	if (SOCKET_ERROR == ::WSASend(_socket, wsaBufs.data(), static_cast<DWORD>(wsaBufs.size()), OUT &numOfBytes, 0, &_sendEvent, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			_sendEvent.owner = nullptr; // RELEASE_REF
			_sendEvent.sendBuffers.clear(); // RELEASE_REF
			_sendRegistered.store(false);
		}
	}
}

void Session::ProcessConnect()
{
	_connectEvent.owner = nullptr; // RELEASE_REF

	_connected.store(true);

	GetService()->AddSession(GetSessionRef());

	OnConnected();

	RegisterRecv();
}

void Session::ProcessDisconnect()
{
	_disconnectEvent.owner = nullptr; // RELEASE_REF

	OnDisconnected();
	GetService()->ReleaseSession(GetSessionRef());
}

void Session::ProcessRecv(int32 numOfBytes)
{
	_recvEvent.owner = nullptr; // RELEASE_REF

	if (numOfBytes == 0)
	{
		Disconnect(L"Recv 0");
		return;
	}

	if (_recvBuffer.OnWrite(numOfBytes) == false)
	{
		Disconnect(L"OnWrite Overflow");
		return;
	}

	int32 dataSize = _recvBuffer.DataSize();
	int32 processLen = OnRecv(_recvBuffer.ReadPos(), dataSize); // ������ �ڵ忡�� ������
	if (processLen < 0 || dataSize < processLen || _recvBuffer.OnRead(processLen) == false)
	{
		Disconnect(L"OnRead Overflow");
		return;
	}
	
	// Ŀ�� ����
	_recvBuffer.Clean();

	// ���� ���
	RegisterRecv();
}

void Session::ProcessSend(int32 numOfBytes)
{
	_sendEvent.owner = nullptr; // RELEASE_REF 
	_sendEvent.sendBuffers.clear(); // RELEASE_REF

	if (numOfBytes == 0)
	{
		Disconnect(L"Send 0");
		return;
	}

	// ������ �ڵ忡�� ������
	OnSend(numOfBytes);

	WRITE_LOCK;
	if (_sendQueue.empty())
		_sendRegistered.store(false);
	else
		RegisterSend();
}

void Session::HandleError(int32 errorCode)
{
	switch (errorCode)
	{
	case WSAECONNRESET:
	case WSAECONNABORTED:
		Disconnect(L"HandleError");
		break;
	default:
		// TODO : Log
		cout << "Handle Error : " << errorCode << endl;
		break;
	}
}

void Session::InitEncryption(const BYTE* key, int32 keyLen)
{
	if (_crypto == nullptr)
	{
		_crypto = new AESCrypto();
	}

	if (!_crypto->Init(key, keyLen))
	{
		cout << "Failed to initialize AES encryption" << endl;
		delete _crypto;
		_crypto = nullptr;
	}
}

SendBufferRef Session::EncryptBuffer(SendBufferRef sendBuffer)
{
	if (_crypto == nullptr)
		return sendBuffer;

	// 원본: [size(2)][id(2)][data...]
	// 암호화+HMAC: [size(2)][encrypted(id+data)][HMAC(32)]
	int32 plainSize = sendBuffer->WriteSize();
	if (plainSize < sizeof(uint16))
		return sendBuffer;

	// id+data 부분만 암호화 (size 제외)
	int32 payloadSize = plainSize - sizeof(uint16);
	int32 encryptedPayloadSize = AESCrypto::GetEncryptedSize(payloadSize);

	// 새 버퍼 생성: [size(2)][encrypted(id+data)][HMAC(32)]
	int32 totalSize = sizeof(uint16) + encryptedPayloadSize + HMAC_SIZE;
	SendBufferRef encryptedBuffer = make_shared<SendBuffer>(totalSize);

	BYTE* bufferPtr = encryptedBuffer->Buffer();

	// size 기록 (전체 패킷 크기)
	*(reinterpret_cast<uint16*>(bufferPtr)) = static_cast<uint16>(totalSize);

	// id+data 부분 암호화
	int32 resultLen = _crypto->Encrypt(
		sendBuffer->Buffer() + sizeof(uint16),  // id+data 시작점
		payloadSize,
		bufferPtr + sizeof(uint16),
		encryptedPayloadSize
	);

	if (resultLen < 0)
	{
		cout << "Encryption failed" << endl;
		return nullptr;
	}

	// HMAC 계산 (암호화된 데이터에 대해)
	BYTE* encryptedData = bufferPtr + sizeof(uint16);
	BYTE* hmacPos = bufferPtr + sizeof(uint16) + resultLen;

	if (!_crypto->ComputeHMAC(encryptedData, resultLen, hmacPos))
	{
		cout << "HMAC computation failed" << endl;
		return nullptr;
	}

	encryptedBuffer->Close(sizeof(uint16) + resultLen + HMAC_SIZE);
	return encryptedBuffer;
}

/*-----------------
	PacketSession
------------------*/

PacketSession::PacketSession()
{
}

PacketSession::~PacketSession()
{
}

// 평문:       [size(2)][id(2)][flags(1)][seq(4)][data...]
// 암호화+HMAC: [size(2)][encrypted(id+flags+seq+data)][HMAC(32)]
int32 PacketSession::OnRecv(BYTE* buffer, int32 len)
{
	int32 processLen = 0;

	while (true)
	{
		int32 dataSize = len - processLen;

		// 최소 size(2) 필요
		if (dataSize < sizeof(uint16))
			break;

		// size 읽기 (공통)
		uint16 packetSize = *(reinterpret_cast<uint16*>(&buffer[processLen]));

		// 최소 크기 검증 (암호화 여부에 따라 다름)
		int32 minSize = GEncryptionEnabled
			? (sizeof(uint16) + 16 + HMAC_SIZE)  // size + AES블록(16) + HMAC
			: sizeof(PacketHeader);               // 평문 헤더

		if (packetSize < minSize)
		{
			cout << "Invalid packet size: " << packetSize << endl;
			return -1;
		}

		// 전체 패킷 수신 대기
		if (dataSize < packetSize)
			break;

		BYTE* packetData = &buffer[processLen];
		int32 packetLen = packetSize;

		// 암호화 ON -> HMAC 검증 + 복호화
		if (GEncryptionEnabled && _crypto)
		{
			// 패킷 구조: [size(2)][encrypted][HMAC(32)]
			int32 encryptedPayloadSize = packetSize - sizeof(uint16) - HMAC_SIZE;

			if (encryptedPayloadSize <= 0)
			{
				cout << "Invalid packet size for HMAC" << endl;
				return -1;
			}

			BYTE* encryptedData = &buffer[processLen + sizeof(uint16)];
			BYTE* receivedHmac = &buffer[processLen + sizeof(uint16) + encryptedPayloadSize];

			// HMAC 검증 (실패 시 패킷 폐기)
			if (!_crypto->VerifyHMAC(encryptedData, encryptedPayloadSize, receivedHmac))
			{
				cout << "HMAC verification failed - packet tampered!" << endl;
				return -1;
			}

			// 복호화 진행
			int32 decryptedLen = _crypto->Decrypt(
				encryptedData,
				encryptedPayloadSize,
				_decryptBuffer + sizeof(uint16),
				sizeof(_decryptBuffer) - sizeof(uint16)
			);

			if (decryptedLen < 0)
			{
				cout << "Decryption failed" << endl;
				return -1;
			}

			// 복호화된 크기로 size 설정
			packetLen = sizeof(uint16) + decryptedLen;

			// 복호화된 패킷도 최소 PacketHeader 크기 이상이어야 함
			if (packetLen < sizeof(PacketHeader))
			{
				cout << "Decrypted packet too small: " << packetLen << endl;
				return -1;
			}

			*(reinterpret_cast<uint16*>(_decryptBuffer)) = static_cast<uint16>(packetLen);
			packetData = _decryptBuffer;
		}

		// 패킷 처리
		PacketHeader* header = reinterpret_cast<PacketHeader*>(packetData);

		// Sequence 검증 (HAS_SEQUENCE 플래그가 있을 때만)
		if (header->flags & PKT_FLAG_HAS_SEQUENCE)
		{
			if (header->sequence == _recvSeq)
			{
				// 동일 seq → 캐시된 응답 재전송
				if (_lastResponse)
					Send(_lastResponse);
				processLen += packetSize;
				continue;
			}
			else if (header->sequence < _recvSeq)
			{
				// 오래된 seq → 리플레이 공격
				return -1;
			}

			_recvSeq = header->sequence;
			_cacheNextResponse = true;  // 다음 Send를 캐시
		}

		OnRecvPacket(packetData, packetLen);

		processLen += packetSize;
	}

	return processLen;
}
