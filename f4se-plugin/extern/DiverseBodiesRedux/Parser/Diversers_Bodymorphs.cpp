#include "Diversers.h"


namespace bodymorphs
{
	Preset::Preset(const std::filesystem::path& path) :
		filename(),
		sex(RE::Actor::Sex::None),
		type(Preset::Type::NONE)
	{
		if (path.extension() == ".json") {
			boost::json::object json_obj;
			try {
				json_obj = boost::json::parse(get_json(path)).as_object();
			} catch (...) {
				return;
			}

			auto pValue = json_obj.if_contains("Gender");
			if (pValue && pValue->is_int64()) {
				auto val = pValue->as_int64();
				sex = RE::Actor::Sex(val == 0 ? 0 : 1);
			}

			json_obj = json_obj.if_contains("BodyMorphs")->as_object();

			if (!json_obj.empty()) {
				int i = 0;
				std::for_each(json_obj.begin(), json_obj.end(), [&](const auto& o) {
					std::string key = o.key();
					float value;
					try {
						value = static_cast<float>(o.value().as_double());
					} catch (...) {
						try {
							value = static_cast<float>(o.value().as_int64());
						} catch (...) {
							logger::info("...FAILED : wrong value type");
							return;
						}
					}
					morphs.emplace(key, std::move(value));
					++i;
				});
			}
		} else if (path.extension() == ".xml") {
			pugi::xml_document doc;
			pugi::xml_parse_result result = doc.load_file(path.string().c_str());

			if (!result) {
				return;
			}

			pugi::xml_node presetNode = doc.child("SliderPresets").child("Preset");
			if (presetNode) {
				for (pugi::xml_node sliderNode = presetNode.child("SetSlider"); sliderNode; sliderNode = sliderNode.next_sibling("SetSlider")) {
					std::string key = sliderNode.attribute("name").value();
					float value = sliderNode.attribute("value").as_float() / 100.0f;  // делим на 100
					morphs.emplace(std::move(key), value);
				}
			}

			if (morphs.size()) {
				if (*path.filename().string().begin() == 'F')
					sex = RE::Actor::Sex::Female;
				else if (*path.filename().string().begin() == 'M')
					sex = RE::Actor::Sex::Male;
			}
		}
		if (sex >= 0 && morphs.size()) {
			filename = path.filename().string();
			if (filename->find("!FAT!") != std::string::npos)
				type = Preset::Type::FAT;
			else if (filename->find("!SLIM!") != std::string::npos)
				type = Preset::Type::SLIM;
			else if (filename->find("!SEXY!") != std::string::npos)
				type = Preset::Type::SEXY;
			else if (filename->find("!ATHL!") != std::string::npos)
				type = Preset::Type::ATHL;
		}
	}

	Preset::Preset(Preset&& other) noexcept
		:
		filename(std::move(other.filename)),
		type(other.type),
		sex(other.sex),
		morphs(std::move(other.morphs)) {}

	bool Preset::empty() const
	{
		return this ? !filename.has_value() : true;
	}

	const std::string& Preset::name() const
	{
		if (!this)
			return empty_string;
		if (filename)
			return filename.value();
		return empty_string;
	}

	RE::Actor::Sex Preset::get_sex() const
	{
		return this ? sex : RE::Actor::Sex::None;
	}

	bool Preset::operator<(const Preset& b) const
	{
		if (sex != b.sex) {
			return static_cast<int>(sex) < static_cast<int>(b.sex);
		}

		if (type != b.type) {
			return static_cast<int>(type) < static_cast<int>(b.type);
		}

		return filename < b.filename;
	}

	Preset& Preset::operator=(Preset&& other) noexcept
	{
		if (this != &other) {
			filename = std::move(other.filename);
			type = other.type;
			sex = other.sex;
			morphs = std::move(other.morphs);
		}
		return *this;
	}

	void Remove(RE::Actor* actor) 
	{
		LooksMenuInterfaces<BodyMorphInterface>::GetInterface()->RemoveMorphsByKeyword(actor, actor->GetSex() == RE::Actor::Sex::Female, global::diversed_kwd());
	}

	bool Preset::apply(RE::Actor* actor) const
	{
		if (!actor)
			return false;

		//LooksMenuInterfaces<BodyMorphInterface>::GetInterface()->RemoveMorphsByKeyword(actor, get_sex() == RE::Actor::Sex::Female, global::diversed_kwd());
		Remove(actor);
		for (const auto& [morphName, morphValue] : morphs) {
			LooksMenuInterfaces<BodyMorphInterface>::GetInterface()->SetMorph(actor, get_sex() == RE::Actor::Sex::Female, morphName, global::diversed_kwd(), morphValue);
		}

		if (!morphs.empty()) {
			true;
		}
		return false;
	}

	Preset::Type Preset::get_bodytype() const
	{
		return type;
	}

	std::vector<Preset*> ApplyFilter(RE::Actor::Sex sex, Preset::Type bodyType)
	{
		std::vector<Preset*> result;
		if (Preset::MAP.empty())
			return result;

		// Если BodyType равен NONE, мы просто фильтруем по sex
		if (bodyType == Preset::Type::NONE) {
			for (auto& [key, preset] : Preset::MAP) {
				if (preset.get_sex() == sex) {
					result.push_back(&preset);
				}
			}
		} else {
			// Используем lower_bound и upper_bound для фильтрации
			auto lower = Preset::MAP.lower_bound(std::to_string(static_cast<int>(sex)));
			auto upper = Preset::MAP.upper_bound(std::to_string(static_cast<int>(sex) + 1));

			for (auto it = lower; it != upper; ++it) {
				if (it->second.get_sex() == sex && it->second.get_bodytype() == bodyType) {
					result.push_back(&it->second);
				}
			}
		}

		return result;
	}

	Preset* GetRandom(bool isFemale, Preset::Type bodyType)
	{
		// Определяем пол в зависимости от isFemale
		RE::Actor::Sex sex = isFemale ? RE::Actor::Sex::Female : RE::Actor::Sex::Male;

		// Фильтруем BodyPreset по половому признаку и типу тела
		std::vector<Preset*> filteredPresets = ApplyFilter(sex, bodyType);

		// Если нет подходящих элементов, возвращаем nullptr
		if (filteredPresets.empty()) {
			return nullptr;
		}

		// Получаем случайный элемент
		thread_local utils::RandomGenerator rnd{};
		int randomIndex = rnd.generate_random_int(0, filteredPresets.size() - 1);

		return filteredPresets[randomIndex];
	}

	Preset* GetRandom(RE::Actor* a)
	{
		return GetRandom(a->GetSex() == RE::Actor::Sex::Female, Preset::Type::NONE);
	}

	void Parse()
	{
		std::stack<std::string> fault;

		std::filesystem::path folder = std::filesystem::current_path() / "Data" / "DiverseBodiesRedux" / "BodyPresets";

		logger::info("Remember, JSONs ans XMLs must be saved in UTF-8 (not UTF-8 BOM)");

		for (const auto& entry : std::filesystem::directory_iterator(folder)) {
			if (!entry.is_directory() && (entry.path().extension() == ".json" || entry.path().extension() == ".xml")) {
				Preset tmp(entry.path());
				if (!tmp.empty()) {
					Preset::MAP.emplace(std::make_pair(std::string_view(tmp.name()), std::move(tmp)));
				} else {
					fault.push(entry.path().filename().string());
				}
			}
		}

		if (fault.size()) {
			logger::info("Parse bodymorph presets failed : ");
			while (fault.size()) {
				logger::info("\t{}", fault.top());
				fault.pop();
			}
		}
	}

	Preset* Get(const std::string& id)
	{
		auto it = Preset::MAP.find(id);
		return it != Preset::MAP.end() ? &it->second : nullptr;
	}
}
