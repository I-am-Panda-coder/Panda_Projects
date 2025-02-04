#pragma once
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

//#define NOLOG

class slog
{
public:
	// Метод для получения единственного экземпляра логгера
	static slog& getInstance(const std::string& filename = "E:\\Games\\Fallout 4\\MO2\\mods\\NAFicator\\NAFicator\\log.txt")
	{
		static slog instance(filename);  // Статический экземпляр
		return instance;
	}

	// Удаляем конструктор копирования и оператор присваивания
	slog(const slog&) = delete;
	slog& operator=(const slog&) = delete;

	// Метод для логирования с форматированием
	template <typename... Args>
	inline void log(const std::string& format, Args... args)
	{
		#ifndef NOLOG
		std::lock_guard<std::mutex> guard(log_mutex);  // Защита от одновременного доступа
		if (log_file.is_open()) {
			log_file << getCurrentTime() << " - " << formatString(format, args...) << std::endl;  // Записываем сообщение в файл
		} else {
			std::cerr << "Лог-файл не открыт!" << std::endl;  // Логируем ошибку
		}
		#endif
	}

	// Перегрузка оператора << для вывода в лог
	template <typename T>
	inline friend slog& operator<<(slog& logger, const T& message)
	{
		#ifndef NOLOG
		std::lock_guard<std::mutex> guard(logger.log_mutex);  // Защита от одновременного доступа
		if (logger.log_file.is_open()) {
			logger.log_file << message;  // Записываем сообщение в файл
		} else {
			std::cerr << "Лог-файл не открыт!" << std::endl;  // Логируем ошибку
		}
		#endif
		return logger;
	}

	// Перегрузка оператора () для форматированного вывода
	template <typename... Args>
	inline void operator()(const std::string& format, Args... args)
	{
		#ifndef NOLOG
		log(format, args...);
		#endif
	}

	// Метод для очистки файла лога
	inline void clearLog()
	{
		#ifndef NOLOG
		std::lock_guard<std::mutex> guard(log_mutex);  // Защита от одновременного доступа
		if (log_file.is_open()) {
			log_file.close();  // Закрываем текущий файл
		}
		log_file.open(log_filename, std::ios::trunc);  // Открываем файл в режиме очистки
		if (!log_file.is_open()) {
			std::cerr << "Не удалось открыть файл для очистки!" << std::endl;
		}
		#endif
	}

private:
	std::ofstream log_file;
	std::mutex log_mutex;
	std::string log_filename;

	// Приватный конструктор
	inline slog(const std::string& filename) :
		log_filename(filename)  // Сохраняем имя файла
	{
		#ifndef NOLOG
		log_file.open(filename, std::ios::app);  // Открываем файл в режиме добавления
		if (!log_file.is_open()) {
			std::cerr << "Не удалось открыть файл для логирования!" << std::endl;
		}
		#endif
	}

	inline ~slog()
	{
		#ifndef NOLOG
		if (log_file.is_open()) {
			log_file.close();  // Закрываем файл при уничтожении логгера
		}
		#endif
	}

	inline std::string getCurrentTime()
	{
		#ifndef NOLOG
		auto now = std::chrono::system_clock::now();
		std::time_t now_c = std::chrono::system_clock::to_time_t(now);
		std::ostringstream oss;
#pragma warning(push)
#pragma warning(disable: 4996)
		oss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");
#pragma warning(pop)
		return oss.str();
		#endif
		#ifdef NOLOG
		return "";
		#endif
	}

	// Метод для форматирования строки
	inline std::string formatString(const std::string& format)
	{
		return format;  // Если нет аргументов, просто возвращаем формат
	}

	template <typename T, typename... Args>
	inline std::string formatString(const std::string& format, T value, Args... args)
	{
		#ifndef NOLOG
		std::ostringstream oss;
		for (size_t i = 0; i < format.size(); ++i) {
			if (format[i] == '{' && i + 1 < format.size() && format[i + 1] == '}') {
				oss << value;                                                    // Вставляем значение
				return oss.str() + formatString(format.substr(i + 2), args...);  // Рекурсивный вызов для оставшейся части
			}
			oss << format[i];  // Вставляем символ
		}
		return oss.str();  // Возвращаем собранную строку
		#endif
		#ifdef NOLOG
		return "";
		#endif
	}
};

#define LOG slog::getInstance()

// Пример использования
// int main()
// {
//     slog& logger = slog::getInstance("log.txt");
//
//     logger << "Это информационное сообщение: "
//            << "Hello, World!";
//     logger << "Ошибка при загрузке файла: "
//            << "file.txt";
//
//     int userId = 123;
//     logger << "Пользователь " << userId << " вошел в систему";
//
//     double balance = 456.78;
//     logger << "Баланс пользователя: " << balance;
//
//     logger("Это информационное сообщение: {}", "Hello, World!");
//     logger("Ошибка при загрузке файла: {}", "file.txt");
//
//     int counter = 1;
//     std::string filename = "example.txt";
//     logger("Файл {} успешно загружен", filename);
//     logger("Обработано {} файлов", counter);
//
//     return 0;
// }



