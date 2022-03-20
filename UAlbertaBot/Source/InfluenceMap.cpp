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
void InfluenceMap::computeStartDepotInfluenceMap()
{
    PROFILE_FUNCTION();
    const BaseLocation* baseLocation = Global::Bases().getPlayerStartingBaseLocation(BWAPI::Broodwar->self());
    m_distanceMap = DistanceMap();
    m_distanceMap.computeDistanceMap(baseLocation->getDepotPosition());
    m_dist = m_distanceMap.getDistanceMap();
    m_width = BWAPI::Broodwar->mapWidth();
    m_height = BWAPI::Broodwar->mapHeight();
    m_influence = Grid<int>(m_width, m_height, 0);
    for (auto& startTilePos : BWAPI::Broodwar->getStartLocations()) // Iterates over all possible starting bases
    {
        BWAPI::Position pos = BWAPI::Position(startTilePos.x,startTilePos.y);
        if (startTilePos.x == baseLocation->getDepotPosition().x && startTilePos.y == baseLocation->getDepotPosition().y) continue; //Continues if the base is ours
        else
        {
            while (pos.x != baseLocation->getDepotPosition().x || pos.y != baseLocation->getDepotPosition().y) // While you have not reached your own base
            {
                m_influence.set(pos.x, pos.y, m_maxInfluence);

                int currentDistance = m_dist.get(pos.x, pos.y);
                int tempDistance = m_dist.get(pos.x, pos.y);

                int aa = -1;

                for (size_t a = 0; a < LegalActions; ++a) //Check all tiles surrounding current tile 
                {
                    const BWAPI::Position nextTile = BWAPI::Position(pos.x + actionX[a], pos.y + actionY[a]);
                    if (m_dist.get(nextTile.x, nextTile.y) != -1) {
                        tempDistance = (std::min(m_dist.get(nextTile.x, nextTile.y), tempDistance));
                    }

                    if (tempDistance == m_dist.get(nextTile.x, nextTile.y)) {
                        aa = a;
                    }
                }
                pos = BWAPI::Position(pos.x + actionX[aa], pos.y + actionY[aa]);

            }
        }
    }
}


/*
* 
*       if (nextTile.distance < tempDistance){
            temp = nextTile
        }
* 
* 
* 
* 
*               m_influence.set(pos.x, pos.y, m_maxInfluence);
                int curDist = m_dist.get(pos.x, pos.y);
                int minDist = curDist;
                const BWAPI::Position nextTile = BWAPI::Position(pos.x, pos.y);
                const BWAPI::Position posNext = BWAPI::Position(pos.x, pos.y); 
                for (size_t a = 0; a < LegalActions; ++a) //Check all tiles surrounding current tile 
                {
                    const BWAPI::Position nextTile = BWAPI::Position(pos.x + actionX[a], pos.y + actionY[a]);
                    if (m_dist.get(nextTile.x, nextTile.y) != -1) {
                        int nextTileDist = m_dist.get(nextTile.x, nextTile.y);
                        minDist = std::min(nextTileDist, minDist);
                    }
                    if (minDist < curDist) {
                        const BWAPI::Position posNext = BWAPI::Position(nextTile.x, nextTile.y);
                    }
                }
                if (minDist < curDist) {
                    const BWAPI::Position pos = posNext;
                }
* 
* 
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
            if (m_influence.get(x, y) == 1) {
                Global::Map().drawTile(x, y, BWAPI::Color(255, 0, 0));
            }
        }
    }
}
