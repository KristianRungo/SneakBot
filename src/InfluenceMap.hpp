
#pragma once

#include "Grid2D.hpp"
#include "StarDraftMap.hpp"
#include "BaseFinder.hpp"
#include "DistanceMap.hpp"


#include <vector>

    class InfluenceMap
    {
        BaseFinder              m_baseFinder;
        const StarDraftMap*     m_map = nullptr;
        
        DistanceMap             m_distanceMap;

        Grid2D<short>           m_influence;
        Grid2D<unsigned char>   m_directions;
        std::vector<Tile>       m_stack;
        std::vector<Base>       m_bases;



        void computeCommonPathInfluence()
        {
            // Set all fields to null value
            m_influence = Grid2D<float>(m_map->width(), m_map->height(), 0);

            // Find all bases 
            m_baseFinder.computeBases(m_map);
            m_bases = m_baseFinder.getBases();

            // Select "random base to be ours

            int ourBase = 1;

       

            //Generate direction map to our base
            Tile ourDepot = m_bases[ourBase].getDepotTile();

            m_distanceMap.compute(*m_map, ourDepot.x, ourDepot.y);

            m_directions = m_distanceMap.getDirections();

            // Find shortest path from all bases to ours and compute influence field

            for (int i = 0; i < m_bases.size(); i++) {
                if (i == ourBase) break;
                Tile currentPos = m_bases[i].getDepotTile();
                while (m_directions[currentPos] != 0) {
                    m_influence[currentPos] = 1;
                    //find næste tile
                    bool downMarker = false;
                    bool upMarker = false;
                    bool rightMarker = false;
                    bool leftMarker = false;

                    if ((m_directions[currentPos] & 1) == 1) {
                        downMarker = true;
                    }
                    if ((m_directions[currentPos] & 2) == 2)){
                        upMarker = true;
                    }
                    if ((m_directions[currentPos] & 4) == 4)){
                        rightMarker = true;
                    }
                    if ((m_directions[currentPos] & 8) == 8)){
                        leftMarker = true;
                    }
                    if (downMarker && upMarker && rightMarker && leftMarker) currentPos.x = currentPos.x - 1;
                    if (downMarker) currentPos.y = currentPos.y + 1;
                    if (upMarker) currentPos.y = currentPos.y - 1;
                    if (rightMarker) currentPos.x = currentPos.x + 1;
                    if (leftMarker) currentPos.x = currentPos.x - 1;
                }
            }
        }

    public:

        InfluenceMap()
        {
            m_stack.reserve(128 * 128);
        }

        InfluenceMap(const StarDraftMap & map)
            : m_map(&map)
        {
            m_stack.reserve(128*128);
            compute(map, gx, gy);
        }

    void compute(const StarDraftMap & map)
        {
            m_gx  = gx;

            computeCommonPathInfluence();
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

