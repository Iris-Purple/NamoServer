#include "pch.h"
#include "ServerMonitor.h"
#include <psapi.h>
#include <iomanip>
#include <sstream>

#pragma comment(lib, "psapi.lib")

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

	// CPU 측정 초기화
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	_numProcessors = sysInfo.dwNumberOfProcessors;
	_self = GetCurrentProcess();
	_cpuInitialized = false;

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
	int32 playerCount = _getPlayerCount ? _getPlayerCount() : 0;

	double cpuUsage = GetCpuUsage();
	size_t memoryMB = GetMemoryUsageMB();

	// 구간 통계 가져오기 및 리셋
	int64 intervalSent = _intervalPacketsSent.exchange(0);
	int64 intervalRecv = _intervalPacketsRecv.exchange(0);

	// PPS 계산 (패킷/초)
	double seconds = _intervalMs / 1000.0;
	int64 totalPPS = static_cast<int64>((intervalSent + intervalRecv) / seconds);

	// 피크 업데이트
	if (sessionCount > _peakSessionCount) _peakSessionCount = sessionCount;
	if (totalPPS > _peakPPS) _peakPPS = totalPPS;

	// 간단한 한 줄 출력
	cout << "[Stats] Sessions: " << sessionCount << "/" << maxSessionCount
		 << " (peak:" << _peakSessionCount << ")"
		 << " | Players: " << playerCount
		 << " | CPU: " << fixed << setprecision(1) << cpuUsage << "%"
		 << " | Mem: " << memoryMB << "MB"
		 << " | PPS: " << totalPPS << " (peak:" << _peakPPS << ")"
		 << endl;
}

double ServerMonitor::GetCpuUsage()
{
	FILETIME ftime, fsys, fuser;
	ULARGE_INTEGER now, sys, user;

	GetSystemTimeAsFileTime(&ftime);
	memcpy(&now, &ftime, sizeof(FILETIME));

	if (!GetProcessTimes(_self, &ftime, &ftime, &fsys, &fuser))
		return 0.0;

	memcpy(&sys, &fsys, sizeof(FILETIME));
	memcpy(&user, &fuser, sizeof(FILETIME));

	if (!_cpuInitialized)
	{
		_lastCPU = now;
		_lastSysCPU = sys;
		_lastUserCPU = user;
		_cpuInitialized = true;
		return 0.0;
	}

	double percent = static_cast<double>((sys.QuadPart - _lastSysCPU.QuadPart) +
		(user.QuadPart - _lastUserCPU.QuadPart));
	percent /= (now.QuadPart - _lastCPU.QuadPart);
	percent /= _numProcessors;
	percent *= 100.0;

	_lastCPU = now;
	_lastSysCPU = sys;
	_lastUserCPU = user;

	return percent;
}

size_t ServerMonitor::GetMemoryUsageMB()
{
	PROCESS_MEMORY_COUNTERS_EX pmc;
	if (GetProcessMemoryInfo(_self, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
	{
		return pmc.PrivateUsage / (1024 * 1024);
	}
	return 0;
}

size_t ServerMonitor::GetWorkingSetMB()
{
	PROCESS_MEMORY_COUNTERS_EX pmc;
	if (GetProcessMemoryInfo(_self, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
	{
		return pmc.WorkingSetSize / (1024 * 1024);
	}
	return 0;
}
