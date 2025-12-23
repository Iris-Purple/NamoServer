#include "pch.h"
#include "Map.h"
#include "Player.h"
#include <filesystem>



bool Map::CanGo(Vector2Int cellPos, bool checkObjects) const
{
    if (cellPos.x < _minX || cellPos.x > _maxX)
        return false;
    if (cellPos.y < _minY || cellPos.y > _maxY)
        return false;

    int x = cellPos.x - _minX;
    int y = _maxY - cellPos.y;
    int idx = GetIndex(y, x);

    return !_collision[idx] && (!checkObjects || _objects[idx] == nullptr);
}

GameObjectRef Map::Find(const Vector2Int& cellPos) const
{
    if (cellPos.x < _minX || cellPos.x > _maxX)
        return nullptr;
    if (cellPos.y < _minY || cellPos.y > _maxY)
        return nullptr;

    int x = cellPos.x - _minX;
    int y = _maxY - cellPos.y;
    return _objects[GetIndex(y, x)];
}

bool Map::ApplyLeave(GameObjectRef gameObject)
{
    RoomRef room = gameObject->_room.load().lock();
    if (room == nullptr)
        return false;

    auto posInfo = gameObject->PosInfo();
    int posX = posInfo->posx();
    int posY = posInfo->posy();

    if (posX < _minX || posX > _maxX)
        return false;
    if (posY < _minY || posY > _maxY)
        return false;

    {
        int x = posX - _minX;
        int y = _maxY - posY;
        int idx = GetIndex(y, x);
        if (_objects[idx] == gameObject)
            _objects[idx] = nullptr;
    }

    return true;
}

bool Map::ApplyMove(GameObjectRef gameObject, Vector2Int dest)
{
    if (ApplyLeave(gameObject) == false)
        return false;

    auto posInfo = gameObject->PosInfo();
    int posX = posInfo->posx();
    int posY = posInfo->posy();

    if (CanGo(dest, true) == false)
        return false;

  
    // 새 위치에 등록
    {
        int x = dest.x - _minX;
        int y = _maxY - dest.y;
        _objects[GetIndex(y, x)] = gameObject;
    }

    // 실제 좌표 이동
    posInfo->set_posx(dest.x);
    posInfo->set_posy(dest.y);
    return true;
}

void Map::LoadMap(int mapId, const std::string& pathPrefix)
{
    // 맵 이름 생성 (Map_001, Map_002, ...)
    char mapName[32];
    snprintf(mapName, sizeof(mapName), "Map_%03d", mapId);

    std::string filePath = pathPrefix + "/" + mapName + ".txt";
    std::ifstream file(filePath);

    if (!file.is_open())
    {
        // 에러 처리 - 로깅 또는 예외
        cout << "not found file" << endl;
        return;
    }
    cout << "Map::LoadMap file: " << filePath << endl;

    file >> _minX >> _maxX >> _minY >> _maxY;
    file.ignore();  // 개행 문자 무시

    int xCount = _maxX - _minX + 1;
    int yCount = _maxY - _minY + 1;

    _collision.resize((size_t)yCount * xCount, false);
    _objects.resize((size_t)yCount * xCount, nullptr);

    std::string line;
    for (int y = 0; y < yCount; y++)
    {
        std::getline(file, line);
        for (int x = 0; x < xCount && x < static_cast<int>(line.size()); x++)
        {
            _collision[GetIndex(y, x)] = (line[x] == '1');
        }
    }
}

Pos Map::Cell2Pos(Vector2Int cell) const
{
    return Pos(_maxY - cell.y, cell.x - _minX);
}

Vector2Int Map::Pos2Cell(Pos pos) const
{
    return Vector2Int(pos.X + _minX, _maxY - pos.Y);
}

std::vector<Vector2Int> Map::FindPath(Vector2Int startCellPos, Vector2Int destCellPos,
    bool checkObject)
{
    int sizeX = GetSizeX();
    int sizeY = GetSizeY();
    int totalSize = sizeX * sizeY;

    // closed 리스트 (방문 여부)
    std::vector<bool> closed(totalSize, false);

    // open 리스트 (발견된 최소 비용)
    std::vector<int> open(totalSize, INT_MAX);

    // 부모 노드 추적
    std::vector<Pos> parent(totalSize);

    // min-heap (작은 F 우선)
    std::priority_queue<PQNode, std::vector<PQNode>, std::greater<PQNode>> pq;

    Pos pos = Cell2Pos(startCellPos);
    Pos dest = Cell2Pos(destCellPos);

    // 시작점 초기화
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

        // 이미 방문했으면 스킵
        if (closed[nodeIdx])
            continue;

        closed[nodeIdx] = true;

        // 목적지 도착
        if (node.Y == dest.Y && node.X == dest.X)
            break;

        // 4방향 탐색
        for (int i = 0; i < 4; i++)
        {
            Pos next(node.Y + _deltaY[i], node.X + _deltaX[i]);

            // 범위 체크
            if (next.Y < 0 || next.Y >= sizeY || next.X < 0 || next.X >= sizeX)
                continue;

            // 이동 가능 체크
            if (next.Y != dest.Y || next.X != dest.X)
            {
                if (CanGo(Pos2Cell(next), checkObject) == false)
                    continue;
            }

            int nextIdx = GetIndex(next.Y, next.X);

            if (closed[nextIdx])
                continue;

            // 비용 계산 (원본 코드의 g=0 유지)
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

std::vector<Vector2Int> Map::CalcCellPathFromParent(const std::vector<Pos>& parent, Pos dest)
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
