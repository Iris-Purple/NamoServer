#include "pch.h"
#include "Session.h"
#include "SocketUtils.h"
#include "Service.h"

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

	int32 plainSize = sendBuffer->WriteSize();
	int32 encryptedSize = AESCrypto::GetEncryptedSize(plainSize);

	// 새 버퍼 생성: [enc_size(2)][encrypted_data]
	int32 totalSize = sizeof(uint16) + encryptedSize;
	SendBufferRef encryptedBuffer = make_shared<SendBuffer>(totalSize);

	// 암호화된 크기를 먼저 기록
	BYTE* bufferPtr = encryptedBuffer->Buffer();
	*(reinterpret_cast<uint16*>(bufferPtr)) = static_cast<uint16>(encryptedSize);

	// 암호화 데이터 기록
	int32 resultLen = _crypto->Encrypt(
		sendBuffer->Buffer(),
		plainSize,
		bufferPtr + sizeof(uint16),
		encryptedSize
	);

	if (resultLen < 0)
	{
		cout << "Encryption failed" << endl;
		return nullptr;
	}

	encryptedBuffer->Close(sizeof(uint16) + resultLen);
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

// 평문:  [size(2)][id(2)][data....][size(2)][id(2)][data....]
// 암호화: [enc_size(2)][encrypted_data...][enc_size(2)][encrypted_data...]
int32 PacketSession::OnRecv(BYTE* buffer, int32 len)
{
	int32 processLen = 0;

	// 암호화 OFF 또는 crypto 미초기화 -> 기존 평문 처리
	if (GEncryptionEnabled == false || _crypto == nullptr)
	{
		while (true)
		{
			int32 dataSize = len - processLen;
			if (dataSize < sizeof(PacketHeader))
				break;

			PacketHeader header = *(reinterpret_cast<PacketHeader*>(&buffer[processLen]));
			if (dataSize < header.size)
				break;

			OnRecvPacket(&buffer[processLen], header.size);
			processLen += header.size;
		}
		return processLen;
	}

	// 암호화 ON -> 복호화 처리
	// 패킷 구조: [enc_size(2)][encrypted_data(enc_size)]
	while (true)
	{
		int32 dataSize = len - processLen;

		// 최소 2바이트 (암호화된 크기) 필요
		if (dataSize < sizeof(uint16))
			break;
			
		uint16 encryptedSize = *(reinterpret_cast<uint16*>(&buffer[processLen]));

		// 암호화된 전체 패킷 수신 대기
		if (dataSize < sizeof(uint16) + encryptedSize)
			break;
		
		// 복호화
		int32 decryptedLen = _crypto->Decrypt(
			&buffer[processLen + sizeof(uint16)],
			encryptedSize,
			_decryptBuffer,
			sizeof(_decryptBuffer)
		);

		if (decryptedLen < 0)
		{
			// 복호화 실패
			cout << "Decryption failed" << endl;
			return -1;
		}

		// 복호화된 패킷 처리
		if (decryptedLen >= sizeof(PacketHeader))
		{
			OnRecvPacket(_decryptBuffer, decryptedLen);
		}

		processLen += sizeof(uint16) + encryptedSize;
	}

	return processLen;
}
