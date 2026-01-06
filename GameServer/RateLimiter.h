#pragma once

#include <unordered_map>


class TokenBucket
{
public:
    TokenBucket(int32 maxPerSecond = 10);
    bool TryConsume();

private:
    void Refill();

    int32  _tokens;
    int32  _maxTokens;
    uint64 _lastRefillTime;
};

class RateLimiter
{
public:
    void AddRule(uint16 packetId, int32 maxPerSecond);
    bool CheckRateLimit(uint16 packetId);

private:
    unordered_map<uint64, TokenBucket> _buckets;
};

