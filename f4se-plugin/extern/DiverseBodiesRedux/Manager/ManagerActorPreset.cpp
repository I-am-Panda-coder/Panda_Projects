#include "ManagerActorPreset.h"
#include <zlib.h>
#include <DiverseBodiesRedux/Hooks/ProcessingSafe.h>

//extern void (*g_OriginalReset3D)(RE::Actor*, bool, RE::RESET_3D_FLAGS, bool, RE::RESET_3D_FLAGS);
extern ProcessingNPC g_processingReset;

namespace dbr_manager
{
	ActorPreset::ActorPreset(RE::Actor* actor)
	{
		if (!actor)
			return;
		
		if (!actor || !RE::fallout_cast<RE::Actor*>(actor))
			return;

		if (global::is_qualified_race(actor) && !global::is_excluded(actor)) {
			auto RND = utils::RandomGenerator{};
			bool isFemale = actor->GetSex() == RE::Actor::Sex::Female;

			auto checkRND = [&RND](int val) -> bool {
				RND.update_seed();
				return RND(1, 99) < val;
			};

			auto& ini = iniSettings::getInstance();

			// Проверка шанса на bodymorph
			int bodyMorphChance = isFemale ? ini.getBodyMorphFemaleChance() : ini.getBodyMorphMaleChance();
			if (bodyMorphChance != 0 && (bodyMorphChance == 100 || RND(1, 99) < bodyMorphChance)) {
				if (auto rnd = bodymorphs::GetRandom(actor); rnd)
					body = rnd->name();
			}

			// Проверка шанса на bodyhair
			int bodyHairChance = isFemale ? ini.getBodyHairFemaleChance() : ini.getBodyHairMaleChance();
			if (bodyHairChance != 0 && (bodyHairChance == 100 || checkRND(bodyHairChance))) {
				if (auto col = overlays::Collection{ actor }; !col.empty())
				overlays = col;
			}

			if (!iniSettings::getInstance().getDisableChangeHeadparts()) {
				auto base = get_face_TESNPC(actor->GetNPC());

				// Проверка шанса на skin
				int skinChance = isFemale ? ini.getSkinFemaleChance() : ini.getSkinMaleChance();
				if (base && skinChance != 0 && (skinChance == 100 || checkRND(skinChance)))
					if (auto rnd = skins::GetRandom(actor); rnd)
						skin = rnd->name();

				// Проверка шанса на hair
				int hairChance = isFemale ? ini.getHairFemaleChance() : ini.getHairMaleChance();
				if (base && hairChance != 0 && (hairChance == 100 || checkRND(hairChance)))
					if (auto rnd = hairs::GetRandom(actor); rnd)
						hair = rnd->hpart();
			}
			
			this->actor = actor;
		}
	}

	ActorPreset::ActorPreset(RE::Actor* actor, int)
	{
		if (!actor)
			return;
		
		if (!actor || !RE::fallout_cast<RE::Actor*>(actor))
			return;

		if (global::is_qualified_race(actor)) {
			auto RND = utils::RandomGenerator{};
			bool isFemale = actor->GetSex() == RE::Actor::Sex::Female;

			auto checkRND = [&RND](int val) -> bool {
				RND.update_seed();
				return RND(1, 99) < val;
			};

			this->actor = actor;
		}
	}

	ActorPreset::ActorPreset(boost::json::value& obj)
	{
		if (obj.is_object())
			deserialize(obj.as_object());
	}

	inline RE::TESNPC* find_base(RE::Actor* actor)
	{
		/*auto base = get_leveled_TESNPC(actor->GetNPC());
		if (base && base->formType != RE::ENUM_FORM_ID::kNPC_)
			base = nullptr;
		return base;*/
		return get_face_TESNPC(actor->GetNPC());
	}

	bool ActorPreset::empty() const
	{
		return (!actor || (!body && !skin && !hair && !overlays));
	}

	bool ActorPreset::apply(bool reset, bool a_reloadAll, RE::RESET_3D_FLAGS a_additionalFlags, bool a_queueReset, RE::RESET_3D_FLAGS a_excludeFlags) const
	{			
		auto reset_actor = [&, this]() {
			
			if (!actor)
				return;

			a_additionalFlags |= get_flags();
			a_additionalFlags |= f3D::kDiverseBodiesFlag;
			a_excludeFlags &= get_rflags();
			a_queueReset = false;

			if (iniSettings::getInstance().getExtendedLogs())
				logger::info("reset_actor : {:0x} : Apply Reset 3d ({}, {}, {}, {})", actor ? actor->formID : 0,
					a_reloadAll ? "true" : "false",
					std::to_string(static_cast<uint16_t>(a_additionalFlags)),
					a_queueReset ? "true" : "false",
					std::to_string(static_cast<uint16_t>(a_excludeFlags)));

			if (reset && actor && actor->GetFullyLoaded3D() && actor->currentProcess && actor->currentProcess->middleHigh)
				actor->Reset3D(a_reloadAll, a_additionalFlags, a_queueReset, a_excludeFlags);
			else if (iniSettings::getInstance().getExtendedLogs())
				logger::info("reset_actor : {:0x} : can't reset 3d, actor not loaded!", actor ? actor->formID : 0);
		};
		
		if (!actor || empty())
			return false;

		/*const RE::Actor* actor_check = RE::fallout_cast<const RE::Actor*>(actor);
		if (!actor_check) {
			logger::critical("apply : Invalid actor object!");
			return false;
		}*/

		try {
			const RE::Actor* actor_check = RE::fallout_cast<const RE::Actor*>(actor);
			if (!actor_check) {
				logger::critical("apply : Invalid actor object!");
				return false;
			}
		} catch (const std::exception& e) {
			logger::critical("apply : RTTI error: {}", e.what());
			return false;
		} catch (...) {
			logger::critical("apply : RTTI error unknown");
		}


		
		if (iniSettings::getInstance().getExtendedLogs())
			logger::info("apply : Apply preset for {}->{:0x}", actor->GetDisplayFullName(), actor->formID);

		if (body) {
			if (auto bp = bodymorphs::Get(*body); bp) {
				if (iniSettings::getInstance().getExtendedLogs())
					logger::info("apply : {:0x} : Apply body : {}", actor->formID, bp->name());
				bp->apply(actor);
			}
		}

		if (overlays) {
			if (iniSettings::getInstance().getExtendedLogs()) {
				std::string overlays_str{ "" };
				bool first = true;
				for (auto& o : overlays->overlays) {
					if (!first)
						overlays_str += ",";
					else
						first = false;
					overlays_str += o.id;
				}
				logger::info("apply : {:0x} : Apply overlays : {}", actor->formID, overlays_str);
			}
			overlays->apply(actor);
		}

		if (!iniSettings::getInstance().getDisableChangeHeadparts()) {
			auto base = get_face_TESNPC(actor->GetNPC());
			if (base) {
				if (skin) {
					if (iniSettings::getInstance().getExtendedLogs())
						logger::info("apply : {} {:0x} : {:0x} Apply skin : {}", actor->GetDisplayFullName(), actor->formID, base->formID, *skin);
					LooksMenuInterfaces<SkinInterface>::GetInterface()->AddSkinOverride(actor, *skin, actor->GetSex() == RE::Actor::Sex::Female);
				}

				auto& ini = iniSettings::getInstance();

				if (hair && (!ini.getIsOnlyIfVanillaHair() || IsVanillaHair(actor)) && (!ini.getSkipIfHatEquipped() || !IsHatEquipped(actor))) {
					if (iniSettings::getInstance().getExtendedLogs())
						logger::info("apply : {} {:0x} : {:0x} Apply hair : {}", actor->GetDisplayFullName(), actor->formID, base->formID, (*hair)->GetFormEditorID());
					hairs::Preset::apply(base, *hair);
				}
			}

			if (reset) {
				reset_actor();
			}
		}

		return true;
	}

	bool ActorPreset::update(f3D flags) const
	{
		if (actor)
			return apply(flags != f3D::kNone, false, flags);
		else
			return false;
	}

	boost::json::object ActorPreset::serialize() const
	{
		boost::json::object obj;

		// Serialize actor form ID
		if (actor) {
			obj["actor_formId"] = actor->formID;
		}

		// Serialize body and skin
		if (body) {
			obj["body"] = *body;
		}
		if (skin) {
			obj["skin"] = *skin;
		}

		// Serialize hair form ID
		if (hair) {
			obj["hair_formId"] = (*hair)->formID;
		}

		// Serialize overlays
		if (overlays) {
			obj["overlays"] = overlays->serialize();
		}

		return obj;
	}

	ActorPreset& ActorPreset::deserialize(boost::json::object& obj)
	{
		// Deserialize actor form ID
		if (obj.contains("actor_formId")) {
			uint32_t formID = obj["actor_formId"].as_int64();
			actor = GetFormByResolvedFormID<RE::Actor>(formID);
		}

		if (!actor)
			return *this;

		if (actor = RE::fallout_cast<RE::Actor*>(actor); !actor)
			return *this;

		// Deserialize body and skin
		if (obj.contains("body")) {
			body = obj["body"].as_string();
		}
		if (obj.contains("skin")) {
			skin = obj["skin"].as_string();
		}

		// Deserialize hair form ID
		if (obj.contains("hair_formId")) {
			uint32_t formID = obj["hair_formId"].as_int64();
			hair = GetFormByResolvedFormID<RE::BGSHeadPart>(formID);
		}

		// Deserialize overlays
		if (obj.contains("overlays")) {
			overlays.emplace(); 
			overlays->deserialize(obj["overlays"].as_object());
		}

		return *this;
	}

	/*
	enum class RESET_3D_FLAGS : uint16_t
	{
		kNone = 0,
		kModel = 1u << 0,
		kSkin = 1u << 1,
		kHead = 1u << 2,
		kFace = 1u << 3,
		kScale = 1u << 4,
		kSkeleton = 1u << 5,
		kInitDefault = 1u << 6,
		kSkyCellSkin = 1u << 7,
		kHavok = 1u << 8,
		kDontAddOutfit = 1u << 9,
		kKeepHead = 1u << 10,
		kDismemberment = 1u << 11
	};
	*/

	f3D ActorPreset::get_flags() const
	{
		if (empty())
			return static_cast<f3D>(~static_cast<uint16_t>(f3D::kNone));
		
		uint16_t f = static_cast<uint16_t>(f3D::kNone);  // Инициализация флага

		// Проверяем наличие тела и устанавливаем соответствующие флаги
		if (body) {
			f |= static_cast<uint16_t>(f3D::kModel);  // Устанавливаем флаг kModel
		}

		// Проверяем наличие кожи и устанавливаем соответствующие флаги
		if (skin) {
			f |= static_cast<uint16_t>(f3D::kSkin);  // Устанавливаем флаг kSkin
			f |= static_cast<uint16_t>(f3D::kFace);  // Устанавливаем флаг kFace
			//f |= static_cast<uint16_t>(f3D::kHead);	
		}

		// Проверяем наличие волос и устанавливаем соответствующие флаги
		if (hair) {
			//f |= static_cast<uint16_t>(f3D::kFace);
			f |= static_cast<uint16_t>(f3D::kHead);			// Устанавливаем флаг kHead
			//f &= ~static_cast<uint16_t>(f3D::kKeepHead);  // Убираем флаг kKeepHead
		}

		// Убираем ненужные флаги
		//f &= ~static_cast<uint16_t>(f3D::kDontAddOutfit);  // Убираем флаг kDontAddOutfit
		//f &= ~static_cast<uint16_t>(f3D::kDismemberment);  // Убираем флаг kDismemberment
		//f &= ~static_cast<uint16_t>(f3D::kInitDefault);    // Убираем флаг kInitDefault
		//f &= ~static_cast<uint16_t>(f3D::kScale);          // Убираем флаг kScale
		//f &= ~static_cast<uint16_t>(f3D::kSkeleton);       // Убираем флаг kSkeleton

		return static_cast<f3D>(f);  // Возвращаем итоговое значение как int
	}

	f3D ActorPreset::get_rflags() const
	{
		return static_cast<f3D>(~static_cast<uint16_t>(get_flags()));
	}

	f3D ActorPreset::update_head_and_return_flags() const
	{
		auto base = get_face_TESNPC(actor->GetNPC());

		auto f = f3D::kNone;
		if (base && hair) {
			if (iniSettings::getInstance().getExtendedLogs())
				logger::info("update_head_and_return_flags : {} {:0x} : {:0x} Apply hair : {}", actor->GetDisplayFullName(), actor->formID, base->formID, (*hair)->GetFormEditorID());
			hairs::Preset::apply(base, *hair);

			f |= f3D::kFace;
			f |= f3D::kHead;
		}

		return f;
	}

	f3D ActorPreset::update_and_return_flags() const
	{
		apply();
		return get_flags();
	}
}
