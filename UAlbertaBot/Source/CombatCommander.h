#pragma once

#include "Common.h"
#include "SquadData.h"

namespace UAlbertaBot
{

class Squad;
class CombatCommander
{
    SquadData       m_squadData;
    BWAPI::Unitset  m_combatUnits;
    bool            m_initialized       = false;
    bool            m_dropSquadCreated  = false;
    bool            m_dropShipFull      = false;
    bool            m_dropCompleted     = false;
    bool            m_transferDropUnits = false;
    bool            m_dropShipMoved     = false;
    bool            m_dropSquadSuicide  = Config::Micro::DropSquadSuicide;

    void updateScoutDefenseSquad();
    void updateDefenseSquads();
    void updateAttackSquads();
    void updateDropSquads();
    void updateIdleSquad();
    void initializeSquads();
    void handleDrop();
    void transferDropUnits();
    void updateDefenseSquadUnits(Squad & defenseSquad, const size_t & flyingDefendersNeeded, const size_t & groundDefendersNeeded);

    int  defendWithWorkers();
    int  numZerglingsInOurBase();
    bool beingBuildingRushed();
    bool isSquadUpdateFrame();

    BWAPI::Unit     findClosestDefender(const Squad & defenseSquad, BWAPI::Position pos, bool flyingDefender);
    BWAPI::Unit     findClosestWorkerToTarget(BWAPI::Unitset & unitsToAssign, BWAPI::Unit target);
    BWAPI::Position getDefendLocation();
    BWAPI::Position getMainAttackLocation();

public:

    CombatCommander();

    void update(const BWAPI::Unitset & combatUnits);

    void drawSquadInformation(int x, int y);
};
}
