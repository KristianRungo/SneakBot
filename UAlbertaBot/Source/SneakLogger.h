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
		std::string   m_strategy = "";
		int			  m_unitslost = 0;
		int			  m_shuttlehealth = 0;
		float		  m_traveltime = 0;
		int			  m_timespotted = 0;
		bool		  m_won = false;


		Game()
		{

		}

		Game(const std::string& strategy)
			: m_strategy(strategy)
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
		friend class Global;

		SneakLogger();

		Game m_game = Game();

		rapidjson::Document     generateJsonObject(Game game);
		bool  					appendToFile(rapidjson::Document doc);
		void					onStart();
		void					onFrame();
		void					onEnd();

	};

}