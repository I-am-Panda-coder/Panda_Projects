#pragma once
#include "F4SE/F4SE.h"
#include "RE/Fallout.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <windows.h>
#include <shlobj.h>
#include <memory>

#include <detourxs/DetourXS.h>
#include <DiverseBodiesRedux/Hooks/ProcessingSafe.h>
#include "version.h"

#define DLLEXPORT __declspec(dllexport)

namespace logger = F4SE::log;
using namespace std::literals;

void InitAfterGameDataWasLoaded();
void InitAfterGameWasStarted();
void InitForms();

using ChangeHeadPartHandler = void (*)(RE::TESNPC*, RE::BGSHeadPart*);
using ChangeHeadPartRemovePartHandler = void (*)(RE::TESNPC*, RE::BGSHeadPart*, bool);

DetourXS detourChangeHeadPartRemovePart;
DetourXS detourChangeHeadPart;

ChangeHeadPartRemovePartHandler g_OriginalChangeHeadPartRemovePart = nullptr;
ChangeHeadPartHandler g_OriginalChangeHeadPart = nullptr;

ProcessingNPC g_processingChangeHeadParts{};

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
	}
}

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
		logger::error("unsupported runtime v{}", ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se);

	const auto messaging = F4SE::GetMessagingInterface();
	if (!messaging || !messaging->RegisterListener(MessageHandler)) {
		logger::info("Failed to get F4SE messaging interface, marking as incompatible.");
		return false;
	} else {
		logger::info("Registered with F4SE messaging interface.");
		logger::info("Starting...");
		InitAfterGameWasStarted();
	}

	return true;
}

void InitAfterGameWasStarted()
{
}

void remove_with_extra(RE::TESNPC* npc, RE::BGSHeadPart* hpart)
{
	for (auto& extra : hpart->extraParts) {
		if (!extra->extraParts.empty())
			remove_with_extra(npc, extra);
		else
			g_OriginalChangeHeadPartRemovePart(npc, extra, false);
	}
	g_OriginalChangeHeadPartRemovePart(npc, hpart, false);
}

inline RE::TESNPC* get_leveled_TESNPC(RE::TESNPC* npc)
{
	if (!npc)
		return nullptr;

	auto& templateFlags = npc->actorData.templateUseFlags;
	if (((templateFlags & RE::TESActorBaseData::TemplateFlags::kFlagTraits) != 0) && npc->templateForms) {
		npc = RE::fallout_cast<RE::TESNPC*>(*npc->templateForms);
		npc = get_leveled_TESNPC(npc);
	}
	return npc;
}

void ProcessChangeHeadPart(RE::TESNPC* npc, RE::BGSHeadPart* hpart, bool bRemoveExtraParts, bool isRemove)
{
	if (!npc || !hpart)
		return;

	auto base_npc = get_leveled_TESNPC(npc);
	npc = base_npc ? base_npc : npc;

	if (g_processingChangeHeadParts.contains(npc->formID)) {
		std::thread([npc, hpart, bRemoveExtraParts, isRemove] {
			while (g_processingChangeHeadParts.contains(npc->formID)) {
				std::this_thread::sleep_for(std::chrono::microseconds(1));
			}
			ProcessChangeHeadPart(npc, hpart, bRemoveExtraParts, isRemove);
		}).detach();
		return;
	}

	g_processingChangeHeadParts.insert(npc->formID);

	if (isRemove) {
		if (bRemoveExtraParts)
			remove_with_extra(npc, hpart);
		else
			g_OriginalChangeHeadPartRemovePart(npc, hpart, false);

	} else {
		g_OriginalChangeHeadPart(npc, hpart);
	}

	g_processingChangeHeadParts.erase(npc->formID);
}

void HookedChangeHeadPartRemovePart(RE::TESNPC* npc, RE::BGSHeadPart* hpart, bool bRemoveExtraParts)
{
	ProcessChangeHeadPart(npc, hpart, bRemoveExtraParts, true);
}

void HookedChangeHeadPart(RE::TESNPC* npc, RE::BGSHeadPart* hpart)
{
	ProcessChangeHeadPart(npc, hpart, false, false);
}

void InitAfterGameDataWasLoaded()
{
	if (GetModuleHandleA("DiverseBodies.dll") != NULL) 
		return;
	
	if (!detourChangeHeadPartRemovePart.Create(reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL)) + 5985632), reinterpret_cast<LPVOID>(HookedChangeHeadPartRemovePart))) {
		logger::error("Failed to create detour for ChangeHeadPartRemovePart");
	} else {
		g_OriginalChangeHeadPartRemovePart = reinterpret_cast<ChangeHeadPartRemovePartHandler>(detourChangeHeadPartRemovePart.GetTrampoline());
		logger::info("Detour for ChangeHeadPartRemovePart created successfully.");
	}
	
	if (!detourChangeHeadPart.Create(reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL)) + 6003504), reinterpret_cast<LPVOID>(HookedChangeHeadPart))) {
		logger::error("Failed to create detour for ChangeHeadPart");
	} else {
		g_OriginalChangeHeadPart = reinterpret_cast<ChangeHeadPartHandler>(detourChangeHeadPart.GetTrampoline());
		logger::info("Detour for ChangeHeadPart created successfully.");
	}
}
