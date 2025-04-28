#include "Diversers.h"
#include <thread>
#include <DiverseBodiesRedux/Hooks/ProcessingSafe.h>

extern void (*g_OriginalChangeHeadPart)(RE::TESNPC*, RE::BGSHeadPart*);
extern void (*g_OriginalChangeHeadPartRemovePart)(RE::TESNPC*, RE::BGSHeadPart*, bool);
extern ProcessingNPC g_processingChangeHeadParts;

namespace hairs
{
	Preset::Preset(RE::BGSHeadPart* h)
	{
		if (h && h->formType == RE::ENUM_FORM_ID::kHDPT && h->type == RE::BGSHeadPart::HeadPartType::kHair) {
			bool canBeMale = !h->flags.any(RE::BGSHeadPart::Flag::kCantBeMale);
			bool canBeFemale = !h->flags.any(RE::BGSHeadPart::Flag::kCantBeFemale);

			if (canBeMale && canBeFemale) {
				id = "b";
			} else if (!canBeMale && !canBeFemale) {
				return;
			} else if (canBeMale) {
				id = "m";
			} else  // canBeFemale == true
			{
				id = "f";
			}

			id += std::to_string(h->formID);

			hair = h;
		}
	}

	bool Preset::is_vanilla() const
	{
		return this ? IsVanillaForm(hair) : false;
	}

	const std::string& Preset::name() const
	{
		return this ? id : empty_string;
	}

	RE::BGSHeadPart* Preset::hpart() const
	{
		return this ? hair : nullptr;
	}

	void Parse()
	{
		logger::info("Init hairs...");
		HairFilter filter{};
		auto HeadParts = RE::TESDataHandler::GetSingleton()->GetFormArray<RE::BGSHeadPart>();

		for (auto& hp : HeadParts) {
			if ((hp->type == RE::BGSHeadPart::HeadPartType::kHair) && !hp->IsExtraPart()) {
				if (!IsVanillaForm(hp)) {
					if (!filter.empty()) {
						if (bool contains = filter.contains(std::string{ hp->formEditorID }); contains) {
							if (!filter.isWhiteList())
								continue;
						} else {
							if (filter.isWhiteList())
								continue;
						}
					}
					Preset tmp{ hp };
					std::string_view id = tmp.name();
					Preset::MAP.emplace(id, std::move(tmp));
				}
			}
		}
		logger::info("Hairs initialized...");
	}

	std::vector<Preset*> ApplyFilter(RE::Actor* actor)
	{
		std::vector<Preset*> result{};

		if (!actor)
			return result;

		/*if (actor = RE::fallout_cast<RE::Actor*>(actor); !actor) {
			logger::critical("hairs::ApplyFilter Invalid actor object!");
			return result;
		}*/

		bool isFemale{};
		if (auto sex = actor->GetSex(); sex == RE::Actor::Sex::Female || sex == RE::Actor::Sex::Male)
			isFemale = sex == RE::Actor::Sex::Female;
		else
			return result;

		std::unordered_set<char> validStarts = isFemale ? std::unordered_set<char>{ 'b', 'f' } : std::unordered_set<char>{ 'b', 'm' };

		size_t totalCount = 0;
		for (const auto& startChar : validStarts) {
			totalCount += std::distance(Preset::MAP.lower_bound(std::string(1, startChar)), Preset::MAP.upper_bound(std::string(1, startChar)));
		}
		result.reserve(totalCount);

		for (auto& p : Preset::MAP) {
			if (validStarts.count(p.first[0]) > 0) {
				result.emplace_back(&p.second);
			}
		}

		return result; 
	}

	Preset* GetRandom(RE::Actor* actor)
	{
		if (!actor)
			return nullptr;

		/*if (actor = RE::fallout_cast<RE::Actor*>(actor); !actor) {
			logger::critical("hairs::GetRandom Invalid actor object!");
			return nullptr;
		}*/
		
		auto filteredPresets = ApplyFilter(actor);
		if (filteredPresets.empty()) {
			return nullptr;
		}

		thread_local utils::RandomGenerator rnd{};
		int randomIndex = rnd.generate_random_int(0, filteredPresets.size() - 1);

		return filteredPresets[randomIndex];
	}

	bool Preset::apply(RE::Actor* actor) const
	{
		if (!actor)
			return false;
		
		/*if (actor = RE::fallout_cast<RE::Actor*>(actor); !actor) {
			logger::critical("hairs::apply Invalid actor object!");
			return false;
		}*/
		bool result = this ? apply(get_face_TESNPC(actor->GetNPC()), hair) : false;
		return result;
	}

	bool Preset::apply(RE::TESNPC* npc, RE::BGSHeadPart* hairpart)
	{
		if (!npc || !hairpart) {
			return false;
		}

		bool log = iniSettings::getInstance().getExtendedLogs();
		if (log)
			logger::info("TESNPC* {:0x} : {}", npc->formID, g_processingChangeHeadParts.print());

		int count{};
		while (g_processingChangeHeadParts.contains(npc->formID)) {
			std::this_thread::sleep_for(std::chrono::microseconds(1));
			++count;
		}

		if (log && count) {
			logger::info("TESNPC* {:0x} waited for {} microseconds", npc->formID, count);
		}

		g_processingChangeHeadParts.insert(npc->formID);
		if (log)
			logger::info("Start processing hair for {:0x}", npc->formID);

		remove_chargenConditions(npc);

		auto get_hair = [](RE::TESNPC* npc) -> RE::BGSHeadPart* {
			auto headParts = npc->GetHeadParts();
			for (auto hp : headParts) {
				if (hp && hp->type == RE::BGSHeadPart::HeadPartType::kHair && !hp->IsExtraPart()) {
					return hp;
				}
			}
			return nullptr;
		};

		auto compare_headparts_extraparts_count = [](RE::BGSHeadPart* lhs, RE::BGSHeadPart* rhs) {
			if (lhs->extraParts.empty() != rhs->extraParts.empty())
				return false;
			if (!lhs->extraParts.empty()) {
				return (lhs->extraParts.size() == rhs->extraParts.size());
			}
			return true;
		};

		auto isHair = [](RE::BGSHeadPart* hpart) {
			return (hpart && hpart->type == RE::BGSHeadPart::HeadPartType::kHair);
		};

		remove_chargen(npc);

		if (auto current_hair = get_hair(npc); current_hair)
			g_OriginalChangeHeadPartRemovePart(npc, current_hair, !current_hair->extraParts.empty());

		hairpart->chargenConditions.head = nullptr;
		g_OriginalChangeHeadPart(npc, hairpart);
		for (auto& hp : hairpart->extraParts) {
			hp->chargenConditions.head = nullptr;
			if (hp)
				g_OriginalChangeHeadPart(npc, hp);
		}

		g_processingChangeHeadParts.erase(npc->formID);
		if (log)
			logger::info("Finished processing hair for {:0x}", npc->formID);
		return true;
	}
}
