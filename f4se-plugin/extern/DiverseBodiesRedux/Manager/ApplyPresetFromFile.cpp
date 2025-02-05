#include "ApplyPresetFromFile.h"
#include <windows.h>
#include <commdlg.h>
#include <iostream>
#include <filesystem>
#include "manager.h"

class MessageCallback0 : public RE::IMessageBoxCallback
{
	void operator()(std::uint8_t a_buttonIdx) override
	{
		switch (a_buttonIdx) {
		case 0:
			//just close;
			break;
		default:
			//just close;
			break;
		}
	}
};

MessageCallback0 uno{};

std::string ApplyBodyPresetFromFileForActor(RE::Actor* actor)
{
	auto validate_actor = [](RE::Actor* actor) {
		if (!actor) {
			//logger::info("nullptr : canceled");
			return false;
		}

		if (actor->formType != RE::ENUM_FORM_ID::kACHR) {
			//logger::info("{}({}) : canceled, is not actor", actor->GetFormEditorID(), uint32ToHexString(actor->formID));
			return false;
		}

		if (!global::is_qualified_race(actor)) {
			//logger::info("{}({}) : canceled, not qualified race", actor->GetFormEditorID(), uint32ToHexString(actor->formID));
			return false;
		}

		return true;
	};

	std::string preset_name{};

	if (!validate_actor(actor)) {
		//RE::MessageMenuManager::GetSingleton()->Create("ERROR", global::message_wrong_actor_or_gender(), &uno, RE::WARNING_TYPES::kInGameMessage, "OK");
		return preset_name;
	}
	
	// Структура для хранения информации о выбранном файле
	OPENFILENAME ofn;
	char szFile[260] = "";                                                                    // Буфер для имени файла
	char szFilter[] = "Body presets (*.json;*.xml)\0*.json;*.xml\0Все файлы (*.*)\0*.*\0\0";  // Фильтр для файлов

	// Получаем текущий путь и добавляем к нему нужную директорию
	std::filesystem::path currentPath = std::filesystem::current_path();
	std::filesystem::path targetDir = currentPath / "Data" / "DiverseBodiesRedux" / "BodyPresets";

	// Инициализация структуры OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;  // Владелец окна
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = szFilter;  // Установка фильтра
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = targetDir.string().c_str();  // Установка начальной директории
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

	// Открытие окна выбора файла
	if (GetOpenFileName(&ofn)) {
		bodymorphs::Preset bodyPreset{ std::filesystem::path(szFile) };
		if (!bodyPreset.empty() && actor->GetSex() == bodyPreset.get_sex()) {
			bodyPreset.apply(actor);
			preset_name = bodyPreset.name();
			//UpdateBodyMorphsForActor(actor);
		}
		else
			RE::MessageMenuManager::GetSingleton()->Create("", global::message_wrong_actor_or_gender(), nullptr, RE::WARNING_TYPES::kInGameMessage);
	} else {
		RE::MessageMenuManager::GetSingleton()->Create("", global::message_wrong_file(), nullptr, RE::WARNING_TYPES::kInGameMessage);
	}

	return (std::move(preset_name));
}
