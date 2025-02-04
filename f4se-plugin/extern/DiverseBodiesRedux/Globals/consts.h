#pragma once

#include <string_view>

using namespace std::string_view_literals;

constexpr std::string_view plugin_name = "DiverseBodiesRedux.esp"sv;
//keywords
const uint32_t formid_kwd_diversed = 0x001;
const uint32_t formid_kwd_excluded = 0x002;
//mgef
const uint32_t formid_mgef_diverse = 0x014;
//global variables
const uint32_t formid_gv_bool_ignore_excluded = 0x10;
const uint32_t formid_gv_bool_ignore_diversed = 0x12;
const uint32_t formid_gv_int_chance_bodymorph_male = 0x16;
const uint32_t formid_gv_int_chance_bodymorph_female = 0x1C;
const uint32_t formid_gv_int_chance_bodyhair_male = 0x1D;
const uint32_t formid_gv_int_chance_bodyhair_female = 0x1E;
const uint32_t formid_gv_int_chance_skin_male = 0x1F;
const uint32_t formid_gv_int_chance_skin_female = 0x20;
const uint32_t formid_gv_int_chance_hair_male = 0x2B;
const uint32_t formid_gv_int_chance_hair_female = 0x30;
const uint32_t formid_gv_bool_ignore_if_hat_equipped = 0x566;
const uint32_t formid_gv_int_performance = 0x567;
const uint32_t formid_gv_bool_only_if_vanilla_hair = 0x569;
const float formid_gv_float_body_update_cooling_time = 0xC51;

//formlist
const uint32_t formid_flst_excluded_npc = 0x734;
const uint32_t formid_flst_qualifying_race = 0x15;

//messages
const uint32_t formid_msg_DB_WrongFile = 0xB;
const uint32_t formid_msg_DB_WrongFileActor = 0xC;

//perk
const uint32_t formid_perk_diverse_perk = 0x7;

//quests
const uint32_t formid_quest_choose_actor_quest = 0x8;
const uint32_t formid_quest_help_quest = 0xF99;

//perk
const uint32_t formid_perk_diverse = 0x7;

//spell in fallout4.esm
const uint32_t formid_spell_AbLegendaryCreatureItem = 0x1CCDA3;

//potion
const uint32_t formid_alch_change_morphs_potion = 0x5;
