#include "InfluenceMap.h"
#include "MapTools.h"
#include "Global.h"
#include "BaseLocation.h"
#include "BaseLocationManager.h"
#include "DistanceMap.h"


using namespace UAlbertaBot;

const size_t LegalActions = 8;
const int actionX[LegalActions] = { 1, -1, 0, 0, 1 ,-1, -1, 1 };
const int actionY[LegalActions] = { 0, 0, 1, -1, -1 , 1 , -1, 1 };

InfluenceMap::InfluenceMap()
{

}

int InfluenceMap::getInfluence(int tileX, int tileY) const
{
    UAB_ASSERT(tileX < m_width&& tileY < m_height, "Index out of range: X = %d, Y = %d", tileX, tileY);

    return m_influence.get(tileX, tileY);
}

int InfluenceMap::getInfluence(const BWAPI::TilePosition& pos) const
{
    return getInfluence(pos.x, pos.y);
}

int InfluenceMap::getInfluence(const BWAPI::Position& pos) const
{
    return getInfluence(BWAPI::TilePosition(pos));
}

void InfluenceMap::computeStartDepotInfluenceMap(DistanceMap distanceMap)
{
    auto& startLocation = BWAPI::Broodwar->self()->getStartLocation();
    m_distanceMap = distanceMap;
    m_width = BWAPI::Broodwar->mapWidth();
    m_height = BWAPI::Broodwar->mapHeight();
    m_influence = Grid<int>(m_width, m_height, 0);
    for (auto& startTilePos : BWAPI::Broodwar->getStartLocations()) // Iterates over all possible starting bases
    {
        BWAPI::TilePosition pos = startTilePos;
        if (startTilePos.x == startLocation.x && startTilePos.y == startLocation.y) continue; //Continues if the base is ours
        else
        {
            while (pos.x != startLocation.x || pos.y != startLocation.y) // While you have not reached your own base
            {
                m_influence.set(pos.x, pos.y, m_maxInfluence);
                int curDist = m_distanceMap.getDistance(pos);
                int nextDist = curDist;
                for (size_t a = 0; a < LegalActions; ++a) //Check all tiles surrounding current tile 
                {
                    BWAPI::TilePosition nextTile(pos.x + actionX[a], pos.y + actionY[a]);
                    if (BWAPI::Broodwar->isWalkable(nextTile.x, nextTile.y)) {
                        nextDist = std::min(m_distanceMap.getDistance(nextTile), nextDist);
                        if (nextDist < curDist) pos = nextTile;
                    }
                }
            }
        }
    }
}

/*
// Computes m_dist[x][y] = ground distance from (startX, startY) to (x,y)
// Uses BFS, since the map is quite large and DFS may cause a stack overflow
void InfluenceMap::computeDistanceMap(const BWAPI::TilePosition& startTile)
{
    PROFILE_FUNCTION();

    m_startTile = startTile;
    m_width = BWAPI::Broodwar->mapWidth();
    m_height = BWAPI::Broodwar->mapHeight();
    m_dist = Grid<int>(m_width, m_height, -1);
    m_sortedTiles.reserve(m_width * m_height);

    // the fringe for the BFS we will perform to calculate distances
    std::vector<BWAPI::TilePosition> fringe;
    fringe.reserve(m_width * m_height);
    fringe.push_back(startTile);
    m_sortedTiles.push_back(startTile);

    m_dist.set(startTile.x, startTile.y, 0);

    for (size_t fringeIndex = 0; fringeIndex < fringe.size(); ++fringeIndex)
    {
        const auto& tile = fringe[fringeIndex];

        // check every possible child of this tile
        for (size_t a = 0; a < LegalActions; ++a)
        {
            const BWAPI::TilePosition nextTile(tile.x + actionX[a], tile.y + actionY[a]);

            // if the new tile is inside the map bounds, is walkable, and has not been visited yet, set the distance of its parent + 1
            if (Global::Map().isWalkable(nextTile) && getInfluence(nextTile) == -1)
            {
                m_dist.set(nextTile.x, nextTile.y, m_dist.get(tile.x, tile.y) + 1);
                fringe.push_back(nextTile);
                m_sortedTiles.push_back(nextTile);
            }
        }
    }
}
*/
void InfluenceMap::draw() const
{
    const int tilesToDraw = 200;
    for (int x = 0; x < m_width; x++) {
        for (int y = 0; y < m_height; y++) {
            Global::Map().drawTile(x, y, BWAPI::Color(255, 0, 0));
        }
    }
}
