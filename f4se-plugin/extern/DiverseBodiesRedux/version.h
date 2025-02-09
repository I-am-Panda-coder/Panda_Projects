#pragma once
#include <array>
#include <cstdint>
#include <string_view>
using namespace std::string_view_literals;

namespace ver
{
	inline constexpr std::size_t MAJOR = 0;
	inline constexpr std::size_t MINOR = 1;
	inline constexpr std::size_t PATCH = 6;

	// Функция для вычисления версии в формате INT
	inline constexpr uint32_t computeVersionInt(std::size_t major, std::size_t minor, std::size_t patch)
	{
		return (static_cast<uint32_t>(major) << 16) | (static_cast<uint32_t>(minor) << 8) | static_cast<uint32_t>(patch);
	}

	// Функция для формирования строковой версии
	inline constexpr auto makeVersionString(std::size_t major, std::size_t minor, std::size_t patch)
	{
		// Простой способ создать строку версии
		return std::array<char, 16>{
			static_cast<char>('0' + major), '.',
			static_cast<char>('0' + minor), '.',
			static_cast<char>('0' + patch), '\0'
		};
	}

	// Генерация строковой версии
	inline constexpr auto VER = makeVersionString(MAJOR, MINOR, PATCH);

	// Генерация имени версии
	inline constexpr std::array<char, 21> generatePluginName()
	{
		return { 'D', 'i', 'v', 'e', 'r', 's', 'e', ' ', 'B', 'o', 'd', 'i', 'e', 's', ' ', 'R', 'E', 'D', 'U', 'X', '\0' };
	}

	inline constexpr std::array<char, 32> generateName()
	{
		// Генерируем имя плагина
		auto pluginName = generatePluginName();

		// Создаем массив для полного имени
		std::array<char, 32> name = {};

		// Копируем имя плагина в массив
		for (size_t i = 0; i < pluginName.size() && pluginName[i] != '\0'; ++i) {
			name[i] = pluginName[i];
		}

		// Копируем версию в массив после имени плагина
		for (size_t i = 0; i < 16 && VER[i] != '\0'; ++i) {
			name[pluginName.size() + i] = VER[i];  // Добавляем версию после имени плагина
		}

		name[pluginName.size() + 5] = '\0';  // Завершаем строку нулем
		return name;
	}

	inline constexpr auto NAME = generateName();

	// Генерация целочисленной версии
	inline constexpr uint32_t VER_INT = computeVersionInt(MAJOR, MINOR, PATCH);

	inline constexpr auto PROJECT = "DiverseBodiesRedux";

	inline const size_t UID = 'DBRX';
}
