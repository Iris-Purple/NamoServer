#include "pch.h"
#include "GameSession.h"
#include <random>

GameSession::~GameSession()
{
	//cout << "~GameSession" << endl;
}

void GameSession::OnConnected()
{
	//cout << "OnConnected" << endl;

	// 암호화 초기화 (GEncryptionEnabled가 true일 때만 실제 동작)
	InitEncryption(GEncryptionKey, 16);

	Protocol::C2S_ENTER_GAME pkt;
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
	Send(sendBuffer);
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

	// TODO : packetId 대역 체크
	ClientPacketHandler::HandlePacket(session, buffer, len);
}

void GameSession::OnSend(int32 len)
{
	//cout << "OnSend Len = " << len << endl;
}

void GameSession::OnDisconnected()
{
	cout << "Disconnected" << endl;
	RemoveActiveSession(GetPacketSessionRef());
}

void GameSession::ResetDelay()
{
	_lastSendTime = chrono::steady_clock::now();
	_nextDelayMs = g_delayDist(g_gen);
}

bool GameSession::IsTimeToSend()
{
	auto now = chrono::steady_clock::now();
	auto elapsed = chrono::duration_cast<chrono::milliseconds>(now - _lastSendTime).count();
	return elapsed >= _nextDelayMs;
}

void GameSession::SendMoveOrAttack()
{
	if (_isAttack)
	{
		SendAttackPacket();
	}
	else
	{
		SendMovePacket();
	}

	_isAttack = !_isAttack;
}

void GameSession::SendAttackPacket()
{
	Protocol::C2S_SKILL pkt;
	pkt.mutable_info()->set_skillid(2);
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
	Send(sendBuffer);
	ResetDelay();
}

void GameSession::SendMovePacket()
{
	// 랜덤 방향 선택
	static uniform_int_distribution<> dirDist(0, 3);
	Protocol::MoveDir dir = static_cast<Protocol::MoveDir>(dirDist(g_gen));

	// 방향에 따라 위치 변경
	switch (dir)
	{
	case Protocol::MoveDir::Up:    _posY++; break;
	case Protocol::MoveDir::Down:  _posY--; break;
	case Protocol::MoveDir::Left:  _posX--; break;
	case Protocol::MoveDir::Right: _posX++; break;
	}

	Protocol::C2S_MOVE pkt;
	Protocol::PositionInfo* posInfo = pkt.mutable_posinfo();
	posInfo->set_state(Protocol::CreatureState::Moving);
	posInfo->set_movedir(dir);
	posInfo->set_posx(_posX);
	posInfo->set_posy(_posY);
	//cout << "SendMove: " << _posX << ", " << _posY << endl;
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
	Send(sendBuffer);

	this_thread::sleep_for(100ms);
	posInfo->set_state(Protocol::CreatureState::Idle);
	sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
	Send(sendBuffer);

	ResetDelay();
}
