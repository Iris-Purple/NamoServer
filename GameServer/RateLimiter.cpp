#include "pch.h"
#include "RateLimiter.h"


TokenBucket::TokenBucket(int32 maxPerSecond)
	: _tokens(maxPerSecond), _maxTokens(maxPerSecond),
	  _lastRefillTime(::GetTickCount64())
{ }

bool TokenBucket::TryConsume()
{
	Refill();
	
	if (_tokens > 0)
	{
		_tokens--;
		return true;
	}
	return false;
}

void TokenBucket::Refill()
{
	uint64 now = ::GetTickCount64();
	uint64 elapsed = now - _lastRefillTime;

	if (elapsed >= 1000)  // 1ÃÊ °æ°ú
	{
		_tokens = _maxTokens;
		_lastRefillTime = now;
	}
}

void RateLimiter::AddRule(uint16 packetId, int32 maxPerSecond)
{
	_buckets.emplace(packetId, TokenBucket(maxPerSecond));
}

bool RateLimiter::CheckRateLimit(uint16 packetId)
{
	auto it = _buckets.find(packetId);
	if (it == _buckets.end())
		return true;
	
	return it->second.TryConsume();
}

