#include "InfluenceMap.h"
#include "MapTools.h"
#include "Global.h"
#include "BaseLocation.h"
#include "BaseLocationManager.h"
#include "DistanceMap.h"
#include "InformationManager.h"


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

float UAlbertaBot::InfluenceMap::getVisionInfluence(int tileX, int tileY)
{
    return m_visionMap.get(tileX, tileY);
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
    const int minXi = std::max(x - m_viewDistance, 0);
    const int maxXi = std::min(x + m_viewDistance, m_width - 1);
    const int minYi = std::max(y - m_viewDistance, 0);
    const int maxYi = std::min(y + m_viewDistance, m_height - 1);
        
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
    m_airDamageMap = Grid<float>(m_width, m_height, 0);
    m_groundDamageMap = Grid<float>(m_width, m_height, 0);
    for (auto& startTilePos : BWAPI::Broodwar->getStartLocations()) // Iterates over all possible starting bases
    {
        BWAPI::Position pos = BWAPI::Position(startTilePos.x,startTilePos.y);
        if (startTilePos.x == baseLocation->getDepotPosition().x && startTilePos.y == baseLocation->getDepotPosition().y) continue; //Continues if the base is ours
        else
        {
            while (pos.x != baseLocation->getDepotPosition().x || pos.y != baseLocation->getDepotPosition().y) // While you have not reached your own base
            {
                m_influence.set(pos.x, pos.y, m_maxInfluence);

                const int currentDistance = m_dist.get(pos.x, pos.y);
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
            const float influence = calcInfluence(x, y);
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

float InfluenceMap::variableRangeInfluence(float distance, int sightRange, float power) {
    return (m_maxInfluence - std::pow((m_maxInfluence * (distance / sightRange)), power));
}

void InfluenceMap::computeVisionMap() {
    BWAPI::Player enemy = BWAPI::Broodwar->enemy();
    m_visionMap = Grid<float>(m_width, m_height, 0);
    const UIMap enemyUnits = Global::Info().getUnitInfo(enemy);
    if (enemyUnits.size() == 0) {
        return;
    }
    int x = -1;
    int y = -1;
    int dist = 10000;
    for (const auto& enemyUnit : enemyUnits) {
        const int sightRange = (enemy->sightRange(enemyUnit.second.type)/32) +1;
        x = enemyUnit.second.lastPosition.x/32;
        y = enemyUnit.second.lastPosition.y/32;
        m_visionMap.set(x, y, m_maxInfluence);

        const int minXi = std::max(x - sightRange, 0);
        const int maxXi = std::min(x + sightRange, m_width - 1);
        const int minYi = std::max(y - sightRange, 0);
        const int maxYi = std::min(y + sightRange, m_height - 1);

        for (int xi = minXi; xi < maxXi; xi++) {
            for (int yi = minYi; yi < maxYi; yi++) {
                dist = distance(x, xi, y, yi);
                if (dist <= sightRange) {
                    m_visionMap.set(xi, yi, std::max(variableRangeInfluence(dist,sightRange, 0.9), m_visionMap.get(xi, yi)));
                }
            }
        }
    }
}
void InfluenceMap::computeAirDamageMap() {
    BWAPI::Player enemy = BWAPI::Broodwar->enemy(); //TODO
    m_airDamageMap = Grid<float>(m_width, m_height, 0);
    const UIMap enemyUnits = Global::Info().getUnitInfo(enemy);
    if (enemyUnits.size() == 0) {
        return;
    }
    int dist = 10000;
    for (const auto& enemyUnit : enemyUnits) {
        
        BWAPI::WeaponType unitGroundWeapon = enemyUnit.second.type.groundWeapon();
        BWAPI::WeaponType unitAirWeapon = enemyUnit.second.type.airWeapon();

        const bool canAttack = enemyUnit.second.type.canAttack();
        const bool canAttackAir = (unitGroundWeapon.targetsAir() || unitGroundWeapon.targetsAir());
        if (!canAttack || !canAttackAir) continue;

        int maxGroundRange = 0;
        int maxAirRange = 0;
        int maxRange = 0;
        float dps = 0.0;
        float damageInfluence = 0.0;


        if (unitGroundWeapon.targetsAir()) {
            maxGroundRange = unitGroundWeapon.maxRange();
            dps = unitGroundWeapon.damageFactor() * unitGroundWeapon.damageAmount() * enemyUnit.first->getType().maxAirHits() * (15.0 / unitGroundWeapon.damageCooldown()); // Damage output

        }

        if (unitAirWeapon.targetsAir()) {
            maxAirRange = enemyUnit.second.type.airWeapon().maxRange();
            dps = unitAirWeapon.damageFactor() * unitAirWeapon.damageAmount() * enemyUnit.first->getType().maxAirHits() * (15.0 / unitAirWeapon.damageCooldown());  // Damage output
        }

        maxRange = (std::max(std::max(maxGroundRange, maxAirRange)/32, 1))+1;


        const int x = enemyUnit.second.lastPosition.x / 32;
        const int y = enemyUnit.second.lastPosition.y / 32;

        damageInfluence = dps / 15.0;

        m_airDamageMap.set(x, y, damageInfluence);


        const int minXi = std::max(x - maxRange, 0);
        const int maxXi = std::min(x + maxRange, m_width - 1);
        const int minYi = std::max(y - maxRange, 0);
        const int maxYi = std::min(y + maxRange, m_height - 1);

        for (int xi = minXi; xi < maxXi; xi++) {
            for (int yi = minYi; yi < maxYi; yi++) {
                dist = distance(x, xi, y, yi);
                if (dist <= maxRange) {
                    m_airDamageMap.set(xi, yi, variableRangeInfluence(dist, maxRange, 0.9) + m_airDamageMap.get(xi, yi));
                }
            }
        }
    }
}
void InfluenceMap::computeGroundDamageMap() {
    BWAPI::Player enemy = BWAPI::Broodwar->self(); //TODO
    m_groundDamageMap = Grid<float>(m_width, m_height, 0);
    const UIMap enemyUnits = Global::Info().getUnitInfo(enemy);
    if (enemyUnits.size() == 0) {
        return;
    }
    int dist = 10000;
    for (const auto& enemyUnit : enemyUnits) {

        BWAPI::WeaponType const unitGroundWeapon = enemyUnit.second.type.groundWeapon();
        BWAPI::WeaponType const unitAirWeapon = enemyUnit.second.type.airWeapon();

        const bool canAttack = enemyUnit.second.type.canAttack();
        const bool canAttackGround = (unitGroundWeapon.targetsGround() || unitAirWeapon.targetsGround());
        if (!canAttack || !canAttackGround) continue;

        int maxGroundRange = 0;
        int maxAirRange = 0;
        int maxRange = 0;
        float dps = 0.0;
        float damageInfluence = 0.0;


        if (unitGroundWeapon.targetsGround()) {
            maxGroundRange = unitGroundWeapon.maxRange();
            dps = unitGroundWeapon.damageFactor() * unitGroundWeapon.damageAmount() * enemyUnit.first->getType().maxGroundHits() * (15.0 / unitGroundWeapon.damageCooldown()); // Damage output

        }

        if (unitAirWeapon.targetsGround()) {
            maxAirRange = unitAirWeapon.maxRange();
            dps = unitAirWeapon.damageFactor() * unitAirWeapon.damageAmount() * enemyUnit.first->getType().maxGroundHits() * (15.0 / unitAirWeapon.damageCooldown());  // Damage output
        }

        maxRange = (std::max(std::max(maxGroundRange, maxAirRange) / 32, 1)) + 1;


        const int x = enemyUnit.second.lastPosition.x / 32;
        const int y = enemyUnit.second.lastPosition.y / 32;

        damageInfluence = dps / 15.0;

        m_groundDamageMap.set(x, y, damageInfluence);


        const int minXi = std::max(x - maxRange, 0);
        const int maxXi = std::min(x + maxRange, m_width - 1);
        const int minYi = std::max(y - maxRange, 0);
        const int maxYi = std::min(y + maxRange, m_height - 1);

        for (int xi = minXi; xi < maxXi; xi++) {
            for (int yi = minYi; yi < maxYi; yi++) {
                dist = distance(x, xi, y, yi);
                if (dist <= maxRange) {
                    m_groundDamageMap.set(xi, yi, variableRangeInfluence(dist, maxRange, 0.9) + m_groundDamageMap.get(xi, yi));

                }
            }
        }
    }
}
void InfluenceMap::draw() const
{
    for (int x = 0; x < m_width; x++) {
        for (int y = 0; y < m_height; y++) {
            if (m_influence.get(x, y) > 2) { // TODO: Set back to 0
                Global::Map().drawTile(x, y, BWAPI::Color(
                    255, 
                    255 - (255 * (m_influence.get(x, y) / m_maxInfluence)), 
                    255 - (255 * (m_influence.get(x, y) / m_maxInfluence))));
            }
            if (m_visionMap.get(x, y) > 0) { // TODO: Set back to 0
                /*Global::Map().drawTile(x, y, BWAPI::Color(
                    255 - (255 * (m_visionMap.get(x, y) / m_maxInfluence)),
                    255 - (255 * (m_visionMap.get(x, y) / m_maxInfluence)),
                    255));
                    */

                    
                Global::Map().drawTile(x, y, BWAPI::Color(
                    255,
                    0,
                    0));
                    
                
            }
            if (m_groundDamageMap.get(x, y) > 3.0) { // TODO: Set back to 0
                Global::Map().drawTile(x, y, BWAPI::Color(
                    0,
                    255,
                    0));
            }
            if (m_airDamageMap.get(x, y) > 3.0) { // TODO: Set back to 0
                Global::Map().drawTile(x, y, BWAPI::Color(
                    0,
                    0,
                    255));
            }
        }
    }
}
