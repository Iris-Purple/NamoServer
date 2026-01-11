/**
 * @file test_data.cpp
 * @brief 게임 데이터 파싱 단위 테스트
 *
 * 테스트 항목:
 * - Stat 데이터 파싱
 * - Skill 데이터 파싱
 * - JSON 역직렬화
 * - Dictionary 변환
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <vector>
#include <unordered_map>
#include <optional>
#include <string>

using json = nlohmann::json;

// ========================================
// 테스트용 데이터 구조체 (Stat.h에서 추출)
// Protocol 의존성 제거
// ========================================

namespace TestData
{
    struct Stat
    {
        int level;
        int maxHp;
        int attack;
        float speed;
        int totalExp;
    };

    inline void from_json(const json& j, Stat& stat)
    {
        j.at("level").get_to(stat.level);
        j.at("maxHp").get_to(stat.maxHp);
        j.at("attack").get_to(stat.attack);
        j.at("speed").get_to(stat.speed);
        j.at("totalExp").get_to(stat.totalExp);
    }

    struct StatData
    {
        std::vector<Stat> stats;

        std::unordered_map<int, Stat> MakeDict() const
        {
            std::unordered_map<int, Stat> dict;
            for (const Stat& stat : stats)
            {
                dict[stat.level] = stat;
            }
            return dict;
        }
    };

    inline void from_json(const json& j, StatData& data)
    {
        j.at("stats").get_to(data.stats);
    }

    // SkillType을 enum으로 정의
    enum class SkillType
    {
        None = 0,
        SkillAuto = 1,
        SkillProjectile = 2
    };

    struct ProjectileInfo
    {
        std::string name;
        float speed;
        int32_t range;
        std::string prefab;
    };

    inline void from_json(const json& j, ProjectileInfo& info)
    {
        j.at("name").get_to(info.name);
        j.at("speed").get_to(info.speed);
        j.at("range").get_to(info.range);
        j.at("prefab").get_to(info.prefab);
    }

    struct Skill
    {
        int32_t id;
        std::string name;
        float cooldown;
        int32_t damage;
        SkillType skillType;
        std::optional<ProjectileInfo> projectile;
    };

    inline void from_json(const json& j, Skill& skill)
    {
        j.at("id").get_to(skill.id);
        j.at("name").get_to(skill.name);
        j.at("cooldown").get_to(skill.cooldown);
        j.at("damage").get_to(skill.damage);
        skill.skillType = static_cast<SkillType>(j.at("skillType").get<int>());

        if (j.contains("projectile") && !j["projectile"].is_null())
        {
            skill.projectile = j["projectile"].get<ProjectileInfo>();
        }
    }

    struct SkillData
    {
        std::vector<Skill> skills;

        std::unordered_map<int, Skill> MakeDict() const
        {
            std::unordered_map<int, Skill> dict;
            for (const Skill& skill : skills)
            {
                dict[skill.id] = skill;
            }
            return dict;
        }
    };

    inline void from_json(const json& j, SkillData& data)
    {
        j.at("skills").get_to(data.skills);
    }
}

// ========================================
// Stat 파싱 테스트
// ========================================

TEST(StatParsingTest, SingleStat)
{
    json j = {
        {"level", 1},
        {"maxHp", 100},
        {"attack", 10},
        {"speed", 5.0f},
        {"totalExp", 0}
    };

    TestData::Stat stat = j.get<TestData::Stat>();

    EXPECT_EQ(stat.level, 1);
    EXPECT_EQ(stat.maxHp, 100);
    EXPECT_EQ(stat.attack, 10);
    EXPECT_FLOAT_EQ(stat.speed, 5.0f);
    EXPECT_EQ(stat.totalExp, 0);
}

TEST(StatParsingTest, StatArray)
{
    json j = {
        {"stats", {
            {{"level", 1}, {"maxHp", 100}, {"attack", 10}, {"speed", 5.0}, {"totalExp", 0}},
            {{"level", 2}, {"maxHp", 150}, {"attack", 15}, {"speed", 5.5}, {"totalExp", 100}},
            {{"level", 3}, {"maxHp", 200}, {"attack", 20}, {"speed", 6.0}, {"totalExp", 250}}
        }}
    };

    TestData::StatData data = j.get<TestData::StatData>();

    EXPECT_EQ(data.stats.size(), 3);
    EXPECT_EQ(data.stats[0].level, 1);
    EXPECT_EQ(data.stats[1].level, 2);
    EXPECT_EQ(data.stats[2].level, 3);
}

TEST(StatParsingTest, MakeDict)
{
    json j = {
        {"stats", {
            {{"level", 1}, {"maxHp", 100}, {"attack", 10}, {"speed", 5.0}, {"totalExp", 0}},
            {{"level", 5}, {"maxHp", 500}, {"attack", 50}, {"speed", 8.0}, {"totalExp", 1000}}
        }}
    };

    TestData::StatData data = j.get<TestData::StatData>();
    auto dict = data.MakeDict();

    EXPECT_EQ(dict.size(), 2);
    EXPECT_EQ(dict[1].maxHp, 100);
    EXPECT_EQ(dict[5].maxHp, 500);
    EXPECT_EQ(dict.count(3), 0); // 레벨 3은 없음
}

TEST(StatParsingTest, LevelProgression)
{
    json j = {
        {"stats", {
            {{"level", 1}, {"maxHp", 100}, {"attack", 10}, {"speed", 5.0}, {"totalExp", 0}},
            {{"level", 2}, {"maxHp", 150}, {"attack", 15}, {"speed", 5.5}, {"totalExp", 100}},
            {{"level", 3}, {"maxHp", 200}, {"attack", 20}, {"speed", 6.0}, {"totalExp", 250}},
            {{"level", 4}, {"maxHp", 250}, {"attack", 25}, {"speed", 6.5}, {"totalExp", 450}},
            {{"level", 5}, {"maxHp", 300}, {"attack", 30}, {"speed", 7.0}, {"totalExp", 700}}
        }}
    };

    TestData::StatData data = j.get<TestData::StatData>();
    auto dict = data.MakeDict();

    // HP가 레벨에 따라 증가하는지 확인
    for (int i = 1; i < 5; ++i)
    {
        EXPECT_LT(dict[i].maxHp, dict[i + 1].maxHp);
        EXPECT_LT(dict[i].attack, dict[i + 1].attack);
    }
}

// ========================================
// Skill 파싱 테스트
// ========================================

TEST(SkillParsingTest, BasicSkill)
{
    json j = {
        {"id", 1},
        {"name", "Auto Attack"},
        {"cooldown", 0.5f},
        {"damage", 10},
        {"skillType", 1}
    };

    TestData::Skill skill = j.get<TestData::Skill>();

    EXPECT_EQ(skill.id, 1);
    EXPECT_EQ(skill.name, "Auto Attack");
    EXPECT_FLOAT_EQ(skill.cooldown, 0.5f);
    EXPECT_EQ(skill.damage, 10);
    EXPECT_EQ(skill.skillType, TestData::SkillType::SkillAuto);
    EXPECT_FALSE(skill.projectile.has_value());
}

TEST(SkillParsingTest, SkillWithProjectile)
{
    json j = {
        {"id", 2},
        {"name", "Arrow Shot"},
        {"cooldown", 1.0f},
        {"damage", 25},
        {"skillType", 2},
        {"projectile", {
            {"name", "Arrow"},
            {"speed", 10.0f},
            {"range", 5},
            {"prefab", "Prefabs/Arrow"}
        }}
    };

    TestData::Skill skill = j.get<TestData::Skill>();

    EXPECT_EQ(skill.id, 2);
    EXPECT_EQ(skill.skillType, TestData::SkillType::SkillProjectile);
    EXPECT_TRUE(skill.projectile.has_value());

    auto& proj = skill.projectile.value();
    EXPECT_EQ(proj.name, "Arrow");
    EXPECT_FLOAT_EQ(proj.speed, 10.0f);
    EXPECT_EQ(proj.range, 5);
    EXPECT_EQ(proj.prefab, "Prefabs/Arrow");
}

TEST(SkillParsingTest, SkillWithNullProjectile)
{
    json j = {
        {"id", 1},
        {"name", "Punch"},
        {"cooldown", 0.3f},
        {"damage", 5},
        {"skillType", 1},
        {"projectile", nullptr}
    };

    TestData::Skill skill = j.get<TestData::Skill>();

    EXPECT_FALSE(skill.projectile.has_value());
}

TEST(SkillParsingTest, SkillArray)
{
    json j = {
        {"skills", {
            {{"id", 1}, {"name", "Auto Attack"}, {"cooldown", 0.5}, {"damage", 10}, {"skillType", 1}},
            {{"id", 2}, {"name", "Arrow Shot"}, {"cooldown", 1.0}, {"damage", 25}, {"skillType", 2},
             {"projectile", {{"name", "Arrow"}, {"speed", 10.0}, {"range", 5}, {"prefab", "Arrow"}}}},
            {{"id", 3}, {"name", "Fireball"}, {"cooldown", 2.0}, {"damage", 50}, {"skillType", 2},
             {"projectile", {{"name", "Fireball"}, {"speed", 8.0}, {"range", 7}, {"prefab", "Fire"}}}}
        }}
    };

    TestData::SkillData data = j.get<TestData::SkillData>();

    EXPECT_EQ(data.skills.size(), 3);
    EXPECT_FALSE(data.skills[0].projectile.has_value());
    EXPECT_TRUE(data.skills[1].projectile.has_value());
    EXPECT_TRUE(data.skills[2].projectile.has_value());
}

TEST(SkillParsingTest, SkillMakeDict)
{
    json j = {
        {"skills", {
            {{"id", 1}, {"name", "Auto"}, {"cooldown", 0.5}, {"damage", 10}, {"skillType", 1}},
            {{"id", 10}, {"name", "Special"}, {"cooldown", 5.0}, {"damage", 100}, {"skillType", 2},
             {"projectile", {{"name", "Beam"}, {"speed", 20.0}, {"range", 10}, {"prefab", "Beam"}}}}
        }}
    };

    TestData::SkillData data = j.get<TestData::SkillData>();
    auto dict = data.MakeDict();

    EXPECT_EQ(dict.size(), 2);
    EXPECT_EQ(dict[1].name, "Auto");
    EXPECT_EQ(dict[10].name, "Special");
    EXPECT_EQ(dict.count(5), 0);
}

// ========================================
// 실제 게임 데이터 시뮬레이션
// ========================================

TEST(GameDataTest, RealStatData)
{
    // 실제 StatData.json과 유사한 구조
    json j = {
        {"stats", {
            {{"level", 1}, {"maxHp", 200}, {"attack", 20}, {"speed", 5.0}, {"totalExp", 0}},
            {{"level", 2}, {"maxHp", 250}, {"attack", 25}, {"speed", 5.2}, {"totalExp", 50}},
            {{"level", 3}, {"maxHp", 300}, {"attack", 30}, {"speed", 5.4}, {"totalExp", 120}},
            {{"level", 4}, {"maxHp", 350}, {"attack", 35}, {"speed", 5.6}, {"totalExp", 200}},
            {{"level", 5}, {"maxHp", 400}, {"attack", 40}, {"speed", 5.8}, {"totalExp", 300}}
        }}
    };

    TestData::StatData data = j.get<TestData::StatData>();
    auto dict = data.MakeDict();

    // 레벨 1 플레이어 스탯 조회
    ASSERT_NE(dict.count(1), 0);
    EXPECT_EQ(dict[1].maxHp, 200);
    EXPECT_EQ(dict[1].attack, 20);

    // 레벨업 시 스탯 증가량 계산
    int hpGain = dict[2].maxHp - dict[1].maxHp;
    EXPECT_EQ(hpGain, 50);
}

TEST(GameDataTest, RealSkillData)
{
    // 실제 SkillData.json과 유사한 구조
    json j = {
        {"skills", {
            {
                {"id", 1},
                {"name", "Auto Attack"},
                {"cooldown", 0.5},
                {"damage", 10},
                {"skillType", 1}
            },
            {
                {"id", 2},
                {"name", "Arrow Shot"},
                {"cooldown", 0.3},
                {"damage", 6},
                {"skillType", 2},
                {"projectile", {
                    {"name", "Arrow"},
                    {"speed", 15.0},
                    {"range", 8},
                    {"prefab", "Prefabs/Arrow"}
                }}
            },
            {
                {"id", 3},
                {"name", "Fireball"},
                {"cooldown", 2.0},
                {"damage", 30},
                {"skillType", 2},
                {"projectile", {
                    {"name", "Fireball"},
                    {"speed", 10.0},
                    {"range", 6},
                    {"prefab", "Prefabs/Fireball"}
                }}
            }
        }}
    };

    TestData::SkillData data = j.get<TestData::SkillData>();
    auto dict = data.MakeDict();

    // 스킬 ID로 조회
    ASSERT_NE(dict.count(1), 0);
    ASSERT_NE(dict.count(2), 0);
    ASSERT_NE(dict.count(3), 0);

    // 쿨다운 검증
    EXPECT_FLOAT_EQ(dict[1].cooldown, 0.5f);
    EXPECT_FLOAT_EQ(dict[3].cooldown, 2.0f);

    // 투사체 스킬 확인
    EXPECT_TRUE(dict[2].projectile.has_value());
    EXPECT_EQ(dict[2].projectile->range, 8);
}

// ========================================
// 에러 처리 테스트
// ========================================

TEST(DataErrorTest, MissingField)
{
    json j = {
        {"level", 1},
        {"maxHp", 100}
        // attack, speed, totalExp 누락
    };

    EXPECT_THROW(
        j.get<TestData::Stat>(),
        json::out_of_range
    );
}

TEST(DataErrorTest, WrongType)
{
    json j = {
        {"level", "one"},  // 문자열이어야 하는데 정수 기대
        {"maxHp", 100},
        {"attack", 10},
        {"speed", 5.0},
        {"totalExp", 0}
    };

    EXPECT_THROW(
        j.get<TestData::Stat>(),
        json::type_error
    );
}

TEST(DataErrorTest, EmptyStats)
{
    json j = {
        {"stats", json::array()}
    };

    TestData::StatData data = j.get<TestData::StatData>();

    EXPECT_EQ(data.stats.size(), 0);
    EXPECT_EQ(data.MakeDict().size(), 0);
}

// ========================================
// 경계값 테스트
// ========================================

TEST(DataEdgeTest, LargeValues)
{
    json j = {
        {"level", 100},
        {"maxHp", 999999},
        {"attack", 99999},
        {"speed", 100.0},
        {"totalExp", 10000000}
    };

    TestData::Stat stat = j.get<TestData::Stat>();

    EXPECT_EQ(stat.level, 100);
    EXPECT_EQ(stat.maxHp, 999999);
}

TEST(DataEdgeTest, ZeroValues)
{
    json j = {
        {"level", 0},
        {"maxHp", 0},
        {"attack", 0},
        {"speed", 0.0},
        {"totalExp", 0}
    };

    TestData::Stat stat = j.get<TestData::Stat>();

    EXPECT_EQ(stat.level, 0);
    EXPECT_EQ(stat.maxHp, 0);
}

TEST(DataEdgeTest, NegativeValues)
{
    json j = {
        {"level", -1},
        {"maxHp", -100},
        {"attack", -10},
        {"speed", -5.0},
        {"totalExp", -50}
    };

    // 음수도 파싱은 됨 (검증은 게임 로직에서)
    TestData::Stat stat = j.get<TestData::Stat>();

    EXPECT_EQ(stat.level, -1);
    EXPECT_EQ(stat.maxHp, -100);
}

TEST(DataEdgeTest, FloatPrecision)
{
    json j = {
        {"level", 1},
        {"maxHp", 100},
        {"attack", 10},
        {"speed", 5.123456789},
        {"totalExp", 0}
    };

    TestData::Stat stat = j.get<TestData::Stat>();

    EXPECT_NEAR(stat.speed, 5.123456789f, 0.0001f);
}

TEST(DataEdgeTest, UnicodeSkillName)
{
    json j = {
        {"id", 1},
        {"name", "화염구"},  // 한글
        {"cooldown", 1.0},
        {"damage", 50},
        {"skillType", 2},
        {"projectile", {
            {"name", "불덩이"},
            {"speed", 8.0},
            {"range", 5},
            {"prefab", "Prefabs/Fire"}
        }}
    };

    TestData::Skill skill = j.get<TestData::Skill>();

    EXPECT_EQ(skill.name, "화염구");
    EXPECT_EQ(skill.projectile->name, "불덩이");
}
