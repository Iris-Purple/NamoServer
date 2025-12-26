#pragma once

#include <atomic>
#include <thread>
#include <functional>

/*------------------
	ServerMonitor
-------------------*/

class ServerMonitor
{
public:
	static ServerMonitor& Instance()
	{
		static ServerMonitor instance;
		return instance;
	}

	ServerMonitor(const ServerMonitor&) = delete;
	ServerMonitor& operator=(const ServerMonitor&) = delete;

	// 모니터링 시작/중지
	void Start(int32 intervalMs = 5000);
	void Stop();

	// TPS 카운터 (클라이언트 요청 처리 시 호출)
	void OnTransaction();

	// 레이턴시 기록 (마이크로초 단위)
	void OnLatency(int64 latencyUs);

	// 연결 카운터 콜백 설정
	void SetSessionCountGetter(function<int32()> getter) { _getSessionCount = getter; }
	void SetMaxSessionCountGetter(function<int32()> getter) { _getMaxSessionCount = getter; }
	void SetPlayerCountGetter(function<int32()> getter) { _getPlayerCount = getter; }
	void SetRoomCountGetter(function<int32()> getter) { _getRoomCount = getter; }

	// 통계 조회
	int64 GetTotalTransactions() const { return _totalTransactions; }

private:
	ServerMonitor() = default;
	~ServerMonitor();

	void MonitorLoop();
	void PrintStats();

private:
	atomic<bool> _running = false;
	thread _monitorThread;
	int32 _intervalMs = 5000;

	// TPS 통계
	atomic<int64> _totalTransactions = 0;
	atomic<int64> _intervalTransactions = 0;

	// 레이턴시 통계 (마이크로초 단위)
	atomic<int64> _totalLatency = 0;
	atomic<int64> _latencyCount = 0;
	atomic<int64> _maxLatency = 0;

	// 콜백
	function<int32()> _getSessionCount;
	function<int32()> _getMaxSessionCount;
	function<int32()> _getPlayerCount;
	function<int32()> _getRoomCount;
};
