#pragma once
#pragma warning(disable:4996)
#include <IniParser/Ini.h>
#include "NAFicator/XML/XMLFile.h"
#include "NAFicator/XML/utils.h"
#include "utils/utility.h"
#include <fstream>
#include "NAFicator/Version.h"
#include <NAFicator/Objects/objects_map.h>
#include <NAFicator/Version.h>
#include <NAFicator/SimpleLog/slog.hpp>


extern ini::map inimap;
#define LOG slog::getInstance()

XMLfile::XMLfile(const std::filesystem::path& source_file) :
	source(source_file)
{
	LOG("Opening file {} ...", source_file.filename().string());

	if (!std::filesystem::exists(source)) {
		source = "";
		return;
	}

	if (std::filesystem::is_directory(source)) {
		source = "";
		return;
	}

	if (source.extension().string() != ".xml"s) {
		source = "";
		return;
	}
	
	std::ifstream read(source);
	if (!read.is_open()) {
		source = "";
		return;
	}

	std::string line;
	while (!read.eof()) {
		line.clear();
		getline(read, line);
		if (!line.empty())
			buffer.push_back(line);
	}
	read.close();
	normalize();

	if (!hasValue)
		LOG("normalize... success\nprepare to make objects...");
	else
		LOG("{} normalize... failed, file skipped", filename());
}

void XMLfile::for_each_string(std::function<void(std::vector<std::string>::iterator&, XMLfile*)> do_work_with_string)
{
	if (this) {
		for (auto it = buffer.begin(), end = buffer.end(); it != end; ++it) {
			if (!it->empty()) {
				do_work_with_string(it, this);
			} else {
				continue;
			}
		}
	}
}

bool XMLfile::normalize()
{
	if (this == nullptr)
		return false;

	std::string dataSet("");
	bool changed_meta_tag = false;

	auto set_sign = [this]() {
		auto it = buffer.begin();
		utils::xml::remove_inline_spaces(*it);
		auto attrs = utils::xml::pop_all_attributes(*it);

		utils::xml::add_attribute(*it, "meta"sv, std::string(ver::NAME.begin(), ver::NAME.end()));

		for (auto& a : attrs) {
			if (a.first == "dataSet")
				utils::xml::add_attribute(*it, a.first, a.second);
		}
	};

	auto change_metatag = [&, this](std::vector<std::string>::iterator& it, XMLfile*) {
		if (!(it == std::vector<std::string>::iterator()))
			return;
		if (!changed_meta_tag) {
			if (dataSet = utils::xml::get_attribute_value(*it, "dataSet"); !dataSet.empty()) {
				changed_meta_tag = utils::string::replace(*it, "<meta "s, "<" + dataSet + "Data "s);

				if (size_t f = it->find("/>"); f != std::string::npos)
					it->erase(f, 1);

				auto get_last_line_tag = [this](const std::string& what) {
					auto s = buffer.rbegin();
					for (; s != buffer.rend() && s->empty(); ++s)
						;
					return s->find("</"s + what + ">"s) != std::string::npos;
				};

				if (!get_last_line_tag(dataSet + "Data"s))
					buffer.push_back("</" + dataSet + "Data>");
			}
		}
	};

	bool commentary_state = false;
	std::function<void(std::vector<std::string>::iterator & it, XMLfile * x)> clean_commentaries = [&](std::vector<std::string>::iterator& it, XMLfile* x) {
		if (it->empty())
			return it;
		auto& str = *it;
		if (commentary_state) {
			size_t commentary_end = str.find("-->");
			if (commentary_end != std::string::npos) {
				str.erase(0, (commentary_end + 3));
				commentary_state = false;
			} else {
				str.clear();
				commentary_state = true;
			}
		} else {
			size_t commentary_start = str.find("<!--");
			if (commentary_start != std::string::npos) {
				size_t commentary_end = str.find("-->");
				if (commentary_end != std::string::npos) {
					str.erase(commentary_start, ((commentary_end + 3) - commentary_start));
					commentary_state = false;
				} else {
					str.erase(commentary_start, str.length() - commentary_start);
					commentary_state = true;
				}
			} else {
				commentary_state = false;
			}
		}
		if (size_t commentary_start = str.find("<!--") != std::string::npos)
			clean_commentaries(it, x);
		return it;
	};

	auto remove_whitespaces = [&, this](std::vector<std::string>::iterator& it, XMLfile*) {
		/*if (!(it == std::vector<std::string>::iterator()))
			return;*/
		utils::xml::remove_inline_spaces(*it);
		utils::string::trim(*it, " \t\n\r"s);
	};

	auto fixes = [&, this](std::vector<std::string>::iterator& it) {
		/*if (!(it == std::vector<std::string>::iterator()))
			return;*/
		//rxl_bp70_anims_positionData.xml "tags= fix
		if (this->filename() == "rxl_bp70_anims_positionData.xml")
		{
			if (it->find("\"tags="))
			{
				utils::replace(*it, "\"tags="s, "tags=\""s);
			}
		}
	};

	std::unordered_set<std::string> empty_attributes;  // переменная для кэширования
	bool parsed_known_empty_attribute = false;         // Инициализация флага
	auto check_lexicography = [&, this](std::vector<std::string>::iterator& it, XMLfile*) {
		/*if (!(it == std::vector<std::string>::iterator()))
			return;*/
		fixes(it);
		
		auto i = it->begin();
		
		auto is_known_empty_attribute = [&](std::string_view attr) {
			// Проверяем, нужно ли обновить empty_attributes
			if (!parsed_known_empty_attribute) {
				parsed_known_empty_attribute = true;
				auto str = inimap.get<std::string>("sSkipEmptyAttributesInLog"s, "General"s);
				// Если строка не пустая, разбиваем её на атрибуты
				if (!str.empty()) {
					std::vector<std::string> attributes;
					utils::delim(str, ","s, attributes);
					// Вставляем атрибуты в unordered_set
					empty_attributes.insert(attributes.begin(), attributes.end());
				}
			}
			// Проверяем наличие атрибута
			return empty_attributes.find(std::string(attr)) != empty_attributes.end();
		};

		auto fill_pair = [&, this](std::string& val, std::string& attr, std::vector<std::pair<std::string, std::string>>& p) {
			if (!attr.empty() && !val.empty()) {
				// Добавляем пару (атрибут, значение) в вектор
				p.emplace_back(std::make_pair(attr, val));
			} else {
				// Проверяем, если атрибут не пустой и не является известным пустым атрибутом
				if (!attr.empty() && !is_known_empty_attribute(attr)) {
					std::ostringstream oss;
					oss << "'check_lexicography': no attribute/value in:\n"
						<< *it << ",\nfile: " << source.filename().string();
					send_warning(oss.str());
				}
			}
			// Очищаем атрибут и значение
			attr.clear();
			val.clear();
		};

		auto check_missed_space = [](std::string::iterator it, std::string& str) {
			// Проверяем, что итератор не указывает на последний элемент и что следующий символ является буквой
			if (*it == '\"' && std::next(it) != str.end() && std::isalpha(static_cast<unsigned char>(*(std::next(it))))) {
				// Вставляем пробел после кавычки
				it = str.insert(std::next(it), ' ');
				// Возвращаем итератор на предыдущую позицию
				--it;
			}
		};

		auto isSpace = [](const char& ch) {
			return (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');
		};

		auto get_prev_symb_without_spaces = [isSpace](std::string& str, std::string::iterator it) -> std::string::iterator {
			// Проверяем, что итератор не указывает на начало строки
			if (it == str.begin()) {
				return it;  // Возвращаем итератор, если он уже на начале строки
			}
			--it;  // Переходим к предыдущему символу
			while (it != str.begin() && isSpace(*it)) {
				--it;  // Продолжаем двигаться назад, пока не найдем не пробельный символ
			}
			// Если `it` указывает на начало строки и это пробел, возвращаем `it` (который указывает на начало)
			return it;
		};

		auto get_next_symb_without_spaces = [isSpace](std::string& str, std::string::iterator it) -> std::string::iterator {
			// Проверяем, что итератор не указывает на конец строки
			if (it == str.end()) {
				return it;  // Возвращаем итератор, если он уже на конце строки
			}
			++it;  // Переходим к следующему символу
			// Продолжаем двигаться вперед, пока не найдем не пробельный символ или не достигнем конца строки
			while (it != str.end() && isSpace(*it)) {
				++it;
			}
			return it;  // Возвращаем итератор на первый не пробельный символ или на конец строки
		};

		auto try_to_replace_slash = [get_prev_symb_without_spaces, get_next_symb_without_spaces](std::string& str, std::string::iterator& it) -> std::string::iterator {
			if (*it == '/') {
				auto prev = get_prev_symb_without_spaces(str, it);
				auto next = get_next_symb_without_spaces(str, it);

				// Проверяем, что prev не указывает на начало строки и next не указывает на конец строки
				if (prev == str.begin() || next == str.end()) {
					return it;  // Не можем ничего сделать, возвращаем итератор
				}

				// Проверяем символы перед и после слэша
				if (*prev != '<' && *next != '>') {
					//logger::info("'try_to_replace_slash' : remove slash in {} : ", str);
					return it = str.erase(it);  // Удаляем слэш и возвращаем итератор
				}
			}
			return it;  // Возвращаем итератор, если ничего не удалено
		};

		enum S
		{
			NODE,
			ATTR,
			VALUE,
			SEARCH
		};
		S state = SEARCH;

		std::string node("");
		std::vector<std::pair<std::string, std::string>> pairs;
		std::string val("");
		std::string attr("");
		bool close_this_node = false;

		for (auto c = it->begin(); c != it->end(); ++c) {
			switch (state) {
			case SEARCH:
				if (*c == '<') {
					if (node.empty()) {
						state = NODE;
						if (std::next(c) != it->end() && *(std::next(c)) == '/') {
							close_this_node = true;
						}
					}
				} else if (isSpace(*c) && std::next(c) != it->end() && std::isalpha(static_cast<unsigned char>(*(std::next(c))))) {
					state = ATTR;
				} else if (*c == '\"' && c != it->begin() && *(std::prev(c)) == '=') {
					state = VALUE;
				}
				// Remove '/'
				if (*c == '/') {
					try_to_replace_slash(*it, c);
				}
				break;

			case VALUE:
				if (*c == '\"') {
					state = SEARCH;
					fill_pair(val, attr, pairs);
				} else {
					val += *c;
				}
				// Control space after '\"'
				check_missed_space(c, *it);
				break;

			case ATTR:
				if (isSpace(*c) || *c == '=') {
					state = SEARCH;
				} else {
					attr += *c;
				}
				// Remove '/'
				if (*c == '/') {
					try_to_replace_slash(*it, c);
				}
				break;

			case NODE:
				if (isSpace(*c)) {
					if (std::next(c) != it->end() && std::isalpha(static_cast<unsigned char>(*(std::next(c))))) {
						state = ATTR;
					} else {
						state = SEARCH;
					}
				} else if (*c == '>') {
					close_this_node = true;
					fill_pair(val, attr, pairs);
					return;  // Выход из функции
				} else {
					node += *c;
				}
				// Remove '/'
				if (*c == '/') {
					try_to_replace_slash(*it, c);
				}
				break;
			}
		}
	};

	auto replace_escape_sequences = [](std::vector<std::string>::iterator& it, XMLfile* xmlFile) {
		std::string& xmlString = *it;                    // Получаем строку из итератора
		std::string result;                              // Результирующая строка
		bool insideQuotes = false;                       // Флаг, указывающий, находимся ли мы внутри кавычек
		size_t firstAngleBracket = xmlString.find('<');  // Позиция первого '<'
		size_t lastAngleBracket = xmlString.rfind('>');  // Позиция последнего '>'

		for (size_t i = 0; i < xmlString.size(); ++i) {
			char currentChar = xmlString[i];

			// Проверяем, открываются ли кавычки
			if (currentChar == '"') {
				insideQuotes = !insideQuotes;  // Переключаем состояние
				result += currentChar;         // Добавляем кавычку в результат
			} else if (insideQuotes) {
				// Находимся внутри кавычек, добавляем символ как есть
				result += currentChar;
			} else {
				// Находимся вне кавычек
				if (i == firstAngleBracket) {
					result += currentChar;  // Оставляем первый '<' как есть
				} else if (i == lastAngleBracket) {
					result += currentChar;  // Оставляем последний '>' как есть
				} else {
					// Замена символов вне кавычек
					if (currentChar == '<') {
						result += "&lt;";
					} else if (currentChar == '>') {
						result += "&gt;";
					} else if (currentChar == '&') {
						// Проверяем, находится ли ';' в пределах 3-5 символов после '&'
						size_t semiColonPos = xmlString.find(';', i);
						if (semiColonPos != std::string::npos && semiColonPos - i >= 3 && semiColonPos - i <= 5) {
							// Если найдено, оставляем как есть
							result += currentChar;
						} else {
							// Это не часть escape-последовательности, заменяем
							result += "&amp;";
						}
					} else {
						result += currentChar;  // Оставляем остальные символы без изменений
					}
				}
			}
		}

		xmlString = result;  // Обновляем строку в векторе
	};

	auto glue_lines = [&, this](std::vector<std::string>::iterator& it, XMLfile*) {
		auto has_end = [](const std::string& str) {
			return !str.empty() && str.back() == '>';
		};

		auto has_begin = [](const std::string& str) {
			return !str.empty() && str.front() == '<';
		};

		// Проверяем, есть ли начало и конец в текущей строке
		if (auto has_b = has_begin(*it), has_e = has_end(*it); has_b && has_e) {
			// Всё в порядке, ничего не нужно делать
		} else if (has_b && !has_e) {
			auto save_it = it;  // Сохраняем итератор

			// Ищем следующую строку для объединения
			while (!has_end(*save_it) && it != buffer.end()) {
				// Пропускаем пустые строки
				do {
					++it;  // Переходим к следующей строке
				} while (it != buffer.end() && it->empty());

				// Проверяем, не достигли ли конца списка
				if (it == buffer.end()) {
					set_critical_error("'glue_lines' : couldn't glue, error in lines : \n" + *save_it + "\n, file : " + source.filename().string());
					return;
				}

				// Проверяем, начинается ли следующая строка с '<'
				if (has_begin(*it)) {
					set_critical_error("'glue_lines' : couldn't glue, error in lines : \n" + *save_it + "\n, file : " + source.filename().string());
					return;
				}

				// Объединяем строки
				*save_it += ' ' + *it;
				it->clear();  // Очищаем объединённую строку
			}

			// Проверяем, есть ли конец в объединенной строке
			if (!has_end(*save_it)) {
				set_critical_error("'glue_lines' : couldn't glue, error in lines : \n" + *save_it + "\n, file : " + source.filename().string());
				return;
			}

			it = save_it;  // Возвращаем итератор на сохранённую строку
		} else if (!has_b && has_e){
			set_critical_error("'glue_lines' : line has no begin '<' symbol : \n" + *it + "\n, file : " + source.filename().string());
			return;
		} else {
			set_critical_error("'glue_lines' : line has no begin '<' symbol, line has no end '>' symbol : \n" + *it + "\n, file : " + source.filename().string());
		}
	};

	auto fix_wrong_case_in_string_line = [&](std::string& str, const std::string_view node, const std::vector<std::string>& attributes) {
		if (str.empty() || node.empty()) {
			return;  // Выход, если строка или узел пустые
		}

		auto str_copy = utils::string::to_lower(std::string_view(str));  // Копируем строку в нижнем регистре

		// Исправляем имя узла
		if (auto pos = str_copy.find('<' + utils::string::to_lower(node)); pos != std::string::npos) {
			str.erase(pos + 1, node.length());  // Удаляем неправильный регистр
			str.insert(pos + 1, node);          // Вставляем правильный регистр
		}

		// Исправляем закрывающий тег
		if (auto pos = str_copy.find("</" + utils::string::to_lower(node)); pos != std::string::npos) {
			str.erase(pos + 2, node.length());  // Удаляем неправильный регистр
			str.insert(pos + 2, node);          // Вставляем правильный регистр
		}

		// Исправляем атрибуты
		for (const auto& attr : attributes) {
			if (attr.empty()) {
				continue;  // Пропускаем пустые атрибуты
			}

			auto lower_attr = utils::string::to_lower(attr) + "=\"";  // Формируем строку для поиска атрибута
			if (auto pos = str_copy.find(lower_attr); pos != std::string::npos) {
				str.erase(pos, attr.length());  // Удаляем неправильный регистр атрибута
				str.insert(pos, attr);          // Вставляем правильный регистр атрибута
			}
		}
	};

	auto compare_and_replace = [](std::string_view bench, std::string& subj) {
		// Сравниваем строки
		bool areEqual = std::equal(bench.begin(), bench.end(), subj.begin(),
			[](char a, char b) { return std::tolower(a) == std::tolower(b); });

		// Если строки равны по содержимому (без учета регистра) и имеют одинаковую длину
		if (areEqual && bench.length() == subj.length()) {
			subj = bench;  // Заменяем subj на bench
		}
	};

	auto fix_duplicate_attributes = [&](std::vector<std::string>::iterator& it, XMLfile* x) {
		auto attrs = utils::pop_all_attributes(*it);
		std::unordered_set<std::string> store;  // Используем unordered_set для более быстрой проверки

		for (auto i = attrs.begin(); i != attrs.end();) {
			if (!store.insert(i->first).second) {  // Если элемент уже существует
				LOG("'fix_duplicate_attributes' removed: {}=\"{}\"", i->first, i->second);
				i = attrs.erase(i);  // Удаляем дублирующийся атрибут
			} else {
				++i;  // Переходим к следующему элементу
			}
		}

		for (const auto& i : attrs) {
			utils::xml::add_attribute(*it, i.first, i.second);
		}
	};

	hasValue = true;

	//Форматирование
	//1й этам очистка от комментариев и лишних пробелов
	for_each_string([&](std::vector<std::string>::iterator& it, XMLfile* x) { 
		if (it == buffer.begin())
			*it = std::move(utils::string::clean_string_from_BOM(*it));
		clean_commentaries(it, this);
		if (!it->empty())
			*it = utils::xml::trim(*it);
		if (!it->empty())
			utils::xml::remove_inline_spaces(*it);
	});

	//2 этап склеивание строк, исправление лексикографических ошибок
	for_each_string([&](std::vector<std::string>::iterator& it, XMLfile* x) {
		glue_lines(it, x);
		if (!it->empty())
			check_lexicography(it, x);
		if (!it->empty())
			fix_duplicate_attributes(it, x);
	});

	//3 этап получение root node
	auto get_root_and_add_filename = [&]() {
		
		auto it = std::find_if(buffer.begin(), buffer.end(), [](const auto& str) { return !str.empty(); });
		if (it == buffer.end()) {
			set_critical_error(filename() + " Buffer is empty");
			return root_node;
		}

		auto get_valid_root_key_generic = [](std::string_view k, bool ignore_case = false) -> std::optional<std::string_view> {
			for (auto it = valid_root_nodes.begin(), end = valid_root_nodes.end(); it != end; ++it) {
				if (ignore_case) {
					if (utils::string::to_lower(k) == utils::string::to_lower(it->first))
						return it->first;
				} else {
					if (it->first == k)
						return it->first;
				}
			}
			return std::nullopt;
		};

		std::string node_name = utils::xml::get_node_name(*it);
		root_node = get_valid_root_key_generic(node_name).value_or("");

		if (root_node.empty()) {
			auto dataSet = utils::xml::get_attribute_value(std::string_view(*it), "dataSet");
			if (!dataSet.empty()) {
				std::string dataSetData = std::string(dataSet) + "Data";
				root_node = get_valid_root_key_generic(dataSetData).value_or(get_valid_root_key_generic(dataSetData, true).value_or(""sv));
				if (root_node.empty()) {
					set_critical_error(filename() + " has no root node");
				} else {
					utils::xml::add_attribute(*it, "filename", filename());
				}
			}
		} else {
			utils::xml::add_attribute(*it, "filename", filename());
		}

		if (!root_node.empty()) {
			if (auto node = utils::xml::get_node_name(*it); node != root_node)
			{
				utils::xml::change_node_name(*it, root_node);
				if (auto p = it->rfind("/>")) {
					it->replace(p, 2, ">");
					auto i = buffer.end();
					while (i != buffer.begin() && std::prev(i)->empty()) i--;
					if (i == buffer.begin()) {
						LOG("No nodes in xml file {}", filename());
						return root_node = "";
					}
					if (i == buffer.end())	{
						buffer.push_back("</"s + std::string(root_node) + ">");
					} else {
						*i = "</"s + std::string(root_node) + ">";
					}
				}
			}
		}
		return root_node;
	}();

	//Исправление для GrayUserBP_animationGroupData.xml
	if (filename() == "GrayUserBP_animationGroupData.xml" && root_node == "animationData") {
		auto fix_animationGroupData_for_GrayUserBP_animationGroupData = [&]() {
			auto open = std::find_if(buffer.begin(), buffer.end(), [](const std::string& str) {
				return !str.empty();
			});
			if (open != buffer.end()) {
				utils::xml::change_node_name(*open, "animationGroupData"s);
				auto rev_open = open == buffer.begin() ? buffer.rend() : std::make_reverse_iterator(open);
				auto close = std::find_if(buffer.rbegin(), rev_open, [](std::string& str) {
					if (auto p = str.find("</animationData>"); p != std::string::npos) {
						str.replace(p + 2, 13, "animationGroupData");
						return true;
					} else
						return false;
				});
			}
		};
		fix_animationGroupData_for_GrayUserBP_animationGroupData();
	}

	if (inimap.get<bool>("bPrintDebugXMLs"s, "General"s))
		print_to_file();

	return hasValue;
}

void XMLfile::send_warning(std::string_view warn)
{
	LOG("File : {}, warn : {}", source.filename().string(), warn);
}

void XMLfile::set_critical_error(std::string_view error)
{
	hasValue = false;
	LOG("File : {}, error : {}", source.filename().string(), error);
}

std::shared_ptr<std::stringstream> XMLfile::make_stringstream() const
{
	auto s = std::make_shared<std::stringstream>();
	std::for_each(buffer.begin(), buffer.end(), [&](const std::string& str) {
		if (!str.empty()) {
			*s << str << "\n";
		}
	});
	return s;
}


std::unordered_map<std::string_view, std::string_view> parse_attributes(std::string_view str, const std::vector<std::string_view>& attributes, const std::unordered_map<std::string_view, std::string_view>& defaults)
{
	std::unordered_map<std::string_view, std::string_view> attrs;
	for (const auto& a : attributes) {
		auto val = utils::xml::get_attribute_value(str, a);
		if (!val.empty())
			attrs.emplace(make_pair(a, val));
		else if (defaults.contains(a)) {
			try {
				val = defaults.at(a);
				attrs.emplace(make_pair(a, val));
			}
			catch (...) {
				continue;
			}
		}
	}
	return attrs;
};

void XMLfile::print_to_file() const
{
	// Определяем путь к файлу
	std::filesystem::path output_path = std::filesystem::current_path() / "Data" / "NAFicator" / "Debug" / filename();

	// Создаем поток для записи в файл
	std::ofstream output_file(output_path);
	if (!output_file.is_open()) {
		LOG("Failed to open file for writing: {}", output_path.string());
		return;
	}

	// Получаем строковый поток и записываем его содержимое в файл
	auto stringstream = make_stringstream();
	output_file << stringstream->str();

	// Закрываем файл
	output_file.close();

	LOG("Successfully written to file: {}", output_path.string());
}
