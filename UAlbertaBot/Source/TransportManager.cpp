#include "TransportManager.h"
#include "BaseLocationManager.h"
#include "Global.h"
#include "Micro.h"
#include "MapTools.h"
#include "InformationManager.h"

using namespace UAlbertaBot;

TransportManager::TransportManager()
{
    sneakInConfig = Config::Strategy::StrategyName == "Protoss_Drop";
}

void TransportManager::executeMicro(const BWAPI::Unitset & targets)
{
    const BWAPI::Unitset & transportUnits = getUnits();

    if (transportUnits.empty())
    {
        return;
    }
}

void TransportManager::calculateMapEdgeVertices()
{
    auto enemyBaseLocation = Global::Bases().getPlayerStartingBaseLocation(BWAPI::Broodwar->enemy());

    if (!enemyBaseLocation)
    {
        return;
    }

    const BWAPI::Position basePosition = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
    const std::vector<BWAPI::TilePosition> & closestTobase = Global::Map().getClosestTilesTo(basePosition);

    std::set<BWAPI::Position> unsortedVertices;

    int minX = std::numeric_limits<int>::max(); int minY = minX;
    int maxX = std::numeric_limits<int>::min(); int maxY = maxX;

    //compute mins and maxs
    for (auto &tile : closestTobase)
    {
        if (tile.x > maxX) maxX = tile.x;
        else if (tile.x < minX) minX = tile.x;

        if (tile.y > maxY) maxY = tile.y;
        else if (tile.y < minY) minY = tile.y;
    }

    m_minCorner = BWAPI::Position(minX, minY) * 32 + BWAPI::Position(16, 16);
    m_maxCorner = BWAPI::Position(maxX, maxY) * 32 + BWAPI::Position(16, 16);

    //add all(some) edge tiles! 
    for (int x = minX; x <= maxX; x += 5)
    {
        unsortedVertices.insert(BWAPI::Position(x, minY) * 32 + BWAPI::Position(16, 16));
        unsortedVertices.insert(BWAPI::Position(x, maxY) * 32 + BWAPI::Position(16, 16));
    }

    for (int y = minY; y <= maxY; y += 5)
    {
        unsortedVertices.insert(BWAPI::Position(minX, y) * 32 + BWAPI::Position(16, 16));
        unsortedVertices.insert(BWAPI::Position(maxX, y) * 32 + BWAPI::Position(16, 16));
    }

    std::vector<BWAPI::Position> sortedVertices;
    BWAPI::Position current = *unsortedVertices.begin();

    m_mapEdgeVertices.push_back(current);
    unsortedVertices.erase(current);

    // while we still have unsorted vertices left, find the closest one remaining to current
    while (!unsortedVertices.empty())
    {
        double bestDist = 1000000;
        BWAPI::Position bestPos;

        for (const BWAPI::Position & pos : unsortedVertices)
        {
            double dist = pos.getDistance(current);

            if (dist < bestDist)
            {
                bestDist = dist;
                bestPos = pos;
            }
        }

        current = bestPos;
        sortedVertices.push_back(bestPos);
        unsortedVertices.erase(bestPos);
    }

    m_mapEdgeVertices = sortedVertices;
}

void TransportManager::drawTransportInformation(int x = 0, int y = 0)
{
    if (!Config::Debug::DrawUnitTargetInfo)
    {
        return;
    }

    for (size_t i(0); i < m_mapEdgeVertices.size(); ++i)
    {
        BWAPI::Broodwar->drawCircleMap(m_mapEdgeVertices[i], 4, BWAPI::Colors::Green, false);
        BWAPI::Broodwar->drawTextMap(m_mapEdgeVertices[i], "%d", i);
    }
}
bool TransportManager::isUnloading() {
    if (m_dropZealots.size() == 0 || !m_transportShip || !m_transportShip->exists() || !(m_transportShip->getHitPoints() > 0))
    {
        return false;
    }


    const BWAPI::UnitCommand currentCommand(m_transportShip->getLastCommand());
    if ((currentCommand.getType() == BWAPI::UnitCommandTypes::Unload_All
        || currentCommand.getType() == BWAPI::UnitCommandTypes::Unload_All_Position)
        && m_transportShip->getLoadedUnits().size() > 0)
    {
        return true;
    }
    else return false;
}
void TransportManager::handleUnload() {
    if (isUnloading()) return;
    if (m_transportShip->canUnloadAll() && m_transportShip->getLastCommand().getType() == BWAPI::UnitCommandTypes::Stop && isSlowEnough()) {
        m_transportShip->unloadAll();
        unloading == true;
    }
}
bool TransportManager::isSlowEnough() {
    const float velocity = std::sqrt(std::pow(m_transportShip->getVelocityX(),2) + std::pow(m_transportShip->getVelocityY(),2));
    if (velocity < transportShipTopSpeed * percentageCutOff) return true;
    else return false;
}
void TransportManager::update(BWAPI::Unitset dropZealots)
{

    m_dropZealots = dropZealots;
    if (!m_transportShip && getUnits().size() > 0)
    {
        m_transportShip = *getUnits().begin();
    }

    // calculate enemy region vertices if we haven't yet
    if (m_mapEdgeVertices.empty())
    {
        calculateMapEdgeVertices();
    }
    if (unload) {
        handleUnload();
    }
    if (sneak && !isUnloading() && !unload) {
        handleSneaking();
    }
    if (!isUnloading() && unloading) { //When done unloading go home
        Micro::SmartMove(m_transportShip, BWAPI::Broodwar->getClosestUnit(Global::Bases().getPlayerStartingBaseLocation(BWAPI::Broodwar->self())->getPosition())->getPosition());
    }
    if (!unload) {
        loadTroops();
        moveTroops();
        moveTransport();

    }
}
bool inPath(std::vector<BWAPI::TilePosition> sneakPath, BWAPI::UnitInterface* m_transportShip, int index) {
    for (int i = 0; i < std::min(index,(int)sneakPath.size()); i++) {
        if (sneakPath[i] == BWAPI::TilePosition(m_transportShip->getPosition())) return true;
    }
    return false;
}

void TransportManager::handleSneaking() {
    if (!m_sneakPath.size() > 0) return;
    
    if (m_sneakPath.size() < 5) m_transportShip->move(BWAPI::Position(Global::Bases().getPlayerStartingBaseLocation(BWAPI::Broodwar->enemy())->getDepotPosition()));

    BWAPI::TilePosition transporterTile = BWAPI::TilePosition(m_transportShip->getPosition());
    if (transporterTile == m_sneakPath[std::min(m_indexInSneak, (int)m_sneakPath.size() - 1)]) {
        m_indexInSneak++;
        BWAPI::Position nextTile = BWAPI::Position(m_sneakPath[std::min(m_indexInSneak + 2, (int)m_sneakPath.size() - 1)]);
        m_transportShip->move(BWAPI::Position(nextTile.x + 16,nextTile.y + 16));
    }
    else if (inPath(m_sneakPath,m_transportShip,m_indexInSneak)) {
        BWAPI::Position nextTile = BWAPI::Position(m_sneakPath[std::min(m_indexInSneak + 2, (int)m_sneakPath.size() - 1)]);
        m_transportShip->move(BWAPI::Position(nextTile.x + 16, nextTile.y + 16));
    }
    else {
        BWAPI::Position nextTile = BWAPI::Position(m_sneakPath[std::min(m_indexInSneak, (int)m_sneakPath.size() - 1)]);
        m_transportShip->move(BWAPI::Position(nextTile.x + 16, nextTile.y + 16));
        //m_indexInSneak++;
    }
    //
}

void TransportManager::loadTroops() {
    if (m_dropZealots.size() == 0 ||!m_transportShip || !m_transportShip->exists() || !(m_transportShip->getHitPoints() > 0))
    {
        return;
    }
    if (isUnloading() || unload) return;
    for(auto & unit:m_dropZealots)
    {
        if ((unit->getType() == BWAPI::UnitTypes::Protoss_Shuttle)) continue;

        m_transportShip->load(unit, true);
    }
}

void TransportManager::unloadAtPosition(BWAPI::Position position){

    if (!m_transportShip || !m_transportShip->exists() || !(m_transportShip->getHitPoints() > 0) || (m_transportShip->getLoadedUnits().size() < 4))
    {
        return;
    }


    if (isUnloading()) return;
    if (!unload) {
        m_transportShip->stop();
        unload = true;
    }
    

}

void TransportManager::moveTransport()
{
    if (!m_transportShip || !m_transportShip->exists() || !(m_transportShip->getHitPoints() > 0) || (m_transportShip->getLoadedUnits().size() < 4) ||  Global::Bases().getPlayerStartingBaseLocation(BWAPI::Broodwar->enemy()) == NULL)
    {
        return;
    }

    // If I didn't finish unloading the troops, wait
    if (isUnloading() || unload) return;
    ///*
    if (sneak && Global::Map().getMapFrame() % 48 == 0 && Global::Map().getMapFrame() > m_frameOnSneak + 500 && !inVision) {
        getSneakPath();

    }
    if (sneak && Global::Map().inVision(m_transportShip->getTilePosition()) && !inVision) {
        getSneakPath();
        inVision = true;
    }
    /**/
    else if (!sneak && sneakInConfig) {
        sneak = true * sneakInConfig;
        const auto enemyBaseLocation = Global::Bases().getPlayerStartingBaseLocation(BWAPI::Broodwar->enemy());
        const auto ourBaseLocation = Global::Bases().getPlayerStartingBaseLocation(BWAPI::Broodwar->self());
        Global::Map().updateCommonPath(ourBaseLocation->getDepotPosition(), enemyBaseLocation->getDepotPosition());
        m_sneakPath = Global::Map().getSneakyPath(BWAPI::TilePosition(m_transportShip->getPosition()), enemyBaseLocation->getDepotPosition());
        m_frameOnSneak = Global::Map().getMapFrame();
        
        
    }
    else if (!sneakInConfig) {
        const auto enemyBaseLocation = Global::Bases().getPlayerStartingBaseLocation(BWAPI::Broodwar->enemy());
        m_transportShip->move(enemyBaseLocation->getPosition());
    }



}

void UAlbertaBot::TransportManager::getSneakPath()
{
    const auto enemyBaseLocation = Global::Bases().getPlayerStartingBaseLocation(BWAPI::Broodwar->enemy());
    m_sneakPath = Global::Map().getSneakyPath(BWAPI::TilePosition(m_transportShip->getPosition()), enemyBaseLocation->getDepotPosition());
    m_indexInSneak = 2;
    /*
    while (m_transportShip->getTilePosition() != m_sneakPath[m_indexInSneak++]) {
        if (m_indexInSneak < m_sneakPath.size()) continue;
        m_indexInSneak = 0;
        break;
    }/**/
}

void TransportManager::moveTroops()
{
    if (!m_transportShip || !m_transportShip->exists() || !(m_transportShip->getHitPoints() > 0) || Global::Bases().getPlayerStartingBaseLocation(BWAPI::Broodwar->enemy()) == NULL)
    {
        return;
    }
    if (isUnloading()) return;
    //unload zealots if close enough or dying
    const int transportHP = m_transportShip->getHitPoints() + m_transportShip->getShields();

    const auto& enemyBaseLocation = Global::Bases().getPlayerStartingBaseLocation(BWAPI::Broodwar->enemy());
    const bool closeEnough = m_transportShip->getDistance(enemyBaseLocation->getPosition()) < m_dropRange;
    const bool dying =  transportHP < 100;
    const bool canUnload = m_transportShip->canUnloadAtPosition(m_transportShip->getPosition());

    if (enemyBaseLocation && (closeEnough || dying) && canUnload)
    {
        //unload troops 
        //and return? 

        // get the unit's current command

        if (!unload) {
            unloadAtPosition(m_transportShip->getPosition());
        }
    }

}

void TransportManager::followPerimeter(int clockwise)
{
    BWAPI::Position goTo = getFleePosition(clockwise);

    if (Config::Debug::DrawUnitTargetInfo)
    {
        BWAPI::Broodwar->drawCircleMap(goTo, 5, BWAPI::Colors::Red, true);
    }

    Micro::SmartMove(m_transportShip, goTo);
}

void TransportManager::followPerimeter(BWAPI::Position to, BWAPI::Position from)
{
    static int following = 0;
    if (following)
    {
        followPerimeter(following);
        return;
    }

    //assume we're near FROM! 
    if (m_transportShip->getDistance(from) < 50 && m_waypoints.empty())
    {
        //compute waypoints
        std::pair<int, int> wpIDX = findSafePath(to, from);
        bool valid = (wpIDX.first > -1 && wpIDX.second > -1);
        UAB_ASSERT_WARNING(valid, "waypoints not valid! (transport mgr)");
        m_waypoints.push_back(m_mapEdgeVertices[wpIDX.first]);
        m_waypoints.push_back(m_mapEdgeVertices[wpIDX.second]);

        BWAPI::Broodwar->printf("WAYPOINTS: [%d] - [%d]", wpIDX.first, wpIDX.second);

        Micro::SmartMove(m_transportShip, m_waypoints[0]);
    }
    else if (m_waypoints.size() > 1 && m_transportShip->getDistance(m_waypoints[0]) < 100)
    {
        BWAPI::Broodwar->printf("FOLLOW PERIMETER TO SECOND WAYPOINT!");
        //follow perimeter to second waypoint! 
        //clockwise or counterclockwise? 
        int closestPolygonIndex = getClosestVertexIndex(m_transportShip);
        UAB_ASSERT_WARNING(closestPolygonIndex != -1, "Couldn't find a closest vertex");

        if (m_mapEdgeVertices[(closestPolygonIndex + 1) % m_mapEdgeVertices.size()].getApproxDistance(m_waypoints[1]) <
            m_mapEdgeVertices[(closestPolygonIndex - 1) % m_mapEdgeVertices.size()].getApproxDistance(m_waypoints[1]))
        {
            BWAPI::Broodwar->printf("FOLLOW clockwise");
            following = 1;
            followPerimeter(following);
        }
        else
        {
            BWAPI::Broodwar->printf("FOLLOW counter clockwise");
            following = -1;
            followPerimeter(following);
        }

    }
    else if (m_waypoints.size() > 1 && m_transportShip->getDistance(m_waypoints[1]) < 50)
    {
        //if close to second waypoint, go to destination!
        following = 0;
        Micro::SmartMove(m_transportShip, to);
    }

}


int TransportManager::getClosestVertexIndex(BWAPI::UnitInterface * unit)
{
    int closestIndex = -1;
    double closestDistance = 10000000;

    for (size_t i(0); i < m_mapEdgeVertices.size(); ++i)
    {
        double dist = unit->getDistance(m_mapEdgeVertices[i]);
        if (dist < closestDistance)
        {
            closestDistance = dist;
            closestIndex = i;
        }
    }

    return closestIndex;
}

int TransportManager::getClosestVertexIndex(BWAPI::Position p)
{
    int closestIndex = -1;
    double closestDistance = 10000000;

    for (size_t i(0); i < m_mapEdgeVertices.size(); ++i)
    {

        double dist = p.getApproxDistance(m_mapEdgeVertices[i]);
        if (dist < closestDistance)
        {
            closestDistance = dist;
            closestIndex = i;
        }
    }

    return closestIndex;
}

std::pair<int, int> TransportManager::findSafePath(BWAPI::Position to, BWAPI::Position from)
{
    BWAPI::Broodwar->printf("FROM: [%d,%d]", from.x, from.y);
    BWAPI::Broodwar->printf("TO: [%d,%d]", to.x, to.y);

    //closest map edge point to destination
    int endPolygonIndex = getClosestVertexIndex(to);
    //BWAPI::Broodwar->printf("end indx: [%d]", endPolygonIndex);

    UAB_ASSERT_WARNING(endPolygonIndex != -1, "Couldn't find a closest vertex");
    BWAPI::Position enemyEdge = m_mapEdgeVertices[endPolygonIndex];

    auto * enemyBaseLocation = Global::Bases().getPlayerStartingBaseLocation(BWAPI::Broodwar->enemy());
    BWAPI::Position enemyPosition = enemyBaseLocation->getPosition();

    //find the projections on the 4 edges
    UAB_ASSERT_WARNING((m_minCorner.isValid() && m_maxCorner.isValid()), "Map corners should have been set! (transport mgr)");
    std::vector<BWAPI::Position> p;
    p.push_back(BWAPI::Position(from.x, m_minCorner.y));
    p.push_back(BWAPI::Position(from.x, m_maxCorner.y));
    p.push_back(BWAPI::Position(m_minCorner.x, from.y));
    p.push_back(BWAPI::Position(m_maxCorner.x, from.y));

    int d1 = p[0].getApproxDistance(enemyPosition);
    int d2 = p[1].getApproxDistance(enemyPosition);
    int d3 = p[2].getApproxDistance(enemyPosition);
    int d4 = p[3].getApproxDistance(enemyPosition);

    //have to choose the two points that are not max or min (the sides!)
    int maxIndex = (d1 > d2 ? d1 : d2) > (d3 > d4 ? d3 : d4) ?
        (d1 > d2 ? 0 : 1) : (d3 > d4 ? 2 : 3);

    int minIndex = (d1 < d2 ? d1 : d2) < (d3 < d4 ? d3 : d4) ?
        (d1 < d2 ? 0 : 1) : (d3 < d4 ? 2 : 3);

    if (maxIndex > minIndex)
    {
        p.erase(p.begin() + maxIndex);
        p.erase(p.begin() + minIndex);
    }
    else
    {
        p.erase(p.begin() + minIndex);
        p.erase(p.begin() + maxIndex);
    }

    //get the one that works best from the two.
    BWAPI::Position waypoint = (enemyEdge.getApproxDistance(p[0]) < enemyEdge.getApproxDistance(p[1])) ? p[0] : p[1];

    int startPolygonIndex = getClosestVertexIndex(waypoint);

    return std::pair<int, int>(startPolygonIndex, endPolygonIndex);

}

BWAPI::Position TransportManager::getFleePosition(int clockwise)
{
    UAB_ASSERT_WARNING(!m_mapEdgeVertices.empty(), "We should have a transport route!");

    // if this is the first flee, we will not have a previous perimeter index
    if (m_currentRegionVertexIndex == -1)
    {
        // so return the closest position in the polygon
        int closestPolygonIndex = getClosestVertexIndex(m_transportShip);

        UAB_ASSERT_WARNING(closestPolygonIndex != -1, "Couldn't find a closest vertex");

        if (closestPolygonIndex == -1)
        {
            return BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
        }
        else
        {
            // set the current index so we know how to iterate if we are still fleeing later
            m_currentRegionVertexIndex = closestPolygonIndex;
            return m_mapEdgeVertices[closestPolygonIndex];
        }
    }
    // if we are still fleeing from the previous frame, get the next location if we are close enough
    else
    {
        double distanceFromCurrentVertex = m_mapEdgeVertices[m_currentRegionVertexIndex].getDistance(m_transportShip->getPosition());

        // keep going to the next vertex in the perimeter until we get to one we're far enough from to issue another move command
        while (distanceFromCurrentVertex < 128*2)
        {
            m_currentRegionVertexIndex = (m_currentRegionVertexIndex + clockwise*1) % m_mapEdgeVertices.size();

            distanceFromCurrentVertex = m_mapEdgeVertices[m_currentRegionVertexIndex].getDistance(m_transportShip->getPosition());
        }

        return m_mapEdgeVertices[m_currentRegionVertexIndex];
    }

}

void TransportManager::setTransportShip(BWAPI::UnitInterface * unit)
{
    m_transportShip = unit;
}

void TransportManager::setFrom(BWAPI::Position from)
{
    if (from.isValid()) { m_from = from; }
}
void TransportManager::setTo(BWAPI::Position to)
{
    if (to.isValid()) { m_to = to; }
}
