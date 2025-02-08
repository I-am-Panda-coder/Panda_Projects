#include "Diversers.h"


namespace overlays
{
	Preset::Overlay::Overlay(boost::json::value& item) :
		id{ std::string{} }, conditions{ ConditionSettings{} }
	{
		if (item.is_object()) {
			auto& obj = item.as_object();
			auto end = obj.end();

			if (auto it = obj.find("id"); it != end && it->value().is_string()) {
				id = it->value().as_string();
			} else {
				id.clear();
			}

			if (id.empty()) {
				logger::info("Overlay item with id missed!");
				return;
			}

			conditions = std::move(ConditionSettings(item));

			if (auto it = obj.find("overlays"); it != end && it->value().is_array()) {
				auto end = it->value();
				for (auto& overlayData : it->value().as_array()) {
					auto tmp = Data(overlayData);
					if (!tmp.empty())
						overlays.emplace(std::move(tmp));
				}
			}

			if (auto it = obj.find("remove"); it != end && it->value().is_string()) {
				auto splited = utils::string::split(it->value().as_string(), ",");
				remove = std::move(decltype(remove)(splited.begin(), splited.end()));
			}
		}
	}

	bool Preset::Overlay::empty() const
	{
		return (id.empty() || overlays.empty());
	}

	Data::Data(boost::json::value& item)
	{
		if (item.is_object()) {
			const auto& obj = item.as_object();
			auto end = obj.end();

			if (auto it = obj.find("id"); it != end && it->value().is_string()) {
				id = it->value().as_string();
				if (!Preset::ValidOverlays.contains(id)) {
					logger::info("Overlay with id '{}' doesn't exists in Looks Menu!", id);
					id.clear();
					return;
				}
			} else
				return;

			settings = std::move(Settings(item));
		}
	}

	bool Data::empty() const
	{
		return (id.empty());
	}

	Settings::Settings(boost::json::value& item) :
		tint{ 0.0f, 0.0f, 0.0f, 1.0f }, offsetUV{ 0.0, 0.0 }, scaleUV{ 1.0, 1.0 }, priority{}
	{
		if (item.is_object()) {
			const auto& obj = item.as_object();
			auto end = obj.end();

			if (auto it = obj.find("tint"); it != end && it->value().is_array()) {
				size_t c{};
				for (auto i : it->value().as_array()) {
					if (c < 4) {
						auto get_i = [&i, &c] {
							if (i.is_double()) {
								return static_cast<float>(i.as_double());
							} else if (i.is_int64()) {
								return static_cast<float>(i.as_int64());
							} else if (c == 3) {
								return 1.0f;
							} else {
								return 0.0f;
							}
						};

						switch (c) {
						case 0:
							tint.r = get_i();
							break;
						case 1:
							tint.g = get_i();
							break;
						case 2:
							tint.b = get_i();
							break;
						case 3:
							tint.a = get_i();
							break;
						}
					}
				}
			}

			if (auto it = obj.find("offsetUV"); it != end && it->value().is_array()) {
				size_t c{};
				for (auto i : it->value().as_array()) {
					if (c < 2) {
						auto get_i = [&i, &c] {
							if (i.is_double()) {
								return static_cast<float>(i.as_double());
							} else if (i.is_int64()) {
								return static_cast<float>(i.as_int64());
							} else {
								return 0.0f;
							}
						};

						switch (c) {
						case 0:
							offsetUV.x = get_i();
							break;
						case 1:
							offsetUV.y = get_i();
							break;
						}
					}
				}
			}

			if (auto it = obj.find("scaleUV"); it != end && it->value().is_array()) {
				size_t c{};
				for (auto i : it->value().as_array()) {
					float f{ 1 };
					if (c < 2) {
						auto get_i = [&i, &c] {
							if (i.is_double()) {
								return static_cast<float>(i.as_double());
							} else if (i.is_int64()) {
								return static_cast<float>(i.as_int64());
							} else {
								return 1.0f;
							}
						};

						switch (c) {
						case 0:
							scaleUV.x = f;
							break;
						case 1:
							scaleUV.y = f;
							break;
						}
					}
				}
			}

			if (auto it = obj.find("priority"); it != end && it->value().is_int64()) {
				if (it->value().is_int64())
					priority = (it->value().as_int64() >= 0) ? it->value().as_int64() : 0;
			}
		}
	}

	Preset::Preset(boost::json::value& item)
	{
		if (item.is_object()) {
			const auto& obj = item.as_object();
			auto end = obj.end();

			if (auto it = obj.find("id"); it != end && it->value().is_string()) {
				id = it->value().as_string();
			} else {
				id.clear();
			}

			if (id.empty()) {
				logger::info("Overlay with id missed!");
				return;
			}

			conditions = std::move(ConditionSettings(item));

			if (auto it = obj.find("remove"); it != end && it->value().is_string()) {
				auto splited = utils::string::split(it->value().as_string(), ",");
				remove = std::move(decltype(remove)(splited.begin(), splited.end()));
			}

			if (auto it = obj.find("details"); it != end && it->value().is_array()) {
				auto end = it->value();
				for (auto overlay : it->value().as_array()) {
					auto tmp = Overlay{ overlay };
					if (!tmp.empty())
						details.emplace(std::move(tmp));
				}
			}
		}
	}

	bool Preset::operator==(const std::string& s) const
	{
		return id == s;
	}

	bool Preset::operator==(const Preset& p) const
	{
		return id == p.id;
	}

	bool Preset::empty() const
	{
		return (id.empty() || (details.empty() && remove.empty()));
	}

	bool Data::is_valid() const
	{
		return (Preset::is_valid(id));
	}

	bool Data::apply(RE::Actor* actor) const
	{
		if (actor && is_valid()) {
			/*logger::info(
			"applied {} [{};{};{};{};{};{};{};{}] overlay id - {}", id, uint32ToHexString(actor->formID), actor->GetSex() == RE::Actor::Female ? "true" : "false",
			uint32ToHexString(actor->formID), settings.priority, id.c_str(), 
			std::string{ std::to_string(settings.tint.r) + "," + std::to_string(settings.tint.g) + "," + std::to_string(settings.tint.b) + "," + std::to_string(settings.tint.a) },
			std::string { std::to_string(settings.offsetUV.x) + "," + std::to_string(settings.offsetUV.y) },
			std::string { std::to_string(settings.scaleUV.x) + "," + std::to_string(settings.scaleUV.y) },
			
			static_cast<uint32_t>(
			LooksMenuInterfaces<OverlayInterface>::GetInterface()->AddOverlay(actor, actor->GetSex() == RE::Actor::Female,
			settings.priority, id.c_str(), settings.tint, settings.offsetUV, settings.scaleUV)
				)
			);*/

			Remove(actor, id.c_str());

			LooksMenuInterfaces<OverlayInterface>::GetInterface()->AddOverlay(actor, actor->GetSex() == RE::Actor::Female,
				settings.priority, id.c_str(), settings.tint, settings.offsetUV, settings.scaleUV);
			return true;
		}
		return false;
	}

	const auto& Preset::get_map()
	{
		return MAP;
	}

	bool Preset::check(const RE::Actor* actor) const
	{
		return conditions.check(actor);
	}

	const auto& Preset::get_remove() const
	{
		return remove;
	}

	const auto& Preset::get_details() const
	{
		return details;
	}

	bool Preset::is_condition_random() const
	{
		return conditions.random;
	}

	bool Preset::Overlay::check(const RE::Actor* actor) const
	{
		return conditions.check(actor);
	}

	bool Preset::Overlay::is_condition_random() const
	{
		return conditions.random;
	}

	const auto& Preset::Overlay::get_remove() const
	{
		return remove;
	}

	const auto& Preset::Overlay::get_overlays() const
	{
		return overlays;
	}

	/*Collection& Collection::create_new_collection_for_actor(const RE::Actor* actor)
	{
		for (const auto& el : Preset::get_map()) {
			const auto& preset = el.second;
			if (preset.empty())
				continue;

			if (!preset.check(actor))
				continue;

			if (preset.get_remove().size()) {
				for (const auto& r : remove) {
					remove.emplace(r);
				}
			}

			if (preset.get_details().size() == 1 || !preset.is_condition_random()) {
				bool applied = false;
				for (const auto& d : preset.get_details()) {
					if (d.empty())
						continue;

					if (!d.check(actor))
						continue;

					if (d.get_remove().size()) {
						for (const auto& r : remove) {
							remove.emplace(r);
						}
					}

					if (d.get_overlays().size() == 1 || !d.is_condition_random()) {
						for (auto& o : d.get_overlays()) {
							overlays.emplace(o);
						}
					} else {
						thread_local utils::RandomGenerator rnd{};
						int randomIndex = rnd.generate_random_int(0, d.get_overlays().size() - 1);
						auto overlay = *std::next(d.get_overlays().begin(), randomIndex);
						overlays.emplace(overlay);
					}
				}
			} else {
				thread_local utils::RandomGenerator rnd{};
				int randomIndex = rnd.generate_random_int(0, preset.get_details().size() - 1);

				auto d = *std::next(preset.get_details().begin(), randomIndex);
				if (d.empty())
					continue;

				if (!d.check(actor))
					continue;

				if (d.get_remove().size()) {
					for (const auto& r : remove) {
						remove.emplace(r);
					}
				}

				if (d.get_overlays().size() == 1 || !d.is_condition_random()) {
					for (auto& o : d.get_overlays()) {
						overlays.emplace(o);
					}
				} else {
					thread_local utils::RandomGenerator rnd{};
					int randomIndex = rnd.generate_random_int(0, d.get_overlays().size() - 1);
					auto overlay = *std::next(d.get_overlays().begin(), randomIndex);
					overlays.emplace(overlay);
				}
			}
		}
	}*/

	//Collection& Collection::create_new_collection_for_actor(const RE::Actor* actor)
	//{
	//	auto apply_overlays = [&](const Preset::Overlay& overlays_set) {
	//		if (overlays_set.get_overlays().size() == 1 || !overlays_set.is_condition_random()) {
	//			for (const auto& o : overlays_set.get_overlays()) {
	//				overlays.emplace(o);
	//			}
	//		} else {
	//			thread_local utils::RandomGenerator rnd{};
	//			int randomIndex = rnd.generate_random_int(0, overlays_set.get_overlays().size() - 1);
	//			auto overlay = *std::next(overlays_set.get_overlays().begin(), randomIndex);
	//			overlays.emplace(overlay);
	//		}
	//	};

	//	auto handle_details = [&](const std::unordered_set<Preset::Overlay, Preset::Overlay::Hash, Preset::Overlay::Equal>& details, bool random) {
	//		if (details.size() == 1 || !random) {
	//			for (const auto& d : details)
	//				apply_overlays(d.get_overlays());
	//		} else {
	//			thread_local utils::RandomGenerator rnd{};
	//			int randomIndex = rnd.generate_random_int(0, preset.get_details().size() - 1);
	//			auto d = *std::next(preset.get_details().begin(), randomIndex);
	//			handle_details({ d });
	//		}
	//		
	//		
	//		for (const auto& d : details) {
	//			if (d.empty() || !d.check(actor))
	//				continue;

	//			if (d.get_remove().size()) {
	//				for (const auto& r : d.get_remove()) {
	//					remove.emplace(r);
	//				}
	//			}

	//			apply_overlays(d);
	//		}
	//	};

	//	for (const auto& el : Preset::get_map()) {
	//		const auto& preset = el.second;
	//		if (preset.empty() || !preset.check(actor))
	//			continue;

	//		if (preset.get_remove().size()) {
	//			for (const auto& r : preset.get_remove()) {
	//				remove.emplace(r);
	//			}
	//		}

	//	
	//		handle_details(preset.get_details(), preset.is_condition_random());
	//	}

	//	return *this;  // Не забудьте вернуть ссылку на текущий объект
	//}

	Collection& Collection::create_new_collection_for_actor(const RE::Actor* actor)
	{
		for (const auto& el : Preset::get_map()) {
			const auto& preset = el.second;
			if (preset.empty() || !preset.check(actor))
				continue;

			if (preset.get_remove().size()) {
				for (const auto& r : preset.get_remove()) {
					remove.emplace(r);
				}
			}

			// Обработка деталей
			if (preset.get_details().size() == 1 || !preset.is_condition_random()) {
				handle_details(actor, preset.get_details());
			} else {
				thread_local utils::RandomGenerator rnd{};
				int randomIndex = rnd.generate_random_int(0, preset.get_details().size() - 1);
				auto d = *std::next(preset.get_details().begin(), randomIndex);
				handle_detail(actor, d);  // Обработка одной детали
			}
		}

		return *this;  // Возвращаем ссылку на текущий объект
	}

	const Preset::Overlay* const Preset::find(const std::string& id)
	{
		auto it = std::find(details.cbegin(), details.cend(), id);
		if (it != details.cend())
			return &(*it);
		else
			return nullptr;
	}

	/*void Collection::merge(const Collection& other)
	{
		std::unordered_set<std::string> tmp;

		auto to_tmp = [&tmp](const auto& overlays) {
			for (const auto& o : overlays) {
				tmp.emplace(o);
			}
		};

		to_tmp(overlays);
		to_tmp(other.overlays);

		overlays = std::move(std::vector<std::string> { tmp.begin(), tmp.end() });
	}*/

	bool Collection::empty() const
	{
		return overlays.empty() && remove.empty();
	}

	void Collection::validate()
	{
		if (empty())
			return;

		for (auto it = overlays.begin(); it != overlays.end(); ++it)
		{
			if (!it->is_valid())
				it = overlays.erase(it);
		}
	}

	bool Preset::Overlay::apply(RE::Actor* actor) const
	{
		if (!this)
			return false;
		
		if (!conditions.check(actor))
			return false;

		if (remove.size()) {
			for (const auto& r : remove) {
				Remove(actor, r);
			}
		}

		if (overlays.size() == 1 || !conditions.random) {
			bool applied = false;
			for (const auto& o : overlays) {
				if (o.apply(actor))
					applied = true;
			}
			return applied;
		} else {
			thread_local utils::RandomGenerator rnd{};
			int randomIndex = rnd.generate_random_int(0, overlays.size() - 1);

			auto it = overlays.begin();
			for (int i = 0; i <= randomIndex; ++i) {
				if (i == randomIndex) {
					return (it->apply(actor));
				}
				++it;
			}
		}
		return false;
	}

	std::size_t Data::Hash::operator()(const Data& overlayData) const
	{
		std::size_t hash = 0;
		hash ^= std::hash<std::string>()(overlayData.id) << 1;         // Хеш id
		hash ^= std::hash<float>()(overlayData.settings.tint.r) << 2;  // Хеш цвета (например, r)
		hash ^= std::hash<float>()(overlayData.settings.tint.g) << 3;  // Хеш цвета (например, g)
		hash ^= std::hash<float>()(overlayData.settings.tint.b) << 4;  // Хеш цвета (например, b)
		hash ^= std::hash<float>()(overlayData.settings.tint.a) << 5;  // Хеш цвета (например, a)
		return hash;
	}

	bool Data::Equal::operator()(const Data& lhs, const Data& rhs) const
	{
		return lhs == rhs;
	}

	bool Data::operator==(const Data& other) const
	{
		return id == other.id;
	}

	bool Preset::Overlay::operator==(const Overlay& other) const
	{
		return id == other.id;
	}

	bool Preset::Overlay::operator==(const std::string& str) const
	{
		return id == str;
	}

	std::size_t Preset::Overlay::Hash::operator()(const Overlay& overlaysArr) const
	{
		std::size_t hash = 0;
		hash ^= std::hash<std::string>()(overlaysArr.id) << 1;
		for (const auto& overlay : overlaysArr.overlays) {
			hash ^= std::hash<std::string>()(overlay.id) << 1;
		}
		return hash;
	}

	bool Preset::Overlay::Equal::operator()(const Overlay& lhs, const Overlay& rhs) const
	{
		return lhs == rhs;
	}

	bool Preset::apply(RE::Actor* actor) const
	{
		if (!this)
			return false;

		if (!conditions.check(actor))
			return false;

		if (remove.size()) {
			for (const auto& r : remove) {
				Remove(actor, r);
			}
		}

		if (details.size() == 1 || !conditions.random) {
			bool applied = false;
			for (const auto& d : details) {
				if (d.apply(actor))
					applied = true;
			}
			return applied;
		} else {
			thread_local utils::RandomGenerator rnd{};
			int randomIndex = rnd.generate_random_int(0, details.size() - 1);

			auto it = details.begin();
			for (int i = 0; i <= randomIndex; ++i) {
				if (i == randomIndex) {
					return it->apply(actor);
				}
				++it;
			}
		}
		return false;
	}

	const std::string& Preset::name() const
	{
		return this ? id : empty_string;
	}

	void Parse()
	{	
		Preset::ValidOverlays = GetValid();

		if (Preset::ValidOverlays.empty()) {
			return;
		}
		
		std::filesystem::path folder;
		try {
			folder = std::filesystem::current_path() / "Data" / "DiverseBodiesRedux" / "Overlays";
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
			if (!entry.is_directory() && entry.path().extension() == ".json") {
				boost::json::value json;
				try {
					json = boost::json::parse(get_json(entry.path()));
				} catch (...) {
					fault.push(entry.path().filename().string());
					continue;
				}
				auto end = json.as_object().end();
				if (auto start = json.as_object().find("presets_array"); start != end && start->value().is_array()) {
					for (auto& val : start->value().as_array()) {
						Preset tmp(val);
						if (!tmp.empty()) {
							Preset::MAP.emplace(tmp.name(), std::move(tmp));
						}
					}
				}
			}
		}

		if (fault.size()) {
			logger::info("Parse Overlays presets failed : ");
			while (fault.size()) {
				logger::info("\t{}", fault.top());
				fault.pop();
			}
		}
	}

	static std::set<std::string> ValidOverlays{};

	const std::set<std::string>& GetValid()
	{
		if (!ValidOverlays.empty())
			return ValidOverlays;

		auto ParseOverlayJSON = [](const std::filesystem::path& path) -> bool
		{
			auto result = false;
			if (path.extension() == ".json") {
				boost::json::value json_value;
				try {
					std::string json_string;
					try {
						json_string = get_json(path);
					}
					catch (std::runtime_error e) {
						logger::error("Failed parse ValidOverlays {} : {}", path.string(), e.what());
						return false;
					}
					json_value = boost::json::parse(json_string);
				} catch (...) {
					return false;
				}

				// Проверяем, является ли json_value массивом
				if (json_value.is_array()) {
					const auto& json_array = json_value.as_array();
					RE::Actor::Sex sex = RE::Actor::Sex::None;
					std::string id("");
					std::set<std::string> overlays;
					int success_counter = 0;
					for (const auto& item : json_array) {
						if (item.is_object()) {
							const auto& obj = item.as_object();
							auto end = obj.end();
							auto it_id = obj.find("id");
							if (it_id == end)
								continue;
							auto it_slots = obj.find("slots");
							if (it_slots == end)
								continue;
							/*auto it_gender = obj.find("gender");
					if (it_gender == end)
						continue;*/

							id = obj.at("id").as_string().c_str();
							const auto& slots = obj.at("slots").as_array();
							for (const auto& slot : slots) {
								if (slot.is_object() && slot.as_object().find("material") != slot.as_object().end()) {
									std::string material = slot.at("material").as_string().c_str();
									// Проверяем существование файла
									if (FileExists(material, "materials")) {
										++success_counter;
									}
								}
							}

							/*auto get_sex_from_int = [](int i) {
						if (i >= -1 && i <= 1)
							return RE::Actor::Sex(i);
						else
							return RE::Actor::Sex(-1);
					};*/

							/*sex = get_sex_from_int(it_gender->value().as_int64());*/
						}
						if (success_counter && !id.empty()) {
							ValidOverlays.emplace(id);
							result = true;
						}
					}
				}
			}
			return result;
		};
		
		std::filesystem::path folder;
		try {
			folder = std::filesystem::current_path() / "Data" / "F4SE" / "Plugins" / "F4EE" / "Overlays";
		} catch (std::bad_alloc e) {
			logger::error("Failed open file {} : {}", folder.string(), e.what());
			return ValidOverlays;
		} catch (std::runtime_error e) {
			logger::error("Failed open file {} : {}", folder.string(), e.what());
			return ValidOverlays;
		} catch (...) {
			logger::error("Failed open file {} : unknown", folder.string());
			return ValidOverlays;
		}
		std::stack<std::string> fault;

		for (const auto& entry : std::filesystem::directory_iterator(folder)) {
			if (entry.is_directory()) {
				for (auto file : std::filesystem::directory_iterator(entry.path())) {
					if (file.path().filename() == "overlays.json") {
						if (!ParseOverlayJSON(file.path()))
							fault.push(file.path().string());
					} else {
						fault.push(file.path().string());
					}
				}
			}
		}

		if (fault.size()) {
			logger::info("Parse Overlays failed : ");
			while (fault.size()) {
				logger::info("\t{}", fault.top());
				fault.pop();
			}
		}

		/*logger::info("Valid Overlays : ");
	for (auto& vo : ValidOverlays)
	{
		logger::info("{}", vo);
	}*/
		if (!ValidOverlays.size())
			logger::warn("Failed to parse valid overlays : no overlays in folder!");

		return ValidOverlays;
	}

	void Remove(RE::Actor* actor, const std::string& id)
	{
		if (!actor)
			return;

		bool found = false;
		LooksMenuInterfaces<OverlayInterface>::GetInterface()->ForEachOverlay(actor, static_cast<bool>(actor->GetSex()), [&found, &actor, &id](int32_t uid, const OverlayInterface::OverlayDataPtr& overlay) {
			if (!found && strcmp(overlay->templateName.get()->c_str(), id.c_str()) == 0) {
				LooksMenuInterfaces<OverlayInterface>::GetInterface()->RemoveOverlay(actor, static_cast<bool>(actor->GetSex()), overlay->uid);
				found = true;
			}
		});
	}

	void Remove(RE::Actor* actor)
	{
		LooksMenuInterfaces<OverlayInterface>::GetInterface()->RemoveAll(actor, actor->GetSex() == RE::Actor::Sex::Female);
	}

	Collection::Collection(const RE::Actor* actor) :
		overlays{},
		remove{}
	{
		*this = create_new_collection_for_actor(actor);
	}

	void Collection::handle_overlays(const RE::Actor*, const Preset::Overlay& overlay)
	{
		if (overlay.get_overlays().size() == 1 || !overlay.is_condition_random()) {
			for (const auto& o : overlay.get_overlays()) {
				overlays.emplace(o);
			}
		} else {
			thread_local utils::RandomGenerator rnd{};
			int randomIndex = rnd.generate_random_int(0, overlay.get_overlays().size() - 1);
			auto overlayToApply = *std::next(overlay.get_overlays().begin(), randomIndex);
			overlays.emplace(overlayToApply);
		}
	}

	void Collection::handle_details(const RE::Actor* actor, const std::unordered_set<Preset::Overlay, Preset::Overlay::Hash, Preset::Overlay::Equal>& details)
	{
		for (const auto& d : details) {
			handle_detail(actor, d);  // Обработка каждой детали
		}
	}

	void Collection::handle_detail(const RE::Actor* actor, const Preset::Overlay& detail)
	{
		if (detail.empty() || !detail.check(actor))
			return;

		if (detail.get_remove().size()) {
			for (const auto& r : detail.get_remove()) {
				remove.emplace(r);
			}
		}

		handle_overlays(actor, detail);
	}

	bool Preset::is_valid(const std::string& overlay_id)
	{
		auto& Valid = GetValid();
		if (overlay_id.empty() || Valid.empty())
			return false;

		return Valid.contains(overlay_id);
	}

	bool Collection::apply(RE::Actor* actor) const
	{
		for (const auto& o : overlays) {
			o.apply(actor);
		}
		return true;
	}

	boost::json::object Settings::serialize() const
	{
		boost::json::object json_obj;

		// Сериализация tint
		json_obj["tint"] = boost::json::object{
			{ "r", tint.r },
			{ "g", tint.g },
			{ "b", tint.b },
			{ "a", tint.a }
		};

		// Сериализация offsetUV
		json_obj["offsetUV"] = boost::json::object{
			{ "x", offsetUV.x },
			{ "y", offsetUV.y }
		};

		// Сериализация scaleUV
		json_obj["scaleUV"] = boost::json::object{
			{ "x", scaleUV.x },
			{ "y", scaleUV.y }
		};

		// Сериализация priority
		json_obj["priority"] = static_cast<uint64_t>(priority);

		return json_obj;
	}

	Settings& Settings::deserialize(const boost::json::value& item)
	{
		if (!item.is_object()) {
			throw std::runtime_error("Expected a JSON object");
		}

		const auto& obj = item.as_object();

		// Десериализация tint
		if (obj.contains("tint")) {
			const auto& tint_obj = obj.at("tint").as_object();
			tint.r = boost::json::value_to<float>(tint_obj.at("r"));
			tint.g = boost::json::value_to<float>(tint_obj.at("g"));
			tint.b = boost::json::value_to<float>(tint_obj.at("b"));
			tint.a = boost::json::value_to<float>(tint_obj.at("a"));
		}

		// Десериализация offsetUV
		if (obj.contains("offsetUV")) {
			const auto& offsetUV_obj = obj.at("offsetUV").as_object();
			offsetUV.x = boost::json::value_to<float>(offsetUV_obj.at("x"));
			offsetUV.y = boost::json::value_to<float>(offsetUV_obj.at("y"));
		}

		// Десериализация scaleUV
		if (obj.contains("scaleUV")) {
			const auto& scaleUV_obj = obj.at("scaleUV").as_object();
			scaleUV.x = boost::json::value_to<float>(scaleUV_obj.at("x"));
			scaleUV.y = boost::json::value_to<float>(scaleUV_obj.at("y"));
		}

		// Десериализация priority
		if (obj.contains("priority")) {
			priority = boost::json::value_to<int>(obj.at("priority"));
		}

		return *this;
	}

	boost::json::object Data::serialize() const
	{
		boost::json::object json_obj;

		// Сериализация id
		json_obj["id"] = id;

		// Сериализация settings
		json_obj["settings"] = settings.serialize();

		return json_obj;
	}

	Data& Data::deserialize(const boost::json::value& item)
	{
		if (!item.is_object()) {
			throw std::runtime_error("Expected a JSON object");
		}

		const auto& obj = item.as_object();

		// Десериализация id
		if (obj.contains("id")) {
			id = boost::json::value_to<std::string>(obj.at("id"));
		}

		// Десериализация settings
		if (obj.contains("settings")) {
			settings.deserialize(obj.at("settings"));
		}

		return *this;
	}
	
	boost::json::object Collection::serialize() const
	{
		boost::json::object json_obj;

		// Сериализация overlays
		boost::json::array overlays_array;
		for (const auto& overlay : overlays) {
			overlays_array.push_back(overlay.serialize());
		}
		json_obj["overlays"] = overlays_array;

		// Сериализация remove
		boost::json::array remove_array;
		for (const auto& item : remove) {
			remove_array.emplace_back(item);
		}
		json_obj["remove"] = remove_array;

		return json_obj;
	}

	Collection& Collection::deserialize(const boost::json::value& item)
	{
		if (!item.is_object()) {
			throw std::runtime_error("Expected a JSON object");
		}

		const auto& obj = item.as_object();

		// Десериализация overlays
		if (obj.contains("overlays")) {
			const auto& overlays_array = obj.at("overlays").as_array();
			for (const auto& overlay_item : overlays_array) {
				Data data;
				data.deserialize(overlay_item);
				overlays.insert(data);  // Добавляем десериализованный объект в набор
			}
		}

		if (obj.contains("remove")) {
			const auto& overlays_array = obj.at("remove").as_array();
			for (const auto& overlay_item : overlays_array) {
				remove.insert(overlay_item.as_string().c_str());
			}
		}

		return *this;
	}
}
