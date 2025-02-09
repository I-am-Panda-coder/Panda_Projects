#include <DiverseBodiesRedux/Parser/HairFilter.h>

#include "F4SE/F4SE.h"
#include "RE/Fallout.h"

namespace logger = F4SE::log;

HairFilter::HairFilter()
{
	std::string path = std::filesystem::current_path().string() + "\\Data\\DiverseBodiesRedux\\Hair";
	std::string whiteListPath = path + "\\WhiteList.txt";
	std::string blackListPath = path + "\\BlackList.txt";

	if (std::filesystem::exists(whiteListPath)) {
		if (!isFileEmpty(whiteListPath)) {
			readFile(whiteListPath);
			isWhiteListRead = true;
			return;  // Завершаем работу конструктора, если WhiteList.txt не пустой
		}
	} else {
		logger::error("WhiteList doesn't exists!");
	}

	if (std::filesystem::exists(blackListPath)) {
		if (!isFileEmpty(blackListPath)) {
			readFile(blackListPath);
			isWhiteListRead = false;
			return;  // Завершаем работу конструктора, если BlackList.txt не пустой
		}
	} else {
		logger::error("BlackList doesn't exists!");
	}

	// Если оба файла пустые или отсутствуют, ничего не делаем
}

const std::set<std::string>& HairFilter::getItems() const
{
	return items;
}

bool HairFilter::isWhiteList() const
{
	return isWhiteListRead;
}

size_t HairFilter::size() const
{
	return items.size();
}

bool HairFilter::empty() const
{
	return items.empty();
}

bool HairFilter::contains(const std::string& item) const
{
	return items.find(item) != items.end();
}

void HairFilter::readFile(const std::string& filePath)
{
	std::ifstream file(filePath);
	std::string line;
	while (std::getline(file, line)) {
		items.insert(line);
	}
}

bool HairFilter::isFileEmpty(const std::string& filePath) const
{
	std::ifstream file(filePath);
	return file.peek() == std::ifstream::traits_type::eof();
}
