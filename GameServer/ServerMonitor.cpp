#include "pch.h"
#include "ServerMonitor.h"
#include <iomanip>

ServerMonitor::~ServerMonitor()
{
	Stop();
}

void ServerMonitor::Start(int32 intervalMs)
{
	if (_running)
		return;

	_intervalMs = intervalMs;
	_running = true;

	cout << "[Monitor] Started (interval: " << _intervalMs / 1000 << "s)" << endl;

	_monitorThread = thread(&ServerMonitor::MonitorLoop, this);
}

void ServerMonitor::Stop()
{
	if (!_running)
		return;

	_running = false;

	if (_monitorThread.joinable())
		_monitorThread.join();

	cout << "[Monitor] Stopped" << endl;
}

void ServerMonitor::OnPacketSend(int32 bytes)
{
	_totalPacketsSent++;
	_totalBytesSent += bytes;
	_intervalPacketsSent++;
	_intervalBytesSent += bytes;
}

void ServerMonitor::OnPacketRecv(int32 bytes)
{
	_totalPacketsRecv++;
	_totalBytesRecv += bytes;
	_intervalPacketsRecv++;
	_intervalBytesRecv += bytes;
}

// TLS로 각 스레드별 시작 시간 저장
thread_local chrono::high_resolution_clock::time_point tls_packetStartTime;

void ServerMonitor::OnPacketProcessStart()
{
	tls_packetStartTime = chrono::high_resolution_clock::now();
}

void ServerMonitor::OnPacketProcessEnd()
{
	auto endTime = chrono::high_resolution_clock::now();
	auto durationUs = chrono::duration_cast<chrono::microseconds>(endTime - tls_packetStartTime).count();

	_totalLatencyUs += durationUs;
	_latencyCount++;
	_intervalLatencyUs += durationUs;
	_intervalLatencyCount++;

	// 최대값 업데이트 (atomic max)
	int64 prevMax = _maxLatencyUs.load();
	while (durationUs > prevMax && !_maxLatencyUs.compare_exchange_weak(prevMax, durationUs));
}

void ServerMonitor::MonitorLoop()
{
	while (_running)
	{
		this_thread::sleep_for(chrono::milliseconds(_intervalMs));

		if (_running)
			PrintStats();
	}
}

void ServerMonitor::PrintStats()
{
	// 현재 값 수집
	int32 sessionCount = _getSessionCount ? _getSessionCount() : 0;
	int32 maxSessionCount = _getMaxSessionCount ? _getMaxSessionCount() : 0;

	// 구간 통계 가져오기 및 리셋
	int64 intervalSent = _intervalPacketsSent.exchange(0);
	int64 intervalRecv = _intervalPacketsRecv.exchange(0);
	int64 intervalLatencyUs = _intervalLatencyUs.exchange(0);
	int64 intervalLatencyCount = _intervalLatencyCount.exchange(0);
	int64 maxLatencyUs = _maxLatencyUs.exchange(0);

	// PPS 계산 (패킷/초)
	double seconds = _intervalMs / 1000.0;
	int64 totalPPS = static_cast<int64>((intervalSent + intervalRecv) / seconds);

	// 평균 Latency 계산 (ms)
	double avgLatencyMs = (intervalLatencyCount > 0) ? (intervalLatencyUs / 1000.0) / intervalLatencyCount : 0.0;
	double maxLatencyMs = maxLatencyUs / 1000.0;

	// 간단한 한 줄 출력
	cout << "[Stats] Sessions: " << sessionCount << "/" << maxSessionCount
		 << " | PPS: " << totalPPS
		 << " | Latency: " << fixed << setprecision(2) << avgLatencyMs << "ms (max:" << maxLatencyMs << "ms)"
		 << endl;
}
