#pragma once

#include "Common.h"
#include "GameCommander.h"
#include "AutoObserver.h"
#include "InfluenceMap.h"

namespace UAlbertaBot
{

class UAlbertaBotModule
{
	GameCommander m_gameCommander;
	AutoObserver  m_autoObserver;
	InfluenceMap  m_influenceMap;

public:

	UAlbertaBotModule();
	void	onStart();
	void	onFrame();
	void	handleRestart(int gameCount);
	void	onEnd(bool isWinner, int gameCount);
	void	onUnitDestroy(BWAPI::Unit unit);
	void	onUnitMorph(BWAPI::Unit unit);
	void	onSendText(std::string text);
	void	onUnitCreate(BWAPI::Unit unit);
	void	onUnitComplete(BWAPI::Unit unit);
	void	onUnitShow(BWAPI::Unit unit);
	void	onUnitHide(BWAPI::Unit unit);
	void	onUnitRenegade(BWAPI::Unit unit);
};

}