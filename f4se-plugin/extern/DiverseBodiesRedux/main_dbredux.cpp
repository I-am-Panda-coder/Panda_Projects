#pragma once
#include "F4SE/F4SE.h"
#include "RE/Fallout.h"
#include <IniParser/Ini.h>
#include <filesystem>
#include <optional>
//#include <format>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>
#include <vector>
#include <stack>
#include <map>
#include <windows.h>
#include <shlobj.h>

#include <DiverseBodiesRedux/Globals/Globals.h>

//#include <PCH.h>
#include <DiverseBodiesRedux/Version.h>
#include <DiverseBodiesRedux/Manager/Manager.h>
#include <DiverseBodiesRedux/Papyrus/PapyrusBind.hpp>


#define DLLEXPORT __declspec(dllexport)

namespace logger = F4SE::log;
using namespace std::literals;

ini::map inimap(std::filesystem::current_path().string() + "\\Data\\DiverseBodiesRedux\\dbr.ini"s);

void InitAfterGameDataWasLoaded();
void InitAfterGameWasStarted();
void InitForms();

void MessageHandler(F4SE::MessagingInterface::Message* a_msg)
{
	if (!a_msg) {
		return;
	}
	switch (a_msg->type) {
	case F4SE::MessagingInterface::kGameDataReady:
		{
			InitAfterGameDataWasLoaded();
			break;
		}
	case F4SE::MessagingInterface::kPreLoadGame:
		{
			dbr_manager::ActorsManager::deserialized = false;
			//TESObjectREFR_Events::RegisterForEvent<RE::TESObjectLoadedEvent, 0x00442EB0>(dbr_manager::ActorsManager::getInstance());
			TESObjectREFR_Events::RegisterForObjectLoaded(dbr_manager::ActorsManager::getInstance());
			//dbr_manager::ActorsManager::getInstance()->deserialize();
			break;
		}
	case F4SE::MessagingInterface::kPreSaveGame:
		{
			//dbr_manager::ActorsManager::getInstance()->serialize();
			break;
		}
	/*case F4SE::MessagingInterface::kPostLoadGame:
		{
			global::add_potion();
			break;
		}*/
	}
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info)
{
	LOG.clearLog();

	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= std::format("{}.log", ver::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
	log->set_level(spdlog::level::trace);
	log->flush_on(spdlog::level::trace);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%m/%d/%Y - %T] [%^%l%$] %v"s);

	logger::info("{} v{}", ver::PROJECT, ver::VER.data());

	LOG("{}", std::string(ver::NAME.begin(), ver::NAME.end()));
	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = std::string(ver::NAME.begin(), ver::NAME.end()).c_str();
	a_info->version = ver::MAJOR;

	if (a_f4se->IsEditor()) {
		LOG("loaded in editor");
		return false;
	}

	const auto ver = a_f4se->RuntimeVersion();
	if (ver != F4SE::RUNTIME_1_10_162 && ver != F4SE::RUNTIME_1_10_163) {
		LOG("unsupported runtime v{}", ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se);
	
	const auto serialization = F4SE::GetSerializationInterface();
	if (!serialization) {
		logger::critical("Failed to get F4SE serialization interface, marking as incompatible.");
		return false;
	} else {
		serialization->SetUniqueID(ver::UID);
		//serialization->SetRevertCallback();
		serialization->SetSaveCallback(dbr_manager::ActorsManager::Serialize);
		serialization->SetLoadCallback(dbr_manager::ActorsManager::Deserialize);
		logger::info("Registered with F4SE serialization interface.");
	}

	const auto messaging = F4SE::GetMessagingInterface();
	if (!messaging || !messaging->RegisterListener(MessageHandler)) {
		LOG("Failed to get F4SE messaging interface, marking as incompatible.");
		return false;
	} else {
		LOG("Registered with F4SE messaging interface.");
		LOG("Starting...");
		InitAfterGameWasStarted();
	}

	const auto papyrus = F4SE::GetPapyrusInterface();
	
	if (!papyrus || !papyrus->Register(papyrus::RegisterFunctions)) {
		logger::critical("Failed to register Papyrus functions!");
	} else {
		logger::info("Registered Papyrus functions.");
	}

	return true;
}

void InitAfterGameWasStarted()
{
	bodymorphs::Parse();
}

void InitAfterGameDataWasLoaded()
{
	game_data_loaded_state = true;
	InitForms();
	overlays::Parse();
	skins::Parse();
	hairs::Parse();
	SetupDetours(LooksMenuInterfaces<ActorUpdateManager>::GetInterface());
}


//void patch_magic_effect()
//{
//	auto get_effect_form_id = [](RE::EffectSetting* item) {
//		return item->formID;
//	};
//
//	auto contains = [get_effect_form_id](const RE::BSTArray<RE::EffectItem*>& listOfEffects, RE::EffectSetting* item) {
//		return std::any_of(listOfEffects.begin(), listOfEffects.end(), [get_effect_form_id, item](const auto& e) {
//			return get_effect_form_id(e->effectSetting) == get_effect_form_id(item);
//		});
//	};
//
//	RE::SpellItem* spell = static_cast<RE::SpellItem*>(RE::TESDataHandler::GetSingleton()->LookupForm(formid_spell_AbLegendaryCreatureItem, "Fallout4.esm"));  // kSPEL
//	RE::EffectSetting* mgef = static_cast<RE::EffectSetting*>(RE::TESDataHandler::GetSingleton()->LookupForm(formid_mgef_diverse, plugin_name));               // kMGEF
//
//	if (!mgef || !spell) {
//		return;
//	}
//
//	if (!contains(spell->listOfEffects, mgef)) {
//		auto newEffectItem = new RE::EffectItem();
//		newEffectItem->effectSetting = mgef;
//		spell->listOfEffects.push_back(newEffectItem);
//	}
//}

