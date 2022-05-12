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
		std::string   m_strategy;			// Which strategy did we employ
		int			  m_unitslost;			// How many units did we lose in the sneak attack (Before drop or after?))
		int			  m_shuttlehealth;		// What was shuttle health + shield at the end of drop
		float		  m_beforesneak;		// Time spent before sneak started
		float		  m_traveltime;			// How long was travel time for shuttle
		float		  m_timespotted;		// for how long was shuttle seen by enemy while on path
		bool		  m_won;				// Did we win
		int			  m_workersbuilt;		// How many of our workers are alive after 
		int			  m_workerslost;		// How many of our workers are alive after 
		float		  m_enemynearbasetime;  // When did the enemy get close to our base
		int			  m_dropUnitKills;
		std::string   m_enemyrace;			// Name of enemy race
		std::string   m_map;				// Name of played map
	
		
	Game()
		: m_strategy("")
		, m_unitslost(0)
		, m_shuttlehealth(0)
		, m_beforesneak(0.0)
		, m_traveltime(0.0)
		, m_timespotted(0.0)
		, m_won(false)
		, m_workerslost(0)
		, m_workersbuilt(0)
		, m_enemynearbasetime(0.0)
		, m_dropUnitKills(0)
	{
	}

	};


	class SneakLogger
	{

		Game					m_game;
		bool  					appendToFile(rapidjson::Document doc);
		rapidjson::Document     generateJsonObject(Game game);
		BWAPI::Position			startingPosition;


		public:
		
		SneakLogger();


		void					onStart();
		void					onUnitCreate(BWAPI::Unit);
		void					onUnitShow(BWAPI::Unit);
		void					onUnitDestroy(BWAPI::Unit unit);
		void					onFrame(bool, bool, int, BWAPI::Position, int,int);
		void					onEnd(bool);

	};
}