#pragma once
#include "F4SE/F4SE.h"
#include "RE/Fallout.h"

#include <algorithm>
#include <memory>
#include <shlobj.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <windows.h>

#include "version.h"

#define DLLEXPORT __declspec(dllexport)

namespace logger = F4SE::log;
using namespace std::literals;

namespace NodeUtils
{
	// Инициализация
	void Initialize();
	void OnGameDataLoaded();

	// Работа с нодами
	RE::NiAVObject* FindNode(RE::TESObjectREFR* refr, const RE::BSFixedString& nodeName);
	void SetNodeScale(RE::NiNode* node, float scale, bool recursive = false);
	void SetNodeScaleIfContains(RE::NiNode* node, float scale, const RE::BSFixedString& searchFilter = "", bool recursive = false);
	float GetNodeScale(RE::NiNode* node);

	// Papyrus функции
	bool SetNodeWithChildren(RE::TESObjectREFR* reference, const RE::BSFixedString& parentNode, float scaleValue, const RE::BSFixedString& filterString);
	bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* vm);

	// Внутренние методы
	namespace Internal
	{
		void SetupLogger();
		bool ContainsStringCI(std::string_view str, std::string_view substr);
		void HandleF4SEMessages(F4SE::MessagingInterface::Message* msg);
	}
}

// Реализация ======================================================

void NodeUtils::Initialize()
{
	Internal::SetupLogger();
	logger::info("Initializing NodeUtils v{}", ver::VERSTRING);
}

void NodeUtils::OnGameDataLoaded()
{
	if (GetModuleHandleA("F4NiNodeManager.dll")) {
		logger::warn("F4NiNodeManager already loaded - skipping initialization");
		return;
	}
	logger::info("Game data loaded - ready for operations");
}

// Поиск и управление нодами ======================================

RE::NiAVObject* NodeUtils::FindNode(RE::TESObjectREFR* ref, const RE::BSFixedString& nodeName)
{
	if (!ref || nodeName.empty()) {
		logger::warn("Invalid reference or node name");
		return nullptr;
	}

	if (auto niObject = ref->Get3D()) {
		return niObject->GetObjectByName(nodeName);
	}

	logger::warn("Reference has no 3D data");
	return nullptr;
}

void NodeUtils::SetNodeScale(RE::NiNode* node, float scale, bool recursive)
{
	if (!node)
		return;

	node->local.scale = scale;
	logger::debug("Set scale {} for node {}", scale, node->name.c_str());

	if (recursive) {
		for (auto& child : node->children) {
			if (child && child->IsNode()) {
				SetNodeScale(static_cast<RE::NiNode*>(child.get()), scale, true);
			}
		}
	}
}

void NodeUtils::SetNodeScaleIfContains(RE::NiNode* node, float scale, const RE::BSFixedString& filter, bool recursive)
{
	if (!node)
		return;

	bool shouldScale = filter.empty() ||
	                   (node->name.data() && Internal::ContainsStringCI(node->name.c_str(), filter.c_str()));

	if (shouldScale) {
		node->local.scale = scale;
		logger::debug("Set scale {} for node {} (filter: {})", scale, node->name.c_str(), filter.c_str());
	}

	if (recursive) {
		for (auto& child : node->children) {
			if (child && child->IsNode()) {
				SetNodeScaleIfContains(static_cast<RE::NiNode*>(child.get()), scale, filter, true);
			}
		}
	}
}

float NodeUtils::GetNodeScale(RE::NiNode* node)
{
	return node ? node->local.scale : -1.0f;
}

// Papyrus интеграция =============================================

bool NodeUtils::SetNodeWithChildren(RE::TESObjectREFR* ref, const RE::BSFixedString& nodeName, float scale, const RE::BSFixedString& filter)
{
	if (!ref) {
		logger::warn("Attempt to scale node on invalid reference");
		return false;
	}

	if (auto node = FindNode(ref, nodeName)) {
		if (auto niNode = dynamic_cast<RE::NiNode*>(node)) {
			SetNodeScaleIfContains(niNode, scale, filter, true);
			return true;
		}
	}

	logger::warn("Failed to find or scale node {} on reference {}", nodeName.c_str(), ref->GetDisplayFullName());
	return false;
}

#define PAPYRUS_BIND(funcName) a_VM->BindNativeMethod("Amputator", #funcName, papyrus::funcName, true)
#define PAPYRUS_BIND_LATENT(funcName, retType) a_VM->BindNativeMethod<retType>(ver::PROJECT, #funcName, papyrus::funcName, true, true)
bool NodeUtils::RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* a_VM)
{
	if (!a_VM)
		return false;

	PAPYRUS_BIND(SetNodeWithChildren);
	logger::info("Registered Papyrus functions");
	return true;
}

// Внутренние утилиты ============================================

namespace NodeUtils::Internal
{
	void SetupLogger()
	{
		auto path = logger::log_directory();
		if (!path)
			return;

		*path /= fmt::format("{}.log", ver::PROJECT);
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

		auto log = std::make_shared<spdlog::logger>("NodeUtils", std::move(sink));
		log->set_level(spdlog::level::debug);
		log->flush_on(spdlog::level::debug);

		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("[%H:%M:%S] [%^%l%$] %v");
	}

	bool ContainsStringCI(std::string_view str, std::string_view substr)
	{
		return std::search(str.begin(), str.end(),
				   substr.begin(), substr.end(),
				   [](char ch1, char ch2) {
					   return std::tolower(ch1) == std::tolower(ch2);
				   }) != str.end();
	}

	void HandleF4SEMessages(F4SE::MessagingInterface::Message* msg)
	{
		if (!msg)
			return;

		switch (msg->type) {
		case F4SE::MessagingInterface::kGameDataReady:
			OnGameDataLoaded();
			break;
		default:
			break;
		}
	}
}

// F4SE интерфейсы ===============================================

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* f4se, F4SE::PluginInfo* info)
{
	NodeUtils::Internal::SetupLogger();

	info->infoVersion = F4SE::PluginInfo::kVersion;
	info->name = ver::PROJECT.data();
	info->version = ver::MAJOR;

	if (f4se->IsEditor()) {
		logger::critical("Editor is not supported");
		return false;
	}

	const auto ver = f4se->RuntimeVersion();
	if (ver < F4SE::RUNTIME_1_10_162 || ver > F4SE::RUNTIME_1_10_163) {
		logger::critical("Unsupported runtime version {}", ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* f4se)
{
	F4SE::Init(f4se);
	NodeUtils::Initialize();

	const auto messaging = F4SE::GetMessagingInterface();
	if (!messaging || !messaging->RegisterListener(NodeUtils::Internal::HandleF4SEMessages)) {
		logger::critical("Failed to register F4SE message listener");
		return false;
	}

	return true;
}
