#pragma once

class SendBufferChunk;

/*----------------
	SendBuffer
-----------------*/

class SendBuffer : enable_shared_from_this<SendBuffer>
{
public:
	SendBuffer(int32 bufferSize);
	~SendBuffer();

	BYTE* Buffer() { return _buffer.data(); }
	int32 WriteSize() { return _writeSize; }
	int32 Capacity() { return static_cast<int32>(_buffer.size()); }

	void CopyData(void* data, int32 len);
	void Close(uint32 writeSize);

	// 레이턴시 측정용 타임스탬프
	void SetStartTime() { _startTime = chrono::steady_clock::now(); }
	chrono::steady_clock::time_point GetStartTime() const { return _startTime; }

private:
	vector<BYTE>	_buffer;
	int32			_writeSize = 0;
	chrono::steady_clock::time_point _startTime;
};

