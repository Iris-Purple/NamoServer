#pragma once

class JobStats
{
public:
	JobStats() = default;
	~JobStats() = default;

	void Record(uint64 waitTime, uint64 processTime)
	{
		_totalWaitTime.fetch_add(waitTime);
		_totalProcessTime.fetch_add(processTime);
		_jobCount.fetch_add(1);

		// max °»½Å (lock-free)
		uint64 prevMax = _maxWaitTime.load();
		while (prevMax < waitTime &&
			!_maxWaitTime.compare_exchange_weak(prevMax, waitTime));

		prevMax = _maxProcessTime.load();
		while (prevMax < processTime &&
			!_maxProcessTime.compare_exchange_weak(prevMax, processTime));
	}

	void PrintAndReset()
	{
		int64 jobCount = _jobCount.exchange(0);
		if (jobCount == 0)
			return;

		uint64 totalWait = _totalWaitTime.exchange(0);
		uint64 totalProcess = _totalProcessTime.exchange(0);
		uint64 maxWait = _maxWaitTime.exchange(0);
		uint64 maxProcess = _maxProcessTime.exchange(0);

		uint64 avgWait = totalWait / jobCount;
		uint64 avgProcess = totalProcess / jobCount;

		cout << "[Jobs] Wait: " << avgWait << "ms (max: " << maxWait << "ms)"
			<< " | Process: " << avgProcess << "ms (max: " << maxProcess << "ms)"
			<< " | Count: " << jobCount
			<< endl;
	}

private:
	atomic<uint64> _totalWaitTime = 0;
	atomic<uint64> _totalProcessTime = 0;
	atomic<uint64> _maxWaitTime = 0;
	atomic<uint64> _maxProcessTime = 0;
	atomic<int64> _jobCount = 0;
};
