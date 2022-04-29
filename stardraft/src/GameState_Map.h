#pragma once

#include "GameState.h"
#include "WorldView.hpp"
#include "StarDraftMap.hpp"
#include "DistanceMap.hpp"
#include "BaseFinder.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <chrono>
#include <iostream>
#include "InfluenceMap.hpp"

class GameState_Map : public GameState
{   
    sf::Font m_font;             
    sf::Text m_text;

    int  m_tileSize         = 32;       // size of tiles (keep 32)
    bool m_drawGrid         = true;     // draw the grid lines
    bool m_drawLoadedInfluence = true;    // Draw the influence
    bool m_drawSneakPath = true; // Draw the influenceNumbers
    bool m_drawWalkTiles    = true;     // draw the walk tiles
    bool m_drawBuildTiles   = true;     // draw the build tiles
    bool m_drawField        = true;     // draw the distance map values?
    bool m_drawDistance     = true;     // draw the distance map (t) directions (f)
    bool m_leftMouseDown    = false;    // is the left mouse button down
    bool m_filesLoaded = false;
    bool m_draw = false;
    int m_pgx               = -1;       // previous goal x
    int m_pgy               = -1;       // previous goal y
    Grid2D<float>       m_loadedVision;
    Grid2D<float>       m_loadedCommonPath;
    Grid2D<float>       m_loadedGround;
    Grid2D<float>       m_loadedAir;
    std::vector<std::tuple<float,float>>       m_loadedSneak;

    sf::VertexArray     m_buildTileArray;
    sf::VertexArray     m_walkTileArray;

    const std::string   m_mapFile;
    StarDraftMap        m_map;            
    DistanceMap         m_field;
    BaseFinder          m_baseFinder;
    std::vector<Base>   m_bases;
    InfluenceMap        m_influenceMap;
    Grid2D<float>       m_influence;
    Tile                ourDepot;

    Vec2                m_drag = { -1, -1 };
    Vec2                m_mouseScreen;
    Vec2                m_mouseWorld;
    Vec2                m_mouseGrid;
    
    WorldView           m_view;
    
    void init();  
    
    void setMapVertexArray();
    void sUserInput();  
    void sRender();
    void loadInfluenceAndSneakFromFiles();

    Grid2D<float> loadFile(std::fstream& file);
    
public:

    GameState_Map(GameEngine& game, const std::string & mapFile);

    void onFrame();
};
