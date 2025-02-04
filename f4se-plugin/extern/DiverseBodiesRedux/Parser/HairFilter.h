#pragma once
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>

class HairFilter
{
public:
	HairFilter();

	const std::set<std::string>& getItems() const;

	bool isWhiteList() const;

	// Проверка, пустой ли набор
	bool empty() const;

	// Получение размера набора
	size_t size() const;

	// Проверка наличия элемента в наборе
	bool contains(const std::string& item) const;

private:
	void readFile(const std::string& filePath);

	bool isFileEmpty(const std::string& filePath) const;

	std::set<std::string> items;
	bool isWhiteListRead = false;
};
