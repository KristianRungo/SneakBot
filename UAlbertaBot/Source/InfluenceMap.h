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
    float                            m_maxInfluence = 1.0;
    DistanceMap                      m_distanceMap;
    Grid<int>                        m_dist;
    Grid<float>                        m_influence;
    Grid<float>                        m_influenced;
    BWAPI::TilePosition              m_startTile;
    std::vector<BWAPI::TilePosition> m_sortedTiles;
    

public:

    InfluenceMap();
    void computeStartDepotInfluenceMap();

    int getInfluence(int tileX, int tileY) const;
    int getInfluence(const BWAPI::TilePosition& pos) const;
    int getInfluence(const BWAPI::Position& pos) const;

    void draw() const;
};

}