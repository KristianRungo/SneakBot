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
#include <fstream>  
#include <algorithm> 


using namespace UAlbertaBot;

const size_t LegalActions = 8;
const int actionX[LegalActions] = { 1, -1, 0, 0, 1 ,-1, -1, 1 };
const int actionY[LegalActions] = { 0, 0, 1, -1, -1 , 1 , -1, 1 };

InfluenceMap::InfluenceMap()
{

}


float InfluenceMap::getCommonInfluence(int tileX, int tileY) const
{
    UAB_ASSERT(tileX < m_width&& tileY < m_height, "Index out of range: X = %d, Y = %d", tileX, tileY);

    return m_common.get(tileX, tileY);
}

float UAlbertaBot::InfluenceMap::getVisionInfluence(int tileX, int tileY)
{
    return m_visionMap.get(tileX, tileY);
}

float InfluenceMap::getCommonInfluence(const BWAPI::TilePosition& pos) const
{
    return getCommonInfluence(pos.x, pos.y);
}

float InfluenceMap::getCommonInfluence(const BWAPI::Position& pos) const
{
    return getCommonInfluence(BWAPI::TilePosition(pos));
}
float InfluenceMap::getVisionAndInfluence(const BWAPI::TilePosition& pos) const {
    int x = pos.x;
    int y = pos.y;
    return m_common.get(x, y) + m_visionMap.get(x, y);
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

std::vector<BWAPI::TilePosition> InfluenceMap::getSneakyPathBad(BWAPI::TilePosition start, BWAPI::TilePosition end) { //This one works..Kinda, but man is it ugly... getSneakyPath2 is much easier to read and is better implemented
    std::cout << "Started pathfinding\n";
    m_goStraight = false;
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

            m_sneakyPath = findShortestPathInClosedQueue(closedQueue, start, end);
            std::reverse(m_sneakyPath.begin(), m_sneakyPath.end());
            //storeInfluenceAndSneakyPath();
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
            //drawClosedQueue(closedGrid, start, end, currentTile); 
            UAB_ASSERT_WARNING(currentTile, "Got stuck in dead end loop"); //CurrentTile enclosed by tiles in closedGrid, so it is stuck.
            m_goStraight = true;
            std::vector<std::vector<std::tuple<BWAPI::TilePosition, BWAPI::TilePosition, float>>> closedQueueTmp = closedQueue;
            while (getNumSurroundingTiles(closedQueue, currentTile) > 2) { //Go backwards untill the other side of the cluster is found. 
                BWAPI::TilePosition nextTile = std::get<0>(closedQueue[currentTile.x][currentTile.y]);
                closedQueueTmp[currentTile.x][currentTile.y] = std::make_tuple(BWAPI::TilePositions::Unknown, BWAPI::TilePositions::Unknown, std::numeric_limits<float>::max());
                //drawClosedQueue(closedQueueTmp, start, end, currentTile);
                //For each tile in the cluster, delete it from the closedGrid
                currentTile = nextTile;
            }
            closedQueue = closedQueueTmp;
            if (currentTile == BWAPI::TilePositions::Unknown) {
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
            continue;
        }
        
        //SAVE ADJACENT TILE AS THE NEXT IN SHORTEST PATH
        selectedAdjacentTile = std::get<1>(closedQueue[currentTile.x][currentTile.y]); 
        //std::cout << selectedAdjacentTile << "\n";
        if (selectedAdjacentTile == BWAPI::TilePositions::Unknown) {
            m_endDist++;
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
        
        //drawClosedQueue(closedGrid);
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
    bool distTest = w_dist > m_endDist;
    bool infTest = getVisionAndInfluence(start) > 0.0;
    
    if (distTest && infTest && !m_goStraight) {
        return w_dist + w_dist * (0.5 *getVisionAndInfluence(start));
    }
    return w_dist;
}
float octDist(BWAPI::TilePosition start, BWAPI::TilePosition end) {
    const float h_diagonal = std::min(std::abs(start.x - end.x), std::abs(start.y - end.y));
    const float h_straight = std::abs(start.x - end.x) + std::abs(start.y - end.y);
    return  (1 * h_straight + (std::sqrt(2) - 2 * 1) * h_diagonal);
}
std::vector<BWAPI::TilePosition> InfluenceMap::findBestShortPath(std::vector<BWAPI::TilePosition> path1, std::vector<BWAPI::TilePosition> path2) {
    float totalVision1 = 0;
    float totalVision2 = 0;
    for (BWAPI::TilePosition tile : path1) {
        totalVision1 += getInfluence(tile);
    }
    for (BWAPI::TilePosition tile : path2) {
        totalVision2 += getInfluence(tile);
    }
    if (path2 < path1) return path2;
    return path1;
}
std::tuple<bool, BWAPI::TilePosition, float, bool> setParent(BWAPI::TilePosition currentTile, std::vector<std::vector<std::tuple<bool, BWAPI::TilePosition, float, bool>>> closedQueue, BWAPI::TilePosition parent) {
    bool closed = std::get<0>(closedQueue[currentTile.x][currentTile.y]);
    float c = std::get<2>(closedQueue[currentTile.x][currentTile.y]);
    bool open = std::get<3>(closedQueue[currentTile.x][currentTile.y]);

    return std::make_tuple(closed, parent, c, open);

}
std::tuple<bool, BWAPI::TilePosition, float, bool> setClosed(BWAPI::TilePosition currentTile, std::vector<std::vector<std::tuple<bool, BWAPI::TilePosition, float, bool>>> closedQueue, bool closed) {
    BWAPI::TilePosition parent = std::get<1>(closedQueue[currentTile.x][currentTile.y]);
    float c = std::get<2>(closedQueue[currentTile.x][currentTile.y]);
    bool open = std::get<3>(closedQueue[currentTile.x][currentTile.y]);

    return std::make_tuple(closed, parent, c, open);

}
std::tuple<bool, BWAPI::TilePosition, float, bool> setC(BWAPI::TilePosition currentTile, std::vector<std::vector<std::tuple<bool, BWAPI::TilePosition, float, bool>>> closedQueue, float c) {
    BWAPI::TilePosition parent = std::get<1>(closedQueue[currentTile.x][currentTile.y]);
    bool closed  = std::get<0>(closedQueue[currentTile.x][currentTile.y]);
    bool open = std::get<3>(closedQueue[currentTile.x][currentTile.y]);

    return std::make_tuple(closed, parent, c, open);

}
std::tuple<bool, BWAPI::TilePosition, float, bool> setOpen(BWAPI::TilePosition currentTile, std::vector<std::vector<std::tuple<bool, BWAPI::TilePosition, float, bool>>> closedQueue, bool open) {
    BWAPI::TilePosition parent = std::get<1>(closedQueue[currentTile.x][currentTile.y]);
    bool closed = std::get<0>(closedQueue[currentTile.x][currentTile.y]);
    float c = std::get<2>(closedQueue[currentTile.x][currentTile.y]);

    return std::make_tuple(closed, parent, c, open);

}
BWAPI::TilePosition getParent(BWAPI::TilePosition currentTile, std::vector<std::vector<std::tuple<bool, BWAPI::TilePosition, float, bool>>> closedQueue) {
    if (currentTile == BWAPI::TilePositions::Unknown) return BWAPI::TilePositions::Unknown;
    return std::get<1>(closedQueue[currentTile.x][currentTile.y]);
}
bool getClosed(BWAPI::TilePosition currentTile, std::vector<std::vector<std::tuple<bool, BWAPI::TilePosition, float, bool>>> closedQueue) {
    if (currentTile == BWAPI::TilePositions::Unknown) return false;
    return std::get<0>(closedQueue[currentTile.x][currentTile.y]);
}
bool getOpen(BWAPI::TilePosition currentTile, std::vector<std::vector<std::tuple<bool, BWAPI::TilePosition, float, bool>>> closedQueue) {
    if (currentTile == BWAPI::TilePositions::Unknown) return false;
    return std::get<3>(closedQueue[currentTile.x][currentTile.y]);
}
float getC(BWAPI::TilePosition currentTile, std::vector<std::vector<std::tuple<bool, BWAPI::TilePosition, float, bool>>> closedQueue) {
    if (currentTile == BWAPI::TilePositions::Unknown) return std::numeric_limits<float>::max();
    return std::get<2>(closedQueue[currentTile.x][currentTile.y]);
}
std::priority_queue<std::tuple<float, float, BWAPI::TilePosition>> removeOldItems(std::priority_queue<std::tuple<float, float, BWAPI::TilePosition>> openQueue, std::vector<std::vector<std::tuple<bool, BWAPI::TilePosition, float, bool>>> closedQueue) {

    BWAPI::TilePosition currentTile = std::get<2>(openQueue.top());


    while (!getOpen(currentTile, closedQueue)) { //If tile should be deleted from openQueue, delete it from openQueue
        openQueue.pop();
        currentTile = std::get<2>(openQueue.top());
    }

    return openQueue;
}
BWAPI::TilePosition InfluenceMap::findNextAdjacentTile(BWAPI::TilePosition currentTile, int a) {
    int x = std::max(std::min(std::abs(currentTile.x + actionX[a]), m_width - 2), 1);
    int y = std::max(std::min(std::abs(currentTile.y + actionY[a]), m_height - 2), 1);
    return BWAPI::TilePosition(x, y);
}
std::vector<BWAPI::TilePosition> InfluenceMap::getSneakyPath(BWAPI::TilePosition start, BWAPI::TilePosition end) {
    
    std::vector<BWAPI::TilePosition> sneakyNormal = getSneakyPath2(start, end);
    m_sneakyPath = sneakyNormal;
    if (m_doRev) {
        std::vector<BWAPI::TilePosition> sneakyRev = getSneakyPath2(end, start);
        std::reverse(sneakyRev.begin(), sneakyRev.end());
        m_sneakyPathRev = sneakyRev;
        m_doRev = false;
        m_firstPath = false;
        if (m_firstPath && m_storePathAndInfluence) storeInfluenceAndSneakyPath();
        return findBestShortPath(sneakyNormal, sneakyRev);

    }

    if (m_firstPath && m_storePathAndInfluence) storeInfluenceAndSneakyPath();
    m_firstPath = false;
    return m_sneakyPath;
}
std::vector<BWAPI::TilePosition> InfluenceMap::getSneakyPath2(BWAPI::TilePosition start, BWAPI::TilePosition end) {
    std::cout << "started pathfinding";
    std::priority_queue<std::tuple<float, float, BWAPI::TilePosition>> openQueue;

    std::vector<std::vector<std::tuple<bool, BWAPI::TilePosition, float, bool>>> closedGrid(
        m_width, std::vector <std::tuple<bool, BWAPI::TilePosition, float, bool>>
        (m_height, std::make_tuple(false, BWAPI::TilePositions::Unknown, std::numeric_limits<float>::max(),false)));


    openQueue.push(std::make_tuple((-1) * (H(start, start, end) ), 0, start));                             //add start to open and closed
    closedGrid[start.x][start.y] = setC(start, closedGrid, 0);
    closedGrid[start.x][start.y] = setOpen(start, closedGrid, true);
    while (!openQueue.empty()) {

        openQueue = removeOldItems(openQueue, closedGrid);
        BWAPI::TilePosition currentTile = std::get<2>(openQueue.top());
        float currentTileWeight = std::get<1>(openQueue.top());
        openQueue.pop();
        closedGrid[currentTile.x][currentTile.y] = setOpen(currentTile, closedGrid, false);                     //Remove from open

        closedGrid[currentTile.x][currentTile.y] = setClosed(currentTile, closedGrid, true);                    //Add to closed

        if (currentTile == end) {                                                                               //End tile found, make shortest path
            std::cout << "Ended pathfinding \n";
            if (pathingGrid.size() == 0) pathingGrid = closedGrid;
            else if (pathingGridRev.size() == 0 && m_doRev) pathingGridRev = closedGrid;
            return generateShortestPath(closedGrid, start, end);
        }
        for (size_t a = 0; a < LegalActions; ++a)                                                               //Check all adjacentTiles
        {
            BWAPI::TilePosition adjacentTile = findNextAdjacentTile(currentTile, a);
            const float adjacentTileH = H(start, adjacentTile, end);
            const float adjacentTileC = C(closedGrid,currentTile, adjacentTile,end);
            const float adjacentTileF = adjacentTileH + adjacentTileC;
            const float adjacentOldC  = getC(adjacentTile, closedGrid);

            if (adjacentTileC < adjacentOldC) {                                                                 //if adjacent tile cost < cost to currentParent 
                if(getOpen(currentTile, closedGrid))   closedGrid[adjacentTile.x][adjacentTile.y] = setOpen(adjacentTile, closedGrid, false);   //Remove adjacent tile from open, if in open
                if (getClosed(adjacentTile, closedGrid)) { //Remove adjacent tile from closed, if in closed
                    closedGrid[adjacentTile.x][adjacentTile.y] = setClosed(adjacentTile, closedGrid, false); 
                    closedGrid[adjacentTile.x][adjacentTile.y] = setParent(adjacentTile, closedGrid, BWAPI::TilePositions::Unknown);
                } 

            }
            if (!getClosed(adjacentTile, closedGrid) && !getOpen(currentTile, closedGrid)) {                     //if tile isn't in closed and open
                openQueue.push(std::make_tuple((-1) * (adjacentTileF), adjacentTileC, adjacentTile));           //set f(adjacentTile) to new F
                closedGrid[adjacentTile.x][adjacentTile.y] = setC(adjacentTile, closedGrid, adjacentTileC);     //set C(adjacentTile) to new C
                closedGrid[adjacentTile.x][adjacentTile.y] = setOpen(adjacentTile, closedGrid, true);           //add to open
                closedGrid[adjacentTile.x][adjacentTile.y] = setParent(adjacentTile, closedGrid, currentTile);  //Make adjacentTile point to currentTile
            }
        }
    }
    std::cout << "Ended pathfinding";
    return { end }; //Open queue is empty, no viable path found, return only the endTile to B-line
}

std::vector<BWAPI::TilePosition> InfluenceMap::generateShortestPath(std::vector<std::vector<std::tuple<bool, BWAPI::TilePosition, float, bool>>> closedQueue, BWAPI::TilePosition start, BWAPI::TilePosition end) {
    std::vector<BWAPI::TilePosition> shortestTilePath;
    shortestTilePath.push_back(end);
    while (shortestTilePath.back() != start) {
        shortestTilePath.push_back(getParent(shortestTilePath.back(), closedQueue));
    }
    std::reverse(shortestTilePath.begin(), shortestTilePath.end());
    return shortestTilePath;
}
float InfluenceMap::C(std::vector<std::vector<std::tuple<bool, BWAPI::TilePosition, float, bool>>> closedQueue, BWAPI::TilePosition currentTile, BWAPI::TilePosition nextTile, BWAPI::TilePosition end) {
    int deltaX = std::abs(currentTile.x - nextTile.x);
    int deltaY = std::abs(currentTile.y - nextTile.y);
    float currentTileC = getC(currentTile, closedQueue);
    float step = currentTileC + ((deltaX * deltaY != 0) ? 1.41 : 1);
    float nextTileDist = octDist(end, currentTile); 
    if (currentTileC > m_startDist && nextTileDist > m_endDist) {
        return step + getInfluence(nextTile);
    }
    return step + getNotCommon(nextTile);
}
float InfluenceMap::H(BWAPI::TilePosition start, BWAPI::TilePosition currentTile, BWAPI::TilePosition end) {
    // Prøv med w_dist * 0.8 når du er uden for start og end. 
    if (currentTile == BWAPI::TilePositions::Unknown) return std::numeric_limits<float>::max();
    const float w_dist = octDist(end, currentTile);
    const float startDist = octDist(start, currentTile);
    if (w_dist > m_endDist && startDist > m_startDist) { 
        return w_dist * m_w + w_dist * (2 * getInfluence(currentTile)) + 200;
    }
    if (w_dist < m_endDist) { 
        return w_dist * m_w;
    }
    if (m_common.get(currentTile.x, currentTile.y) == 0) { 
        return w_dist * m_w + w_dist * (2 * getInfluence(currentTile)) + 200;
    }
    if (m_visionMap.get(currentTile.x, currentTile.y) == 0) return (w_dist * m_startWeight) + getInfluence(currentTile) + 1000;
    return (w_dist * m_startWeight) + getNotCommon(currentTile) + 1000;
}
float InfluenceMap::getInfluence(BWAPI::TilePosition pos) { //Returns the sum of all influences
    int x = pos.x;
    int y = pos.y;
    float common = m_common.get(x, y);
    float vision = (m_visionMap.get(x, y) > 0) ? m_maxInfluence : 0;
    float ground = m_groundDamageMap.get(x, y) * m_damageModifyer;
    float    air = m_airDamageMap.get(x, y) * m_damageModifyer;
    return common + vision + ground + air  * 2;
}

float InfluenceMap::getNotCommon(BWAPI::TilePosition pos) {
    int x = pos.x;
    int y = pos.y;
    const float vision = m_visionMap.get(x, y);
    const float ground = m_groundDamageMap.get(x, y) * m_damageModifyer;
    const float    air = m_airDamageMap.get(x, y) * m_damageModifyer;
    return vision + ground + air;
}
float InfluenceMap::distance(int x1, int x2, int y1, int y2) {
    return std::sqrt(std::pow((x2 - x1), 2) + std::pow((y2 - y1) * 1.0, 2));
}

float InfluenceMap::influence(float distance, float power) {
    return (m_maxInfluence - std::pow((m_maxInfluence * (distance / m_viewDistance)), power));
}                  

float InfluenceMap::calcInfluence(int x, int y) {
    if (m_common.get(x, y) == m_maxInfluence) return 0.0;
    const int minXi = std::max(x - m_viewDistance, 0);
    const int maxXi = std::min(x + m_viewDistance, m_width - 1);
    const int minYi = std::max(y - m_viewDistance, 0);
    const int maxYi = std::min(y + m_viewDistance, m_height - 1);
        
    float lowestDist = m_viewDistance + 1.0;
    for (int xi = minXi; xi < maxXi; xi++) {
        for (int yi = minYi; yi < maxYi; yi++) {
            if (x == xi && y == yi) continue;
            else if (m_common.get(xi, yi) == m_maxInfluence) {
                const float dist = distance(x, xi, y, yi);
                if (dist <= m_viewDistance) {
                    lowestDist = std::min(dist, lowestDist);
                }
            }
        }
    }
    if (lowestDist <= 8.0) {
        return (m_maxInfluence - std::pow((m_maxInfluence * (lowestDist / m_viewDistance)), 4));
    }
    else return 0.0;
}

void InfluenceMap::computeCommonPath(BWAPI::TilePosition end) {
    PROFILE_FUNCTION();
    BWAPI::TilePosition start = m_depotPosition;
    m_common = Grid<float>(m_width, m_height, 0);
    m_influenced = Grid<float>(m_width, m_height, 0);

    BWAPI::Position pos = BWAPI::Position(end.x, end.y);
  
    while (pos.x != start.x || pos.y != start.y) // While you have not reached your own base
    {
        m_common.set(pos.x, pos.y, m_maxInfluence);

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
            m_common.set(x, y, std::max(m_common.get(x, y), m_influenced.get(x, y)));
        }
    }
}
void InfluenceMap::init() 
{
    PROFILE_FUNCTION();
    const BaseLocation* baseLocation = Global::Bases().getPlayerStartingBaseLocation(BWAPI::Broodwar->self());
    m_depotPosition = baseLocation->getDepotPosition();
    m_distanceMap = DistanceMap();
    m_distanceMap.computeDistanceMap(baseLocation->getDepotPosition());
    m_dist = m_distanceMap.getDistanceMap();
    m_width = BWAPI::Broodwar->mapWidth();
    m_height = BWAPI::Broodwar->mapHeight();
    m_common = Grid<float>(m_width, m_height, 0);
    m_influenced = Grid<float>(m_width, m_height, 0);
    m_visionMap = Grid<float>(m_width, m_height, 0);
    m_airDamageMap = Grid<float>(m_width, m_height, 0);
    m_groundDamageMap = Grid<float>(m_width, m_height, 0);
    m_firstPath = true;
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
                    m_visionMap.set(xi, yi, std::max(variableRangeInfluence(dist,sightRange, 4), m_visionMap.get(xi, yi)));
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
                    m_airDamageMap.set(xi, yi, variableRangeInfluence(dist, maxRange, 4) + m_airDamageMap.get(xi, yi));
                }
            }
        }
    }
}
void InfluenceMap::computeGroundDamageMap() {
    BWAPI::Player enemy = BWAPI::Broodwar->enemy(); //TODO
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
                    m_groundDamageMap.set(xi, yi, variableRangeInfluence(dist, maxRange, 4) + m_groundDamageMap.get(xi, yi));

                }
            }
        }
    }
}
void InfluenceMap::draw() const
{
    return;
    for (int x = 0; x < m_width; x++) {
        for (int y = 0; y < m_height; y++) {
            if (m_common.get(x, y) > 0 && m_drawCommonPath) { // TODO: Set back to 0
                Global::Map().drawTile(x, y, BWAPI::Color(
                    255,
                    255 - (255 * (m_common.get(x, y) / m_maxInfluence)),
                    255 - (255 * (m_common.get(x, y) / m_maxInfluence))));
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
            if (m_groundDamageMap.get(x, y) > 5) { // TODO: Set back to 0
                Global::Map().drawTile(x, y, BWAPI::Color(
                    0,
                    255,
                    0));
            }
            if (m_airDamageMap.get(x, y) > 5) { // TODO: Set back to 0
                Global::Map().drawTile(x, y, BWAPI::Color(
                    0,
                    0,
                    255));
            }
            
            
            if (!pathingGrid.size() > 0 && !m_drawExploredTiles) continue;
            else if (std::get<2>(pathingGrid[x][y]) != std::numeric_limits<float>::max()) {
                Global::Map().drawTile(x, y, BWAPI::Color(
                    0,
                    255,
                    255));
            }

            if (!pathingGridRev.size() > 0 && !m_doRev)continue;
            if (std::get<2>(pathingGridRev[x][y]) != std::numeric_limits<float>::max()) {
                Global::Map().drawTile(x, y, BWAPI::Color(
                    0,
                    255,
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
    if (!m_doRev) return;
    for (BWAPI::TilePosition tile : m_sneakyPathRev) {
        Global::Map().drawTile(tile.x, tile.y, BWAPI::Color(
            128,
            0,
            128));
    }
}
bool InfluenceMap::inVision(const BWAPI::TilePosition& pos) const {
    return (m_visionMap.get(pos.x,pos.y) > 0) ? true : false;
}
void InfluenceMap::storeInfluenceAndSneakyPath() {

    //Save shortest path
    std::ofstream foutSneaky("shortestPath.txt");
    if (!foutSneaky.is_open()) return;
    

    for (int i = 0; i < m_sneakyPath.size(); i++) {
        foutSneaky << m_sneakyPath[i] << "\n";
    }
    foutSneaky.close();

    //Save commonpathinfluencemap
    std::ofstream foutCommon("commonPath.txt");
    if (!foutCommon.is_open()) return;

    for (int x = 0; x < m_width; x++) {
        for (int y = 0; y < m_height; y++) {
            if (y != m_height - 1)  foutCommon << m_common.get(x, y) << ",";
            else foutCommon << m_common.get(x, y) << "";
        }
        foutCommon << "\n";
    }
    foutCommon.close();

    //Save vision map
    std::ofstream foutVision("visionMap.txt");
    if (!foutVision.is_open()) return;

    for (int x = 0; x < m_width; x++) {
        for (int y = 0; y < m_height; y++) {
            if (y != m_height - 1)  foutVision << m_visionMap.get(x, y) << ",";
            else foutVision << m_visionMap.get(x, y) << "";;
        }
        foutVision << "\n";
    }
    foutVision.close();

    //Save groundDamage map
    std::ofstream foutGround("groundMap.txt");
    if (!foutGround.is_open()) return;

    for (int x = 0; x < m_width; x++) {
        for (int y = 0; y < m_height; y++) {
            if (y != m_height - 1)  foutGround << m_groundDamageMap.get(x, y) << ",";
            else foutGround << m_groundDamageMap.get(x, y) << "";;
        }
        foutGround << "\n";
    }
    foutGround.close();

    //Save airDamage map
    std::ofstream foutAir("airMap.txt");
    if (!foutAir.is_open()) return;

    for (int x = 0; x < m_width; x++) {
        for (int y = 0; y < m_height; y++) {
            if (y != m_height - 1)  foutAir << m_airDamageMap.get(x, y) << ",";
            else foutAir << m_airDamageMap.get(x, y) << "";;
        }
        foutAir << "\n";
    }
    foutAir.close();

}
