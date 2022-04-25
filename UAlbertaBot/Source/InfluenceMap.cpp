#include "InfluenceMap.h"
#include "MapTools.h"
#include "Global.h"
#include "BaseLocation.h"
#include "BaseLocationManager.h"
#include "DistanceMap.h"
#include "InformationManager.h"

#include <iterator>
#include <chrono>
#include <thread>
#include <functional>
#include <queue>
#include <vector>
#include <iostream>
#include <algorithm> 


using namespace UAlbertaBot;

const size_t LegalActions = 8;
const int actionX[LegalActions] = { 1, -1, 0, 0, 1 ,-1, -1, 1 };
const int actionY[LegalActions] = { 0, 0, 1, -1, -1 , 1 , -1, 1 };

InfluenceMap::InfluenceMap()
{

}

float InfluenceMap::getInfluence(int tileX, int tileY) const
{
    UAB_ASSERT(tileX < m_width&& tileY < m_height, "Index out of range: X = %d, Y = %d", tileX, tileY);

    return m_influence.get(tileX, tileY);
}

float UAlbertaBot::InfluenceMap::getVisionInfluence(int tileX, int tileY)
{
    return m_visionMap.get(tileX, tileY);
}

float InfluenceMap::getInfluence(const BWAPI::TilePosition& pos) const
{
    return getInfluence(pos.x, pos.y);
}

float InfluenceMap::getInfluence(const BWAPI::Position& pos) const
{
    return getInfluence(BWAPI::TilePosition(pos));
}
float InfluenceMap::getVisionAndInfluence(const BWAPI::TilePosition& pos) const {
    int x = pos.x;
    int y = pos.y;
    return m_influence.get(x, y) + m_visionMap.get(x, y);
}

void drawClosedQueue(std::vector<std::vector<std::tuple<BWAPI::TilePosition, BWAPI::TilePosition, float>>> closedQueue, BWAPI::TilePosition start, BWAPI::TilePosition end, BWAPI::TilePosition pos) {
    std::cout << "\n";
    int row = 0;
    for (auto& line : closedQueue) {
        for (int i = 0; i < line.size() - 1; i++) {
            float cVal = std::get<2>(line[i]);
            if      (BWAPI::TilePosition(i,row) == start) std::cout << "S";
            else if (BWAPI::TilePosition(i,row) == end) std::cout << "E";
            else if (BWAPI::TilePosition(i,row) == pos) std::cout << "C";
            else std::cout << (cVal == std::numeric_limits<float>::max()) ? 1 : 0;
        }
        std::cout << "\n";
        row++;
    }
}
int InfluenceMap::getNumSurroundingTiles(std::vector<std::vector<std::tuple<BWAPI::TilePosition, BWAPI::TilePosition, float>>> closedQueue, BWAPI::TilePosition tile) {
    int adjacentTiles = 0;
    for (size_t a = 0; a < LegalActions; ++a) // Count adjacentTiles that are good
    {
        const int x = std::min(std::abs(tile.x + actionX[a]), m_width - 1);
        const int y = std::min(std::abs(tile.y + actionY[a]), m_height - 1);
        const BWAPI::TilePosition adjacentTile = BWAPI::TilePosition(x, y);
        if (std::get<2>(closedQueue[x][y]) == std::numeric_limits<float>::max()) continue;
        adjacentTiles++;
    }
    return adjacentTiles;
}

std::vector<BWAPI::TilePosition>  InfluenceMap::findShortestPathInClosedQueue
(std::vector<std::vector<std::tuple<BWAPI::TilePosition, BWAPI::TilePosition, float>>> closedQueue, BWAPI::TilePosition start, BWAPI::TilePosition end) { //This works allright, but it could be better
    std::vector<BWAPI::TilePosition> shortestTilePath;
    BWAPI::TilePosition currentTile = end;
    shortestTilePath.push_back(currentTile);
    while (currentTile != start) { 
        if (getNumSurroundingTiles(closedQueue, currentTile) > 2) { //Cluster of nodes incomming
            BWAPI::TilePosition clusterEndTile = currentTile;
            while (getNumSurroundingTiles(closedQueue, clusterEndTile) > 2) { //Go backwards untill the other side of the cluster is found. 
                clusterEndTile = std::get<0>(closedQueue[clusterEndTile.x][clusterEndTile.y]);
                if (clusterEndTile == BWAPI::TilePositions::Unknown) clusterEndTile = end;
            }
            while (currentTile != clusterEndTile) { //For each tile, find the adjacent tile closest to the end of the cluster, push it to the shortestTilePath and check from that tile
                BWAPI::TilePosition bestTile = currentTile;
                for (size_t a = 0; a < LegalActions; ++a)
                {
                    const int x = std::min(std::abs(currentTile.x + actionX[a]), m_width - 1);
                    const int y = std::min(std::abs(currentTile.y + actionY[a]), m_height - 1);
                    const BWAPI::TilePosition adjacentTile = BWAPI::TilePosition(x, y);
                    if (std::get<2>(closedQueue[x][y]) == std::numeric_limits<float>::max()) continue;
                    if (weightedDist(bestTile, clusterEndTile) > weightedDist(adjacentTile, clusterEndTile)) {
                        bestTile = adjacentTile;
                    }
                }
                currentTile = bestTile;
                shortestTilePath.push_back(currentTile);
            }
        }
        else {
            currentTile = std::get<0>(closedQueue[currentTile.x][currentTile.y]);
            shortestTilePath.push_back(currentTile);
        }
    }
    return shortestTilePath;
    
}


std::vector<BWAPI::TilePosition>  InfluenceMap::findShortestPathInClosedQueue2
(std::vector<std::vector<std::tuple<BWAPI::TilePosition, BWAPI::TilePosition, float>>> closedQueue, BWAPI::TilePosition start, BWAPI::TilePosition end) { 
    std::vector<BWAPI::TilePosition> shortestTilePath;
    BWAPI::TilePosition currentTile = end;
    while (currentTile != start) {


        if (getNumSurroundingTiles(closedQueue, currentTile) > 2) { //More than one option ahead
            BWAPI::TilePosition clusterEndTile = currentTile;
            std::vector<std::vector<float>> clusterTiles(m_width, std::vector<float>(m_height, -1));;
            std::vector<std::vector<BWAPI::TilePosition>> clusterTilesPoint(m_width, std::vector<BWAPI::TilePosition>(m_height, BWAPI::TilePositions::Unknown));;
            while (getNumSurroundingTiles(closedQueue, clusterEndTile) > 2 && clusterEndTile != start) { //Go backwards untill the other side of the cluster is found. 
                clusterTiles[clusterEndTile.x][clusterEndTile.y] = std::numeric_limits<float>::max();
                clusterEndTile = std::get<0>(closedQueue[clusterEndTile.x][clusterEndTile.y]);
                if (clusterEndTile == BWAPI::TilePositions::Unknown || clusterEndTile == std::get<0>(closedQueue[clusterEndTile.x][clusterEndTile.y])) clusterEndTile = start;
            }
            clusterTiles[clusterEndTile.x][clusterEndTile.y] = std::numeric_limits<float>::max();
            BWAPI::TilePosition clusterStartTile = currentTile;
            clusterTiles[currentTile.x][currentTile.y] = 0;
            while (currentTile != clusterEndTile) { //Calculate shortest path for all tiles 
                for (size_t a = 0; a < LegalActions; ++a)
                {
                    const int x = std::min(std::abs(currentTile.x + actionX[a]), m_width - 1);
                    const int y = std::min(std::abs(currentTile.y + actionY[a]), m_height - 1);
                    const BWAPI::TilePosition adjacentTile = BWAPI::TilePosition(x, y);
                    if (clusterTiles[x][y] == -1) continue;
                    if (clusterTiles[x][y] < clusterTiles[currentTile.x][currentTile.y] + (float)(((currentTile.x - adjacentTile.x) * (currentTile.y - adjacentTile.y) != 0) ? 1.41 : 1.0)) continue;
                    clusterTilesPoint[x][y] = currentTile;
                    clusterTiles[x][y] = clusterTiles[currentTile.x][currentTile.y] + (float)(((currentTile.x - adjacentTile.x) * (currentTile.y - adjacentTile.y) != 0) ? 1.41 : 1.0);

                }
                currentTile = std::get<0>(closedQueue[currentTile.x][currentTile.y]);
            }
            currentTile = clusterEndTile;
            std::vector<BWAPI::TilePosition> shortestPathInCluster;
            while (currentTile != clusterStartTile) {
                shortestPathInCluster.push_back(currentTile);
                currentTile = clusterTilesPoint[currentTile.x][currentTile.y];
            }
            std::reverse(shortestPathInCluster.begin(), shortestPathInCluster.end());
            for (BWAPI::TilePosition tile : shortestPathInCluster) shortestTilePath.push_back(tile);
            currentTile = std::get<0>(closedQueue[clusterEndTile.x][clusterEndTile.y]);
            if (currentTile == BWAPI::TilePositions::Unknown) continue; 
            shortestTilePath.push_back(currentTile);

        }
        else {
            
            if (currentTile == BWAPI::TilePositions::Unknown) return shortestTilePath;
            currentTile = std::get<0>(closedQueue[currentTile.x][currentTile.y]);
            shortestTilePath.push_back(currentTile);
        }
    }
    return shortestTilePath;
    

}

std::vector<BWAPI::TilePosition> InfluenceMap::getSneakyPath(BWAPI::TilePosition start, BWAPI::TilePosition end) {
    std::cout << "Started pathfinding\n";
    int stuckCounter = 0;
    std::priority_queue<std::tuple<float, float, BWAPI::TilePosition>> openQueue;
    std::vector<std::vector<std::tuple<BWAPI::TilePosition, BWAPI::TilePosition, float>>> closedQueue(
        m_width, std::vector <std::tuple<BWAPI::TilePosition, BWAPI::TilePosition, float>>
        (m_height, std::make_tuple(BWAPI::TilePositions::Unknown, BWAPI::TilePositions::Unknown, std::numeric_limits<float>::max())));
    //Array with tileposition tuples, to point forwards and backwards first is back, second is forward, the current cost to travel to node is defaulted to the biggest float value
    std::vector<BWAPI::TilePosition> shortestTilePath;
    int possibleNodesToVisit = m_width * m_height;
    openQueue.push(std::make_tuple((-1) * weightedDist(start, end), 0, start)); //Set start position with weightedDistance and distance traveled
    closedQueue[start.x][start.y] = std::make_tuple(BWAPI::TilePositions::Unknown, BWAPI::TilePositions::Unknown, 0); //Set travel cost of start node to 0
    while (openQueue.size() != possibleNodesToVisit) { //WHILE THERE ARE NODES LEFT TO EXPLORE! 
        BWAPI::TilePosition currentTile = std::get<2>(openQueue.top());//Get best tile
        float currentTileVal = std::get<1>(openQueue.top());//Get best tile cost
        if (currentTile.x == end.x && currentTile.y == end.y) { // CALC AND RETURN BEST PATH IF WE HAVE REACHED OUR GOAL!
            std::cout << "Found end" << "\n";

            m_sneakyPath = findShortestPathInClosedQueue2(closedQueue, start, end);
            std::reverse(m_sneakyPath.begin(), m_sneakyPath.end());
            m_goStraight = false;
            for (auto tile : m_sneakyPath) {
                if (tile == BWAPI::TilePositions::Unknown)
                {
                    std::cout << "Its here boss!";
                }
            }
            return m_sneakyPath;

        }
        BWAPI::TilePosition currentTileParent = std::get<1>(closedQueue[currentTile.x][currentTile.y]);
        if (currentTileParent == currentTile) closedQueue[currentTile.x][currentTile.y] = std::make_tuple(std::get<0>(closedQueue[currentTile.x][currentTile.y]), BWAPI::TilePositions::Unknown, std::get<2>(closedQueue[currentTile.x][currentTile.y]));
        float parentTileVal = (currentTileParent == BWAPI::TilePositions::Unknown) ? std::numeric_limits<float>::max() : std::get<2>(closedQueue[currentTileParent.x][currentTileParent.y]);
        //Get parent. If parent doesn't exist, set parentTileVal to max float nr (This is good)
        for (size_t a = 0; a < LegalActions; ++a) // CHECK ALL ADJACENT TILES!
        {
            int x = std::max(std::min(std::abs(currentTile.x + actionX[a]), m_width - 2),1);
            int y = std::max(std::min(std::abs(currentTile.y + actionY[a]), m_height - 2),1);
            const BWAPI::TilePosition adjacentTile = BWAPI::TilePosition(x, y);
            if (std::get<1>(closedQueue[adjacentTile.x][adjacentTile.y]) != BWAPI::TilePositions::Unknown) { 
                continue;//THE TILE YOU ARE LOOKING AT IS ALLREADY IN THE SHORTEST PATH!
            }
            if ((weightedDist(adjacentTile, end) + cVal(currentTileVal,a, adjacentTile) < weightedDist(currentTileParent, end) + parentTileVal) && adjacentTile != currentTile) {
                closedQueue[currentTile.x][currentTile.y] = std::make_tuple(std::get<0>(closedQueue[currentTile.x][currentTile.y]), adjacentTile, currentTileVal);
                parentTileVal = cVal(currentTileVal, a, adjacentTile);
                currentTileParent = adjacentTile;
                //IF ADJACENT TILE IS CLOSER THAN THE CURRENT PARENT THEN MAKE IT THE PARENT
            }
            openQueue.push(std::make_tuple((-1) * (weightedDist(adjacentTile, end)) - cVal(currentTileVal, a,adjacentTile),cVal(currentTileVal,a, adjacentTile), adjacentTile));
        }
        if (parentTileVal == std::numeric_limits<float>::max()) { //If parentTileVal was never changed, we never found an adjacent tile that was closer, or wasn't allready part of our shortest path
            //std::cout << "I got stuck :S";
            stuckCounter++;
            if (stuckCounter > 10) {
                std::cout << "even more fucked!";
            }
            //drawClosedQueue(closedQueue, start, end, currentTile); 
            UAB_ASSERT_WARNING(currentTile, "Got stuck in dead end loop"); //CurrentTile enclosed by tiles in closedQueue, so it is stuck.
            m_goStraight = true;
            std::vector<std::vector<std::tuple<BWAPI::TilePosition, BWAPI::TilePosition, float>>> closedQueueTmp = closedQueue;
            while (getNumSurroundingTiles(closedQueue, currentTile) > 2) { //Go backwards untill the other side of the cluster is found. 
                BWAPI::TilePosition nextTile = std::get<0>(closedQueue[currentTile.x][currentTile.y]);
                closedQueueTmp[currentTile.x][currentTile.y] = std::make_tuple(BWAPI::TilePositions::Unknown, BWAPI::TilePositions::Unknown, std::numeric_limits<float>::max());
                //drawClosedQueue(closedQueueTmp, start, end, currentTile);
                //For each tile in the cluster, delete it from the closedQueue
                currentTile = nextTile;
            }
            closedQueue = closedQueueTmp;
            if (currentTile == BWAPI::TilePositions::Unknown) {
                std::cout << "All messed up, will now just B-line from base";
                drawClosedQueue(closedQueue, start, end, currentTile);
                while (!openQueue.empty()) openQueue.pop();
                closedQueue = std::vector<std::vector<std::tuple<BWAPI::TilePosition, BWAPI::TilePosition, float>>> (
                    m_width, std::vector <std::tuple<BWAPI::TilePosition, BWAPI::TilePosition, float>>
                    (m_height, std::make_tuple(BWAPI::TilePositions::Unknown, BWAPI::TilePositions::Unknown, std::numeric_limits<float>::max())));
                //clear openQueue
                currentTile = start;
                openQueue.push(std::make_tuple((-1) * weightedDist(start, end), 0, start)); 
                closedQueue[start.x][start.y] = std::make_tuple(BWAPI::TilePositions::Unknown, BWAPI::TilePositions::Unknown, 0);
            }
            closedQueue[currentTile.x][currentTile.y] = std::make_tuple(std::get<0>(closedQueue[currentTile.x][currentTile.y]), BWAPI::TilePositions::Unknown, std::get<2>(closedQueue[currentTile.x][currentTile.y]));
            //Make current tile forget next tile
            while (!openQueue.empty()) openQueue.pop();
            //clear openQueue
            openQueue.push(std::make_tuple((-1) * (weightedDist(currentTile, end)) - std::get<2>(closedQueue[currentTile.x][currentTile.y]), std::get<2>(closedQueue[currentTile.x][currentTile.y]), currentTile));

            //drawClosedQueue(closedQueue, start, end, currentTile);
            continue;
        }
        
        //SAVE ADJACENT TILE AS THE NEXT IN SHORTEST PATH
        selectedAdjacentTile = std::get<1>(closedQueue[currentTile.x][currentTile.y]); 
        //std::cout << selectedAdjacentTile << "\n";
        if (selectedAdjacentTile == BWAPI::TilePositions::Unknown) {
            //drawClosedQueue(closedQueue, start, end, currentTile);
            UAB_ASSERT_WARNING(currentTile, "Stuck :(, incrementing min distance");
            std::cout << "Stuck :(, incrementing min distance";
            //drawClosedQueue(closedQueue,start,end, currentTile);
            m_minDist++;
        }
        else {
            closedQueue[selectedAdjacentTile.x][selectedAdjacentTile.y] = std::make_tuple(
                currentTile,
                std::get<1>(closedQueue[selectedAdjacentTile.x][selectedAdjacentTile.y]),
                parentTileVal);
            if (currentTile == std::get<2>(openQueue.top())) { //If the optimal move is to move away from the center, reset the openQueue, and add only the adjacentTile
                while (!openQueue.empty()) openQueue.pop();
                openQueue.push(std::make_tuple((-1) * weightedDist(selectedAdjacentTile, end) - parentTileVal, parentTileVal, selectedAdjacentTile));
            }
        }
        m_prevTile = selectedAdjacentTile;
        
        //drawClosedQueue(closedQueue);
        //std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    UAB_ASSERT_WARNING(start, "End not reachable from start");
    std::cout << "End not reachable from start";
    m_goStraight = false;
    return shortestTilePath;
}
float InfluenceMap::cVal(float prevC, int a, BWAPI::TilePosition tile) {
    return prevC + ((actionX[a] * actionY[a] != 0) ? 1.41 : 1);// +(getInfluence(tile));
}


float InfluenceMap::weightedDist(BWAPI::TilePosition start, BWAPI::TilePosition end) {
    if (start == BWAPI::TilePositions::Unknown) return std::numeric_limits<float>::max();
    const float h_diagonal = std::min(std::abs(start.x - end.x), std::abs(start.y - end.y));
    const float h_straight = std::abs(start.x - end.x) + std::abs(start.y - end.y);
    const float w_dist = (std::sqrt(2) * h_diagonal + (h_straight - (2.0 * h_diagonal))) + m_airDamageMap.get(start.x,start.y);
    //const float distToOwnBase = distance(start.x, m_depotPosition.x, start.y, m_depotPosition.y);
    //if (distToOwnBase < 15) return w_dist;
    bool distTest = w_dist > m_minDist;
    bool infTest = getVisionAndInfluence(start) > 0.0;
    
    if (distTest && infTest && !m_goStraight) {
        return w_dist + w_dist * (0.5 *getVisionAndInfluence(start));
    }
    return w_dist;
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

void InfluenceMap::computeCommonPath(BWAPI::TilePosition start, BWAPI::TilePosition end) {
    PROFILE_FUNCTION();
    m_drawCommonPath = true;
    m_distanceMap = DistanceMap();
    m_distanceMap.computeDistanceMap(start);
    m_dist = m_distanceMap.getDistanceMap();

    m_influence = Grid<float>(m_width, m_height, 0);
    m_influenced = Grid<float>(m_width, m_height, 0);

    BWAPI::Position pos = BWAPI::Position(end.x, end.y);
  
    while (pos.x != start.x || pos.y != start.y) // While you have not reached your own base
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
            if (m_dist.get(x, y) > 20) m_influence.set(x, y, std::max(m_influence.get(x, y), m_influenced.get(x, y)));
            else m_influence.set(x, y, 0);
        }
    }
}
void InfluenceMap::computeStartDepotInfluenceMap()
{
    PROFILE_FUNCTION();
    const BaseLocation* baseLocation = Global::Bases().getPlayerStartingBaseLocation(BWAPI::Broodwar->self());
    m_depotPosition = baseLocation->getDepotPosition();
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
            if (m_dist.get(x,y) > 10) m_influence.set(x, y,std::max(m_influence.get(x,y),m_influenced.get(x,y)));
            else m_influence.set(x, y, 0);
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
            if (m_influence.get(x, y) > 0 && m_drawCommonPath) { // TODO: Set back to 0
                Global::Map().drawTile(x, y, BWAPI::Color(
                    255,
                    255 - (255 * (m_influence.get(x, y) / m_maxInfluence)),
                    255 - (255 * (m_influence.get(x, y) / m_maxInfluence))));
            }
            if (m_visionMap.get(x, y) > 1) { // TODO: Set back to 0
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
    for (BWAPI::TilePosition tile : m_sneakyPath) {
        Global::Map().drawTile(tile.x, tile.y, BWAPI::Color(
            128,
            0,
            128));
    }
}
bool InfluenceMap::inVision(const BWAPI::TilePosition& pos) const {
    return (m_visionMap.get(pos.x,pos.y) > 0) ? true : false;
}
