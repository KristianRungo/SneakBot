
#pragma once

#include "Grid2D.hpp"
#include "StarDraftMap.hpp"
#include "BaseFinder.hpp"
#include "DistanceMap.hpp"


#include <vector>

class InfluenceMap
{
    BaseFinder              m_baseFinder;
    const StarDraftMap* m_map = nullptr;

    DistanceMap             m_distanceMap;


    Grid2D<float>           m_influence;
    Grid2D<float>           m_influenced;
    Grid2D<unsigned char>   m_directions;
    std::vector<Tile>       m_stack;
    std::vector<Base>       m_bases;
    float                   p = 1.0; //Max influence
    float                   d = 5.0; //Max distance of influence
    float                   influenceTmp;
    Tile                    ourDepot;



    Grid2D<float> computeCommonPathInfluence()
    {
        // Set all fields to null value
        m_influence = Grid2D<float>(m_map->width(), m_map->height(), 0.0);
        m_influenced = Grid2D<float>(m_map->width(), m_map->height(), 0.0);
        // Select "random base to be ours

        int ourBase = 1;


        //Generate direction map to our base
        ourDepot = m_bases[ourBase].getDepotTile();

        m_distanceMap.compute(*m_map, ourDepot.x, ourDepot.y);

        m_directions = m_distanceMap.getDirections();

        // Find shortest path from all bases to ours and compute influence field

        for (unsigned int i = 0u; i < m_bases.size(); i++) {
            if (i == ourBase) continue;
            Tile currentPos = m_bases[i].getDepotTile();

            while ((currentPos.x != ourDepot.x) || (currentPos.y != ourDepot.y)) {
                m_influence.set(currentPos.x, currentPos.y, p);
                //find næste tile, prioritizing going down and to the right

                if ((m_directions.get(currentPos.x, currentPos.y) & 1) == 1) {
                    currentPos.y += 1;
                }
                else if ((m_directions.get(currentPos.x, currentPos.y) & 2) == 2) {
                    currentPos.y -= 1;
                }
                else if ((m_directions.get(currentPos.x, currentPos.y) & 4) == 4) {
                    currentPos.x += 1;
                }
                else if ((m_directions.get(currentPos.x, currentPos.y) & 8) == 8) {
                    currentPos.x -= 1;
                }
            }
        }

        for (size_t x = 0; x < m_map->width(); x++)
        {
            for (size_t y = 0; y < m_map->height(); y++)
            {
                if (m_directions.get(x, y) != 0) m_influenced.set(x, y, calcInfluence(x, y));
            }
        }

        for (size_t x = 0; x < m_map->width(); x++)
        {
            for (size_t y = 0; y < m_map->height(); y++)
            {
                float infOrg = m_influence.get(x, y);
                float infDer = m_influenced.get(x, y);
                if (infOrg > 0) m_influence.set(x, y, infOrg);
                else m_influence.set(x, y, infDer + infOrg);
            }
        }
        return m_influence;
    }

    float dist(int x1, int x2, int y1, int y2) {
        return std::sqrt(std::pow((x2 - x1), 2) + std::pow((y2 - y1)*1.0, 2));
    }
    float influence(int x,int y) {
        float distance = d + 1;
        for (int xi = 0; xi < m_map->width(); xi++)
        {
            for (int yi = 0; yi < m_map->height(); yi++)
            {
                if (xi == x && yi == y){
                    continue;
                }
                else if (m_influence.get(xi, yi) > 0) distance = std::min(dist(x, xi, y, yi), distance);
            }
        }
        return (p - pow((p * (distance / d)), .90));
    }
    
    float calcInfluence(int x, int y) {

        return influence(x, y);
    }

    
    /*float influence(int x1, int x2, int y1, int y2) {
        float distance = dist(x1, x2, y1, y2);
        if (distance <= d) {
            float dis = dist(x1, x2, y1, y2);
            float influence = p-pow((p*(dis/d)),2);
            return dis;
        }
        else return 0;
    }
    float calcInfluence(int x, int y) {
        influenceTmp = -1;
        for (int xi = 0; xi < m_map->width(); xi++)
        {
            for (int yi = 0; yi < m_map->height(); yi++)
            {
                if (influenceTmp > 0) return influenceTmp;
                if (m_influence.get(xi, yi) > 0) {
                    influenceTmp = std::max(influence(x, xi, y, yi), influenceTmp);
                   
                }
            }
        }
        return influenceTmp;
    }*/


public:

    InfluenceMap()
    {
        m_stack.reserve(128 * 128);
    }

    InfluenceMap(const StarDraftMap& map, std::vector<Base> m_basesIn)
        : m_map(&map)
    {
        m_bases = m_basesIn;
        m_stack.reserve(128 * 128);
        compute(map, m_basesIn);
    }
    Grid2D<float> compute(const StarDraftMap& map, std::vector<Base> m_basesIn)
    {
        m_map = &map;
        m_bases = m_basesIn;
        return computeCommonPathInfluence();
    }

    Tile getOurDepot() {
        return ourDepot;
    }


    inline size_t width() const
    {
        return m_map ? m_map->width() : 0;
    }

    inline size_t height() const
    {
        return m_map ? m_map->height() : 0;
    }

    inline const std::vector<Tile>& getClosestTiles() const
    {
        return m_stack;
    }
    

};

