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

	// CPU/메모리 측정
	double GetCpuUsage();
	size_t GetMemoryUsageMB();
	size_t GetWorkingSetMB();

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

	// 피크 기록
	int32 _peakSessionCount = 0;
	int32 _peakPlayerCount = 0;
	double _peakCpuUsage = 0.0;
	size_t _peakMemoryMB = 0;
	int64 _peakPPS = 0;

	// CPU 측정용
	ULARGE_INTEGER _lastCPU = {};
	ULARGE_INTEGER _lastSysCPU = {};
	ULARGE_INTEGER _lastUserCPU = {};
	int _numProcessors = 0;
	HANDLE _self = nullptr;
	bool _cpuInitialized = false;

	// 콜백
	function<int32()> _getSessionCount;
	function<int32()> _getMaxSessionCount;
	function<int32()> _getPlayerCount;
	function<int32()> _getRoomCount;
};
