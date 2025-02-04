#pragma once
#include "F4SE/F4SE.h"
#include "RE/Fallout.h"

#include "DiverseBodiesRedux/Globals/consts.h"

#ifndef player
#	define player GetPlayer()
#endif

namespace logger = F4SE::log;
using f3D = RE::RESET_3D_FLAGS;
using namespace std::literals;

inline static std::string empty_string = "";

inline static bool game_data_loaded_state = false;

class global
{
	inline static RE::BGSKeyword* kwd_diversed = nullptr;
	inline static RE::BGSKeyword* kwd_excluded = nullptr;
	inline static RE::TESGlobal* b_ignore_excluded = nullptr;
	inline static RE::TESGlobal* b_ignore_diversed = nullptr;
	inline static RE::TESGlobal* i_chance_bodymorph_male = nullptr;
	inline static RE::TESGlobal* i_chance_bodymorph_female = nullptr;
	inline static RE::TESGlobal* i_chance_bodyhair_male = nullptr;
	inline static RE::TESGlobal* i_chance_bodyhair_female = nullptr;
	inline static RE::TESGlobal* i_chance_skin_male = nullptr;
	inline static RE::TESGlobal* i_chance_skin_female = nullptr;
	inline static RE::TESGlobal* i_chance_hair_male = nullptr;
	inline static RE::TESGlobal* i_chance_hair_female = nullptr;

	inline static RE::BGSListForm* flst_excluded_npc = nullptr;
	inline static RE::BGSListForm* flst_qualifying_race = nullptr;
	inline static RE::TESGlobal* cur_processing = nullptr;
	inline static RE::TESGlobal* max_processing = nullptr;
	inline static RE::TESGlobal* b_ignore_if_hat_equipped = nullptr;
	inline static RE::TESGlobal* b_only_if_vanilla_hair = nullptr;
	inline static RE::TESGlobal* float_body_update_cooling_time = nullptr;

	inline static RE::BGSMessage* msg_DB_WrongFile = nullptr;
	inline static RE::BGSMessage* msg_DB_WrongFileActor = nullptr;

	inline static RE::BGSPerk* perk_diverse = nullptr;
	inline static RE::AlchemyItem* alch_change_morphs_potion = nullptr;

public:

	//global variables
	static RE::TESGlobal* ignore_excluded();
	static RE::TESGlobal* ignore_diversed();
	static RE::TESGlobal* chance_bodymorph_male();
	static RE::TESGlobal* chance_bodymorph_female();
	static RE::TESGlobal* chance_bodyhair_male();
	static RE::TESGlobal* chance_bodyhair_female();
	static RE::TESGlobal* chance_skin_male();
	static RE::TESGlobal* chance_skin_female();
	static RE::TESGlobal* chance_hair_male();
	static RE::TESGlobal* chance_hair_female();
	static RE::TESGlobal* processing_max();
	static RE::TESGlobal* ignore_hair_if_hat();
	static RE::TESGlobal* only_if_vanilla_hair();

	//keywords
	static RE::BGSKeyword* diversed_kwd();
	static RE::BGSKeyword* excluded_kwd();
	
	//formlists
	static RE::BGSListForm* excluded_npc();
	static RE::BGSListForm* qualified_race();

	//fnc
	static bool is_diversed(RE::Actor* actor);
	static bool is_ignore_diversed();
	static bool is_ignore_excluded();
	static bool is_excluded(RE::Actor* actor);
	static bool is_qualified_race(RE::Actor* actor);
	static bool is_ignore_hair_if_hat();
	static bool is_only_if_vanilla_hair();
	static int get_update_wait_time();
	static int get_processing_max();

	//messages
	static const char* message_wrong_file();
	static const char* message_wrong_actor_or_gender();

	//perk
	static RE::BGSPerk* get_perk();

	//potion
	static RE::AlchemyItem* get_potion();
	//static void add_potion();
};

void InitForms();
template <class T>
T* get_re_ptr(T*& ptr, uint32_t form, const std::string_view& plugin);
std::string uint32ToHexString(uint32_t value);
bool FileExists(const std::string& filename, const std::string& type);
uint32_t GetThreadsMax();
bool IsVanillaForm(const RE::TESForm* const form) noexcept;
bool IsCreatedForm(const RE::TESForm* const form) noexcept;
bool IsVanillaHair(const RE::Actor* const actor) noexcept;
RE::BGSHeadPart* GetHairHeadPart(RE::Actor* actor);
bool IsHatEquipped(RE::Actor* actor);

constexpr RE::RESET_3D_FLAGS combine3DFlags(std::initializer_list<RE::RESET_3D_FLAGS> flags)
{
	size_t combined = 0;
	for (auto flag : flags) {
		combined |= static_cast<size_t>(flag);
	}
	return (RE::RESET_3D_FLAGS)(combined);
}

constexpr RE::RESET_3D_FLAGS disable3DFlags(std::initializer_list<RE::RESET_3D_FLAGS> flagsToDisable)
{
	size_t currentFlags = {
		static_cast<size_t>(RE::RESET_3D_FLAGS::kModel) |
		static_cast<size_t>(RE::RESET_3D_FLAGS::kSkin) |
		static_cast<size_t>(RE::RESET_3D_FLAGS::kHead) |
		static_cast<size_t>(RE::RESET_3D_FLAGS::kFace) |
		static_cast<size_t>(RE::RESET_3D_FLAGS::kScale) |
		static_cast<size_t>(RE::RESET_3D_FLAGS::kSkeleton) |
		static_cast<size_t>(RE::RESET_3D_FLAGS::kInitDefault) |
		static_cast<size_t>(RE::RESET_3D_FLAGS::kSkyCellSkin) |
		static_cast<size_t>(RE::RESET_3D_FLAGS::kHavok) |
		static_cast<size_t>(RE::RESET_3D_FLAGS::kDontAddOutfit) |
		static_cast<size_t>(RE::RESET_3D_FLAGS::kKeepHead) |
		static_cast<size_t>(RE::RESET_3D_FLAGS::kDismemberment)
	};

	// Выключаем переданные флаги
	for (auto flag : flagsToDisable) {
		currentFlags &= ~static_cast<size_t>(flag);  // Выключаем флаг
	}
	return (RE::RESET_3D_FLAGS)(currentFlags);
}

RE::PlayerCharacter* GetPlayer();

bool check(RE::Actor* actor);

template <typename T>
inline T* GetFormByFormID(uint32_t form_id)
{

	/*RE::TESForm* form{ nullptr };
	form = RE::TESForm::GetFormByID(form_id);
	if (form && form->formType == formType || formType == RE::ENUM_FORM_ID::kNONE) {
		return reinterpret_cast<T*>(form);
	} else {
		logger::warn("GetFormByResolvedFormID : Wrong formType : {}", form_id);
		return nullptr;
	}*/

	RE::TESForm* form = RE::TESForm::GetFormByID(form_id);
	if (form) {
		T* result = RE::fallout_cast<T*>(form);
		return result;
	}

	return nullptr;
}

template <typename T>
inline T* GetFormByResolvedFormID(uint32_t form_id)
{
	auto a_intfc = F4SE::GetSerializationInterface();
	std::optional<unsigned long> resolvedFormId = a_intfc->ResolveFormID(form_id);

	if (resolvedFormId) {
		return GetFormByFormID<T>(static_cast<uint32_t>(*resolvedFormId));
	} else {
		logger::warn("GetFormByResolvedFormID : Unable to resolve formId : {}", form_id);
		return nullptr;
	}
}

template <typename T>
inline T* GetFormByResolvedFormID(const std::string& token)
{
	auto a_intfc = F4SE::GetSerializationInterface();
	std::optional<uint32_t> formId;

	try {
		formId = std::stoul(token);
	} catch (const std::invalid_argument& e) {
		logger::warn("GetFormByResolvedFormID : Invalid argument for stoul: {}", e.what());
		return nullptr;
	} catch (const std::out_of_range& e) {
		logger::warn("GetFormByResolvedFormID : Out of range for stoul: {}", e.what());
		return nullptr;
	}

	// Попробуем разрешить formId
	auto resolvedFormId = a_intfc->ResolveFormID(formId.value());

	if (resolvedFormId) {
		return GetFormByFormID<T>(resolvedFormId.value());
	} else {
		logger::warn("GetFormByResolvedFormID : Unable to resolve formId for token : {}", token);
		return nullptr;
	}
}

inline auto copy_headparts(RE::TESNPC* npc)
{
	if (!npc)
		return std::vector<RE::BGSHeadPart*>{};

	auto hparts = npc->GetHeadParts();
	return std::vector<RE::BGSHeadPart*>(hparts.begin(), hparts.end());
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

inline void remove_chargenConditions(RE::TESNPC* npc)
{
	if (npc) {
		npc = get_leveled_TESNPC(npc);
		if (npc && npc->formType == RE::ENUM_FORM_ID::kNPC_) {
			auto hparts = npc->GetHeadParts();
			for (auto& hp : hparts) {
				if (hp && hp->chargenConditions && hp->chargenConditions.head)
					hp->chargenConditions.head = nullptr;
			}
		}
	}
}

inline void remove_chargen_from_all_tesnpc(RE::TESNPC* npc)
{
	if (npc) {
		npc->actorData.actorBaseFlags |= RE::TESActorBase::ACTOR_BASE_DATA_FLAGS::kFlagIsCharGenFacePreset;
		remove_chargenConditions(npc);
		if (npc->faceNPC && npc->faceNPC->formType == RE::ENUM_FORM_ID::kNPC_) {
			remove_chargen_from_all_tesnpc(npc->faceNPC);
		}

		auto& templateFlags = npc->actorData.templateUseFlags;
		if (((templateFlags & RE::TESActorBaseData::TemplateFlags::kFlagTraits) != 0) && npc->templateForms) {
			npc = reinterpret_cast<RE::TESNPC*>(*npc->templateForms);
			if (npc && npc->formType == RE::ENUM_FORM_ID::kNPC_) {
				remove_chargen_from_all_tesnpc(npc);
			}
		}
	}
}

inline RE::TESNPC* find_base(RE::Actor* actor)
{
	auto base = get_leveled_TESNPC(actor->GetNPC());
	if (base && base->formType != RE::ENUM_FORM_ID::kNPC_)
		base = nullptr;
	return base;
}
