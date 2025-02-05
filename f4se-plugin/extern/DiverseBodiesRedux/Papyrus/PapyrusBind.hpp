#pragma once
#include <DiverseBodiesRedux/version.h>
#include <F4SE/F4SE.h>
#include <RE/Fallout.h>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>

#include <DiverseBodiesRedux/Manager/ApplyPresetFromFile.h>
#include <DiverseBodiesRedux/Manager/Manager.h>
#include <DiverseBodiesRedux/Globals/Globals.h>
#include <NAFicator/ThreadPool/ThreadPool.h>

namespace logger = F4SE::log;

#define PAPYRUS_BIND(funcName) a_VM->BindNativeMethod(ver::PROJECT, #funcName, papyrus::funcName, true)
#define PAPYRUS_BIND_LATENT(funcName, retType) a_VM->BindNativeMethod<retType>(ver::PROJECT, #funcName, papyrus::funcName, true, true)

namespace papyrus
{
	bool IsSerializeFinished(std::monostate)
	{
		return ::IsSerializeFinished();
	}

	void ResetActorPreset(std::monostate, RE::Actor* actor)
	{
		if (!::IsSerializeFinished())
			return;
		auto p_preset = Find(actor);
		if (p_preset)
			p_preset->body.reset();
		if (p_preset->skin)
			p_preset->skin.reset();
		if (p_preset->overlays)
			p_preset->overlays.reset();
		if (p_preset->hair)
			p_preset->hair.reset();
	}
	
	/*void ResetBody(RE::Actor* actor) 
	{
		if (!IsSerializeFinished())
			return;
		auto p_preset = Find(actor);
		if (p_preset)
			p_preset->body.reset();
	}

	void ResetHair(RE::Actor* actor)
	{
		if (!IsSerializeFinished())
			return;
		auto p_preset = Find(actor);
		if (p_preset)
			p_preset->hair.reset();
	}

	void ResetSkin(RE::Actor* actor)
	{
		if (!IsSerializeFinished())
			return;
		auto p_preset = Find(actor);
		if (p_preset)
			p_preset->skin.reset();
	}*/

	/*void ResetOverlays(RE::Actor* actor)
	{
		if (!IsSerializeFinished())
			return;
		auto p_preset = Find(actor);
		if (p_preset)
			p_preset->overlays.reset();
	}*/

	void ApplyBodyPresetFromFile(std::monostate, RE::Actor* actor)
	{
		dbr_manager::ActorPreset preset;
		auto p_preset = Find(actor);
		if (!p_preset) {
			preset = dbr_manager::ActorPreset(actor, int{});
			p_preset = &preset;
			auto body_preset = ApplyBodyPresetFromFileForActor(actor);
			if (!body_preset.empty())
				p_preset->body = std::move(body_preset);
		} else {
			auto body_preset = ApplyBodyPresetFromFileForActor(actor);
			if (!body_preset.empty())
				p_preset->body = std::move(body_preset);
		}

		if (p_preset && p_preset->body) {
			bodymorphs::Get(*p_preset->body)->apply(actor);
			LooksMenuInterfaces<BodyMorphInterface>::GetInterface()->UpdateMorphs(actor);
		}
	}

	void ApplyRandomHair(std::monostate, RE::Actor* actor)
	{
		auto npc = find_base(actor);
		if (!npc)
			return;

		dbr_manager::ActorPreset preset;
		auto p_preset = Find(actor);
		auto hair_preset = hairs::GetRandom(actor);
		if (!p_preset)
		{
			preset = dbr_manager::ActorPreset(actor, int{});
			p_preset = &preset;
			
			if (hair_preset && hair_preset->hpart())
				p_preset->hair = hair_preset->hpart();
			if (p_preset->hair)
				if (dbr_manager::ActorsManager::move_to_map(std::move(*p_preset)))
					actor->Reset3D(false, RE::RESET_3D_FLAGS::kDiverseBodiesFlag, false, RE::RESET_3D_FLAGS::kNone);
		} else {
			if (hair_preset && hair_preset->hpart())
				p_preset->hair = hair_preset->hpart();
			if (p_preset->hair)
				actor->Reset3D(false, RE::RESET_3D_FLAGS::kDiverseBodiesFlag, false, RE::RESET_3D_FLAGS::kNone);
		}
	}

	void UpdateSettings(std::monostate)
	{
		iniSettings::getInstance().update();
	}

	inline bool RegisterFunctions(RE::BSScript::IVirtualMachine* a_VM)
	{
		PAPYRUS_BIND(IsSerializeFinished);
		PAPYRUS_BIND(ApplyBodyPresetFromFile);
		PAPYRUS_BIND(ApplyRandomHair);
		PAPYRUS_BIND(UpdateSettings);
		return true;
	}
}

#undef PAPYRUS_BIND
#undef PAPYRUS_BIND_LATENT
