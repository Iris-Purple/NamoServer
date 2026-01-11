/**
 * @file test_rate_limiter.cpp
 * @brief TokenBucket 및 RateLimiter 단위 테스트
 *
 * 테스트 항목:
 * - TokenBucket 기본 동작
 * - 토큰 소비 및 리필
 * - RateLimiter 규칙 추가/확인
 * - 다중 패킷 ID 처리
 */

#include <gtest/gtest.h>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <cstdint>

// ========================================
// 테스트용 TokenBucket 클래스
// GetTickCount64() 대신 std::chrono 사용
// ========================================

class TestTokenBucket
{
public:
    TestTokenBucket(int32_t maxPerSecond = 10)
        : _tokens(maxPerSecond), _maxTokens(maxPerSecond),
          _lastRefillTime(std::chrono::steady_clock::now())
    {
    }

    bool TryConsume()
    {
        Refill();

        if (_tokens > 0)
        {
            _tokens--;
            return true;
        }
        return false;
    }

    int32_t GetTokens() const { return _tokens; }
    int32_t GetMaxTokens() const { return _maxTokens; }

    // 테스트용: 시간을 강제로 진행
    void AdvanceTime(int milliseconds)
    {
        _lastRefillTime -= std::chrono::milliseconds(milliseconds);
    }

private:
    void Refill()
    {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - _lastRefillTime).count();

        if (elapsed >= 1000)
        {
            _tokens = _maxTokens;
            _lastRefillTime = now;
        }
    }

    int32_t _tokens;
    int32_t _maxTokens;
    std::chrono::steady_clock::time_point _lastRefillTime;
};

class TestRateLimiter
{
public:
    void AddRule(uint16_t packetId, int32_t maxPerSecond)
    {
        _buckets.emplace(packetId, TestTokenBucket(maxPerSecond));
    }

    bool CheckRateLimit(uint16_t packetId)
    {
        auto it = _buckets.find(packetId);
        if (it == _buckets.end())
            return true;

        return it->second.TryConsume();
    }

    TestTokenBucket* GetBucket(uint16_t packetId)
    {
        auto it = _buckets.find(packetId);
        if (it == _buckets.end())
            return nullptr;
        return &it->second;
    }

private:
    std::unordered_map<uint16_t, TestTokenBucket> _buckets;
};

// ========================================
// TokenBucket 기본 동작 테스트
// ========================================

TEST(TokenBucketTest, InitialTokens)
{
    TestTokenBucket bucket(10);

    EXPECT_EQ(bucket.GetTokens(), 10);
    EXPECT_EQ(bucket.GetMaxTokens(), 10);
}

TEST(TokenBucketTest, DefaultTokens)
{
    TestTokenBucket bucket;

    EXPECT_EQ(bucket.GetMaxTokens(), 10);
}

TEST(TokenBucketTest, ConsumeToken)
{
    TestTokenBucket bucket(5);

    EXPECT_TRUE(bucket.TryConsume());
    EXPECT_EQ(bucket.GetTokens(), 4);

    EXPECT_TRUE(bucket.TryConsume());
    EXPECT_EQ(bucket.GetTokens(), 3);
}

TEST(TokenBucketTest, ConsumeAllTokens)
{
    TestTokenBucket bucket(3);

    EXPECT_TRUE(bucket.TryConsume());  // 2 left
    EXPECT_TRUE(bucket.TryConsume());  // 1 left
    EXPECT_TRUE(bucket.TryConsume());  // 0 left
    EXPECT_FALSE(bucket.TryConsume()); // denied
    EXPECT_FALSE(bucket.TryConsume()); // still denied
}

TEST(TokenBucketTest, RefillAfterOneSecond)
{
    TestTokenBucket bucket(5);

    // 모든 토큰 소비
    for (int i = 0; i < 5; ++i)
    {
        EXPECT_TRUE(bucket.TryConsume());
    }
    EXPECT_FALSE(bucket.TryConsume());

    // 1초 경과 시뮬레이션
    bucket.AdvanceTime(1000);

    // 토큰이 리필되어야 함
    EXPECT_TRUE(bucket.TryConsume());
    EXPECT_EQ(bucket.GetTokens(), 4);
}

TEST(TokenBucketTest, NoRefillBeforeOneSecond)
{
    TestTokenBucket bucket(5);

    // 모든 토큰 소비
    for (int i = 0; i < 5; ++i)
    {
        bucket.TryConsume();
    }

    // 999ms 경과 - 리필 안됨
    bucket.AdvanceTime(999);
    EXPECT_FALSE(bucket.TryConsume());
}

TEST(TokenBucketTest, SingleToken)
{
    TestTokenBucket bucket(1);

    EXPECT_TRUE(bucket.TryConsume());
    EXPECT_FALSE(bucket.TryConsume());

    bucket.AdvanceTime(1000);
    EXPECT_TRUE(bucket.TryConsume());
    EXPECT_FALSE(bucket.TryConsume());
}

// ========================================
// RateLimiter 테스트
// ========================================

TEST(RateLimiterTest, NoRule_AlwaysAllow)
{
    TestRateLimiter limiter;

    // 규칙이 없으면 항상 허용
    EXPECT_TRUE(limiter.CheckRateLimit(1000));
    EXPECT_TRUE(limiter.CheckRateLimit(1000));
    EXPECT_TRUE(limiter.CheckRateLimit(9999));
}

TEST(RateLimiterTest, AddRule)
{
    TestRateLimiter limiter;
    limiter.AddRule(1000, 5);

    auto* bucket = limiter.GetBucket(1000);
    ASSERT_NE(bucket, nullptr);
    EXPECT_EQ(bucket->GetMaxTokens(), 5);
}

TEST(RateLimiterTest, CheckRateLimit_WithRule)
{
    TestRateLimiter limiter;
    limiter.AddRule(1000, 3);

    EXPECT_TRUE(limiter.CheckRateLimit(1000));
    EXPECT_TRUE(limiter.CheckRateLimit(1000));
    EXPECT_TRUE(limiter.CheckRateLimit(1000));
    EXPECT_FALSE(limiter.CheckRateLimit(1000));
}

TEST(RateLimiterTest, MultiplePacketIds)
{
    TestRateLimiter limiter;
    limiter.AddRule(1000, 2);  // C2S_ENTER_GAME
    limiter.AddRule(2002, 10); // C2S_MOVE

    // 각 패킷 ID는 독립적인 버킷
    EXPECT_TRUE(limiter.CheckRateLimit(1000));
    EXPECT_TRUE(limiter.CheckRateLimit(1000));
    EXPECT_FALSE(limiter.CheckRateLimit(1000));

    // C2S_MOVE는 아직 10개 가능
    for (int i = 0; i < 10; ++i)
    {
        EXPECT_TRUE(limiter.CheckRateLimit(2002));
    }
    EXPECT_FALSE(limiter.CheckRateLimit(2002));
}

TEST(RateLimiterTest, UnregisteredPacket)
{
    TestRateLimiter limiter;
    limiter.AddRule(1000, 2);

    // 등록되지 않은 패킷 ID는 항상 허용
    EXPECT_TRUE(limiter.CheckRateLimit(9999));
    EXPECT_TRUE(limiter.CheckRateLimit(9999));
    EXPECT_TRUE(limiter.CheckRateLimit(9999));
}

// ========================================
// 실제 시나리오 테스트
// ========================================

TEST(RateLimiterScenarioTest, MoveSpamPrevention)
{
    TestRateLimiter limiter;

    // 이동 패킷: 초당 20회 제한
    const uint16_t PKT_C2S_MOVE = 2002;
    limiter.AddRule(PKT_C2S_MOVE, 20);

    // 정상 플레이어: 초당 10회 이동
    for (int i = 0; i < 10; ++i)
    {
        EXPECT_TRUE(limiter.CheckRateLimit(PKT_C2S_MOVE));
    }

    // 스피드핵 시도: 추가로 15회 시도
    int allowed = 0;
    int denied = 0;
    for (int i = 0; i < 15; ++i)
    {
        if (limiter.CheckRateLimit(PKT_C2S_MOVE))
            allowed++;
        else
            denied++;
    }

    // 10번 더 허용 (총 20회), 5번은 거부
    EXPECT_EQ(allowed, 10);
    EXPECT_EQ(denied, 5);
}

TEST(RateLimiterScenarioTest, SkillCooldownEnforcement)
{
    TestRateLimiter limiter;

    // 스킬 패킷: 초당 5회 제한 (기본 스킬 쿨다운)
    const uint16_t PKT_C2S_SKILL = 2004;
    limiter.AddRule(PKT_C2S_SKILL, 5);

    // 빠른 스킬 연타 시도
    int successCount = 0;
    for (int i = 0; i < 10; ++i)
    {
        if (limiter.CheckRateLimit(PKT_C2S_SKILL))
            successCount++;
    }

    EXPECT_EQ(successCount, 5);
}

TEST(RateLimiterScenarioTest, ChatFloodPrevention)
{
    TestRateLimiter limiter;

    // 채팅 패킷: 초당 3회 제한
    const uint16_t PKT_C2S_CHAT = 3000;
    limiter.AddRule(PKT_C2S_CHAT, 3);

    // 도배 시도
    EXPECT_TRUE(limiter.CheckRateLimit(PKT_C2S_CHAT));
    EXPECT_TRUE(limiter.CheckRateLimit(PKT_C2S_CHAT));
    EXPECT_TRUE(limiter.CheckRateLimit(PKT_C2S_CHAT));
    EXPECT_FALSE(limiter.CheckRateLimit(PKT_C2S_CHAT)); // 도배 차단

    // 1초 후 다시 가능
    auto* bucket = limiter.GetBucket(PKT_C2S_CHAT);
    bucket->AdvanceTime(1000);

    EXPECT_TRUE(limiter.CheckRateLimit(PKT_C2S_CHAT));
}

// ========================================
// 경계값 테스트
// ========================================

TEST(TokenBucketEdgeTest, ZeroTokens)
{
    // 0 토큰은 의미 없지만 크래시하면 안됨
    TestTokenBucket bucket(0);

    EXPECT_FALSE(bucket.TryConsume());
}

TEST(TokenBucketEdgeTest, LargeTokenCount)
{
    TestTokenBucket bucket(10000);

    for (int i = 0; i < 5000; ++i)
    {
        EXPECT_TRUE(bucket.TryConsume());
    }

    EXPECT_EQ(bucket.GetTokens(), 5000);
}

TEST(RateLimiterEdgeTest, DuplicateRule)
{
    TestRateLimiter limiter;

    limiter.AddRule(1000, 5);
    limiter.AddRule(1000, 10); // 중복 추가 시도

    // emplace는 기존 값을 덮어쓰지 않음
    auto* bucket = limiter.GetBucket(1000);
    EXPECT_EQ(bucket->GetMaxTokens(), 5);
}
