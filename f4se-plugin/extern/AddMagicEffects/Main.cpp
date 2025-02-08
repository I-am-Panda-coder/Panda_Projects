#pragma once
#include "F4SE/F4SE.h"
#include "RE/Fallout.h"
#include <filesystem>
#include <optional>
//#include <format>
#include <shlobj.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stack>
#include <string>
#include <vector>
#include <windows.h>

//#include <PCH.h>
#include <AddMagicEffects/Version.h>

#define DLLEXPORT __declspec(dllexport)

namespace logger = F4SE::log;
using namespace std::literals;

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info)
{
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

	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = std::string(ver::NAME.begin(), ver::NAME.end()).c_str();
	a_info->version = ver::MAJOR;

	if (a_f4se->IsEditor()) {
		logger::info("loaded in editor");
		return false;
	}

	const auto ver = a_f4se->RuntimeVersion();
	if (ver != F4SE::RUNTIME_1_10_162 && ver != F4SE::RUNTIME_1_10_163) {
		logger::info("unsupported runtime v{}", ver.string());
		return false;
	}

	return true;
}

constexpr uint32_t formid_spell_AbLegendaryCreatureItem = 0;

auto get_spells()
{
	std::vector<RE::TESForm*> forms {};
	forms.push_back(RE::TESForm::GetFormByEditorID("ChangeAnimArchetypeSpell"));
	forms.push_back(RE::TESForm::GetFormByEditorID("ChangeAnimArchetypeSpell2"));
	forms.push_back(RE::TESForm::GetFormByEditorID("ChangeAnimArchetypeSpell3"));
	forms.push_back(RE::TESForm::GetFormByEditorID("ChangeAnimArchetypeSpell4"));
	forms.push_back(RE::TESForm::GetFormByEditorID("ChangeAnimGFSpell5"));
	forms.push_back(RE::TESForm::GetFormByEditorID("ChangeAnimGFSpell6"));
	return forms;
}

void patch_magic_effect()
{
	auto get_effect_form_id = [](RE::EffectSetting* item) {
		return item->formID;
	};

	auto contains = [get_effect_form_id](const RE::BSTArray<RE::EffectItem*>& listOfEffects, RE::EffectSetting* item) {
		return std::any_of(listOfEffects.begin(), listOfEffects.end(), [get_effect_form_id, item](const auto& e) {
			return get_effect_form_id(e->effectSetting) == get_effect_form_id(item);
		});
	};

	RE::SpellItem* spell = static_cast<RE::SpellItem*>(RE::TESDataHandler::GetSingleton()->LookupForm(formid_spell_AbLegendaryCreatureItem, "Fallout4.esm"));  // kSPEL
	
	auto forms = get_effects();
	for (auto& i : forms) {
		//RE::EffectSetting* mgef = static_cast<RE::EffectSetting*>(RE::TESDataHandler::GetSingleton()->LookupForm(formid_mgef_diverse, plugin_name));  // kMGEF
		RE::EffectSetting* mgef = static_cast<RE::EffectSetting*>(i);  // kMGEF
		if (!mgef || !spell) {
			return;
		}

		if (!contains(spell->listOfEffects, mgef)) {
			auto newEffectItem = new RE::EffectItem();
			newEffectItem->effectSetting = mgef;
			spell->listOfEffects.push_back(newEffectItem);
		}
	}
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se);

	return true;
}

