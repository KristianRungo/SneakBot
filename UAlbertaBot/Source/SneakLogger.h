#pragma once
#include "Common.h"
#include "ParseUtils.h"
#include "UnitUtil.h"
#include "TimerManager.h"
#include<string>
#include<vector>
#include<fstream>
#include "rapidjson\document.h"
#include "../../BOSS/source/Timer.hpp"


namespace UAlbertaBot
{
	struct Game
	{
		std::string   m_strategy;       // Which strategy did we employ
		int			  m_unitslost;		// How many units did we lose in the sneak attack (Before drop or after?))
		int			  m_shuttlehealth;  // What was shuttle health + shield at the end of drop
		float		  m_beforesneak;	// Time spent before sneak started
		float		  m_traveltime;     // How long was travel time for shuttle
		float		  m_timespotted;	// for how long was shuttle seen by enemy while on path
		bool		  m_won;			// Did we win
		std::string   m_enemyrace;		// Name of enemy race
		std::string   m_map;			// Name of played map
	
		
	Game()
		: m_strategy("")
		, m_unitslost(0)
		, m_shuttlehealth(0)
		, m_traveltime(0.0)
		, m_timespotted(0.0)
		, m_won(false)
	{
	}

	};

	class SneakLogger
	{

		rapidjson::Document     generateJsonObject(Game game);
		bool  					appendToFile(rapidjson::Document doc);

		float						dropFull;
		float						dropCompleted;

		public:
		
		SneakLogger();

		Game					m_game;

		void					onStart();
		void					onFrame(bool, bool, int, BWAPI::Position);
		void					onEnd(bool);

	};

}