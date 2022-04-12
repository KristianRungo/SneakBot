#include<string>
#include<vector>
#include<fstream>
#include "SneakLogger.h"
#include "Common.h"
#include "StrategyManager.h"
#include "UnitUtil.h"
#include "BaseLocationManager.h"
#include "Global.h"
#include "InformationManager.h"
#include "WorkerManager.h"
#include "Logger.h"
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
	d.AddMember("description", val, allocator);

	val.SetInt(game.m_unitslost);
	d.AddMember("UnitsLost", val, allocator);

	val.SetInt(game.m_shuttlehealth);
	d.AddMember("ShuttleHealth", val, allocator);

	val.SetDouble(game.m_traveltime);
	d.AddMember("TravelTime", val, allocator);

	val.SetInt(game.m_timespotted);
	d.AddMember("TimeSpotted", val, allocator);

	val.SetBool(game.m_won);
	d.AddMember("Won", val, allocator);

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

	std::fseek(fp, 0, SEEK_SET);
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
	Writer<FileWriteStream> writer(os);
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
}

void UAlbertaBot::SneakLogger::onFrame()
{
}

void UAlbertaBot::SneakLogger::onEnd()
{
	appendToFile(generateJsonObject(m_game));
}




