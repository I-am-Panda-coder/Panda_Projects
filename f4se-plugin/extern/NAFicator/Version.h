#pragma once
#include <array>
#include <cstdint>
#include <string_view>
using namespace std::string_view_literals;

namespace ver
{
	inline constexpr std::size_t MAJOR = 0;
	inline constexpr std::size_t MINOR = 9;
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
	inline constexpr std::array<char, 32> generateName()
	{
		std::array<char, 32> name = { 'P', 'a', 'n', 'd', 'a', ' ', 'N', 'A', 'F', 'i', 'c', 'a', 't', 'o', 'r', ' ', '\0' };
		// Копируем версию в массив
		for (size_t i = 0; i < 16 && VER[i] != '\0'; ++i) {
			name[17 + i] = VER[i];  // 17 - длина "Panda NAFicator "
		}
		name[17 + 5] = '\0';  // Завершаем строку нулем
		return name;
	}

	inline constexpr auto NAME = generateName();

	// Генерация целочисленной версии
	inline constexpr uint32_t VER_INT = computeVersionInt(MAJOR, MINOR, PATCH);

	inline constexpr auto PROJECT = "NAFicator";
}

//namespace ver
//{
//	inline constexpr std::size_t MAJOR = 0;
//	inline constexpr std::size_t MINOR = 7;
//	inline constexpr std::size_t PATCH = 0;
//	inline constexpr uint32_t VER_INT = 0x0070;
//	inline constexpr auto VER = "0.7.0"sv;
//	inline constexpr auto NAME = "Panda NAFicator 0.7.0"sv;
//	inline constexpr auto PROJECT = "NAFicator"sv;
//}
