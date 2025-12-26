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

void ServerMonitor::OnTransaction()
{
	_totalTransactions++;
	_intervalTransactions++;
}

void ServerMonitor::OnLatency(int64 latencyUs)
{
	_totalLatency += latencyUs;
	_latencyCount++;

	// 최대값 갱신 (lock-free)
	int64 prevMax = _maxLatency.load();
	while (prevMax < latencyUs && !_maxLatency.compare_exchange_weak(prevMax, latencyUs))
	{
	}
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
	int64 intervalTransactions = _intervalTransactions.exchange(0);

	// 레이턴시 통계 가져오기 및 리셋
	int64 totalLatency = _totalLatency.exchange(0);
	int64 latencyCount = _latencyCount.exchange(0);
	int64 maxLatency = _maxLatency.exchange(0);

	// TPS 계산 (트랜잭션/초)
	double seconds = _intervalMs / 1000.0;
	int64 tps = static_cast<int64>(intervalTransactions / seconds);

	// 평균 레이턴시 계산 (마이크로초 -> 밀리초)
	double avgLatencyMs = (latencyCount > 0) ? (totalLatency / static_cast<double>(latencyCount)) / 1000.0 : 0.0;
	double maxLatencyMs = maxLatency / 1000.0;

	// 간단한 한 줄 출력
	cout << "[Stats] Sessions: " << sessionCount << "/" << maxSessionCount
		 << " | TPS: " << tps
		 << " | Latency: " << fixed << setprecision(2) << avgLatencyMs << "ms (max: " << maxLatencyMs << "ms)"
		 << endl;
}
