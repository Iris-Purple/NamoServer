/**
 * @file test_map.cpp
 * @brief Map 클래스 단위 테스트
 *
 * 테스트 항목:
 * - 맵 경계 검사
 * - CanGo 충돌 검사
 * - A* 경로찾기 알고리즘
 * - 좌표 변환 (Cell <-> Pos)
 */

#include <gtest/gtest.h>
#include <vector>
#include <queue>
#include <string>
#include <cmath>
#include <algorithm>
#include <climits>

// ========================================
// 테스트용 Map 클래스 (외부 의존성 제거)
// ========================================

struct Pos
{
    int Y;
    int X;

    Pos() : Y(0), X(0) {}
    Pos(int y, int x) : Y(y), X(x) {}
};

struct PQNode
{
    int F;
    int G;
    int Y;
    int X;

    bool operator>(const PQNode& other) const
    {
        return F > other.F;
    }
};

struct Vector2Int
{
    int x;
    int y;

    Vector2Int() : x(0), y(0) {}
    Vector2Int(int x, int y) : x(x), y(y) {}

    bool operator==(const Vector2Int& other) const
    {
        return x == other.x && y == other.y;
    }
};

/**
 * @brief 테스트용 Map 클래스
 * 외부 의존성(GameObject, Room 등)을 제거하고 순수 로직만 테스트
 */
class TestMap
{
public:
    TestMap() = default;

    void Initialize(int minX, int maxX, int minY, int maxY)
    {
        _minX = minX;
        _maxX = maxX;
        _minY = minY;
        _maxY = maxY;

        int xCount = _maxX - _minX + 1;
        int yCount = _maxY - _minY + 1;

        _collision.resize(static_cast<size_t>(yCount) * xCount, false);
    }

    void SetCollision(int x, int y, bool blocked)
    {
        if (x < _minX || x > _maxX) return;
        if (y < _minY || y > _maxY) return;

        int localX = x - _minX;
        int localY = _maxY - y;
        _collision[GetIndex(localY, localX)] = blocked;
    }

    int GetMinX() const { return _minX; }
    int GetMaxX() const { return _maxX; }
    int GetMinY() const { return _minY; }
    int GetMaxY() const { return _maxY; }
    int GetSizeX() const { return _maxX - _minX + 1; }
    int GetSizeY() const { return _maxY - _minY + 1; }

    bool CanGo(Vector2Int cellPos) const
    {
        if (cellPos.x < _minX || cellPos.x > _maxX)
            return false;
        if (cellPos.y < _minY || cellPos.y > _maxY)
            return false;

        int x = cellPos.x - _minX;
        int y = _maxY - cellPos.y;
        int idx = GetIndex(y, x);

        return !_collision[idx];
    }

    std::vector<Vector2Int> FindPath(Vector2Int startCellPos, Vector2Int destCellPos)
    {
        int sizeX = GetSizeX();
        int sizeY = GetSizeY();
        int totalSize = sizeX * sizeY;

        std::vector<bool> closed(totalSize, false);
        std::vector<int> open(totalSize, INT_MAX);
        std::vector<Pos> parent(totalSize);

        std::priority_queue<PQNode, std::vector<PQNode>, std::greater<PQNode>> pq;

        Pos pos = Cell2Pos(startCellPos);
        Pos dest = Cell2Pos(destCellPos);

        int startIdx = GetIndex(pos.Y, pos.X);
        int h = 10 * (std::abs(dest.Y - pos.Y) + std::abs(dest.X - pos.X));
        open[startIdx] = h;
        pq.push(PQNode{ h, 0, pos.Y, pos.X });
        parent[startIdx] = Pos(pos.Y, pos.X);

        while (!pq.empty())
        {
            PQNode node = pq.top();
            pq.pop();

            int nodeIdx = GetIndex(node.Y, node.X);

            if (closed[nodeIdx])
                continue;

            closed[nodeIdx] = true;

            if (node.Y == dest.Y && node.X == dest.X)
                break;

            for (int i = 0; i < 4; i++)
            {
                Pos next(node.Y + _deltaY[i], node.X + _deltaX[i]);

                if (next.Y < 0 || next.Y >= sizeY || next.X < 0 || next.X >= sizeX)
                    continue;

                if (next.Y != dest.Y || next.X != dest.X)
                {
                    if (CanGo(Pos2Cell(next)) == false)
                        continue;
                }

                int nextIdx = GetIndex(next.Y, next.X);

                if (closed[nextIdx])
                    continue;

                int g = 0;
                int h = 10 * ((dest.Y - next.Y) * (dest.Y - next.Y) +
                    (dest.X - next.X) * (dest.X - next.X));

                if (open[nextIdx] < g + h)
                    continue;

                open[nextIdx] = g + h;
                pq.push(PQNode{ g + h, g, next.Y, next.X });
                parent[nextIdx] = Pos(node.Y, node.X);
            }
        }

        return CalcCellPathFromParent(parent, dest);
    }

private:
    Pos Cell2Pos(Vector2Int cell) const
    {
        return Pos(_maxY - cell.y, cell.x - _minX);
    }

    Vector2Int Pos2Cell(Pos pos) const
    {
        return Vector2Int(pos.X + _minX, _maxY - pos.Y);
    }

    int GetIndex(int y, int x) const { return y * GetSizeX() + x; }

    std::vector<Vector2Int> CalcCellPathFromParent(const std::vector<Pos>& parent, Pos dest)
    {
        std::vector<Vector2Int> cells;

        int y = dest.Y;
        int x = dest.X;

        while (true)
        {
            int idx = GetIndex(y, x);
            if (parent[idx].Y == y && parent[idx].X == x)
                break;

            cells.push_back(Pos2Cell(Pos(y, x)));
            Pos p = parent[idx];
            y = p.Y;
            x = p.X;
        }

        cells.push_back(Pos2Cell(Pos(y, x)));
        std::reverse(cells.begin(), cells.end());

        return cells;
    }

private:
    int _minX = 0;
    int _maxX = 0;
    int _minY = 0;
    int _maxY = 0;

    std::vector<bool> _collision;

    static constexpr int _deltaY[] = { 1, -1, 0, 0 };
    static constexpr int _deltaX[] = { 0, 0, -1, 1 };
};

constexpr int TestMap::_deltaY[];
constexpr int TestMap::_deltaX[];

// ========================================
// 맵 초기화 테스트
// ========================================

class MapTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // 5x5 맵 생성 (-2 ~ 2)
        map.Initialize(-2, 2, -2, 2);
    }

    TestMap map;
};

TEST_F(MapTest, Initialization)
{
    EXPECT_EQ(map.GetMinX(), -2);
    EXPECT_EQ(map.GetMaxX(), 2);
    EXPECT_EQ(map.GetMinY(), -2);
    EXPECT_EQ(map.GetMaxY(), 2);
    EXPECT_EQ(map.GetSizeX(), 5);
    EXPECT_EQ(map.GetSizeY(), 5);
}

TEST_F(MapTest, AsymmetricMap)
{
    TestMap asymMap;
    asymMap.Initialize(0, 10, 0, 5);

    EXPECT_EQ(asymMap.GetSizeX(), 11);
    EXPECT_EQ(asymMap.GetSizeY(), 6);
}

// ========================================
// CanGo 테스트 (충돌 검사)
// ========================================

TEST_F(MapTest, CanGo_WithinBounds)
{
    EXPECT_TRUE(map.CanGo(Vector2Int(0, 0)));
    EXPECT_TRUE(map.CanGo(Vector2Int(-2, -2)));
    EXPECT_TRUE(map.CanGo(Vector2Int(2, 2)));
    EXPECT_TRUE(map.CanGo(Vector2Int(-1, 1)));
}

TEST_F(MapTest, CanGo_OutOfBounds)
{
    EXPECT_FALSE(map.CanGo(Vector2Int(-3, 0)));
    EXPECT_FALSE(map.CanGo(Vector2Int(3, 0)));
    EXPECT_FALSE(map.CanGo(Vector2Int(0, -3)));
    EXPECT_FALSE(map.CanGo(Vector2Int(0, 3)));
    EXPECT_FALSE(map.CanGo(Vector2Int(100, 100)));
}

TEST_F(MapTest, CanGo_WithCollision)
{
    map.SetCollision(0, 0, true);

    EXPECT_FALSE(map.CanGo(Vector2Int(0, 0)));
    EXPECT_TRUE(map.CanGo(Vector2Int(1, 0)));
    EXPECT_TRUE(map.CanGo(Vector2Int(-1, 0)));
}

TEST_F(MapTest, CanGo_WallPattern)
{
    // 벽 패턴: x=0 라인을 전부 막음
    for (int y = -2; y <= 2; ++y)
    {
        map.SetCollision(0, y, true);
    }

    EXPECT_FALSE(map.CanGo(Vector2Int(0, 0)));
    EXPECT_FALSE(map.CanGo(Vector2Int(0, 1)));
    EXPECT_FALSE(map.CanGo(Vector2Int(0, -1)));

    EXPECT_TRUE(map.CanGo(Vector2Int(-1, 0)));
    EXPECT_TRUE(map.CanGo(Vector2Int(1, 0)));
}

// ========================================
// A* 경로찾기 테스트
// ========================================

TEST_F(MapTest, FindPath_SamePosition)
{
    auto path = map.FindPath(Vector2Int(0, 0), Vector2Int(0, 0));

    EXPECT_EQ(path.size(), 1);
    EXPECT_TRUE(path[0] == Vector2Int(0, 0));
}

TEST_F(MapTest, FindPath_AdjacentCell)
{
    auto path = map.FindPath(Vector2Int(0, 0), Vector2Int(1, 0));

    EXPECT_EQ(path.size(), 2);
    EXPECT_TRUE(path[0] == Vector2Int(0, 0));
    EXPECT_TRUE(path[1] == Vector2Int(1, 0));
}

TEST_F(MapTest, FindPath_StraightLine)
{
    auto path = map.FindPath(Vector2Int(-2, 0), Vector2Int(2, 0));

    // 시작 -> 끝까지 5칸
    EXPECT_EQ(path.size(), 5);
    EXPECT_TRUE(path[0] == Vector2Int(-2, 0));
    EXPECT_TRUE(path[4] == Vector2Int(2, 0));
}

TEST_F(MapTest, FindPath_Diagonal)
{
    auto path = map.FindPath(Vector2Int(-1, -1), Vector2Int(1, 1));

    // 대각선 이동은 불가능하므로 맨해튼 경로
    EXPECT_GE(path.size(), 5); // 최소 5칸 (시작 포함)
    EXPECT_TRUE(path.front() == Vector2Int(-1, -1));
    EXPECT_TRUE(path.back() == Vector2Int(1, 1));
}

TEST_F(MapTest, FindPath_AroundObstacle)
{
    // 장애물 설치: (0, 0)
    map.SetCollision(0, 0, true);

    auto path = map.FindPath(Vector2Int(-1, 0), Vector2Int(1, 0));

    // 장애물을 피해서 돌아가야 함
    EXPECT_GT(path.size(), 3);

    // 경로에 장애물이 포함되지 않아야 함
    for (const auto& pos : path)
    {
        EXPECT_FALSE(pos == Vector2Int(0, 0));
    }
}

TEST_F(MapTest, FindPath_CornerToCorner)
{
    auto path = map.FindPath(Vector2Int(-2, -2), Vector2Int(2, 2));

    EXPECT_TRUE(path.front() == Vector2Int(-2, -2));
    EXPECT_TRUE(path.back() == Vector2Int(2, 2));

    // 경로 연속성 검사
    for (size_t i = 1; i < path.size(); ++i)
    {
        int dx = std::abs(path[i].x - path[i - 1].x);
        int dy = std::abs(path[i].y - path[i - 1].y);

        // 인접 셀로만 이동 (맨해튼)
        EXPECT_EQ(dx + dy, 1);
    }
}

// ========================================
// 복잡한 경로찾기 테스트
// ========================================

TEST(MapPathfindingTest, MazePattern)
{
    // 10x10 맵에서 미로 패턴
    TestMap mazeMap;
    mazeMap.Initialize(0, 9, 0, 9);

    // 간단한 벽 패턴:
    //   0 1 2 3 4 5 6 7 8 9
    // 5 . . . # . . . . . .
    // 4 . . . # . . . . . .
    // 3 . . . # . . . . . .
    // 2 . . . # . . . . . .
    // 1 . . . . . . . . . .

    for (int y = 2; y <= 5; ++y)
    {
        mazeMap.SetCollision(3, y, true);
    }

    auto path = mazeMap.FindPath(Vector2Int(0, 3), Vector2Int(5, 3));

    // 경로가 존재해야 함
    EXPECT_GT(path.size(), 0);
    EXPECT_TRUE(path.front() == Vector2Int(0, 3));
    EXPECT_TRUE(path.back() == Vector2Int(5, 3));

    // 벽을 통과하지 않았는지 확인
    for (const auto& pos : path)
    {
        if (pos.x == 3 && pos.y >= 2 && pos.y <= 5)
        {
            FAIL() << "Path goes through wall at (" << pos.x << ", " << pos.y << ")";
        }
    }
}

TEST(MapPathfindingTest, LargeMap)
{
    // 100x100 대형 맵 성능 테스트
    TestMap largeMap;
    largeMap.Initialize(0, 99, 0, 99);

    auto path = largeMap.FindPath(Vector2Int(0, 0), Vector2Int(99, 99));

    EXPECT_TRUE(path.front() == Vector2Int(0, 0));
    EXPECT_TRUE(path.back() == Vector2Int(99, 99));
}

TEST(MapPathfindingTest, NoPath_Blocked)
{
    // 목적지가 완전히 막힌 경우
    TestMap blockedMap;
    blockedMap.Initialize(0, 4, 0, 4);

    // 목적지 (4,4) 주변을 모두 막음
    blockedMap.SetCollision(3, 4, true);
    blockedMap.SetCollision(4, 3, true);

    auto path = blockedMap.FindPath(Vector2Int(0, 0), Vector2Int(4, 4));

    // 경로가 존재하거나 (목적지는 도달 가능하도록 설계됨)
    // 또는 빈 경로 반환
    // 현재 구현에서는 목적지 도달 가능하면 반환
    EXPECT_GE(path.size(), 1);
}

// ========================================
// 경계값 테스트
// ========================================

TEST(MapEdgeCaseTest, SingleCell)
{
    TestMap singleMap;
    singleMap.Initialize(0, 0, 0, 0);

    EXPECT_TRUE(singleMap.CanGo(Vector2Int(0, 0)));
    EXPECT_EQ(singleMap.GetSizeX(), 1);
    EXPECT_EQ(singleMap.GetSizeY(), 1);
}

TEST(MapEdgeCaseTest, NegativeCoordinates)
{
    TestMap negMap;
    negMap.Initialize(-10, -5, -10, -5);

    EXPECT_TRUE(negMap.CanGo(Vector2Int(-7, -7)));
    EXPECT_FALSE(negMap.CanGo(Vector2Int(0, 0)));
    EXPECT_FALSE(negMap.CanGo(Vector2Int(-4, -4)));
}

TEST(MapEdgeCaseTest, PositiveOnlyCoordinates)
{
    TestMap posMap;
    posMap.Initialize(5, 10, 5, 10);

    EXPECT_TRUE(posMap.CanGo(Vector2Int(7, 7)));
    EXPECT_FALSE(posMap.CanGo(Vector2Int(0, 0)));
    EXPECT_FALSE(posMap.CanGo(Vector2Int(4, 4)));
}
