#include<string>
#include<vector>
#include<fstream>
#include "SneakLogger.h"
#include "Common.h"
#include "UnitUtil.h"
#include "Global.h"
#include "MapTools.h"
#include "rapidjson\document.h"
#include "JSONTools.h"
#include "Global.h"
#include "rapidjson/filewritestream.h"
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <cstdio>


using namespace UAlbertaBot;
using namespace rapidjson;


rapidjson::Document UAlbertaBot::SneakLogger::generateJsonObject(Game game)
{
	Document d;
	d.SetObject();

	auto& allocator = d.GetAllocator();

	Value val(kObjectType);

	val.SetString(game.m_strategy.c_str(), static_cast<SizeType>(game.m_strategy.length()), allocator);
	d.AddMember("Strategy", val, allocator);

	val.SetInt(game.m_unitslost);
	d.AddMember("UnitsLost", val, allocator);

	val.SetInt(game.m_shuttlehealth);
	d.AddMember("ShuttleHealth", val, allocator);

	val.SetDouble(game.m_beforesneak);
	d.AddMember("BeforeSneak", val, allocator);

	val.SetDouble(game.m_traveltime);
	d.AddMember("TravelTime", val, allocator);

	val.SetDouble(game.m_timespotted);
	d.AddMember("TimeSpotted", val, allocator);

	val.SetDouble(game.m_enemynearbasetime);
	d.AddMember("EnemyNearBase", val, allocator);

	val.SetInt(game.m_workersbuilt);
	d.AddMember("WorkersBuilt", val, allocator);

	val.SetInt(game.m_workerslost);
	d.AddMember("WorkersLost", val, allocator);

	val.SetBool(game.m_won);
	d.AddMember("Won", val, allocator);

	val.SetString(game.m_enemyrace.c_str(), static_cast<SizeType>(game.m_enemyrace.length()), allocator);
	d.AddMember("EnemyRace", val, allocator);

	val.SetString(game.m_map.c_str(), static_cast<SizeType>(game.m_map.length()), allocator);
	d.AddMember("MapPlayed", val, allocator);

	rapidjson::StringBuffer strbuf;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(strbuf);
	d.Accept(writer);

	return d;
}

// Make the game struct in the onstart function
// append to it on end


bool SneakLogger::appendToFile(rapidjson::Document doc)
{
		const std::string fileName = Config::Strategy::LoggingDir + Config::Strategy::StrategyName + ".txt";
		FILE* fp = fopen(fileName.c_str(), "rb+");
	try
	{
		std::fseek(fp, 0, SEEK_SET);
	}
	catch (const std::exception&)
	{
		return false;
	}
	
	if (getc(fp) != '[')
	{
		std::fclose(fp);
		return false;
	}

	bool isEmpty = false;
	if (getc(fp) == ']')
		isEmpty = true;

	std::fseek(fp, -1, SEEK_END);
	if (getc(fp) != ']')
	{
		std::fclose(fp);
		return false;
	}

	fseek(fp, -1, SEEK_END);
	if (!isEmpty)
		fputc(',', fp);

	char writeBuffer[265];


	FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
	PrettyWriter<FileWriteStream> writer(os);
	doc.Accept(writer);

	std::fputc(']', fp);
	fclose(fp);
	return true;
}

UAlbertaBot::SneakLogger::SneakLogger()
{
}

void UAlbertaBot::SneakLogger::onStart()
{
	m_game.m_strategy = Config::Strategy::StrategyName;
	m_game.m_enemyrace = BWAPI::Broodwar->enemy()->getRace().getName();
	m_game.m_map = BWAPI::Broodwar->mapFileName();
	startingPosition = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
}

void UAlbertaBot::SneakLogger::onFrame(bool full, bool completed, int health, BWAPI::Position pos, int unitsLost)
{
	float time = (float)BWAPI::Broodwar->elapsedTime() * 0.625;

	if (!(Config::Strategy::StrategyName == "Protoss_Drop" || Config::Strategy::StrategyName == "Protoss_DirectDrop")) return;

	if (full && Global::Sneak().m_game.m_beforesneak == 0.0) {
		m_game.m_beforesneak = time;
	}

	if (completed && Global::Sneak().m_game.m_traveltime == 0.0) {
		m_game.m_traveltime = (time - m_game.m_beforesneak);
		m_game.m_shuttlehealth = health;
	}

	if (Global::Map().m_influenceMap.getVisionInfluence(pos.x / 32, pos.y / 32) > 0.0 && m_game.m_timespotted == 0.0) {
		m_game.m_timespotted = time;
	}
	
	if (unitsLost != 0) {
		m_game.m_unitslost = unitsLost;
	}

	if (time > 400.0 && m_game.m_workerslost == 0) { //  right after usual drop time
		BWAPI::Unitset units = BWAPI::Broodwar->self()->getUnits();
		for (auto& unit: units) {
			if (unit->getType().isWorker()) {
				m_game.m_workerslost++;
			}
		}
	}
	


}

void UAlbertaBot::SneakLogger::onEnd(bool isWinner)
{
	m_game.m_won = isWinner;
	appendToFile(generateJsonObject(m_game));
	m_game = Game();
}

void UAlbertaBot::SneakLogger::onUnitCreate(BWAPI::Unit unit) {

	if (unit == nullptr) {
		return;
	}

	if (unit->getPlayer() == BWAPI::Broodwar->enemy()) return;
	
	if (unit->getType().isWorker()) {
		m_game.m_workersbuilt = m_game.m_workersbuilt + 1;
	}
}

void UAlbertaBot::SneakLogger::onUnitShow(BWAPI::Unit unit)
{
	if (unit == nullptr) {
		return;
	}

	if (unit->getPlayer() != BWAPI::Broodwar->enemy() || (unit->getType().isWorker())) return;

	if ((unit->getPosition().getApproxDistance(startingPosition) < 1000) && m_game.m_enemynearbasetime == 0.0) {
		m_game.m_enemynearbasetime = (float)BWAPI::Broodwar->elapsedTime() * 0.625;
	}

}

void UAlbertaBot::SneakLogger::onUnitDestroy(BWAPI::Unit unit) {

	if (unit == nullptr) {
		return;
	}

	if (unit->getPlayer() == BWAPI::Broodwar->enemy()) return;

	if (unit->getType().isWorker() && m_game.m_workersbuilt > 0) {
		m_game.m_workerslost = m_game.m_workerslost + 1;
	}
}


