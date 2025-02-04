#pragma once
#include <NAFicator/NAFicate.h>
extern ini::map inimap;
extern bool voodoo_ready;
extern void MessageHandler(F4SE::MessagingInterface::Message* a_msg);
extern void outputCachedMessage();

void AfterGameDataReady()
{
	LOG("Do things after game data ready ...");
	fixes::hkxficate();
	fixes::remove_missed_animations();
	fixes::check_and_clear_furniture_bad_links();
	fixes::update_tags();
	fixes::print_all();
	outputCachedMessage();
}

bool HkxFileExists(const std::string& filename)
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
	
	std::filesystem::path filePath = std::filesystem::current_path() / "Data" / "meshes" / filename;

	// Проверка существования файла в файловой системе
	if (std::filesystem::exists(filePath)) {
		//logger::info("'FileExists' true (loose) : {}", filename);
		return true;
	}

	// Проверка наличия файла в архиве
	RE::BSTSmartPointer<RE::BSResource::Stream, RE::BSTSmartPointerIntrusiveRefCount> a_result = nullptr;
	auto files = RE::BSResource::GetOrCreateStream(("meshes/" + filename).c_str(), a_result);

	if (a_result) {
		a_result->DoClose();
	}

	return LogAndReturn(filename, files);
}

bool process_file(const std::string& p_src, std::vector<XMLfile>& xmls, std::vector<std::string>& skipped)
{
	try {
		XMLfile xml_file(static_cast<const std::filesystem::path>(p_src));
		if (xml_file.has_value()) {
			xmls.push_back(xml_file);
			return true;
		} else {
			skipped.push_back(p_src + " : has no value");
		}
	} catch (const std::exception& e) {
		LOG("Error processing {}: {}", p_src, e.what());
	} catch (...) {
		LOG("Unknown error processing {}", p_src);
	}
	return false;
}

// Функция для обработки файлов
void parse_files(const std::filesystem::path& from, std::vector<XMLfile>& xmls, std::vector<std::string>& skipped)
{
	xmls.clear();
	skipped.clear();

	auto RaceDataFile = std::filesystem::current_path() / "Data" / "NAFicator" / "NAF_raceData_Fallout4.xml";
	
	for (const auto& entry : std::filesystem::directory_iterator(from)) {
		auto p_src = entry.path().string();
		if (!RaceDataFile.empty()) {
			if (std::filesystem::exists(RaceDataFile) && std::filesystem::file_size(RaceDataFile) >= 10) {
				process_file(RaceDataFile.string(), xmls, skipped);
				RaceDataFile.clear();
			} else {
				LOG("ERROR missed or empty {}", RaceDataFile.string());
			}
		}

		try {
			if (entry.file_size() < 10) {
				skipped.emplace_back(p_src + " : < 10 bytes");
				continue;
			}
			if (entry.path().extension() != ".xml") {
				skipped.emplace_back(p_src + " : not .xml");
				continue;
			}
			process_file(p_src, xmls, skipped);
		} catch (const std::exception& e) {
			LOG("Error processing entry {}: {}", p_src, e.what());
		} catch (...) {
			LOG("Unknown error processing entry {}", p_src);
		}
	}
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

RE::TESForm* get_form_by_editor_id(std::string editor_id)
{
	RE::TESForm* obj;
	obj = obj->GetFormByEditorID(editor_id);
	return obj;
}


void START(const std::filesystem::path& from)
{
	std::vector<XMLfile> store;
	std::vector<std::string> skipped;	

	parse_files(from, store, skipped);

	/*for_each_file(store, [&](XMLfile& x) {
		LOG("{} - {}", x.filename(), x.get_root());	
	},"");*/

	LOG("Start parsing data...");

	std::vector<std::shared_ptr<std::stringstream>> xfiles;
	xfiles.reserve(store.size());

	for_each_file(
		store, [&](XMLfile& x) {
			xfiles.emplace_back(x.make_stringstream());		
	},
	"");

	fixes::merge_positions_optionals();

	NAFicator::parse_XML_files(xfiles);
	if (voodoo_ready)
		AfterGameDataReady();
	voodoo_ready = true;

	//=================================================
	/*if (inimap.get<bool>("bRaceData", "General"))
		for_each_file(patch_raceData, "raceData");
	if (inimap.get<bool>("bFurnitureData", "General"))
		for_each_file(patch_remove_missing_furniture, "furnitureData");
	if (inimap.get<bool>("bAnimationData", "General"))
		for_each_file(patch_replace_forms_to_hkx, "animationData");
	if (inimap.get<bool>("bRemoveWrongNodes", "General")) {
		for_each_file(patch_remove_missed_links, "animationGroupData");
		for_each_file(patch_remove_missed_links, "positionData");
		for_each_file(patch_remove_missed_links, "positionTreeData");
	}
	if (inimap.get<bool>("bFixTags", "General"))
		for_each_file(patch_fix_nofurn_and_bed_tags, "positionData");
	if (inimap.get<int>("iRemoveFixWrongZOffsets", "General") && inimap.get<float>("fzOffsetMax", "General") > 0)
		for_each_file(patch_fix_wrong_offset);

	for (auto& x : xmls) {
		std::thread([&]() {
			if (x.has_value())
				x.save_to_file(to);
		}).join();
	}*/

	/*for (auto& r : XMLfile::removed_nodes) {
			logger::info("REMOVED ARRAY : {},{},{} ", r.first, r.second.first, r.second.second);
	}*/

}

void for_each_file(std::vector<XMLfile>& xmls, std::function<void(XMLfile&)> apply, const std::string& rootNode)
{
	std::vector<std::future<void>> futures;  // Вектор для хранения будущих задач
	for (auto& x : xmls) {
		if (x.has_value() && (rootNode.empty() || (rootNode == x.get_root()))) {
				apply(x);
		}
	}
}
//
//void patch_raceData(XMLfile& x)
//{
//	static std::filesystem::path* path_to_naficator_race_data = nullptr;
//
//	auto fix_raceData = [&](std::list<std::string>::iterator& it, XMLfile*)->std::list<std::string>::iterator& {
//		
//		if (!path_to_naficator_race_data) {
//			path_to_naficator_race_data = new std::filesystem::path(std::filesystem::current_path().string() + "\\Data\\NAFicator\\NAF_raceData_Fallout4.xml"s);
//			path_to_naficator_race_data->make_preferred();
//			if (!std::filesystem::exists(*path_to_naficator_race_data)) {
//				logger::critical("'patch_raceData' : no raceData file {}", path_to_naficator_race_data->string());
//				return it;
//			}
//			utils::copy_file(path_to_naficator_race_data->string(), (std::filesystem::path(std::filesystem::current_path().string() + "\\Data\\NAF"s)).string());
//		}
//		
//		if (auto node = utils::get_node_name(*it); node == "defaults"s) {
//			utils::remove_attribute(*it, "femaleSWF");
//			utils::remove_attribute(*it, "maleSWF");
//			utils::remove_inline_spaces(*it);
//			return it;
//		}
//		else if (node != "race"s)
//			return it;
//
//		if (inimap.get<bool>("bOverrideRaceData", "General")) {
//
//			
//			if (auto t = RE::TESDataHandler::GetSingleton()->LookupModByName(get_source(x, it)); !t) {
//				it->clear();
//				return it;
//			}
//			
//			auto search_skeleton_in_base_by_value = [&](std::string& skeleton_value) {
//				if (skeleton_value.empty())
//					return ""s;
//					
//				std::fstream base(*path_to_naficator_race_data, std::ios::in);
//				if (auto e = base.rdstate(); e != 0) {
//					base.close();
//					base.clear();
//					base.seekg(0);
//					base.open(*path_to_naficator_race_data);
//					if (e = base.rdstate(); e != 0) {
//						logger::critical("'patch_raceData' : couldn't read from {}, e : {}", path_to_naficator_race_data->string(), e);
//						base.close();
//						return ""s;
//					}
//				}
//
//				std::string base_str(""s);
//				while (!base.eof()) {
//					getline(base, base_str);
//					
//					if (utils::get_attribute_value(base_str, "skeleton") == skeleton_value) {
//						break;
//					} else {
//						base_str.clear();
//					}
//				}
//				base.close();
//				return base_str;
//			};
//			
//			auto change_attributes = [](std::string& str, std::string& base) {
//				auto base_a = utils::get_all_attributes(base);
//				auto str_a = utils::pop_all_attributes(str);
//
//				for (auto& s : str_a) {
//					if (s.first == "form")
//						utils::add_attribute(str, s.first, s.second);
//				}
//
//				for (auto& s : base_a) {
//					if (s.first != "form")
//						utils::add_attribute(str, s.first, s.second);
//				}
//
//				utils::remove_inline_spaces(str);
//			};
//
//			
//			if (auto val = utils::get_attribute_value(*it, "skeleton"s); !val.empty()) {
//				if (auto base_str = search_skeleton_in_base_by_value(val); !base_str.empty()) {
//					change_attributes(*it, base_str);
//					return it;
//				}
//			}
//		}
//		
//		utils::replace(*it, "requiresAnimationReset", "requiresReset");
//		utils::remove_attribute(*it, "femaleSWF");
//		utils::remove_attribute(*it, "maleSWF");
//		utils::remove_attribute(*it, "id");
//
//		auto all_attributes = utils::get_all_attributes(*it);
//		if (all_attributes.size() == 1) {
//			it->clear();
//		}
//
//		if (!it->empty()) {
//			utils::remove_inline_spaces(*it);
//		}
//		return it;
//	};
//
//	x.for_each_string(fix_raceData);
//}
//
//std::string get_source(XMLfile& x, std::list<std::string>::iterator& it)
//{
//	if (it->empty())
//		return ""s;
//	std::string res = x.get_attribute_value(*it, "idleSource");
//	if (res.empty())
//		res = x.get_attribute_value(*it, "source");
//
//	return utils::remove_escape_sequence(res);
//};
//
//void patch_replace_forms_to_hkx (XMLfile& x) {
//
//	auto forms_to_hkx = [](std::list<std::string>::iterator& it, XMLfile* x)->std::list<std::string>::iterator& {
//
//		auto get_hkx = [](std::list<std::string>::iterator& it, const std::string& form, const std::string src, XMLfile* x, bool& removed) {
//
//			auto remove = [&] {
//				if (inimap.get<bool>("bRemoveWrongNodes", "General")) {
//					removed = x->remove_this_node(it);
//					logger::info("'patch_replace_forms_to_hkx' : delete bad node {} Node : {}", removed ? "success!"s : "failed..."s, *it);
//				} else
//					logger::critical("'patch_replace_forms_to_hkx' : no valid form in file : {}\n string : {}", x->get_file().string(), *it);
//			};
//			
//			auto CAST = [](std::shared_ptr<Form> frm) -> IdleForm* {
//				if (frm)
//				{
//					return dynamic_cast<IdleForm*>(frm.get());
//				}
//				return nullptr;
//			};
//			if (std::shared_ptr<Form> f = RE::TESDataHandler::GetSingleton()->LookupModByName(src) ? extract(make_form<Form>(form, src)) : nullptr; CAST(f)) {
//				auto hkx = reinterpret_cast<IdleForm*>(f.get())->hkx();
//				if (hkx.empty()) {
//					logger::warn("'patch_replace_forms_to_hkx' : empty idle. This can be okay, it can be stop idle : {}\n string : {}", x->get_file().string(), *it);
//				} else if (HkxFileExists(hkx)) {
//					return hkx;
//				} else {
//					put(make_form<IdleForm>(form, src, hkx));
//					remove();
//					logger::error("'patch_replace_forms_to_hkx' : no such file : {}\n, form : {}, source : {}, xml : {}", hkx, form, src, x->get_file().string());
//				}
//				return ""s;
//			} else if (RE::TESIdleForm* tesform = static_cast<RE::TESIdleForm*>(get_form_from_string(form, src)); tesform) {
//				if (tesform->formType != RE::ENUM_FORM_ID::kIDLE) {
//					logger::error("'patch_replace_forms_to_hkx' : form is not IDLE form : {}, source : {}, xml : {}", form, src, x->get_file().string());
//					remove();
//					return ""s;
//				} else if (std::string hkx = std::string(tesform->animFileName.c_str()); hkx.empty()) {
//					return hkx;
//				} else if (HkxFileExists(hkx)) {
//					return hkx;
//				} else {
//					put(make_form<IdleForm>(form, src, hkx));
//					remove();
//					logger::error("'patch_replace_forms_to_hkx' : no such file : {}\n, form : {}, source : {}, xml : {}", hkx, form, src, x->get_file().string());
//				}
//				return ""s;
//			} else {
//				remove();
//				return ""s;
//			}
//		};
//
//		bool removed = false;
//		std::string t = utils::get_node_name(*it);
//
//		if (t == "actor") {
//			if (std::string idleForm(x->get_attribute_value(*it, "idleForm"s)), src(get_source(*x, it)); !idleForm.empty() && !src.empty()) {
//				auto hkx = get_hkx(it, idleForm, src, x, removed);
//				if (!removed) {
//					utils::remove_attribute(*it, "idleForm"s);
//					utils::remove_attribute(*it, "source");
//					utils::add_attribute(*it, "file", hkx);
//					utils::remove_inline_spaces(*it);
//					put(make_form<IdleForm>(idleForm, src, hkx));
//				}
//			}
//		} else if (t == "idle") {
//			if (std::string form(x->get_attribute_value(*it, "form"s)), src(get_source(*x, it)); !form.empty() && !src.empty()) {
//				auto hkx = get_hkx(it, form, src, x, removed);
//				if (!removed) {
//					utils::remove_attribute(*it, "form"s);
//					utils::remove_attribute(*it, "source");
//					auto attributes = utils::get_all_attributes(*it);
//					it->clear();
//					while (it->empty() || (utils::get_node_name(*it) != "actor"s)) {
//						if (it->empty() && it == x->buffer.begin()) {
//							x->set_critical_error("'patch_replace_forms_to_hkx' : out of range exception, :\n"s + x->get_file().string());
//							return it;
//						}
//						--it;
//					}
//					for (auto p : attributes) {
//						utils::add_attribute(*it, p.first, p.second);
//					}
//					utils::add_attribute(*it, "file"s, hkx);
//					utils::remove_inline_spaces(*it);
//					put(make_form<IdleForm>(form, src, hkx));
//				}
//			}
//		}
//		return it;
//	};
//
//	x.for_each_string(forms_to_hkx);
//}
//
//void patch_remove_missing_furniture(XMLfile& x)
//{
//	auto remove_missing = [](std::list<std::string>::iterator& it, XMLfile* x) -> std::list<std::string>::iterator& {
//		auto remove = [&](std::string& str) {
//			logger::info("'patch_remove_missing_furniture' : removed furniture node in file : {}, node : \n{}, source : {}", x->get_file().string(), str, get_source(*x, it));
//			str.clear();
//		};
//
//		if (utils::get_node_name(*it) != "furniture"s)
//			return it;
//		
//		std::string& str = *it;
//		auto frm = utils::get_attribute_value(str, "form"s);
//		if (frm.empty()) {
//			remove(*it);
//			
//		} else if (auto src = get_source(*x, it); src.empty()) {
//			remove(*it);
//		} else if (auto tesform = get_form_from_string(frm, src); tesform) {
//			return it;
//		} else if (!tesform) {
//			if (auto edid = utils::get_attribute_value(*it, "id"s); !edid.empty()) {
//				edid = utils::remove_escape_sequence(edid);
//				if (tesform = get_form_by_editor_id(edid); !tesform) {
//					remove(*it);
//				} else {
//					utils::remove_attribute(str, "form"s);
//					utils::add_attribute(str, "form"s, utils::to_hex_string(tesform->formID));
//				}
//
//			} else {
//				remove(*it);
//			}
//		}
//		return it;
//	};
//
//	x.for_each_string(remove_missing);
//}
//
//#define VALUE second.second
//#define NODE second.first
//#define ROOT first
//void patch_remove_missed_links(XMLfile& x)
//{	
//	auto root = x.get_root();
//	std::vector<std::string> root_nodes;
//	std::vector<std::string> nodes;
//	std::vector<std::string> attributes_names;
//
//	auto get_value = [&](std::string& str) {
//		std::string res("");
//		for (auto& a : attributes_names) {
//			if (res = utils::get_attribute_value(str, a); !res.empty())
//				return res;
//		}
//		return res;
//	};
//
//
//
//	auto remove_missed_animation_links = [&](std::list<std::string>::iterator& it, XMLfile* x) -> std::list<std::string>::iterator&
//	{
//		auto validate = [](const std::string& val, std::vector<std::string> arr) {
//			for (auto& n : arr) {
//				if (n == val)
//					return true;
//			}
//			return false;
//		};
//		
//		if (get_value(*it).empty())
//			return it;
//		
//		for (auto& n : XMLfile::removed_nodes) {
//			if (validate(n.ROOT, root_nodes)) {
//				if (validate(utils::get_node_name(*it), nodes)) {
//					if (get_value(*it) == n.VALUE) {
//						x->remove_this_node(it);
//					}
//				}
//			}
//		}
//		return it;
//	};
//
//	if (root == "animationGroupData"s) {
//		root_nodes.push_back("animationData"s);
//		nodes.push_back("stage");
//		attributes_names.push_back("animation"s);
//		x.for_each_string(remove_missed_animation_links);
//	} 
//	else if (root == "positionData"s) {
//		root_nodes.push_back("animationData"s);
//		root_nodes.push_back("animationGroupData"s);
//		nodes.push_back("position");
//		attributes_names.push_back("animation"s);
//		attributes_names.push_back("animationGroup"s);
//		attributes_names.push_back("id");
//		x.for_each_string(remove_missed_animation_links);
//		
//	} else if (root == "positionTreeData"s) {
//		root_nodes.push_back("positionData"s);
//		nodes.push_back("branch");
//		attributes_names.push_back("positionID"s);
//		attributes_names.push_back("position"s);
//		x.for_each_string(remove_missed_animation_links);
//	}
//}
//#undef VALUE
//#undef NODE
//#undef ROOT
//
//void patch_fix_wrong_offset(XMLfile& x)
//{
//	auto proccess_offset_value = [&](std::string& value) {
//		std::vector<std::string> xyza;
//		utils::delim(value, ","s, xyza);
//
//		if (!xyza.size()) {
//			return ""s;
//		} else if (xyza.size() < 3 || xyza.size() > 4) {
//			logger::warn("'fix_offset' : wrong offset count, file : {}", x.filename());
//			return ""s;
//		} else {
//			float z = 0;
//			try {
//				z = std::stof(xyza[2]);
//			} catch (...) {
//				logger::warn("'fix_offset' : wrong offset data, file : {}", x.filename());
//				return ""s;
//			}
//
//			if (z > inimap.get<float>("fzOffsetMax", "General")) {
//				if (xyza.size() >= 3) {
//					if (auto mode = inimap.get<int>("iRemoveFixWrongZOffsets", "General"); mode == 1) {
//						return (xyza[0] + ',' + xyza[1] + ',' + "0"s + ',' + xyza[2]);
//					} else if (mode == 2) {
//						return (xyza[0] + ',' + xyza[1] + ',' + "0"s);
//					}
//				}
//			}
//		}
//		return ""s;
//	};
//
//	auto fix_leito = [&](std::string& str, std::string& offset) {
//		if (offset == "0,-3,2,0,0:0,0,0,0"s) {  //leito fixes
//			utils::replace(str, offset, "0,-3.2,0,0:0,0,0,0"s);
//			offset = "0,-3.2,0,0:0,0,0,0"s;
//		} else if (offset == "0,1,2,0,0:0,0,0,0"s) {  //leito fixes
//			utils::replace(str, offset, "0.1,2,0,0:0,0,0,0"s);
//			offset = "0.1,2,0,0:0,0,0,0"s;
//		}
//	};
//	
//	auto fix_offset = [&](std::list<std::string>::iterator& it, XMLfile*) -> std::list<std::string>::iterator& {
//		if (auto node = utils::get_node_name(*it); node == "animationOffset"s) {
//			if (auto offset = utils::get_attribute_value(*it, "offset"s); !offset.empty()) {
//				if (auto v = proccess_offset_value(offset); !v.empty()) {
//					utils::remove_attribute(*it, "offset"s);
//					utils::add_attribute(*it, "offset"s, v);
//					utils::remove_inline_spaces(*it);
//				}
//			}
//		}
//		else if (node == "defaults" || node == "animation") {
//			if (auto offset = utils::get_attribute_value(*it, "offset"s); !offset.empty()) {
//				if (node == "animation")
//					fix_leito(*it, offset);
//				std::vector<std::string> offset_arr;
//				utils::delim(offset, ":"s, offset_arr);
//				for (auto& o : offset_arr) {
//					if (auto v = proccess_offset_value(o); !v.empty()) {
//						o = v;
//					}
//				}
//
//				std::string new_offset_value("");
//
//				for (auto o = offset_arr.begin(); o != offset_arr.end(); ++o) {
//					new_offset_value += *o;
//					if (o != offset_arr.end() - 1)
//						new_offset_value += ":"s;
//				}
//
//				utils::remove_attribute(*it, "offset"s);
//				utils::add_attribute(*it, "offset"s, new_offset_value);
//				utils::remove_inline_spaces(*it);
//			}
//		}
//		return it;
//	};
//
//	if (x.get_root() == "positionData"s || x.get_root() == "animationData"s)
//		x.for_each_string(fix_offset);
//}
//
//void patch_fix_nofurn_and_bed_tags(XMLfile& x)
//{
//	auto run_fixes = [&](std::list<std::string>::iterator& it, XMLfile*) -> std::list<std::string>::iterator& {
//		if (utils::get_node_name(*it) == "position"s)
//		{	
//			/*std::string _log("");
//			std::size_t _lcount = 0;
//
//			auto log = [&](std::string& what) {
//				if (_lcount)
//					_log += '\n';
//				_log += '\t' + what;
//			};*/
//
//			std::string loc("");
//			std::vector<std::string> locations;
//			std::vector<std::string> tags;
//			std::vector<std::string> all_remove_tags = { "singlebed",
//				"doublebed",
//				"mattress", "strippole" };
//
//			bool l_parsed = false;
//			auto get_locations = [&]() -> std::vector<std::string>& {
//				if (!l_parsed) {
//					auto loc = utils::get_attribute_value(*it, "location"s);
//					loc = utils::to_lower_case(loc);
//					utils::delim(loc, ","s, locations), l_parsed = true;
//				}
//				return locations;
//			};
//
//			bool t_parsed = false;
//			auto get_tags = [&]() -> std::vector<std::string>& {
//				if (!l_parsed) {
//					auto tag = utils::get_attribute_value(*it, "location"s);
//					tag = utils::to_lower_case(tag);
//					utils::delim(utils::get_attribute_value(*it, "tags"s), ","s, tags), t_parsed = true;
//				}
//				return tags;
//			};
//
//			auto has_tag = [&](const std::string& tag) {
//				if (tags.empty())
//					get_tags;
//				for (auto& t : tags) {
//					if (t == tag)
//						return true;
//				}
//				return false;
//			};
//
//			auto tdelete = [&](const std::string& tag) {
//				if (tags.empty())
//					get_tags;
//				for (auto it = tags.begin(); it != tags.end();) {
//					if (*it == tag) {
//						it = tags.erase(it);
//						continue;
//					}
//					++it;
//				}
//			};
//
//			auto tinsert = [&](const std::string& tag) {
//				if (tags.empty())
//					get_tags;
//				if (tags.empty())
//					tags.insert(tags.begin(), tag);
//				else
//					tags.insert(tags.end(), ',' + tag);
//			};
//
//			auto patch_nofurn = [&]() {
//				auto& l = get_locations();
//
//				auto has_nofurn = has_tag("nofurn");
//				if (l.empty() && !has_nofurn)
//					tinsert("nofurn");
//				else if (!l.empty() && has_nofurn)
//					tdelete("nofurn");
//			};
//
//			auto remove_tags = [&]() {
//				for (auto it = get_tags().begin(); it != get_tags().end();) {
//					bool r = false;
//					for (auto& t : all_remove_tags) {
//						if (*it == t) {
//							it = get_tags().erase(it);
//							r = true;
//						}
//					}
//					if (r)
//						continue;
//					++it;
//				}
//			};
//
//			auto patch_beds_tags = [&]() {
//				remove_tags();
//				for (auto& i : get_locations()) {
//					if (i == "single_bed") {
//						tinsert("singlebed");
//					} else if (i == "double_bed") {
//						tinsert("doublebed");
//					} else if (i == "mattress") {
//						tinsert("mattress");
//					} else if (i == "strip_pole") {
//						tinsert("strippole");
//					}
//				}
//			};
//
//			auto remove_actors_tag = [&]() {
//				auto& tg = get_tags();
//				if (tg.empty())
//					return;
//
//				auto isF_M = [](const std::string& tag) {
//					
//					if (auto it = tag.begin(); *it == 'f' || *it == 'm') {
//						if (++it; *it == '_' || it == tag.end()) {
//							if (it = tag.end() - 1; *it == 'f' || *it == 'm') {
//								if (it == tag.begin()) {
//									return true;
//								} else if (--it; *it == '_') {
//									return true;
//								}
//							}
//						}
//					}
//					return false;
//				};
//
//				for (auto it = tg.begin(); it != tg.end();) {
//					if (isF_M(*it)) {
//						it = tags.erase(it);
//						continue;
//					}
//					++it;
//				}
//			};
//
//			// DO NOT WORK
//			/*std::mutex l; 
//			int current_priority = x.contents.get_priority();
//			auto get_priority_tags = [&](XMLfile& xml){
//				if (xml.contents.get_priority() < current_priority)
//					return;
//
//				auto nodes = xml.contents.get_all_nodes();
//				if (nodes.size()) {
//					auto id = utils::get_attribute_value(*it, "id");
//					for (auto& n : nodes) {
//						auto i = n.second.first;
//						auto end = n.second.second;
//						while (i != end)
//						{
//							if (utils::get_attribute_value(*i, "position") == id) {
//								std::unique_lock lck(l);
//								if (xml.contents.get_priority() < current_priority)
//									return;
//								utils::remove_attribute(*it, "tags"s);
//								utils::add_attribute(*it, "tags"s, utils::get_attribute_value(*i, "tags"s));
//								current_priority = xml.contents.get_priority();
//								logger::info("current priority : {}, updated tags=\"{}\"", current_priority, utils::get_attribute_value(*it, "tags"s));
//								return;
//							}
//							++i;
//						}
//					}
//				}
//			};*/
//
//			
//			//DO NOT WORK
//			/*auto patch_tags_from_tagdata = [get_priority_tags]() {
//				for_each_file(get_priority_tags, "tagData");
//			};
//
//			logger::info("start update tags");
//			patch_tags_from_tagdata();
//			logger::info("end update tags");*/
//			
//			patch_nofurn();
//			patch_beds_tags();
//			utils::remove_inline_spaces(*it);
//		}
//		return it;
//	};
//
//	x.for_each_string(run_fixes);
//}
//
//std::shared_ptr<std::pair<XMLfile*, std::pair<buf_iterator, buf_iterator>>> find_key_node
//	(const std::string& root_node, const std::string& attr, const std::string& val)
//{
//	std::shared_ptr<std::pair<XMLfile*, std::pair<buf_iterator, buf_iterator>>> result = nullptr;
//	if (!XMLfile::is_valid_root(root_node))
//		return result;
//
//	auto set_result = [&](XMLfile* x, buf_iterator& b, buf_iterator& e) {
//		auto beg = x->buffer.begin();
//		--beg;
//		if (x && b != beg && e != x->buffer.end()) {
//			if (!result.get()) {
//				result = std::shared_ptr<std::pair<XMLfile*, std::pair<buf_iterator, buf_iterator>>>(new std::pair(x, std::pair(b, e)));
//			} else {
//				result->first = x;
//				result->second.first = b;
//				result->second.second = e;
//			}
//		}
//	};
//
//	std::string key;
//	try {
//		key = XMLfile::valid_root_nodes.at(root_node).first;
//	}
//	catch (...)
//	{
//		return result;
//	}
//	
//	int _priority = INT_MIN;
//
//	auto proccess_string = [&](XMLfile& x) {
//		auto& buf = x.buffer;
//		auto end = buf.end();
//		int priority = INT_MIN;
//
//		auto is_one_string_node = [&](buf_iterator& it) {
//			if (utils::get_node_name(*(it)) == key)
//				if (it->find("/>") != std::string::npos || it->find("</" + key + '>') != std::string::npos)
//					return true;
//			return false;
//		};
//
//		auto get_begin = [&](buf_iterator& it) {
//			auto before_beg = buf.begin();
//			--before_beg;
//			while (utils::get_node_name(*it) != key && it != before_beg) {
//				--it;
//			}
//			return it;
//		};
//
//		auto get_end = [&](buf_iterator& it) {
//			auto end_key = "</" + key + '>';
//			while (utils::get_node_name(*(it++)) != key && it != end);
//			return it;
//		};
//
//		try {
//			priority = std::stoi(x.get_defaults("priority"),nullptr, 10);
//		}
//		catch (...)
//		{
//			logger::info("'find_key_node : proccess_string' failed read priority, file : {}", x.filename());
//			return;
//		}
//
//		if (priority <= _priority)
//			return;
//
//		for (auto it = buf.begin(); it != end; ++it)
//		{
//			if (utils::get_attribute_value(*it, attr) == val)
//			{
//				buf_iterator tmp_beg;
//				buf_iterator tmp_end;
//				if (is_one_string_node(it)) {
//					tmp_end = ++tmp_beg;
//					set_result(&x, tmp_beg, tmp_end);
//					return;
//				}
//			}
//		}
//	};
//
//	for_each_file(proccess_string, root_node);
//
//	return result;
//}


bool HkxFileExists(std::string filename)
{
	//	//boost::replace_all(filename, "\\", "/");
	//	//filename = ("Data/meshes/" + filename);
	//
	//	if (std::filesystem::exists(boost::replace_all_copy(std::filesystem::current_path().string() + "/Data/meshes/"s + filename, "\\", "/"))) {
	//		//logger::info("'FileExists' true (loose) : {}", filename);
	//		return true;
	//	}
	//
	//	RE::BSTSmartPointer<RE::BSResource::Stream, RE::BSTSmartPointerIntrusiveRefCount> a_result = nullptr;
	//	auto files = RE::BSResource::GetOrCreateStream(("meshes/" + filename).c_str(), a_result);
	//
	//	if (a_result.get())
	//		a_result->DoClose();
	//
	//	switch (files) {
	//		case RE::BSResource::ErrorCode::kNone: {
	//			//logger::info("'FileExists' true (archived) : {}", filename);
	//			return true;
	//		}
	//		case RE::BSResource::ErrorCode::kNotExist: {
	//			logger::info("'FileExists' false : {} - kNotExist", filename);
	//			return false;
	//		}
	//		case RE::BSResource::ErrorCode::kInvalidPath: {
	//			logger::info("'FileExists' false : {} - kInvalidPath", filename);
	//			return false;
	//		}
	//		case RE::BSResource::ErrorCode::kFileError: {
	//			logger::info("'FileExists' true (archived) : {} - kFileError", filename);
	//			return true;
	//		}
	//		case RE::BSResource::ErrorCode::kInvalidType: {
	//			logger::info("'FileExists' true (archived) : {} - kInvalidType", filename);
	//			return true;
	//		}
	//		case RE::BSResource::ErrorCode::kMemoryError: {
	//			logger::info("'FileExists' true (archived) : {} - kMemoryError", filename);
	//			return true;
	//		}
	//		case RE::BSResource::ErrorCode::kBusy: {
	//			logger::info("'FileExists' true (archived) : {} - kBusy", filename);
	//			return true;
	//		}
	//		case RE::BSResource::ErrorCode::kInvalidParam: {
	//			logger::info("'FileExists' true (archived) : {} - kInvalidParam", filename);
	//			return true;
	//		}
	//		case RE::BSResource::ErrorCode::kUnsupported: {
	//			logger::info("'FileExists' true (archived) : {} - kUnsupported", filename);
	//			return true;
	//		}
	//		default: {
	//			logger::info("'FileExists' false : {} - this should never happens", filename);
	//			return false;
	//		}
	//	}
	//}

//	void parse_files(const std::filesystem::path& from, std::vector<XMLfile> xmls, std::vector<std::string> skipped)
//{
//	xmls.clear();
//	for (auto& entry : std::filesystem::directory_iterator(from.c_str())) {
//		std::thread([&]() {
//			std::filesystem::path p_src = entry.path();
//			p_src.make_preferred();
//			if (entry.file_size() < 10)
//				return;  //continue
//			if (p_src.extension() != ".xml")
//				return;  //continue
//
//			//std::filesystem::path p_dest = (where + '/' + p_src.filename().string());
//
//			//if ((ini.get<bool>("bReplaceIfFileExist", "General") ? false : std::filesystem::exists(p_dest)) && !ignore_skip)
//			//	continue;
//
//			XMLfile file(p_src);
//			if (file.has_value())
//				xmls.push_back(file);
//			else
//				skipped.push_back(p_src.string());
//		}).join();
//	}
}
