#pragma once

#include "Common.h"
#include "Grid.hpp"
#include "DistanceMap.h"
#include "BaseLocation.h"

namespace UAlbertaBot
{
class InfluenceMap
{
    int                              m_width = 0;
    int                              m_height = 0;
    int                              m_viewDistance = 8;
    int                              m_minDist = 8;
    float                            m_w = 5;
    float                            m_damageModifyer = 2;
    bool                             m_goStraight = false;
    float                            m_maxInfluence = 2.0;
    bool                             m_drawCommonPath = true;
    DistanceMap                      m_distanceMap;
    Grid<int>                        m_dist;
    Grid<float>                      m_commonpath;
    Grid<float>                      m_influenced;
    Grid<float>                      m_airDamageMap;
    Grid<float>                      m_groundDamageMap;
    BWAPI::TilePosition              m_prevTile = BWAPI::TilePositions::Unknown;
    std::vector<BWAPI::TilePosition> m_sneakyPath;
    BWAPI::TilePosition              m_depotPosition;
    BWAPI::TilePosition              m_startTile;
    BWAPI::TilePosition              selectedAdjacentTile;
    BWAPI::Unitset                   m_enemyUnits;
    std::vector<BWAPI::TilePosition> findShortestPathInClosedQueue(std::vector<std::vector<std::tuple<BWAPI::TilePosition, BWAPI::TilePosition, float>>> closedQueue, BWAPI::TilePosition start, BWAPI::TilePosition end);
    std::vector<BWAPI::TilePosition> generateShortestPath(std::vector<std::vector<std::tuple<bool, BWAPI::TilePosition, float>>> closedQueue, BWAPI::TilePosition start, BWAPI::TilePosition end);
    float distance(int x1, int x2, int y1, int y2);
    int getNumSurroundingTiles(std::vector<std::vector<std::tuple<BWAPI::TilePosition, BWAPI::TilePosition, float>>> closedQueue, BWAPI::TilePosition tile);
    float influence(float distance, float power);
    float calcInfluence(int x, int y);
    float variableRangeInfluence(float distance, int sightRange, float power);
    float weightedDist(BWAPI::TilePosition start, BWAPI::TilePosition end);
    float cVal(float prevC, int a, BWAPI::TilePosition tile);
    void  storeInfluenceAndSneakyPath();
    BWAPI::TilePosition findNextAdjacentTile(BWAPI::TilePosition currentTile, int a);
    float C(std::vector<std::vector<std::tuple<bool, BWAPI::TilePosition, float>>> closedQueue, BWAPI::TilePosition currentTile, BWAPI::TilePosition nextTile);
    float H(BWAPI::TilePosition start, BWAPI::TilePosition end);
public:

    InfluenceMap();

    Grid<float>                      m_visionMap;
    void init();
    void computeCommonPath(BWAPI::TilePosition end);
    void computeVisionMap();
    void computeAirDamageMap();
    void computeGroundDamageMap();
    std::vector<BWAPI::TilePosition> getSneakyPath(BWAPI::TilePosition start, BWAPI::TilePosition end);
    std::vector<BWAPI::TilePosition> getSneakyPath2(BWAPI::TilePosition start, BWAPI::TilePosition end);
    float getCommonInfluence(int tileX, int tileY) const;
    float getCommonInfluence(const BWAPI::TilePosition& pos) const;
    float getCommonInfluence(const BWAPI::Position& pos) const;
    float getVisionAndInfluence(const BWAPI::TilePosition& pos) const;
    bool  inVision(const BWAPI::TilePosition& pos) const;
    float getVisionInfluence(int tileX, int tileY);
    float getInfluence(BWAPI::TilePosition pos);

    void draw() const;
};

}