bool g_gameDataReady = false;
#pragma once
#include <string>
#include "Data/Constants.h"
#include "Misc/Utility.h"
#include "Data/Events.h"
#include "RE/ExtraREClasses.h"
#include "Serialization/General.h"
#include "Tasks/TaskFunctor.h"
#include "PackageOverride/EvalHook.h"
#include "LooksMenu/LooksMenu.h"
#include "FaceAnimation/AnimationData.h"
#include "Data/Global.h"
#include "Data/CommandEngine.h"
#include "BodyAnimation/GraphHook.h"
#include "BodyAnimation/SmartIdle.h"
#include "CamHook/CamHook.h"
#include "Misc/GameUtil.h"
#include "Data/Uid.h"
#include "Menu/NAFHUDMenu/SceneHUD.h"
#include "Scene/IScene.h"
#include "Scene/SceneManager.h"
#include "Scene/SceneBase.h"
#include "FaceAnimation/FaceUpdateHook.h"
#include "Tasks/GameLoopHook.h"
#include "Menu/Menu.h"
#include "Scripts/Papyrus.h"
#include "Serialization/Serialization.h"
#include "API/API.h"
//Bridge
#include "Bridge/Bridge.h"
#include "Bridge/Papyrus/Papyrus.h"
#include <filesystem>
#include "Bridge/IniParser/Ini.hpp"

RE::BSScript::IVirtualMachine* g_VM;

namespace
{
	void MessageHandler(F4SE::MessagingInterface::Message* a_msg)
	{
		if (!a_msg) {
			return;
		}

		switch (a_msg->type) {
			case F4SE::MessagingInterface::kGameDataReady:
			{
				if (static_cast<bool>(a_msg->data)) {
					if (GetModuleHandleA("NAFBridge.dll")) {
						RE::MessageMenuManager::GetSingleton()->Create("NAF BRIDGE WARNING!",
							"It seems you've been updated from older Bridge's version, but didn't delete NAFBridge.dll. You should delete it before you can proceed. It is in GameFolder/Data/F4SE/Plugins/NAFBridge.dll. \nIf you're using MO2 it will be placed in MO2/Mods/Bridge-mod-folder/F4SE/Plugins/NAFBridge.dll", nullptr, RE::WARNING_TYPES::kSystem);
					}
					
					logger::info("Game data finished loading, registering NAF menu...");

					auto ui = RE::UI::GetSingleton();
					if (ui) {
						ui->RegisterSink(Tasks::TimerThread::GetSingleton());
						ui->RegisterSink(Menu::HUDManager::GetSingleton());
						Menu::Register(ui);
					}
					
					logger::info("Linking references...");
					Utility::StartPerformanceCounter();

					Data::Global::InitGameData();

					double performanceSeconds = Utility::GetPerformanceCounter();
					logger::info("Finished linking in {:.3f}s", performanceSeconds);
					logger::info("Ready!");

					g_gameDataReady = true;

					Data::Events::Send(Data::Events::GAME_DATA_READY);

					std::string file("");  
					if (std::filesystem::exists(MCM_INI_PATH))
						file = MCM_INI_PATH;

					if (!file.empty()) {
						ini::map map(file);

						if (map.get<bool>("bdebugAnimations"s, "Debug"s))
						{
							[&]() {
								for (auto ref : Data::Global::Animations) {
									auto anim = ref.second.second.get();
									std::string res("\n");

									res += "ANIMATION id : ", res += anim->id;
									res += "\n\tloadPriority : ", res += std::to_string(anim->loadPriority);
									res += "\n\tactors count : ", res += std::to_string(anim->slots.size());
									res += "\n\ttags : ";
									for (auto& tag : anim->tags) {
										res += "\n\t\t"s + tag.data();
									}

									logger::info("{}\n", res);
								}
							}();
						}

						if (map.get<bool>("bdebugPositions"s, "Debug"s)) {
							auto getPosTypeString = [](size_t type) {
								switch (type) {
								case 0u:
									return "kAnimation"s;
								case 1u:
									return "kAnimationGroup"s;
								case 2u:
									return "kPositionTree"s;
								default:
									std::string unknown("unknown type ");
									unknown += type;
									return unknown;
								}
							};

							[&]() {
								for (auto ref : Data::Global::Positions) {
									auto pos = ref.second.second.get();
									std::string res("\n");

									res += "POSITION id : ", res += pos->id;
									res += "\n\tBaseAnimation : ", res += pos->GetBaseAnimation().get()->id;
									res += "\n\tHidden : ", res += pos->hidden ? "true" : "false";
									res += "\n\tloadPriority : ", res += std::to_string(pos->loadPriority);
									res += "\n\ttype : ", res += getPosTypeString(pos->posType);
									res += "\n\tstartEquipSet : ", pos->startEquipSet;
									res += "\n\tstopEquipSet : ", res += pos->stopEquipSet;
									res += "\n\tstartMorphSet : ", res += pos->startMorphSet;
									res += "\n\toffset : ", res += Scene::offset_to_string(pos->offset);

									res += "\nTags : ";
									for (auto t : pos->tags) {
										res += "\n\t", res += t;
									}

									res += "Locations : ";
									for (auto loc : pos->locations) {
										res += "\n\t", res += loc;
									}

									logger::info("{}\n", res);
								}
							}();
						}

						if (map.get<bool>("bdebugFaceAnims"s, "Debug"s)) {
							[&]() {
								for (auto& ref : Data::Global::FaceAnims) {
									auto fanim = ref.second.second.get();

									std::string res("\n");
									res += "FACE ANIM id : ", res += fanim->id;
									res += "\n\tloadPriority : ", res += std::to_string(fanim->loadPriority);

									logger::info("{}\n", res);
								}
							}();
						}

						if (map.get<bool>("bdebugMorphSets"s, "Debug"s)) {
							[&]() {
								for (auto ref : Data::Global::MorphSets) {
									auto m = ref.second.second.get();
									std::string res("\n");

									res += "MORPH SET id : ", res += m->id;
									res += "\n\tloadPriority : ", res += std::to_string(m->loadPriority);
									res += "\n\tmorphs : ";
									for (auto& mrph : m->morphs) {
										res += "\n\t\tname : "s + mrph.second.data()->name + "\tvalue : "s + std::to_string(mrph.second.data()->value);
									}

									logger::info("{}\n", res);
								}
							}();
						}

						if (map.get<bool>("bdebugEquipmentSets"s, "Debug"s)) {
							[&]() {
								for (auto ref : Data::Global::EquipmentSets) {
									auto m = ref.second.second.get();
									std::string res("\n");

									res += "EQUIPMENT SET id : ", res += m->id;
									res += "\n\tloadPriority : ", res += std::to_string(m->loadPriority);
									/*res += "\n\tbipedSlots : ";
							for (auto& es : m->bipedSlotNames) {
								res += "\n\t\t["s + std::to_string(es.second) + "]"s);
							}
							res += "\n\tequipmentData : ";
							for (auto& ed : m->datas) {
								res += "\n\t\t["s + std::to_string(ed.first.) + "]"s);
							}*/

									logger::info("{}\n", res);
								}
							}();
						}

						if (map.get<bool>("bdebugActions"s, "Debug"s)) {
							[&]() {
								for (auto ref : Data::Global::Actions) {
									auto m = ref.second.second.get();
									std::string res("\n");

									res += "ACTION id : ", res += m->id;
									res += "\n\tloadPriority : ", res += std::to_string(m->loadPriority);

									logger::info("{}\n", res);
								}
							}();
						}

						if (map.get<bool>("bdebugAnimationGroups"s, "Debug"s)) {
							[&]() {
								for (auto ref : Data::Global::AnimationGroups) {
									auto m = ref.second.second.get();
									std::string res("\n");

									res += "ANIMATION GROUP id : ", res += m->id;
									res += "\n\tloadPriority : ", res += std::to_string(m->loadPriority);

									logger::info("{}\n", res);
								}
							}();
						}

						if (map.get<bool>("bdebugPositionTrees"s, "Debug"s)) {
							[&]() {
								for (auto ref : Data::Global::PositionTrees) {
									auto m = ref.second.second.get();
									std::string res("\n");

									res += "POSITION TREE id : ", res += m->id;
									res += "\n\tloadPriority : ", res += std::to_string(m->loadPriority);

									logger::info("{}\n", res);
								}
							}();
						}

						if (map.get<bool>("bdebugRaces"s, "Debug"s)) {
							[&]() {
								for (auto ref : Data::Global::Races) {
									auto m = ref.second.second.get();
									std::string res("\n");

									res += "RACE id : ", res += m->id;
									res += "\n\tloadPriority : ", res += std::to_string(m->loadPriority);
									res += "\n\tbaseForm : ", res += m->baseForm.get()->formEditorID;
									if (m->startEvent.has_value())
										res += "\n\tstartEvent : ", res += m->startEvent.value();
									if (m->stopEvent.has_value())
										res += "\n\tstopEvent : ", res += m->stopEvent.value();
									if (m->graph.has_value())
										res += "\n\tgraph : ", res += m->graph.value();
									res += "\n\trequiresReset : ", res += m->requiresReset ? "true" : "false";
									res += "\n\trequiresForceLoop : ", res += m->requiresForceLoop ? "true" : "false";

									logger::info("{}\n", res);
								}
							}();
						}

						if (map.get<bool>("bdebugGraphInfos"s, "Debug"s)) {
							[&]() {
								for (auto ref : Data::Global::GraphInfos) {
									auto m = ref.second.second.get();
									std::string res("\n");

									res += "GRAPH INFO id : ", res += m->id;
									res += "\n\tloadPriority : ", res += std::to_string(m->loadPriority);

									logger::info("{}\n", res);
								}
							}();
						}

						if (map.get<bool>("bdebugOverlays"s, "Debug"s)) {
							[&]() {
								for (auto ref : Data::Global::Overlays) {
									auto set = ref.second.second.get();
									std::string res("\n");

									res += "OVERLAY id : ", res += set->id;
									res += "\n\tloadPriority : ", res += std::to_string(set->loadPriority);
									res += "\n\tfileName : ", res += set->fileName;
									res += "\n\tduration : ", res += std::to_string(set->duration);
									res += "\n\tquantity : ", res += std::to_string(set->quantity);

									res += "\noverlays : ";
									for (auto o : set->overlays) {
										res += "\n\ttemplate : ", res += o.Template;
										res += "\talpha : ", res += std::to_string(o.alpha);
										res += "\tisFemale : ", res += o.isFemale ? "true" : "false";
									}

									logger::info("{}\n", res);
								}
							}();
						}

						if (map.get<bool>("bdebugProtectedKeywords"s, "Debug"s)) {
							[&]() {
								std::string res("\n");
								res += "PROTECTED KEYWORD : ";
								auto formlist = RE::TESDataHandler::GetSingleton()->LookupForm<RE::BGSListForm>(0x31752, "AAF.esm"s);
								for (auto& ProtectedKwds : formlist->arrayOfForms) {
									res += "\n\tid : 0x", res += std::format("{:x}", ProtectedKwds->formID), res += "\t", res += ProtectedKwds->GetFormEditorID();
								}
								logger::info("{}\n", res);
							}();
						}

						if (map.get<bool>("bdebugFurnitures"s, "Debug"s)) {
							[&]() {
								std::string res("\n");
								auto formlist = RE::TESDataHandler::GetSingleton()->LookupForm<RE::BGSListForm>(0x2E7D, "AAF.esm"s);
								if (formlist == nullptr)
									return;
								formlist->ClearData();

								for (auto& el : Data::Global::Furnitures) {
									auto furniture = el.second.second;
									for (auto& form : furniture.get()->forms) {
										formlist->arrayOfForms.push_back(form.get());
									}
								}

								res += "PARSED FURNITURE : ";
								for (auto form : RE::TESDataHandler::GetSingleton()->LookupForm<RE::BGSListForm>(0x2E7D, "AAF.esm"s)->arrayOfForms) {
									if (form)
										res += "\n\tid : 0x", res += std::format("{:x}", form->formID), res += "\t", res += form->GetFormEditorID();
								}
								logger::info("{}\n", res);
							}();
						}
					}
				}

				break;
			}
			case F4SE::MessagingInterface::kPostLoad:
			{
				LooksMenu::Init();
			}
			default:
				break;
		}
	}

	void InitializeHooking() {
		auto& trampoline = F4SE::GetTrampoline();
		trampoline.create(64);
		Tasks::GameLoopHook::RegisterHook(trampoline);
		BodyAnimation::GraphHook::RegisterHook();
		FaceAnimation::FaceUpdateHook::RegisterHook(trampoline);
		Menu::HUDManager::RegisterHook(trampoline);
		PackageOverride::EvalHook::RegisterHook();
		CamHook::RegisterHook();
	}
}

void ReadIni();

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info)
{
	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::PATCH;

	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= std::format("{}.log", Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
	log->set_level(spdlog::level::trace);
	log->flush_on(spdlog::level::trace);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%m/%d/%Y - %T] [%^%l%$] %v"s);

	logger::info("{} v{}", Version::PROJECT, Version::NAME);

	if (a_f4se->IsEditor()) {
		return false;
	}

	const auto ver = a_f4se->RuntimeVersion();
	if (ver < F4SE::RUNTIME_1_10_162) {
		logger::critical("{} does not support runtime v{}", Version::PROJECT, ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se);

	ReadIni();

	InitializeHooking();

	const auto messaging = F4SE::GetMessagingInterface();
	if (!messaging || !messaging->RegisterListener(MessageHandler)) {
		logger::critical("Failed to get F4SE messaging interface, marking as incompatible.");
		return false;
	} else {
		logger::info("Registered with F4SE messaging interface.");
	}

	const auto serialization = F4SE::GetSerializationInterface();
	if (!serialization) {
		logger::critical("Failed to get F4SE serialization interface, marking as incompatible.");
		return false;
	} else {
		serialization->SetUniqueID('NAF');
		serialization->SetSaveCallback(Serialization::SaveCallback);
		serialization->SetLoadCallback(Serialization::LoadCallback);
		serialization->SetRevertCallback(Serialization::RevertCallback);
		logger::info("Registered with F4SE serialization interface.");
	}

	const auto papyrus = F4SE::GetPapyrusInterface();
	if (!papyrus || !papyrus->Register(Papyrus::RegisterFunctions) || !papyrus->Register(Papyrus::RegisterBridgeFunctions)) {
		logger::critical("Failed to register Papyrus functions!");
	} else {
		logger::info("Registered Papyrus functions.");
	}

	Data::Global::Init();

	bridge::InitializeBridge();

	//logger::info("{:s} initialization successful, waiting for game data load.", Version::PROJECT);
	logger::info("{:s} initialization successful, waiting for game data load.", PLUGINVERSTR);

	return true;
}

void ReadIni()
{
	auto copyFile = [](const char* SRC, const char* DEST) {
		std::ifstream src(SRC, std::ios::binary);
		std::ofstream dest(DEST, std::ios::binary);
		dest << src.rdbuf();
		return src && dest;
	};

	auto GetDefaultSettings = []() {
		std::string s("[Settings]\n");
		s += "fdefaultDuration=60.000000\n"s;
		s += "idefaultFurniturePrefence=10\n"s;
		s += "fdefaultFurnitureScanRadius=3000.0\n"s;
		s += "bdefaultIgnoreCombat=0\n"s;
		s += "bdefaultSkipWalk=0\n"s;
		s += "bdefaultSwapFemaleActorInArray=1\n"s;
		s += "bdefaultHideHud=1\n"s;
		s += "bdefaultSlowDrying=1\n"s;
		s += "sdefaultExcludeTags=pose,utility\n"s;
		s += "\n"s;
		s += "[Overrides]\n"s;
		s += "foverridesDuration=-1.000000\n"s;
		s += "ioverridesFurniturePrefence=-1\n"s;
		s += "foverridesFurnitureScanRadius=-500.000000\n"s;
		s += "ioverridesIgnoreCombat=-1\n"s;
		s += "ioverridesSkipWalk=-1\n"s;
		s += "boverridesEmptyInclTags=0\n"s;
		s += "\n"s;
		s += "[Debug]\n"s;
		s += "bdebugMessages=0\n"s;
		s += "fdebugSlowScriptTime=0.000000\n"s;
		s += "bdebugAnimations=0\n"s;
		s += "bdebugPositions=0\n"s;
		s += "bdebugFaceAnims=0\n"s;
		s += "bdebugMorphSets=0\n"s;
		s += "bdebugEquipmentSets=0\n"s;
		s += "bdebugActions=0\n"s;
		s += "bdebugAnimationGroups=0\n"s;
		s += "bdebugPositionTrees=0\n"s;
		s += "bdebugRaces=0\n"s;
		s += "bdebugGraphInfos=0\n"s;
		s += "bdebugOverlays=0\n"s;
		s += "bdebugProtectedKeywords=0\n"s;
		s += "bdebugFurnitures=0\n"s;
		return s;
	};

	auto AllKeys = []() {
		std::vector<std::tuple<std::string, std::string, std::string>> m;
		m.push_back(std::tuple("60.000000"s, "fdefaultDuration"s, "Settings"s));
		m.push_back(std::tuple("10"s, "idefaultFurniturePrefence"s, "Settings"s));
		m.push_back(std::tuple("3000.0"s, "fdefaultFurnitureScanRadius"s, "Settings"s));
		m.push_back(std::tuple("0"s, "bdefaultIgnoreCombat"s, "Settings"s));
		m.push_back(std::tuple("0"s, "bdefaultSkipWalk"s, "Settings"s));
		m.push_back(std::tuple("1"s, "bdefaultSwapFemaleActorInArray"s, "Settings"s));
		m.push_back(std::tuple("1"s, "bdefaultHideHud"s, "Settings"s));
		m.push_back(std::tuple("1"s, "bdefaultSlowDrying"s, "Settings"s));
		m.push_back(std::tuple("pose,utility"s, "sdefaultExcludeTags"s, "Settings"s));

		m.push_back(std::tuple("-1.000000"s, "foverridesDuration"s, "Overrides"s));
		m.push_back(std::tuple("-1"s, "ioverridesFurniturePrefence"s, "Overrides"s));
		m.push_back(std::tuple("-500.000000"s, "foverridesFurnitureScanRadius"s, "Overrides"s));
		m.push_back(std::tuple("-1"s, "ioverridesIgnoreCombat"s, "Overrides"s));
		m.push_back(std::tuple("-1"s, "ioverridesSkipWalk"s, "Overrides"s));
		m.push_back(std::tuple("0"s, "boverridesEmptyInclTags"s, "Overrides"s));

		m.push_back(std::tuple("0"s, "bdebugMessages"s, "Debug"s));
		m.push_back(std::tuple("0"s, "fdebugSlowScriptTime"s, "Debug"s));
		m.push_back(std::tuple("0"s, "bdebugAnimations"s, "Debug"s));
		m.push_back(std::tuple("0"s, "bdebugPositions"s, "Debug"s));
		m.push_back(std::tuple("0"s, "bdebugFaceAnims"s, "Debug"s));
		m.push_back(std::tuple("0"s, "bdebugMorphSets"s, "Debug"s));
		m.push_back(std::tuple("0"s, "bdebugEquipmentSets"s, "Debug"s));
		m.push_back(std::tuple("0"s, "bdebugActions"s, "Debug"s));
		m.push_back(std::tuple("0"s, "bdebugAnimationGroups"s, "Debug"s));
		m.push_back(std::tuple("0"s, "bdebugPositionTrees"s, "Debug"s));
		m.push_back(std::tuple("0"s, "bdebugRaces"s, "Debug"s));
		m.push_back(std::tuple("0"s, "bdebugGraphInfos"s, "Debug"s));
		m.push_back(std::tuple("0"s, "bdebugOverlays"s, "Debug"s));
		m.push_back(std::tuple("0"s, "bdebugProtectedKeywords"s, "Debug"s));
		m.push_back(std::tuple("0"s, "bdebugFurnitures"s, "Debug"s));
		return m;
	};

	std::string file = [copyFile, GetDefaultSettings]() {
		if (std::filesystem::exists(MCM_INI_PATH)) {
			return std::string(MCM_INI_PATH);
		} else if (std::filesystem::exists(MCM_INI_PATH_ALT)) {
			std::ofstream file(MCM_INI_PATH_ALT);
			if (file.is_open()) {
				file << GetDefaultSettings();
				file.close();
				copyFile(MCM_INI_PATH_ALT, MCM_INI_PATH);
				return std::string(MCM_INI_PATH);
			}
		}
		return ""s;
	}();

	if (!file.empty()) {
		ini::map map(file);
		std::vector<std::tuple<std::string, std::string, std::string>> settings = AllKeys();
		for (auto& s : settings) {
			if (!map.contains(get<1>(s), get<2>(s)))
				map.set(get<0>(s), get<1>(s), get<2>(s));
		}
	}
}
