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

#include <DiverseBodiesRedux/BodiesDiverser/ApplyPresetFromFile.h>
#include <DiverseBodiesRedux/BodiesDiverser/Manager.h>
#include <DiverseBodiesRedux/Globals/Globals.h>
#include <NAFicator/ThreadPool/ThreadPool.h>

namespace logger = F4SE::log;

#define PAPYRUS_BIND(funcName) a_VM->BindNativeMethod(ver::PROJECT, #funcName, papyrus::funcName, true)
#define PAPYRUS_BIND_LATENT(funcName, retType) a_VM->BindNativeMethod<retType>(ver::PROJECT, #funcName, papyrus::funcName, true, true)

extern std::map<std::string_view, bodymorphs::BodyPreset> BodyPresets;
extern std::vector<OverlayPreset> OverlayPresets;

// Dummy classes
class Bodymorph
{};
class Bodyhair
{};
class Skin
{};
class HairStyle
{};

namespace papyrus
{
	bool check_for_mcm_enable(RE::Actor* actor, decltype(global::chance_bodymorph_female) get_chance_fnc)
	{
		auto is_enable = static_cast<bool>(get_chance_fnc()->value);
		return !(actor->GetSex() == RE::Actor::Sex::Female && !is_enable) && !(actor->GetSex() == RE::Actor::Sex::Male && !is_enable);
	}

	bool apply_if_enabled(RE::Actor* actor, decltype(global::chance_bodymorph_female) chance_func, std::function<bool(RE::Actor*)> apply_func)
	{
		return check_for_mcm_enable(actor, chance_func) && apply_func(actor);
	}

	bool apply_overlays(RE::Actor* actor)
	{
		if (!OverlayPresets.empty()) {
			bool applied = false;
			for (auto& overlay_preset : OverlayPresets) {
				if (overlay_preset.apply(actor))
					applied = true;
			}
			if (applied) {
				UpdateOverlaysForActor(actor);
			}
			return true;
		}
		return false;
	}

	//void Apply(std::monostate, RE::Actor* actor)
	//{
	//	if (!check(actor))
	//		return;

	//	ThreadHandler<ApplyRandomEverything>::get_thread()->enqueue([actor]() {
	//		bool need3dreset = false;
	//		bool needSkinReset = false;
	//		if (check_for_mcm_enable(actor, actor->GetSex() == RE::Actor::Male ? global::chance_bodyhair_male : global::chance_bodyhair_female)) {
	//			if (apply_overlays(actor))
	//				UpdateOverlaysForActor(actor);
	//		}

	//		//actor->GetNPC()->formID; //formID имеет тип uint32_t. Разные actor могут иметь общий actor->GetNPC()->formID, нужно выставить такой лок, что бы в одно время мог обрабатываться только один актёр с таким actor->GetNPC()->formID
	//		std::this_thread::sleep_for(std::chrono::milliseconds(global::get_update_wait_time()));
	//		if (apply_if_enabled(actor, actor->GetSex() == RE::Actor::Male ? global::chance_skin_male : global::chance_skin_female, [](RE::Actor* actor) {
	//				return ApplyRandomBodyskin(actor);
	//			})) {
	//			needSkinReset = true;
	//		}
	//		
	//		if ((!global::is_ignore_hair_if_hat() || !IsHatEquipped(actor)) && (!global::is_only_if_vanilla_hair() || IsVanillaHair(actor))) {
	//			if (apply_if_enabled(actor, actor->GetSex() == RE::Actor::Male ? global::chance_hair_male : global::chance_hair_female, [](RE::Actor* actor) {
	//					return ApplyRandomHairStyle(actor);
	//				})) {
	//				need3dreset = true;
	//			}
	//		}

	//		if (needSkinReset)
	//			UpdateSkinForActor(actor);
	//		else if (need3dreset)
	//			UpdateHeadPartsForActor(actor);

	//		//снимаем лок

	//		if (apply_if_enabled(actor, actor->GetSex() == RE::Actor::Male ? global::chance_bodymorph_male : global::chance_bodymorph_female,
	//			[](RE::Actor* actor) {
	//			auto bm_preset = GetRandomBodyPreset(actor->GetSex(), BodyPreset::BodyType::NONE);
	//			return bm_preset ? bm_preset->apply(actor) : false;
	//		}))
	//			UpdateBodyMorphsForActor(actor);
	//	});
	//}

	void Apply(std::monostate, RE::Actor* actor)
	{
		if (!check(actor))
			return;

		ThreadHandler<ApplyPresetClass>::get_thread()->enqueue([actor, formID]() {
			bool need3dreset = false;
			bool needSkinReset = false;
			ActorPreset preset{};

			preset.actor = actor;

			if (check_for_mcm_enable(actor, actor->GetSex() == RE::Actor::Male ? global::chance_bodyhair_male : global::chance_bodyhair_female)) {
				if (apply_overlays(actor))
					UpdateOverlaysForActor(actor);
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(global::get_update_wait_time()));

			if (apply_if_enabled(actor, actor->GetSex() == RE::Actor::Male ? global::chance_skin_male : global::chance_skin_female, [](RE::Actor* actor) {
					return ApplyRandomBodyskin(actor);
				})) {
				needSkinReset = true;
			}

			if ((!global::is_ignore_hair_if_hat() || !IsHatEquipped(actor)) && (!global::is_only_if_vanilla_hair() || IsVanillaHair(actor))) {
				if (apply_if_enabled(actor, actor->GetSex() == RE::Actor::Male ? global::chance_hair_male : global::chance_hair_female, [](RE::Actor* actor) {
						return ApplyRandomHairStyle(actor);
					})) {
					need3dreset = true;
				}
			}

			if (needSkinReset)
				UpdateSkinForActor(actor);
			else if (need3dreset)
				UpdateHeadPartsForActor(actor);

			if (apply_if_enabled(actor, actor->GetSex() == RE::Actor::Male ? global::chance_bodymorph_male : global::chance_bodymorph_female,
					[](RE::Actor* actor) {
						auto bm_preset = GetRandomBodyPreset(actor->GetSex(), BodyPreset::BodyType::NONE);
						return bm_preset ? bm_preset->apply(actor) : false;
					}))
				UpdateBodyMorphsForActor(actor);
		});
	}

	void ApplyRandomBodyMorphPreset(std::monostate, RE::Actor* actor, int type)
	{
		if (!check(actor))
			return;

		ThreadHandler<Bodymorph>::get_thread()->enqueue([actor, type]() {
			auto preset = GetRandomBodyPreset(actor->GetSex(), GetTypeFromInt(type));
			if (preset && preset->apply(actor)) {
				UpdateBodyMorphsForActor(actor);
			}
		});
	}

	void ApplyRandomBodyHair(std::monostate, RE::Actor* actor)
	{
		if (!check(actor))
			return;

		ThreadHandler<Bodyhair>::get_thread()->enqueue([actor]() {
			if (apply_overlays(actor))
				UpdateOverlaysForActor(actor);
		});
	}

	void ApplyRandomSkin(std::monostate, RE::Actor* actor)
	{
		if (!check(actor))
			return;

		ThreadHandler<Skin>::get_thread()->enqueue([actor]() {
			if (ApplyRandomBodyskin(actor)) {
				UpdateSkinForActor(actor);
			}
		});
	}

	void ApplyRandomHair(std::monostate, RE::Actor* actor)
	{
		if (!check(actor))
			return;

		if (global::is_ignore_hair_if_hat() && IsHatEquipped(actor))
			return;

		ThreadHandler<HairStyle>::get_thread()->enqueue([actor]() {
			if (!global::is_only_if_vanilla_hair() || !IsVanillaHair(actor)) {
				if (ApplyRandomHairStyle(actor)) {
					UpdateHeadPartsForActor(actor);
				}
			}
		});
	}

	bool IsVanillaForm(std::monostate, RE::TESForm* form)
	{
		return ::IsVanillaForm(form);
	}

	RE::BGSHeadPart* GetHairHeadPart(std::monostate, RE::Actor* actor)
	{
		return ::GetHairHeadPart(actor);
	}

	void ApplyBodyPresetFromFileForActor(std::monostate, RE::Actor* actor)
	{
		::ApplyBodyPresetFromFileForActor(actor);
	}

	inline bool RegisterFunctions(RE::BSScript::IVirtualMachine* a_VM)
	{
		PAPYRUS_BIND(Apply);
		PAPYRUS_BIND(ApplyBodyPresetFromFileForActor);
		PAPYRUS_BIND(ApplyRandomBodyMorphPreset);
		PAPYRUS_BIND(ApplyRandomBodyHair);
		PAPYRUS_BIND(ApplyRandomSkin);
		PAPYRUS_BIND(ApplyRandomHair);
		PAPYRUS_BIND(IsVanillaForm);
		PAPYRUS_BIND(GetHairHeadPart);
		return true;
	}
}

#undef PAPYRUS_BIND
#undef PAPYRUS_BIND_LATENT
