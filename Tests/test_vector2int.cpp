/**
 * @file test_vector2int.cpp
 * @brief Vector2Int 구조체 단위 테스트
 *
 * 테스트 항목:
 * - 기본 생성자
 * - 정적 방향 함수 (Up, Down, Left, Right)
 * - 산술 연산 (+, -, +=)
 * - 비교 연산 (==)
 * - 거리/크기 계산 (magnitude, sqrMagnitude, CellDistFromZero)
 */

#include <gtest/gtest.h>
#include <cmath>

// Vector2Int 구조체 정의 (Map.h에서 추출)
struct Vector2Int
{
    int x;
    int y;

    Vector2Int() : x(0), y(0) {}
    Vector2Int(int x, int y) : x(x), y(y) {}

    static Vector2Int Up() { return Vector2Int(0, 1); }
    static Vector2Int Down() { return Vector2Int(0, -1); }
    static Vector2Int Left() { return Vector2Int(-1, 0); }
    static Vector2Int Right() { return Vector2Int(1, 0); }

    int sqrMagnitude() const { return x * x + y * y; }
    float magnitude() const { return std::sqrt(static_cast<float>(sqrMagnitude())); }
    int CellDistFromZero() const { return std::abs(x) + std::abs(y); }

    Vector2Int operator+(const Vector2Int& other) const
    {
        return Vector2Int(x + other.x, y + other.y);
    }
    Vector2Int operator-(const Vector2Int& other) const
    {
        return Vector2Int(x - other.x, y - other.y);
    }

    bool operator==(const Vector2Int& other) const
    {
        return x == other.x && y == other.y;
    }

    Vector2Int& operator+=(const Vector2Int& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }
};

// ========================================
// 생성자 테스트
// ========================================

TEST(Vector2IntTest, DefaultConstructor)
{
    Vector2Int v;
    EXPECT_EQ(v.x, 0);
    EXPECT_EQ(v.y, 0);
}

TEST(Vector2IntTest, ParameterizedConstructor)
{
    Vector2Int v(3, 5);
    EXPECT_EQ(v.x, 3);
    EXPECT_EQ(v.y, 5);
}

TEST(Vector2IntTest, NegativeValues)
{
    Vector2Int v(-10, -20);
    EXPECT_EQ(v.x, -10);
    EXPECT_EQ(v.y, -20);
}

// ========================================
// 정적 방향 함수 테스트
// ========================================

TEST(Vector2IntTest, DirectionUp)
{
    Vector2Int up = Vector2Int::Up();
    EXPECT_EQ(up.x, 0);
    EXPECT_EQ(up.y, 1);
}

TEST(Vector2IntTest, DirectionDown)
{
    Vector2Int down = Vector2Int::Down();
    EXPECT_EQ(down.x, 0);
    EXPECT_EQ(down.y, -1);
}

TEST(Vector2IntTest, DirectionLeft)
{
    Vector2Int left = Vector2Int::Left();
    EXPECT_EQ(left.x, -1);
    EXPECT_EQ(left.y, 0);
}

TEST(Vector2IntTest, DirectionRight)
{
    Vector2Int right = Vector2Int::Right();
    EXPECT_EQ(right.x, 1);
    EXPECT_EQ(right.y, 0);
}

// ========================================
// 산술 연산 테스트
// ========================================

TEST(Vector2IntTest, AdditionOperator)
{
    Vector2Int a(3, 4);
    Vector2Int b(1, 2);
    Vector2Int result = a + b;

    EXPECT_EQ(result.x, 4);
    EXPECT_EQ(result.y, 6);
}

TEST(Vector2IntTest, SubtractionOperator)
{
    Vector2Int a(5, 10);
    Vector2Int b(2, 3);
    Vector2Int result = a - b;

    EXPECT_EQ(result.x, 3);
    EXPECT_EQ(result.y, 7);
}

TEST(Vector2IntTest, AdditionAssignmentOperator)
{
    Vector2Int a(1, 2);
    Vector2Int b(3, 4);
    a += b;

    EXPECT_EQ(a.x, 4);
    EXPECT_EQ(a.y, 6);
}

TEST(Vector2IntTest, ChainedAddition)
{
    Vector2Int a(1, 1);
    Vector2Int b(2, 2);
    Vector2Int c(3, 3);
    Vector2Int result = a + b + c;

    EXPECT_EQ(result.x, 6);
    EXPECT_EQ(result.y, 6);
}

TEST(Vector2IntTest, MovementSimulation)
{
    // 플레이어가 (0,0)에서 시작하여 오른쪽으로 3칸, 위로 2칸 이동
    Vector2Int position(0, 0);

    for (int i = 0; i < 3; ++i)
        position += Vector2Int::Right();

    for (int i = 0; i < 2; ++i)
        position += Vector2Int::Up();

    EXPECT_EQ(position.x, 3);
    EXPECT_EQ(position.y, 2);
}

// ========================================
// 비교 연산 테스트
// ========================================

TEST(Vector2IntTest, EqualityOperator_Equal)
{
    Vector2Int a(5, 10);
    Vector2Int b(5, 10);
    EXPECT_TRUE(a == b);
}

TEST(Vector2IntTest, EqualityOperator_NotEqual)
{
    Vector2Int a(5, 10);
    Vector2Int b(5, 11);
    EXPECT_FALSE(a == b);
}

TEST(Vector2IntTest, EqualityWithZero)
{
    Vector2Int a;
    Vector2Int b(0, 0);
    EXPECT_TRUE(a == b);
}

// ========================================
// 거리/크기 계산 테스트
// ========================================

TEST(Vector2IntTest, SqrMagnitude)
{
    Vector2Int v(3, 4);
    // 3^2 + 4^2 = 9 + 16 = 25
    EXPECT_EQ(v.sqrMagnitude(), 25);
}

TEST(Vector2IntTest, SqrMagnitude_Zero)
{
    Vector2Int v(0, 0);
    EXPECT_EQ(v.sqrMagnitude(), 0);
}

TEST(Vector2IntTest, Magnitude)
{
    Vector2Int v(3, 4);
    // sqrt(25) = 5.0
    EXPECT_FLOAT_EQ(v.magnitude(), 5.0f);
}

TEST(Vector2IntTest, Magnitude_Unit)
{
    Vector2Int v(1, 0);
    EXPECT_FLOAT_EQ(v.magnitude(), 1.0f);
}

TEST(Vector2IntTest, CellDistFromZero)
{
    Vector2Int v(3, 4);
    // |3| + |4| = 7 (맨해튼 거리)
    EXPECT_EQ(v.CellDistFromZero(), 7);
}

TEST(Vector2IntTest, CellDistFromZero_Negative)
{
    Vector2Int v(-3, -4);
    // |-3| + |-4| = 7
    EXPECT_EQ(v.CellDistFromZero(), 7);
}

TEST(Vector2IntTest, CellDistFromZero_Mixed)
{
    Vector2Int v(-5, 3);
    // |-5| + |3| = 8
    EXPECT_EQ(v.CellDistFromZero(), 8);
}

// ========================================
// 엣지 케이스 테스트
// ========================================

TEST(Vector2IntTest, LargeValues)
{
    Vector2Int v(10000, 10000);
    EXPECT_EQ(v.sqrMagnitude(), 200000000);
}

TEST(Vector2IntTest, DirectionOpposites)
{
    Vector2Int up = Vector2Int::Up();
    Vector2Int down = Vector2Int::Down();
    Vector2Int sum = up + down;

    EXPECT_EQ(sum.x, 0);
    EXPECT_EQ(sum.y, 0);
}

TEST(Vector2IntTest, DirectionLeftRight)
{
    Vector2Int left = Vector2Int::Left();
    Vector2Int right = Vector2Int::Right();
    Vector2Int sum = left + right;

    EXPECT_EQ(sum.x, 0);
    EXPECT_EQ(sum.y, 0);
}
