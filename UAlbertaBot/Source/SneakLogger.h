#pragma once
#include "Common.h"
#include "ParseUtils.h"
#include "UnitUtil.h"
#include<string>
#include<vector>
#include<fstream>
#include "rapidjson\document.h"


namespace UAlbertaBot
{
	struct Game
	{
		std::string   m_strategy;
		int			  m_unitslost;
		int			  m_shuttlehealth;
		float		  m_traveltime;
		int			  m_timespotted;
		bool		  m_won;
	
		
	Game()
		: m_strategy("")
		, m_unitslost(0)
		, m_shuttlehealth(0)
		, m_traveltime(0)
		, m_timespotted(0)
		, m_won(false)
	{
	}

	};

	class SneakLogger
	{

		rapidjson::Document     generateJsonObject(Game game);
		bool  					appendToFile(rapidjson::Document doc);
	public:

		SneakLogger();

		Game					m_game;

		void					onStart();
		void					onFrame();
		void					onEnd();

	};

}