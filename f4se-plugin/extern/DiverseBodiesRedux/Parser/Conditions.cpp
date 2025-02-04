#include "Conditions.h"
#include <utils/utility.h>
#include <utils/RandomGenerator.h>

ConditionSettings::ConditionSettings(boost::json::value& item) :
	gender{ RE::Actor::Sex::None }, chance{ 100 }, random{}
{
	if (item.is_object()) {
		const auto& obj = item.as_object();
		auto end = obj.end();

		auto it = obj.find("gender");
		if (it != end && it->value().is_int64()) {
			int i = it->value().as_int64();
			if (i < 0)
				gender = RE::Actor::Sex::None;
			else
				gender = i ? RE::Actor::Sex::Female : RE::Actor::Sex::Male;
		}

		it = obj.find("chance");
		if (it != end && it->value().is_int64()) {
			int i = it->value().as_int64();
			if (i < 0)
				chance = 0;
			else
				chance = i > 100 ? 100 : i;
		}

		it = obj.find("distribution");
		if (it != end && it->value().is_string())
			if (it->value().as_string() == "random")
				random = true;

		it = obj.find("in_faction");
		if (it != end && it->value().is_string()) {
			auto v = utils::string::split(it->value().as_string(), ",");
			for (const auto& _v : v) {
				auto pair = utils::string::split(_v, ":");
				if (pair.size() >= 2) {
					auto form = get_form_from_string(pair[1], pair[0]);
					if (form && form->formType == RE::ENUM_FORM_ID::kFACT)
						InFaction.emplace_back(static_cast<RE::TESFaction*>(form));
				}
			}
		}

		it = obj.find("not_in_faction");
		if (it != end && it->value().is_string()) {
			auto v = utils::string::split(it->value().as_string(), ",");
			for (const auto& _v : v) {
				auto pair = utils::string::split(_v, ":");
				if (pair.size() >= 2) {
					auto form = get_form_from_string(pair[1], pair[0]);
					if (form && form->formType == RE::ENUM_FORM_ID::kFACT)
						NotInFaction.emplace_back(static_cast<RE::TESFaction*>(form));
				}
			}
		}

		it = obj.find("has_keyword");
		if (it != end && it->value().is_string()) {
			auto v = utils::string::split(it->value().as_string(), ",");
			for (const auto& _v : v) {
				auto pair = utils::string::split(_v, ":");
				if (pair.size() >= 2) {
					auto form = get_form_from_string(pair[1], pair[0]);
					if (form && form->formType == RE::ENUM_FORM_ID::kKYWD)
						HasKeyword.emplace_back(static_cast<RE::BGSKeyword*>(form));
				}
			}
		}

		it = obj.find("has_no_keyword");
		if (it != end && it->value().is_string()) {
			auto v = utils::string::split(it->value().as_string(), ",");
			for (const auto& _v : v) {
				auto pair = utils::string::split(_v, ":");
				if (pair.size() >= 2) {
					auto form = get_form_from_string(pair[1], pair[0]);
					if (form && form->formType == RE::ENUM_FORM_ID::kKYWD)
						HasKeyword.emplace_back(static_cast<RE::BGSKeyword*>(form));
				}
			}
		}
	}
}

bool ConditionSettings::check(const RE::Actor* actor) const
{
	if (!actor)
		return false;

	if (gender != RE::Actor::Sex::None && const_cast<RE::Actor*>(actor)->GetSex() != gender)
		return false;

	auto HasKeywordInList = [](const RE::Actor* actor, std::vector<RE::BGSKeyword*> keywords) {
		for (auto kwd : keywords) {
			if (actor->HasKeyword(kwd))
				return true;
		}
		return false;
	};

	if (HasKeyword.size() && !HasKeywordInList(actor, HasKeyword))
		return false;

	if (HasNotKeyword.size() && HasKeywordInList(actor, HasNotKeyword))
		return false;

	auto IsInFaction = [](const RE::Actor* actor, std::vector<RE::TESFaction*> factions) {
		for (auto f : factions) {
			if (actor->IsInFaction(f))
				return true;
		}
		return false;
	};

	if (InFaction.size() && !IsInFaction(actor, InFaction))
		return false;

	if (NotInFaction.size() && IsInFaction(actor, NotInFaction))
		return false;

	if (chance > 0 && chance < 100) {
		thread_local utils::RandomGenerator rnd{};
		int randomInt = rnd.generate_random_int(1, 100);
		if (chance < randomInt)
			return false;
	} else if (chance <= 0)
		return false;

	return true;
}

RE::TESForm* get_form_from_string(const std::string& xFormID, const std::string& plugin)
{
	if (xFormID.empty() || plugin.empty()) {
		return nullptr;
	}
	std::string trimmedID = xFormID.length() > 6 ? xFormID.substr(xFormID.length() - 6) : xFormID;
	trimmedID.erase(trimmedID.begin(), std::find_if(trimmedID.begin(), trimmedID.end(), [](char c) { return c != '0'; }));

	if (!trimmedID.empty() && trimmedID.front() == 'x') {
		trimmedID.erase(trimmedID.begin());
	}

	if (trimmedID.empty()) {
		return nullptr;
	}

	trimmedID = "0x" + trimmedID;

	try {
		uint32_t FormID_uint = std::stoul(trimmedID, nullptr, 16);
		return RE::TESDataHandler::GetSingleton()->LookupForm<RE::TESForm>(FormID_uint, plugin);
	} catch (const std::invalid_argument&) {
		return nullptr;
	} catch (const std::out_of_range&) {
		return nullptr;
	} catch (...) {
		return nullptr;
	}
}

bool SkinValidLess::operator()(const std::pair<std::string, ConditionSettings>& lhs, const std::pair<std::string, ConditionSettings>& rhs) const
{
	auto& [l_string, l_conds] = lhs;
	auto& [r_string, r_conds] = rhs;
	auto l_sex = l_conds.gender;
	auto r_sex = r_conds.gender;
	typedef RE::Actor::Sex Sex;

	// Упорядочиваем по Sex: Male < None < Female
	if (l_sex == r_sex) {
		// Если Sex одинаковый, упорядочиваем по строкам
		return l_string < r_string;
	}

	// Порядок: Male (0) < None (-1) < Female (1)
	if (l_sex == Sex::Male) {
		return r_sex != Sex::Male;		// Male идет перед None и Female
	} else if (l_sex == Sex::None) {
		return r_sex == Sex::Female;	// None идет перед Female и после Male
	} else {							// l_sex == Sex::Female
		return false;					// Female всегда идет после Male и None
	}
}
