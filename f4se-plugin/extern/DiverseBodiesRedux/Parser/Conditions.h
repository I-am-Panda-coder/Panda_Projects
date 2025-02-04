#pragma once
#include <string>
#include <vector>
#include <random>

#include "F4SE/F4SE.h"
#include "RE/Fallout.h"

#include <boost/json.hpp>

using namespace boost::json;
namespace logger = F4SE::log;

struct ConditionSettings
{
	RE::Actor::Sex gender = RE::Actor::Sex::None;
	int chance{ 100 };
	bool random{};

	std::vector<RE::BGSKeyword*> HasKeyword;     //kKYWD
	std::vector<RE::BGSKeyword*> HasNotKeyword;  //kKYWD
	std::vector<RE::TESFaction*> InFaction;      //kFACT
	std::vector<RE::TESFaction*> NotInFaction;   //kFACT

	ConditionSettings() = default;
	ConditionSettings(boost::json::value& item);

	bool check(const RE::Actor* actor) const;
};

class SkinValidLess
{
public:
	bool operator()(const std::pair<std::string, ConditionSettings>& lhs, const std::pair<std::string, ConditionSettings>& rhs) const;
};

RE::TESForm* get_form_from_string(const std::string& xFormID, const std::string& plugin);
