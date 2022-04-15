#include<string>
#include<vector>
#include<fstream>
#include "SneakLogger.h"
#include "Common.h"
#include "UnitUtil.h"
#include "BaseLocationManager.h"
#include "Global.h"
#include "InformationManager.h"
#include "WorkerManager.h"
#include "TimerManager.h"
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

	val.SetInt(game.m_timespotted);
	d.AddMember("TimeSpotted", val, allocator);

	val.SetBool(game.m_won);
	d.AddMember("Won", val, allocator);

	val.SetString(game.m_enemyrace.c_str(), static_cast<SizeType>(game.m_enemyrace.length()), allocator);
	d.AddMember("Enemy Race", val, allocator);

	val.SetString(game.m_map.c_str(), static_cast<SizeType>(game.m_map.length()), allocator);
	d.AddMember("Map Played", val, allocator);

	rapidjson::StringBuffer strbuf;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(strbuf);
	d.Accept(writer);

	return d;
}

// Make the game struct in the onstart function
// append to it on end


bool UAlbertaBot::SneakLogger::appendToFile(rapidjson::Document doc)
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
	m_game.m_map = BWAPI::Broodwar->mapFileName();
}

void UAlbertaBot::SneakLogger::onFrame(bool full, bool completed, int health)
{
	BWAPI::Broodwar->getFrameCount();

	if (Config::Strategy::StrategyName != "Protoss_Drop") return;


	if (full && Global::Sneak().m_game.m_beforesneak == 0.0) {
		dropShipFullFrame = BWAPI::Broodwar->getFrameCount();
		Global::Sneak().m_game.m_beforesneak = dropShipFullFrame / 24;
	}

	if (completed && Global::Sneak().m_game.m_traveltime == 0.0) {
		dropCompletedFrame = BWAPI::Broodwar->getFrameCount();
		Global::Sneak().m_game.m_traveltime = (dropCompletedFrame - dropShipFullFrame) / 24;
		m_game.m_shuttlehealth = health;
	}
}

void UAlbertaBot::SneakLogger::onEnd(bool isWinner)
{
	m_game.m_won = isWinner;

	std::string enemyName = BWAPI::Broodwar->enemy()->getRace().getName();
	m_game.m_enemyrace = enemyName;
	appendToFile(generateJsonObject(m_game));
}





