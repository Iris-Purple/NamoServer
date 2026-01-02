#pragma once
#include "IocpCore.h"
#include "IocpEvent.h"
#include "NetAddress.h"
#include "RecvBuffer.h"
#include "AESCrypto.h"

class Service;

/*--------------
	PacketFlags
---------------*/
constexpr uint8 PKT_FLAG_HAS_SEQUENCE = 0x01;

/*--------------
	Session
---------------*/

class Session : public IocpObject
{
	friend class Listener;
	friend class IocpCore;
	friend class Service;

	enum
	{
		BUFFER_SIZE = 0x10000, // 64KB
	};

public:
	Session();
	virtual ~Session();

public:
						/* �ܺο��� ��� */
	void				Send(SendBufferRef sendBuffer);
	bool				Connect();
	void				Disconnect(const WCHAR* cause);

	shared_ptr<Service>	GetService() { return _service.lock(); }
	void				SetService(shared_ptr<Service> service) { _service = service; }

public:
						/* ���� ���� */
	void				SetNetAddress(NetAddress address) { _netAddress = address; }
	NetAddress			GetAddress() { return _netAddress; }
	SOCKET				GetSocket() { return _socket; }
	bool				IsConnected() { return _connected; }
	SessionRef			GetSessionRef() { return static_pointer_cast<Session>(shared_from_this()); }

private:
						/* �������̽� ���� */
	virtual HANDLE		GetHandle() override;
	virtual void		Dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

private:
						/* ���� ���� */
	bool				RegisterConnect();
	bool				RegisterDisconnect();
	void				RegisterRecv();
	void				RegisterSend();

	void				ProcessConnect();
	void				ProcessDisconnect();
	void				ProcessRecv(int32 numOfBytes);
	void				ProcessSend(int32 numOfBytes);

	void				HandleError(int32 errorCode);

protected:
						/* ������ �ڵ忡�� ������ */
	virtual void		OnConnected() { }
	virtual int32		OnRecv(BYTE* buffer, int32 len) { return len; }
	virtual void		OnSend(int32 len) { }
	virtual void		OnDisconnected() { }

private:
	weak_ptr<Service>	_service;
	SOCKET				_socket = INVALID_SOCKET;
	NetAddress			_netAddress = {};
	atomic<bool>		_connected = false;

protected:
	/* 암호화 */
	AESCrypto*			_crypto = nullptr;

	/* Sequence (리플레이 공격 방지) */
	uint32				_sendSeq = 0;

	/* 응답 캐시 (재전송용) */
	bool				_cacheNextResponse = false;
	SendBufferRef		_lastResponse = nullptr;

public:
	void				InitEncryption(const BYTE* key, int32 keyLen);
	SendBufferRef		EncryptBuffer(SendBufferRef sendBuffer);

private:
	USE_LOCK;
	RecvBuffer				_recvBuffer;

							/* �۽� ���� */
	queue<SendBufferRef>	_sendQueue;
	atomic<bool>			_sendRegistered = false;

private:
						/* IocpEvent ���� */
	ConnectEvent		_connectEvent;
	DisconnectEvent		_disconnectEvent;
	RecvEvent			_recvEvent;
	SendEvent			_sendEvent;
};

/*-----------------
	PacketSession
------------------*/
#pragma pack(push, 1)
struct PacketHeader
{
	uint16 size;
	uint16 id;
	uint8  flags;
	uint32 sequence;
};
#pragma pack(pop)


class PacketSession : public Session
{
public:
	PacketSession();
	virtual ~PacketSession();

	PacketSessionRef	 GetPacketSessionRef() { return static_pointer_cast<PacketSession>(shared_from_this()); }

protected:
	virtual int32		OnRecv(BYTE* buffer, int32 len) sealed;
	virtual void		OnRecvPacket(BYTE* buffer, int32 len) abstract;

protected:
	// Sequence 카운터 (리플레이 공격 방지)
	uint32				_recvSeq = 0;

private:
	// 복호화용 버퍼 (최대 패킷 크기)
	BYTE				_decryptBuffer[0x10000];
};