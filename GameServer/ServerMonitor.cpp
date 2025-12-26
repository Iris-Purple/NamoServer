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

	// TPS 계산 (트랜잭션/초)
	double seconds = _intervalMs / 1000.0;
	int64 tps = static_cast<int64>(intervalTransactions / seconds);

	// 간단한 한 줄 출력
	cout << "[Stats] Sessions: " << sessionCount << "/" << maxSessionCount
		 << " | TPS: " << tps
		 << endl;
}
