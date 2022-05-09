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
    int                              m_endDist = m_viewDistance + 2;
    float                            m_startDist = m_viewDistance + 2;
    float                            m_w = 2;
    float                            m_damageModifyer = 2;
    bool                             m_goStraight = false;
    float                            m_maxInfluence = 1;
    bool                             m_drawCommonPath = true;
    bool                             m_drawExploredTiles = false;
    bool                             m_doRev = true;
    DistanceMap                      m_distanceMap;
    Grid<int>                        m_dist;
    Grid<float>                      m_common;
    Grid<float>                      m_influenced;
    Grid<float>                      m_airDamageMap;
    Grid<float>                      m_groundDamageMap;
    BWAPI::TilePosition              m_prevTile = BWAPI::TilePositions::Unknown;
    std::vector<BWAPI::TilePosition> m_sneakyPath;
    std::vector<BWAPI::TilePosition> m_sneakyPathRev;
    BWAPI::TilePosition              m_depotPosition;
    BWAPI::TilePosition              m_startTile;
    BWAPI::TilePosition              selectedAdjacentTile;
    BWAPI::Unitset                   m_enemyUnits;
    bool                             m_firstPath;
    bool                             m_storePathAndInfluence = false;
    std::vector<std::vector<std::tuple<bool, BWAPI::TilePosition, float, bool>>> pathingGrid;
    std::vector<std::vector<std::tuple<bool, BWAPI::TilePosition, float, bool>>> pathingGridRev;

    std::vector<BWAPI::TilePosition> findShortestPathInClosedQueue(std::vector<std::vector<std::tuple<BWAPI::TilePosition, BWAPI::TilePosition, float>>> closedQueue, BWAPI::TilePosition start, BWAPI::TilePosition end);
    std::vector<BWAPI::TilePosition> generateShortestPath(std::vector<std::vector<std::tuple<bool, BWAPI::TilePosition, float, bool>>> closedQueue, BWAPI::TilePosition start, BWAPI::TilePosition end);
    float distance(int x1, int x2, int y1, int y2);
    int getNumSurroundingTiles(std::vector<std::vector<std::tuple<BWAPI::TilePosition, BWAPI::TilePosition, float>>> closedQueue, BWAPI::TilePosition tile);
    float influence(float distance, float power);
    float calcInfluence(int x, int y);
    float variableRangeInfluence(float distance, int sightRange, float power);
    float weightedDist(BWAPI::TilePosition start, BWAPI::TilePosition end);
    float cVal(float prevC, int a, BWAPI::TilePosition tile);
    void  storeInfluenceAndSneakyPath();
    BWAPI::TilePosition findNextAdjacentTile(BWAPI::TilePosition currentTile, int a);
    float C(std::vector<std::vector<std::tuple<bool, BWAPI::TilePosition, float, bool>>> closedQueue, BWAPI::TilePosition currentTile, BWAPI::TilePosition nextTile, BWAPI::TilePosition end);
    float H(BWAPI::TilePosition start, BWAPI::TilePosition current, BWAPI::TilePosition end);
    float getNotCommon(BWAPI::TilePosition pos);
public:

    InfluenceMap();

    Grid<float>                      m_visionMap;
    void init();
    void computeCommonPath(BWAPI::TilePosition end);
    void computeVisionMap();
    void computeAirDamageMap();
    void computeGroundDamageMap();
    std::vector<BWAPI::TilePosition> getSneakyPathBad(BWAPI::TilePosition start, BWAPI::TilePosition end);
    std::vector<BWAPI::TilePosition> getSneakyPath2(BWAPI::TilePosition start, BWAPI::TilePosition end);
    std::vector<BWAPI::TilePosition> getSneakyPath(BWAPI::TilePosition start, BWAPI::TilePosition end);
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