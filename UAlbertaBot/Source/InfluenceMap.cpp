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


float InfluenceMap::distance(int x1, int x2, int y1, int y2) {
    return std::sqrt(std::pow((x2 - x1), 2) + std::pow((y2 - y1) * 1.0, 2));
}

float InfluenceMap::influence(float distance, float power) {
    return (m_maxInfluence - std::pow((m_maxInfluence * (distance / m_viewDistance)), power));
}                       

float InfluenceMap::calcInfluence(int x, int y) {
    if (m_influence.get(x, y) == m_maxInfluence) return 0.0;
    int minXi = std::max(x - m_viewDistance, 0);
    int maxXi = std::min(x + m_viewDistance, m_width - 1);
    int minYi = std::max(y - m_viewDistance, 0);
    int maxYi = std::min(y + m_viewDistance, m_height - 1);
        
    float lowestDist = m_viewDistance + 1.0;
    for (int xi = minXi; xi < maxXi; xi++) {
        for (int yi = minYi; yi < maxYi; yi++) {
            if (x == xi && y == yi) continue;
            else if (m_influence.get(xi, yi) == m_maxInfluence) {
                const float dist = distance(x, xi, y, yi);
                if (dist <= m_viewDistance) {
                    lowestDist = std::min(dist, lowestDist);
                }
            }
        }
    }
    if (lowestDist <= 8.0) {
        return (m_maxInfluence - std::pow((m_maxInfluence * (lowestDist / m_viewDistance)), .9));
        //return influence(maxInfluence, maxDistance, lowestDist, 2.0);
    }
    else return 0.0;
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
    m_influence = Grid<float>(m_width, m_height, 0);
    m_influenced = Grid<float>(m_width, m_height, 0);
    m_visionMap = Grid<float>(m_width, m_height, 0);
    m_visionSourceMap = Grid<float>(m_width, m_height, 0);
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
                        tempDistance = (std::min(m_dist.get(nextTile.x, nextTile.y), tempDistance));  //Store lowest distance 
                    }

                    if (tempDistance == m_dist.get(nextTile.x, nextTile.y)) {
                        aa = a;   //If this tile is the best, remember its modifyers
                    }
                }
                pos = BWAPI::Position(pos.x + actionX[aa], pos.y + actionY[aa]); //Ready next iteration by moving to next tile

            }
        }
    }
    for (int x = 0; x < m_width; x++) {
        for (int y = 0; y < m_height; y++) {
            float influence = calcInfluence(x, y);
            if (influence > 0.0) { 
                m_influenced.set(x, y, influence); 
            }
        }
    }
    for (int x = 0; x < m_width; x++) {
        for (int y = 0; y < m_height; y++) {
            m_influence.set(x, y,std::max(m_influence.get(x,y),m_influenced.get(x,y)));
        }
    }
} 

void InfluenceMap::calculateVisionMap() {
    BWAPI::Player Enemy = BWAPI::Broodwar->enemy();
    BWAPI::Unitset enemyUnits = Enemy->getUnits();
    std::vector<BWAPI::Position> positions(enemyUnits.size());
    for (BWAPI::Unit unit : enemyUnits){
        BWAPI::Position unitPos = unit->getPosition();
        m_visionSourceMap.set(unitPos.x,unitPos.y, m_maxInfluence);
    }
    
}
void InfluenceMap::draw() const
{
    const int tilesToDraw = 200;
    for (int x = 0; x < m_width; x++) {
        for (int y = 0; y < m_height; y++) {
            if (m_influence.get(x, y) > 0) {
                Global::Map().drawTile(x, y, BWAPI::Color(255, 
                                                          255 - (255 * (m_influence.get(x, y) / m_maxInfluence)), 
                                                          255 - (255 * (m_influence.get(x, y) / m_maxInfluence))));
            }
        }
    }
}
