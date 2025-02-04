#include "Diversers.h"

namespace skins
{
	void Parse()
	{
		std::filesystem::path folder;
		try {
			folder = std::filesystem::current_path() / "Data" / "F4SE" / "Plugins" / "F4EE" / "Skin";
		} catch (std::bad_alloc e) {
			logger::error("Failed open file {} : {}", folder.string(), e.what());
			return;
		} catch (std::runtime_error e) {
			logger::error("Failed open file {} : {}", folder.string(), e.what());
			return;
		} catch (...) {
			logger::error("Failed open file {} : unknown", folder.string());
			return;
		}

		std::stack<std::string> fault;

		for (const auto& entry : std::filesystem::directory_iterator(folder)) {
			if (entry.is_directory()) {
				for (auto file : std::filesystem::directory_iterator(entry.path())) {
					if (file.path().filename() == "skin.json") {
						boost::json::value json_value{};
						try {
							json_value = boost::json::parse(get_json(file.path()));
						} catch (...) {
							fault.push(file.path().string());
							continue;
						}
						if (json_value.is_array()) {
							auto& json_array = json_value.as_array();
							for (auto& item : json_array) {
								if (Preset tmp{ item }; !tmp.empty()) {
									auto name = tmp.name();
									Preset::MAP.emplace(std::move(name), std::move(tmp));
								}
							}
						} else
							fault.push(file.path().string());
					} else {
						fault.push(file.path().string());
					}
				}
			}
		}

		if (fault.size()) {
			logger::info("Parse Skins failed : ");
			while (fault.size()) {
				logger::info("\t{}", fault.top());
				fault.pop();
			}
		}

		logger::info("Valid Skins : ");
		for (auto& [_, preset] : Preset::MAP) {
			auto& id = preset.name();
			auto& gender = preset.conditions.gender;
			logger::info("{} - {}", id, gender == RE::Actor::Sex::None ? "Any" : gender == RE::Actor::Sex::Female ? "Female" : "Male");
		}
	}

	Preset::Preset(boost::json::value& item)
	{
		if (item.is_object()) {
			const auto& obj = item.as_object();
			auto end = obj.end();
			auto it = obj.find("id");
			if (it == end)
				return;

			if (it->value().is_string()) {
				id = it->value().as_string();
				if (id.empty())
					return;
			} else
				return;

			it = obj.find("gender");
			if (it != end && it->value().is_int64()) {
				int i = it->value().as_int64();
				if (i < 0)
					conditions.gender = RE::Actor::Sex::None;
				else
					conditions.gender = i ? RE::Actor::Sex::Female : RE::Actor::Sex::Male;
			}

			bool break_cycle = false;
			if (conditions.gender == RE::Actor::Sex::Male || conditions.gender == RE::Actor::Sex::None) {
				if (it = obj.find("maleFace"); it != end && it->value().is_string()) {
					auto v = utils::string::split(it->value().as_string(), "|");
					if (v.size() == 2) {
						auto form = get_form_from_string(v[1], v[0]);
						if (!form)
							break_cycle = true;
					}
				}

				if (break_cycle)
					return;

				if (it = obj.find("maleHeadRear"); it != end && it->value().is_string()) {
					auto v = utils::string::split(it->value().as_string(), "|");
					if (v.size() == 2) {
						auto form = get_form_from_string(v[1], v[0]);
						if (!form)
							break_cycle = true;
					}
				}
			}

			if (break_cycle)
				return;

			if (conditions.gender == RE::Actor::Sex::Female || conditions.gender == RE::Actor::Sex::None) {
				if (it = obj.find("femaleFace"); it != end && it->value().is_string()) {
					auto v = utils::string::split(it->value().as_string(), "|");
					if (v.size() == 2) {
						auto form = get_form_from_string(v[1], v[0]);
						if (!form)
							break_cycle = true;
					}
				}

				if (break_cycle)
					return;

				if (it = obj.find("femaleHeadRear"); it != end && it->value().is_string()) {
					auto v = utils::string::split(it->value().as_string(), "|");
					if (v.size() == 2) {
						auto form = get_form_from_string(v[1], v[0]);
						if (!form)
							break_cycle = true;
					}
				}
			}

			if (break_cycle)
				return;

			if (it = obj.find("skin"); it != end && it->value().is_string()) {
				auto v = utils::string::split(it->value().as_string(), "|");
				if (v.size() == 2) {
					auto form = get_form_from_string(v[1], v[0]);
					if (!form)
						break_cycle = true;
				}
			}
		}
	}

	bool Preset::empty() const
	{
		return this ? id.empty() : true;
	}

	const std::string& Preset::name() const
	{
		return this ? id : empty_string;
	}

	void Remove(RE::Actor* actor)
	{
		auto g = LooksMenuInterfaces<SkinInterface>::GetInterface();
		g->RemoveSkinOverride(actor);
	}

	std::vector<Preset*> ApplyFilter(RE::Actor* actor)
	{
		std::vector<Preset*> result{};
		for (auto& preset : Preset::MAP) {
			if (preset.second.conditions.check(actor))
				result.emplace_back(&preset.second);
		}
		return std::move(result);
	}

	Preset* GetRandom(RE::Actor* actor)
	{
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
		if (!actor || id.empty())
			return false;

		if (!conditions.check(actor))
			return false;

		remove_chargen(actor);

		return LooksMenuInterfaces<SkinInterface>::GetInterface()->AddSkinOverride(actor, id, actor->GetSex() == RE::Actor::Sex::Female);
	}

	Preset* Get(const std::string& id)
	{
		auto it = Preset::MAP.find(id);
		return it != Preset::MAP.end() ? &it->second : nullptr;
	}
}
