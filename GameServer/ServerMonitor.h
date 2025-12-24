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

	// 패킷 카운터 (외부에서 호출)
	void OnPacketSend(int32 bytes);
	void OnPacketRecv(int32 bytes);

	// Latency 측정 (패킷 처리 시작/완료)
	void OnPacketProcessStart();
	void OnPacketProcessEnd();

	// 연결 카운터 콜백 설정
	void SetSessionCountGetter(function<int32()> getter) { _getSessionCount = getter; }
	void SetMaxSessionCountGetter(function<int32()> getter) { _getMaxSessionCount = getter; }
	void SetPlayerCountGetter(function<int32()> getter) { _getPlayerCount = getter; }
	void SetRoomCountGetter(function<int32()> getter) { _getRoomCount = getter; }

	// 통계 조회
	int64 GetTotalPacketsSent() const { return _totalPacketsSent; }
	int64 GetTotalPacketsRecv() const { return _totalPacketsRecv; }
	int64 GetTotalBytesSent() const { return _totalBytesSent; }
	int64 GetTotalBytesRecv() const { return _totalBytesRecv; }

private:
	ServerMonitor() = default;
	~ServerMonitor();

	void MonitorLoop();
	void PrintStats();

private:
	atomic<bool> _running = false;
	thread _monitorThread;
	int32 _intervalMs = 5000;

	// 패킷 통계
	atomic<int64> _totalPacketsSent = 0;
	atomic<int64> _totalPacketsRecv = 0;
	atomic<int64> _totalBytesSent = 0;
	atomic<int64> _totalBytesRecv = 0;

	// 구간별 통계 (PPS 계산용)
	atomic<int64> _intervalPacketsSent = 0;
	atomic<int64> _intervalPacketsRecv = 0;
	atomic<int64> _intervalBytesSent = 0;
	atomic<int64> _intervalBytesRecv = 0;

	// Latency 측정용
	atomic<int64> _totalLatencyUs = 0;  // 마이크로초 단위 누적
	atomic<int64> _latencyCount = 0;
	atomic<int64> _intervalLatencyUs = 0;
	atomic<int64> _intervalLatencyCount = 0;
	atomic<int64> _maxLatencyUs = 0;

	// 콜백
	function<int32()> _getSessionCount;
	function<int32()> _getMaxSessionCount;
	function<int32()> _getPlayerCount;
	function<int32()> _getRoomCount;
};
