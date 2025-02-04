#pragma once
#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <Bridge/Utility.hpp>


namespace ini
{
	template <class T1, class T2>
	bool is_almost_same_v = std::is_same_v<const T1&, const T2&>;
	
	class map
	{
		using SectionMap = std::map<std::string, std::string>;
		using IniMap = std::map<std::string, SectionMap>;
		std::string pth = "";

		IniMap inimap;

	public:
		map(const std::string& path) :
			pth(path)
		{
			std::ifstream stream(path);
			std::string current_section("default");
			if (!stream.is_open()) {
				throw std::runtime_error("file doesn't exist : "s + pth);
			}

			std::string str;
			while (!stream.eof()) {
				GetIniLine(stream, str);
				if (str.empty())
					continue;
				if (std::string s = GetSection(str); !s.empty()) {
					current_section = s;
				} else {
					if (current_section.empty())
						continue;
					if (std::vector<std::string> pair = GetKeyValuePair(str); pair.size() == 2) {
						inimap[current_section][pair.at(0)] = pair.at(1);
					}
				}
			}
		}

		bool contains(std::string& key, std::string& section)
		{
			if (!inimap.contains(section)) {
				return false;
			}

			if (!inimap[section].contains(key)) {
				return false;
			}
			return true;
		}

		template <class T>
		inline T get(const std::string& key, const std::string& section);

		inline void set(const std::string val, const std::string& key, const std::string& section)
		{
			std::ifstream read(pth);
			if (!read.is_open()) {
				throw std::runtime_error("file doesn't exist : "s + pth);
			}

			bool section_found = false;
			bool key_found = false;

			std::vector<std::string> buf;
			std::string current_section;
			std::string str;
			std::string copy;
			while (!read.eof()) {
				getline(read, str);

				copy = str;
				RemoveComments(copy);
				RemoveSideSpaces(copy);

				if (key_found || copy.empty()) {
					buf.push_back(str + '\n');
					continue;
				}

				if (std::string s = GetSection(copy); !s.empty()) {
					if (current_section == section && !key_found) {
						size_t line = buf.size() - 1;
						while (IsUnused(buf[line])) line--;
						line++;
						std::string new_line = key + "="s + val + '\n';
						buf.insert(buf.begin() + line, new_line);
						key_found = true;
					}
					current_section = s;
					buf.push_back(str + '\n');
					continue;
				} else {
					if (current_section != section) {
						buf.push_back(str + '\n');
						continue;
					}
					section_found = true;
					if (size_t f = copy.find(key + "="s); f != std::string::npos) {
						f = str.find(key + "="s);
						if (f != std::string::npos) {
							f += key.length() + 1;
							size_t semi = str.find(';', f);
							size_t len = semi < str.length() ? semi : str.length();
							len -= f;
							str.erase(f, len);
							str.insert(f,val);
							key_found = true;
						}
					}
					buf.push_back(str + '\n');
				}
			}
			if (!section_found) {
				size_t line = buf.size() - 1;
				while (IsUnused(buf[line])) line--;
				line++;
				buf.insert(buf.begin() + line, "["s + section + "]"s + '\n');
			}
			if (!key_found) {
				size_t line = buf.size() - 1;
				while (IsUnused(buf[line])) line--;
				line++;
				std::string new_line = key + "="s + val + '\n';
				buf.insert(buf.begin() + line, new_line);
			}
			read.close();
			std::ofstream write(pth);
			if (!write.is_open()) {
				throw std::runtime_error("file doesn't exist : "s + pth);
			}

			for (auto& s : buf)
			{
				write << s;
			}

			write.close();
		}
	};

	template <>
	inline std::string map::get(const std::string& key, const std::string& section)
	{
		if (!inimap.contains(section)) {
			throw std::invalid_argument(section);
		}

		if (!inimap[section].contains(key)) {
			throw std::invalid_argument(key);
		}

		return inimap.at(section).at(key);
	}

	template <>
	inline std::uint8_t map::get(const std::string& key, const std::string& section)
	{
		if (!inimap.contains(section)) {
			throw std::invalid_argument(section);
		}

		if (!inimap[section].contains(key)) {
			throw std::invalid_argument(key);
		}

		try {
			auto val = std::stoul(inimap.at(section).at(key));
			if (val > std::numeric_limits<std::uint8_t>::max()) {
				throw std::out_of_range(section + ":"s + key);
			}
			return static_cast<std::uint8_t>(val);
		} catch (std::out_of_range e) {
			throw std::out_of_range(section + ":"s + key);
		} catch (...) {
			throw std::invalid_argument(section + ":"s + key);
		}
	}

	template <>
	inline std::uint16_t map::get(const std::string& key, const std::string& section)
	{
		if (!inimap.contains(section)) {
			throw std::invalid_argument(section);
		}

		if (!inimap[section].contains(key)) {
			throw std::invalid_argument(key);
		}

		try {
			auto val = std::stoul(inimap.at(section).at(key));
			if (val > std::numeric_limits<std::uint16_t>::max()) {
				throw std::out_of_range(section + ":"s + key);
			}	
			return static_cast<std::uint16_t>(val);
		} catch (std::out_of_range e) {
			throw std::out_of_range(section + ":"s + key);
		} catch (...) {
			throw std::invalid_argument(section + ":"s + key);
		}
	}

	template <>
	inline std::uint32_t map::get(const std::string& key, const std::string& section)
	{
		if (!inimap.contains(section)) {
			throw std::invalid_argument(section);
		}

		if (!inimap[section].contains(key)) {
			throw std::invalid_argument(key);
		}

		try {
			auto val = std::stoul(inimap.at(section).at(key));
			if (val > std::numeric_limits<std::uint32_t>::max()) {
				throw std::out_of_range(section + ":"s + key);
			}
			return static_cast<std::uint16_t>(val);
		} catch (std::out_of_range e) {
			throw std::out_of_range(section + ":"s + key);
		} catch (...) {
			throw std::invalid_argument(section + ":"s + key);
		}
	}

	template <>
	inline std::uint64_t map::get(const std::string& key, const std::string& section)
	{
		if (!inimap.contains(section)) {
			throw std::invalid_argument(section);
		}

		if (!inimap[section].contains(key)) {
			throw std::invalid_argument(key);
		}

		try {
			auto val = std::stoul(inimap.at(section).at(key));
			if (val > std::numeric_limits<std::uint64_t>::max()) {
				throw std::out_of_range(section + ":"s + key);
			}
			return static_cast<std::uint16_t>(val);
		} catch (std::out_of_range e) {
			throw std::out_of_range(section + ":"s + key);
		} catch (...) {
			throw std::invalid_argument(section + ":"s + key);
		}
	}

	template <>
	inline int map::get(const std::string& key, const std::string& section)
	{
		if (!inimap.contains(section)) {
			throw std::invalid_argument(section);
		}

		if (!inimap[section].contains(key)) {
			throw std::invalid_argument(section + ":"s + key);
		}

		try {
			auto val = std::stoi(inimap.at(section).at(key));
			return val;
		} catch (std::out_of_range e) {
			throw std::out_of_range(section + ":"s + key);
		} catch (...) {
			throw std::invalid_argument(section + ":"s + key);
		}
	}

	template <>
	inline float map::get(const std::string& key, const std::string& section)
	{
		if (!inimap.contains(section)) {
			throw std::invalid_argument(section);
		}

		if (!inimap[section].contains(key)) {
			throw std::invalid_argument(section + ":"s + key);
		}

		try {
			auto val = std::stof(inimap.at(section).at(key));
			return val;
		} catch (std::out_of_range e) {
			throw std::out_of_range(section + ":"s + key);
		} catch (...) {
			throw std::invalid_argument(section + ":"s + key);
		}
	}

	template <>
	inline double map::get(const std::string& key, const std::string& section)
	{
		if (!inimap.contains(section)) {
			throw std::invalid_argument(section);
		}

		if (!inimap[section].contains(key)) {
			throw std::invalid_argument(section + ":"s + key);
		}

		try {
			auto val = std::stod(inimap.at(section).at(key));
			return val;
		} catch (std::out_of_range e) {
			throw std::out_of_range(section + ":"s + key);
		} catch (...) {
			throw std::invalid_argument(section + ":"s + key);
		}
	}

	template <>
	inline bool map::get(const std::string& key, const std::string& section)
	{
		if (!inimap.contains(section)) {
			throw std::invalid_argument(section);
		}

		if (!inimap[section].contains(key)) {
			throw std::invalid_argument(section + ":"s + key);
		}

		std::string res = inimap.at(section).at(key);

		if (res == "true") 
			return true;
		if (res == "false")
			return false;

		try {
			auto val = std::stoi(res);
			return val ? true : false;
		} catch (std::out_of_range e) {
			throw std::out_of_range(section + ":"s + key);
		} catch (...) {
			throw std::invalid_argument(section + ":"s + key);
		}
	}


}
