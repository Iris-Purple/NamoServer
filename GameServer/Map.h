#pragma once

#include <vector>
#include <queue>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <memory>

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

    // C++에서는 priority_queue가 기본적으로 max-heap이므로
    // 작은 F를 우선하려면 > 연산자를 사용
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

class Map
{
public:
    Map() = default;
    ~Map() = default;

    // Getters
    int GetMinX() const { return _minX; }
    int GetMaxX() const { return _maxX; }
    int GetMinY() const { return _minY; }
    int GetMaxY() const { return _maxY; }
    int GetSizeX() const { return _maxX - _minX + 1; }
    int GetSizeY() const { return _maxY - _minY + 1; }

    // 핵심 기능
    bool CanGo(Vector2Int cellPos, bool checkObjects = true) const;
    GameObjectRef Find(const Vector2Int& cellPos) const;
    bool ApplyLeave(GameObjectRef gameObject);
    bool ApplyMove(GameObjectRef player, Vector2Int dest);
    void LoadMap(int mapId, const std::string& pathPrefix = "../Common/MapData");

    // A* 길찾기
    std::vector<Vector2Int> FindPath(Vector2Int startCellPos, Vector2Int destCellPos,
        bool checkObject = true);

private:
    // 좌표 변환 헬퍼
    Pos Cell2Pos(Vector2Int cell) const;
    Vector2Int Pos2Cell(Pos pos) const;

    // 2D 인덱스 → 1D 인덱스 변환 (성능 최적화)
    int GetIndex(int y, int x) const { return y * GetSizeX() + x; }

    std::vector<Vector2Int> CalcCellPathFromParent(const std::vector<Pos>& parent, Pos dest);

private:
    int _minX = 0;
    int _maxX = 0;
    int _minY = 0;
    int _maxY = 0;

    // 1D 배열로 관리 (cache-friendly)
    vector<bool> _collision;
    vector<GameObjectRef> _objects; 
    

    // A* 방향 상수
    static constexpr int _deltaY[] = { 1, -1, 0, 0 };
    static constexpr int _deltaX[] = { 0, 0, -1, 1 };
    static constexpr int _cost[] = { 10, 10, 10, 10 };
};