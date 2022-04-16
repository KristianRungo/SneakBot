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
    int                              m_minDist = 4;
    float                            m_maxInfluence = 1.0;
    DistanceMap                      m_distanceMap;
    Grid<int>                        m_dist;
    Grid<float>                      m_influence;
    Grid<float>                      m_influenced;
    Grid<float>                      m_airDamageMap;
    Grid<float>                      m_groundDamageMap;
    Grid<float>                      m_visionMap;
    BWAPI::TilePosition              m_prevTile = BWAPI::TilePositions::Unknown;
    std::vector<BWAPI::TilePosition> m_sneakyPath;
    BWAPI::TilePosition              m_depotPosition;
    BWAPI::TilePosition              m_startTile;
    BWAPI::TilePosition              selectedAdjacentTile;
    BWAPI::Unitset                   m_enemyUnits;
    std::vector<BWAPI::TilePosition> findShortestPathInClosedQueue(std::vector<std::vector<std::tuple<BWAPI::TilePosition, BWAPI::TilePosition, float>>> closedQueue, BWAPI::TilePosition start, BWAPI::TilePosition end);
    float distance(int x1, int x2, int y1, int y2);
    int getNumSurroundingTiles(std::vector<std::vector<std::tuple<BWAPI::TilePosition, BWAPI::TilePosition, float>>> closedQueue, BWAPI::TilePosition tile);
    float influence(float distance, float power);
    float calcInfluence(int x, int y);
    float variableRangeInfluence(float distance, int sightRange, float power);
    float weightedDist(BWAPI::TilePosition start, BWAPI::TilePosition end);
    float InfluenceMap::cVal(float prevC, int a, BWAPI::TilePosition tile);

public:

    InfluenceMap();

    void computeStartDepotInfluenceMap();
    void computeVisionMap();
    void computeAirDamageMap();
    void computeGroundDamageMap();
    std::vector<BWAPI::TilePosition> getSneakyPath(BWAPI::TilePosition start, BWAPI::TilePosition end);
    float getInfluence(int tileX, int tileY) const;
    float getInfluence(const BWAPI::TilePosition& pos) const;
    float getInfluence(const BWAPI::Position& pos) const;
    float getVision(const BWAPI::TilePosition& pos) const;

    void draw() const;
};

}