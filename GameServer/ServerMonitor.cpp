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

	cout << "\n";
	cout << "========================================" << endl;
	cout << "     SERVER MONITOR STARTED" << endl;
	cout << "     Interval: " << _intervalMs / 1000.0 << " seconds" << endl;
	cout << "========================================" << endl;
	cout << "\n";

	_monitorThread = thread(&ServerMonitor::MonitorLoop, this);
}

void ServerMonitor::Stop()
{
	if (!_running)
		return;

	_running = false;

	if (_monitorThread.joinable())
		_monitorThread.join();

	cout << "\n";
	cout << "========================================" << endl;
	cout << "     SERVER MONITOR STOPPED" << endl;
	cout << "========================================" << endl;
	cout << "\n";
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
	int32 roomCount = _getRoomCount ? _getRoomCount() : 0;

	double cpuUsage = GetCpuUsage();
	size_t memoryMB = GetMemoryUsageMB();
	size_t workingSetMB = GetWorkingSetMB();

	// 구간 통계 가져오기 및 리셋
	int64 intervalSent = _intervalPacketsSent.exchange(0);
	int64 intervalRecv = _intervalPacketsRecv.exchange(0);
	int64 intervalBytesSent = _intervalBytesSent.exchange(0);
	int64 intervalBytesRecv = _intervalBytesRecv.exchange(0);

	// PPS 계산 (패킷/초)
	double seconds = _intervalMs / 1000.0;
	int64 sendPPS = static_cast<int64>(intervalSent / seconds);
	int64 recvPPS = static_cast<int64>(intervalRecv / seconds);
	int64 totalPPS = sendPPS + recvPPS;

	// 대역폭 계산 (KB/s)
	double sendKBps = (intervalBytesSent / 1024.0) / seconds;
	double recvKBps = (intervalBytesRecv / 1024.0) / seconds;

	// 피크 업데이트
	if (sessionCount > _peakSessionCount) _peakSessionCount = sessionCount;
	if (playerCount > _peakPlayerCount) _peakPlayerCount = playerCount;
	if (cpuUsage > _peakCpuUsage) _peakCpuUsage = cpuUsage;
	if (memoryMB > _peakMemoryMB) _peakMemoryMB = memoryMB;
	if (totalPPS > _peakPPS) _peakPPS = totalPPS;

	// 시간 가져오기
	auto now = chrono::system_clock::now();
	auto time = chrono::system_clock::to_time_t(now);
	tm localTime;
	localtime_s(&localTime, &time);

	// 출력
	cout << "\n";
	cout << "┌──────────────────────────────────────────────────────────────┐" << endl;
	cout << "│                    SERVER STATUS REPORT                      │" << endl;
	cout << "│              " << put_time(&localTime, "%Y-%m-%d %H:%M:%S") << "                          │" << endl;
	cout << "├──────────────────────────────────────────────────────────────┤" << endl;

	// 연결 정보
	cout << "│  [CONNECTIONS]                                               │" << endl;
	cout << "│    Current Sessions : " << setw(6) << sessionCount
		 << " / " << setw(6) << maxSessionCount
		 << "  (Peak: " << setw(6) << _peakSessionCount << ")       │" << endl;
	cout << "│    Active Players   : " << setw(6) << playerCount
		 << "            (Peak: " << setw(6) << _peakPlayerCount << ")       │" << endl;
	cout << "│    Active Rooms     : " << setw(6) << roomCount
		 << "                                   │" << endl;

	cout << "├──────────────────────────────────────────────────────────────┤" << endl;

	// 시스템 리소스
	cout << "│  [SYSTEM RESOURCES]                                          │" << endl;

	// CPU 바
	int cpuBarLen = static_cast<int>(cpuUsage / 5); // 20칸 = 100%
	cout << "│    CPU Usage        : " << setw(5) << fixed << setprecision(1) << cpuUsage << "% [";
	for (int i = 0; i < 20; i++)
		cout << (i < cpuBarLen ? "█" : "░");
	cout << "]   │" << endl;

	cout << "│    Memory (Commit)  : " << setw(6) << memoryMB << " MB"
		 << "         (Peak: " << setw(6) << _peakMemoryMB << " MB)   │" << endl;
	cout << "│    Memory (Working) : " << setw(6) << workingSetMB << " MB"
		 << "                              │" << endl;

	cout << "├──────────────────────────────────────────────────────────────┤" << endl;

	// 네트워크 통계
	cout << "│  [NETWORK STATS]                                             │" << endl;
	cout << "│    Packets/sec      : Send " << setw(6) << sendPPS
		 << "  |  Recv " << setw(6) << recvPPS
		 << "  |  Total " << setw(6) << totalPPS << " │" << endl;
	cout << "│    Bandwidth (KB/s) : Send " << setw(6) << fixed << setprecision(1) << sendKBps
		 << "  |  Recv " << setw(6) << recvKBps << "               │" << endl;
	cout << "│    Peak PPS         : " << setw(6) << _peakPPS << "                                    │" << endl;

	cout << "├──────────────────────────────────────────────────────────────┤" << endl;

	// 누적 통계
	cout << "│  [CUMULATIVE]                                                │" << endl;
	cout << "│    Total Sent       : " << setw(10) << _totalPacketsSent.load() << " packets  ("
		 << setw(8) << fixed << setprecision(2) << (_totalBytesSent.load() / (1024.0 * 1024.0)) << " MB)  │" << endl;
	cout << "│    Total Recv       : " << setw(10) << _totalPacketsRecv.load() << " packets  ("
		 << setw(8) << fixed << setprecision(2) << (_totalBytesRecv.load() / (1024.0 * 1024.0)) << " MB)  │" << endl;

	cout << "└──────────────────────────────────────────────────────────────┘" << endl;
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
