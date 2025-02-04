#include "Globals.h"

namespace logger = F4SE::log;

void InitForms()
{
	logger::info("Initialized form : {} ({})", global::diversed_kwd()->formEditorID, uint32ToHexString(global::diversed_kwd()->formID));
	logger::info("Initialized form : {} ({})", global::excluded_kwd()->formEditorID, uint32ToHexString(global::excluded_kwd()->formID));
	logger::info("Initialized form : {} ({})", global::excluded_npc()->GetFormEditorID(), uint32ToHexString(global::excluded_npc()->formID));
	logger::info("Initialized form : {} ({})", global::qualified_race()->GetFormEditorID(), uint32ToHexString(global::qualified_race()->formID));
	logger::info("Initialized form : {} ({})", global::processing_max()->formEditorID, uint32ToHexString(global::processing_max()->formID));
	logger::info("Initialized form : {} ({}) : {}", global::ignore_excluded()->formEditorID, uint32ToHexString(global::ignore_excluded()->formID), static_cast<bool>(global::ignore_excluded()->value));
	logger::info("Initialized form : {} ({}) : {}", global::ignore_diversed()->formEditorID, uint32ToHexString(global::ignore_diversed()->formID), static_cast<bool>(global::ignore_diversed()->value));

	logger::info("Initialized form : {} ({}) : {}", global::chance_bodymorph_male()->formEditorID, uint32ToHexString(global::chance_bodymorph_male()->formID), static_cast<int>(global::chance_bodymorph_male()->value));
	logger::info("Initialized form : {} ({}) : {}", global::chance_bodymorph_female()->formEditorID, uint32ToHexString(global::chance_bodymorph_female()->formID), static_cast<int>(global::chance_bodymorph_female()->value));
	logger::info("Initialized form : {} ({}) : {}", global::chance_bodyhair_male()->formEditorID, uint32ToHexString(global::chance_bodyhair_male()->formID), static_cast<int>(global::chance_bodyhair_male()->value));
	logger::info("Initialized form : {} ({}) : {}", global::chance_bodyhair_female()->formEditorID, uint32ToHexString(global::chance_bodyhair_female()->formID), static_cast<int>(global::chance_bodyhair_female()->value));
	logger::info("Initialized form : {} ({}) : {}", global::chance_skin_male()->formEditorID, uint32ToHexString(global::chance_skin_male()->formID), static_cast<int>(global::chance_skin_male()->value));
	logger::info("Initialized form : {} ({}) : {}", global::chance_skin_female()->formEditorID, uint32ToHexString(global::chance_skin_female()->formID), static_cast<int>(global::chance_skin_female()->value));
	logger::info("Initialized form : {} ({}) : {}", global::chance_hair_male()->formEditorID, uint32ToHexString(global::chance_hair_male()->formID), static_cast<int>(global::chance_hair_male()->value));
	logger::info("Initialized form : {} ({}) : {}", global::chance_hair_female()->formEditorID, uint32ToHexString(global::chance_hair_female()->formID), static_cast<int>(global::chance_hair_female()->value));
	logger::info("Initialized form : {} ({}) : {}", global::ignore_hair_if_hat()->formEditorID, uint32ToHexString(global::ignore_hair_if_hat()->formID), static_cast<int>(global::ignore_hair_if_hat()->value));
}

RE::TESGlobal* global::chance_bodymorph_male()
{
	return i_chance_bodymorph_male ? i_chance_bodymorph_male : get_re_ptr<RE::TESGlobal>(i_chance_bodymorph_male, formid_gv_int_chance_bodymorph_male, plugin_name);
}

RE::TESGlobal* global::chance_bodymorph_female()
{
	return i_chance_bodymorph_female ? i_chance_bodymorph_female : get_re_ptr<RE::TESGlobal>(i_chance_bodymorph_female, formid_gv_int_chance_bodymorph_female, plugin_name);
}

RE::TESGlobal* global::chance_bodyhair_male()
{
	return i_chance_bodyhair_male ? i_chance_bodyhair_male : get_re_ptr<RE::TESGlobal>(i_chance_bodyhair_male, formid_gv_int_chance_bodyhair_male, plugin_name);
}

RE::TESGlobal* global::chance_bodyhair_female()
{
	return i_chance_bodyhair_female ? i_chance_bodyhair_female : get_re_ptr<RE::TESGlobal>(i_chance_bodyhair_female, formid_gv_int_chance_bodyhair_female, plugin_name);
}
RE::TESGlobal* global::chance_skin_male()
{
	return i_chance_skin_male ? i_chance_skin_male : get_re_ptr<RE::TESGlobal>(i_chance_skin_male, formid_gv_int_chance_skin_male, plugin_name);
}

RE::TESGlobal* global::chance_skin_female()
{
	return i_chance_skin_female ? i_chance_skin_female : get_re_ptr<RE::TESGlobal>(i_chance_skin_female, formid_gv_int_chance_skin_female, plugin_name);
}

RE::TESGlobal* global::chance_hair_male()
{
	return i_chance_hair_male ? i_chance_hair_male : get_re_ptr<RE::TESGlobal>(i_chance_hair_male, formid_gv_int_chance_hair_male, plugin_name);
}

RE::TESGlobal* global::chance_hair_female()
{
	return i_chance_hair_female ? i_chance_hair_female : get_re_ptr<RE::TESGlobal>(i_chance_hair_female, formid_gv_int_chance_hair_female, plugin_name);
}

RE::TESGlobal* global::ignore_diversed()
{
	return b_ignore_diversed ? b_ignore_diversed : get_re_ptr<RE::TESGlobal>(b_ignore_diversed, formid_gv_bool_ignore_diversed, plugin_name);
}

RE::TESGlobal* global::ignore_excluded()
{
	return b_ignore_excluded ? b_ignore_excluded : get_re_ptr<RE::TESGlobal>(b_ignore_excluded, formid_gv_bool_ignore_excluded, plugin_name);
}

std::string uint32ToHexString(uint32_t value)
{
	std::stringstream ss;
	ss << "0x" << std::hex << std::setw(8) << std::setfill('0') << value;
	return ss.str();
}

bool global::is_diversed(RE::Actor* actor)
{
	if (!actor)
		return false;

	return global::diversed_kwd() ? actor->HasKeyword(global::diversed_kwd()) : false;
}

bool global::is_excluded(RE::Actor* actor)
{
	if (!actor)
		return false;

	if (global::excluded_kwd() && actor->HasKeyword(global::excluded_kwd()))
		return true;

	auto get_actor_base = [](RE::Actor* a) -> RE::TESActorBase* {
		auto base = a->GetTemplateActorBase();
		return base ? base : get_leveled_TESNPC(a->GetNPC());
	};

	if (global::excluded_npc() && (global::excluded_npc()->ContainsItem(actor) || global::excluded_npc()->ContainsItem(get_actor_base(actor))))
		return true;

	return false;
}

bool global::is_qualified_race(RE::Actor* actor)
{
	if (!actor)
		return false;

	if (global::qualified_race() && global::qualified_race()->ContainsItem(actor->race))
		return true;

	return false;
}

bool global::is_ignore_diversed()
{
	return ignore_diversed()->value;
}

bool global::is_ignore_excluded()
{
	return ignore_excluded()->value;
}

bool global::is_ignore_hair_if_hat()
{
	return ignore_hair_if_hat()->value;
}

bool global::is_only_if_vanilla_hair()
{
	return only_if_vanilla_hair()->value;
}


template <class T>
T* get_re_ptr(T*& ptr, uint32_t form, const std::string_view& plugin)
{
	if (!ptr)
		ptr = static_cast<T*>(RE::TESDataHandler::GetSingleton()->LookupForm(form, plugin));
	return ptr;
}

RE::BGSKeyword* global::diversed_kwd()
{
	return kwd_diversed ? kwd_diversed : get_re_ptr<RE::BGSKeyword>(kwd_diversed, formid_kwd_diversed, plugin_name);
}
RE::BGSKeyword* global::excluded_kwd()
{
	return kwd_excluded ? kwd_excluded : get_re_ptr<RE::BGSKeyword>(kwd_excluded, formid_kwd_excluded, plugin_name);
}
RE::BGSListForm* global::excluded_npc()
{
	return flst_excluded_npc ? flst_excluded_npc : get_re_ptr<RE::BGSListForm>(flst_excluded_npc, formid_flst_excluded_npc, plugin_name);
}
RE::BGSListForm* global::qualified_race()
{
	return flst_qualifying_race ? flst_qualifying_race : get_re_ptr<RE::BGSListForm>(flst_qualifying_race, formid_flst_qualifying_race, plugin_name);
}

RE::TESGlobal* global::processing_max()
{
	return max_processing ? max_processing : get_re_ptr<RE::TESGlobal>(max_processing, formid_gv_int_performance, plugin_name);
}

int global::get_processing_max()
{
	if (!game_data_loaded_state)
		return 10;
	return global::processing_max()->value;
}

RE::TESGlobal* global::ignore_hair_if_hat()
{
	return b_ignore_if_hat_equipped ? b_ignore_if_hat_equipped : get_re_ptr<RE::TESGlobal>(b_ignore_if_hat_equipped, formid_gv_bool_ignore_if_hat_equipped, plugin_name);
}

RE::TESGlobal* global::only_if_vanilla_hair()
{
	return b_only_if_vanilla_hair ? b_only_if_vanilla_hair : get_re_ptr<RE::TESGlobal>(b_only_if_vanilla_hair, formid_gv_bool_only_if_vanilla_hair, plugin_name);
}


bool FileExists(const std::string& filename, const std::string& type)
{
	auto LogAndReturn = [](const std::string& filename, RE::BSResource::ErrorCode code) {
		switch (code) {
		case RE::BSResource::ErrorCode::kNone:
			return true;
		case RE::BSResource::ErrorCode::kNotExist:
			logger::info("'FileExists' false : {} - kNotExist", filename);
			return false;
		case RE::BSResource::ErrorCode::kInvalidPath:
			logger::info("'FileExists' false : {} - kInvalidPath", filename);
			return false;
		default:
			logger::info("'FileExists' true (archived) : {} - Error code: {}", filename, static_cast<int>(code));
			return true;
		}
	};

	std::filesystem::path filePath = (!type.empty()) ? std::filesystem::current_path() / "Data" / type / filename : std::filesystem::current_path() / "Data" / filename;

	// Проверка существования файла в файловой системе
	if (std::filesystem::exists(filePath)) {
		//logger::info("'FileExists' true (loose) : {}", filename);
		return true;
	}

	// Проверка наличия файла в архиве
	RE::BSTSmartPointer<RE::BSResource::Stream, RE::BSTSmartPointerIntrusiveRefCount> a_result = nullptr;
	auto files = (!type.empty()) ? RE::BSResource::GetOrCreateStream((type + "/" + filename).c_str(), a_result) : RE::BSResource::GetOrCreateStream(filename.c_str(), a_result);

	if (a_result) {
		a_result->DoClose();
	}

	return LogAndReturn(filename, files);
}

uint32_t GetThreadsMax()
{
	return global::processing_max()->value < 1 ? 1 : global::processing_max()->value;
}

bool IsVanillaHair(const RE::Actor* const actor) noexcept
{
	if (!actor)
		return false;

	auto npc = get_leveled_TESNPC(actor->GetNPC());
	if (!npc)
		return false;

	/*RE::BGSHeadPart**& headParts = npc->headParts;
	int8_t headPartsSize = npc->numHeadParts;

	for (int i = 0; i < headPartsSize; ++i) {
		if (!headParts[i]->IsExtraPart())
			return IsVanillaHair(headParts[i]);
	}*/

	auto headParts = npc->GetHeadParts();
	for (auto& hp : headParts)
	{
		if (hp->type == RE::BGSHeadPart::HeadPartType::kHair && !hp->IsExtraPart())
			return IsVanillaForm(hp);
	}

	return false;
}

bool IsVanillaForm(const RE::TESForm* const form) noexcept
{
	if (!form)
		return false;

	return form->formID <= 0xFFFFFFu;
}

bool IsCreatedForm(const RE::TESForm* const form) noexcept
{
	if (!form)
		return false;

	return form->formID >= 0xFF000000u;
}

RE::BGSHeadPart* GetHairHeadPart(RE::Actor* actor)
{
	if (!actor)
		return nullptr;

	auto npc = get_leveled_TESNPC(actor->GetNPC());
	if (!npc)
		return nullptr;

	auto headParts = npc->GetHeadParts();
	for (auto& hp : headParts) {
		if (hp && hp->type == RE::BGSHeadPart::HeadPartType::kHair && !hp->IsExtraPart())
			return hp;
	}

	return nullptr;
}

//bool IsHatEquipped(RE::Actor* actor)
//{
//	auto biped = actor->GetBiped(false);
//	for (uint32_t i = 0; i < biped->QRefCount(); ++i) {
//		if (auto& obj = biped.get()[i].object[0].parent.object; obj != NULL)
//			if (obj->formType && (obj->formType == RE::ENUM_FORM_ID::kARMO || obj->formType == RE::ENUM_FORM_ID::kARMA))
//				return true;
//	}
//	return false;
//}

bool IsHatEquipped(RE::Actor* actor)
{
	if (!actor) {
		return false;
	}

	auto biped = actor->GetBiped(false);
	if (!biped || biped.get() == nullptr)
		return false;

	auto& obj = biped->object[0].parent.object;

	if (obj != NULL) {
		// Проверяем, что объект и его родитель инициализированы
		if (obj->formType &&
			(obj->formType == RE::ENUM_FORM_ID::kARMO ||
				obj->formType == RE::ENUM_FORM_ID::kARMA)) {
			return true;
		}

	}

	return false;
}

bool check(RE::Actor* actor)
{
	if (!actor) {
		//logger::info("nullptr : canceled");
		return false;
	}

	if (actor->formType != RE::ENUM_FORM_ID::kACHR) {
		//logger::info("{}({}) : canceled, is not actor", actor->GetFormEditorID(), uint32ToHexString(actor->formID));
		return false;
	}

	if (!global::is_qualified_race(actor)) {
		//logger::info("{}({}) : canceled, not qualified race", actor->GetFormEditorID(), uint32ToHexString(actor->formID));
		return false;
	}

	if (global::is_excluded(actor)) {
		//logger::info("{}({}) : canceled, is excluded", actor->GetFormEditorID(), uint32ToHexString(actor->formID));
		return false;
	}

	if (global::is_diversed(actor) && !global::is_ignore_diversed()) {
		//logger::info("{}({}) : canceled, already diversified", actor->GetFormEditorID(), uint32ToHexString(actor->formID));
		return false;
	}

	return true;
}

const char* global::message_wrong_file()
{
	if (!msg_DB_WrongFile)
		msg_DB_WrongFile = get_re_ptr<RE::BGSMessage>(msg_DB_WrongFile, formid_msg_DB_WrongFile, plugin_name);

	auto desc = msg_DB_WrongFile->descriptionText;
	auto txt = desc.GetText(*msg_DB_WrongFile->GetFile());
	return txt.GetString();
}
const char* global::message_wrong_actor_or_gender()
{
	if (!msg_DB_WrongFileActor)
		msg_DB_WrongFileActor = get_re_ptr<RE::BGSMessage>(msg_DB_WrongFileActor, formid_msg_DB_WrongFileActor, plugin_name);

	auto form = static_cast<RE::TESForm*>(msg_DB_WrongFileActor);
	RE::BSStringT<char> string{};

	msg_DB_WrongFileActor->GetDescription(string, form);
	return string.c_str();
}


int global::get_update_wait_time()
{
	if (!game_data_loaded_state)
		return 5;
	if (!float_body_update_cooling_time)
		float_body_update_cooling_time = get_re_ptr<RE::TESGlobal>(float_body_update_cooling_time, formid_gv_float_body_update_cooling_time, plugin_name);
	return float_body_update_cooling_time->value > 0 ? float_body_update_cooling_time->value : 1;
}

RE::BGSPerk* global::get_perk()
{
	if (!perk_diverse)
		perk_diverse = get_re_ptr<RE::BGSPerk>(perk_diverse, formid_perk_diverse, plugin_name);
	return perk_diverse;
}

RE::AlchemyItem* global::get_potion()
{
	if (!alch_change_morphs_potion)
		alch_change_morphs_potion = get_re_ptr<RE::AlchemyItem>(alch_change_morphs_potion, formid_alch_change_morphs_potion, plugin_name);
	return alch_change_morphs_potion;
}

RE::PlayerCharacter* GetPlayer()
{
	static RE::PlayerCharacter* ptr_player = GetFormByFormID<RE::PlayerCharacter>(14);
	return ptr_player;
}

//void global::add_potion()
//{
//	auto potion_formId = global::get_potion()->formID;
//	bool has_potion();
//	if (!player)!player->GetFullyLoaded3D() || !player->inventoryList)
//		return;
//	for (auto i = 0u; i < player->inventoryList->data.size(); ++i) {
//		auto& item = player->inventoryList->data[i];
//		if (item.object->formID == potion_formId)
//		{
//			if (auto count = item.GetCount(); count > 1)
//			{
//				auto item = RE::TESObjectREFR::RemoveItemData(global::get_potion(), count - 1);
//				player->RemoveItem(item);
//			} else if (count == 0) {
//				RE::BSTSmartPointer<RE::ExtraDataList> extra {};
//				player->AddInventoryItem(global::get_potion(), extra, 1, nullptr, nullptr, nullptr);
//			}
//		}
//	}
//}
