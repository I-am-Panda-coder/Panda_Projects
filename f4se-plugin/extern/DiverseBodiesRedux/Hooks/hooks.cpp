#include "hooks.h"
#include "DiverseBodiesRedux/Manager/ManagerActorPreset.h"

extern bool IsSerializeFinished();
extern bool IsInActorsMap(uint32_t actorId);
extern bool IsInActorsMap(RE::Actor* actor);
extern dbr_manager::ActorPreset* Find(RE::Actor*);
extern std::optional<bool> IsActorExcluded(RE::Actor* actor);

extern concurrency::concurrent_queue<std::pair<uint32_t, void*>> looksmenu_hooked_queue;

//ProcessingNPC g_processingReset{};
ProcessingNPC g_processingChangeHeadParts{};

TESObjectLoadedEventHandler g_OriginalReceiveEventObjectLoaded = nullptr;
TESInitScriptEventHandler g_OriginalReceiveEventInitScript = nullptr;
//Reset3DHandler g_OriginalReset3D = nullptr;
ChangeHeadPartRemovePartHandler g_OriginalChangeHeadPartRemovePart = nullptr;
ChangeHeadPartHandler g_OriginalChangeHeadPart = nullptr;
//Update3DModelHandler g_OriginalUpdate3DModel = nullptr;
DoUpdate3DModelHandler g_OriginalDoUpdate3DModel = nullptr;

DetourXS detourObjectLoaded;
DetourXS detourInitScript;
//DetourXS detourReset3D;
DetourXS detourChangeHeadPartRemovePart;
DetourXS detourChangeHeadPart;
//DetourXS detourUpdate3DModel;
DetourXS detourDoUpdate3DModel;

RE::BSEventNotifyControl HookedReceiveEventObjectLoaded(ActorUpdateManager* manager, RE::TESObjectLoadedEvent* evn, void* dispatcher)
{
	return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl HookedReceiveEventInitScript(ActorUpdateManager* manager, RE::TESInitScriptEvent* evn, void* dispatcher)
{
	return RE::BSEventNotifyControl::kContinue;  // Отключаем бодиген
}

std::mutex mapMutex;
std::condition_variable mapCV;

//void HookedReset3D(RE::Actor* actor, bool a_reloadAll, RE::RESET_3D_FLAGS a_additionalFlags, bool a_queueReset, RE::RESET_3D_FLAGS a_excludeFlags)
//{
//	if (!actor) {
//		logger::warn("HookedReset3D: Actor is null.");
//		return;
//	}
//
//	auto check_flags = [&]()->bool {
//		return (static_cast<uint16_t>(a_additionalFlags & f3D::kDiverseBodiesFlag) || static_cast<uint16_t>(a_additionalFlags) || (actor->currentProcess && 
//			(static_cast<uint16_t>(actor->currentProcess->GetAll3DUpdateFlags()) & ~static_cast<uint16_t>(a_excludeFlags))));
//	};
//
//	auto npc = get_leveled_TESNPC(actor->GetNPC());
//	if (!npc) {
//		logger::info("HookedReset3D: NPC is null for actor {:x}.", actor->formID);
//	}
//
//	if (extended_log) {
//		logger::info("Reset3D requested for actor {:x}, npc template {}, AIProcess state {}, args(reloadAll {}, addFlags {}, queueReset {}, excludeFlags {})",
//			actor->formID,
//			npc ? std::format("{:x}", npc->formID) : "NULL",
//			actor->currentProcess ? std::to_string(actor->currentProcess->GetAll3DUpdateFlags()) : "no process",
//			a_reloadAll ? "true" : "false",
//			static_cast<uint16_t>(a_additionalFlags),
//			a_queueReset ? "true" : "false",
//			static_cast<uint16_t>(a_excludeFlags));
//	}
//
//	/*if (npc && g_processingReset.contains(npc->formID)) {
//		std::thread([=] {
//			while (g_processingReset.contains(npc->formID))
//				std::this_thread::sleep_for(std::chrono::milliseconds(100));
//			HookedReset3D(actor, a_reloadAll, a_additionalFlags, a_queueReset, a_excludeFlags);
//		}).detach();
//		return;
//	}*/
//
//	if (check_flags())
//		if (auto preset = Find(actor); preset && !preset->empty()) {
//			if (!npc) {
//				logger::info("HookedReset3D: NPC is null for actor {:x}.", actor->formID);
//			} else if (npc) {
//				//g_processingReset.insert(npc->formID);
//				a_additionalFlags |= f3D::kDiverseBodiesFlag;
//
//				preset->apply(true, a_reloadAll, a_additionalFlags, a_queueReset, a_excludeFlags);
//				return;
//			}	
//		}
//
//	if (extended_log)
//		logger::info("{} : Reset 3d ({}, {}, {}, {}", std::format("{:x}", actor->formID),
//			a_reloadAll ? "true" : "false",
//			std::to_string(static_cast<uint16_t>(a_additionalFlags)),
//			a_queueReset ? "true" : "false",
//			std::to_string(static_cast<uint16_t>(a_excludeFlags)));
//
//	g_OriginalReset3D(actor, a_reloadAll, a_additionalFlags, a_queueReset, a_excludeFlags);
//}

//void HookedUpdate3DModel(RE::AIProcess* process, RE::Actor* actor, RE::TESObjectREFR* ref, bool force)
//{
//	logger::info("HookedUpdate3DModel {:x} {:x} {}", actor ? actor->formID : 0x0, ref ? ref->formID : 0x0, force ? "true" : "false");
//	g_OriginalUpdate3DModel(process, actor, ref, force);
//}

void HookedDoUpdate3DModel(RE::AIProcess* process, RE::Actor* actor, RE::RESET_3D_FLAGS flags)
{
	bool have_dbr_flag = bool(flags & f3D::kDiverseBodiesFlag);
	if (have_dbr_flag)
		flags &= static_cast<f3D>(~static_cast<uint16_t>(f3D::kDiverseBodiesFlag));

	if (flags != f3D::kNone)
		if (auto preset = Find(actor); preset) {
			constexpr auto headflags = static_cast<f3D>(static_cast<uint16_t>(f3D::kHead) | static_cast<uint16_t>(f3D::kFace));
			if (auto presetFlags = preset->get_flags(); bool(presetFlags & headflags) && bool(flags & (f3D::kHead | f3D::kFace))) {
				preset->update();
				flags |= presetFlags;
			}
		}
	
	g_OriginalDoUpdate3DModel(process, actor, flags);

	//if (have_dbr_flag) {
	////	std::this_thread::sleep_for(std::chrono::seconds(1));
	//	auto npc = get_leveled_TESNPC(actor->GetNPC());
	//	if (npc) g_processingReset.erase(npc->formID);
	//}
}

void remove_with_extra(RE::TESNPC* npc, RE::BGSHeadPart* hpart) {
	for (auto& extra : hpart->extraParts) {
		if (!extra->extraParts.empty())
			remove_with_extra(npc, extra);
		else
			g_OriginalChangeHeadPartRemovePart(npc, extra, false);
	}
	g_OriginalChangeHeadPartRemovePart(npc, hpart, false);
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
	//g_OriginalChangeHeadPartRemovePart(npc, hpart, bRemoveExtraParts);
}

void HookedChangeHeadPart(RE::TESNPC* npc, RE::BGSHeadPart* hpart)
{
	ProcessChangeHeadPart(npc, hpart, false, false);
	//g_OriginalChangeHeadPart(npc, hpart);
}

void SetupDetours(ActorUpdateManager* manager)
{
	uintptr_t baseAddr = reinterpret_cast<uintptr_t>(GetModuleHandleA("f4ee.dll"));

	if (!detourObjectLoaded.Create(reinterpret_cast<LPVOID>(baseAddr + 0x3DE0), reinterpret_cast<LPVOID>(HookedReceiveEventObjectLoaded))) {
		logger::error("Failed to create detour for ReceiveEventObjectLoaded");
	} else {
		g_OriginalReceiveEventObjectLoaded = reinterpret_cast<TESObjectLoadedEventHandler>(detourObjectLoaded.GetTrampoline());
		logger::info("Detour for ReceiveEventObjectLoaded created successfully.");
	}

	if (!detourInitScript.Create(reinterpret_cast<LPVOID>(baseAddr + 0x3C40), reinterpret_cast<LPVOID>(HookedReceiveEventInitScript))) {
		logger::error("Failed to create detour for ReceiveEventInitScript");
	} else {
		g_OriginalReceiveEventInitScript = reinterpret_cast<TESInitScriptEventHandler>(detourInitScript.GetTrampoline());
		logger::info("Detour for ReceiveEventInitScript created successfully.");
	}

	/*if (!detourReset3D.Create(reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL)) + REL::ID(302888).offset()), reinterpret_cast<LPVOID>(HookedReset3D))) {
		logger::error("Failed to create detour for Reset3DHandler");
	} else {
		g_OriginalReset3D = reinterpret_cast<Reset3DHandler>(detourReset3D.GetTrampoline());
		logger::info("Detour for Reset3DHandler created successfully.");
	}*/

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

	//if (!detourUpdate3DModel.Create(reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL)) + 0xE3C9C0), reinterpret_cast<LPVOID>(HookedUpdate3DModel))) {
	//	logger::error("Failed to create detour for Update3DModel");
	//} else {
	//	g_OriginalUpdate3DModel = reinterpret_cast<Update3DModelHandler>(detourUpdate3DModel.GetTrampoline());
	//	logger::info("Detour for Update3DModel created successfully.");
	//}

	if (!detourDoUpdate3DModel.Create(reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL)) + 0xE60860), reinterpret_cast<LPVOID>(HookedDoUpdate3DModel))) {
		logger::error("Failed to create detour for DoUpdate3DModel");
	} else {
		g_OriginalDoUpdate3DModel = reinterpret_cast<DoUpdate3DModelHandler>(detourDoUpdate3DModel.GetTrampoline());
		logger::info("Detour for DoUpdate3DModel created successfully.");
	}
}
