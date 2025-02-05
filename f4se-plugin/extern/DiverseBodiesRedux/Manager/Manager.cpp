#include "Manager.h"
#include <type_traits>
#include <ppl.h>
#include <utility>
#include <cstdint>
#include <concurrent_queue.h>
#include <concurrent_unordered_map.h>
#include <zlib.h>

using f3D = RE::RESET_3D_FLAGS;

extern bool extended_log;
extern TESObjectLoadedEventHandler g_OriginalReceiveEventObjectLoaded;
concurrency::concurrent_queue<std::pair<uint32_t, void*>> looksmenu_hooked_queue;

namespace dbr_manager
{
	bool ActorsManager::proccess_actor_after_loaded(RE::Actor* actor) 
	{	
		if (actor) {
			if (auto it = actors_map.find(actor->formID); it == actors_map.end()) {
				if (move_to_map(ActorPreset(actor))) {
					if (extended_log) {
						logger::info("Add to map {}->{}", actor->GetDisplayFullName(), std::format("{:x}", actor->formID));
					}
					actor->Reset3D(false, RE::RESET_3D_FLAGS::kDiverseBodiesFlag, false, RE::RESET_3D_FLAGS::kNone);
					return true;
				}
			}
		}
		return false;
	}

	void SendLooksMenuLoadEvent(RE::TESObjectLoadedEvent a_event, RE::BSTEventSource<RE::TESObjectLoadedEvent>* a_source)
	{
		auto evn = new RE::TESObjectLoadedEvent(a_event);
		g_OriginalReceiveEventObjectLoaded(LooksMenuInterfaces<ActorUpdateManager>::GetInterface(), evn, a_source);
		std::thread([evn] {
			std::this_thread::sleep_for(std::chrono::seconds(30));
			delete evn;
		}).detach();
	}
	
	RE::BSEventNotifyControl ActorsManager::ProcessEvent(const RE::TESObjectLoadedEvent& a_event, RE::BSTEventSource<RE::TESObjectLoadedEvent>* a_source)
	{
		/*if (a_event.loaded) {
			RE::Actor* actor = GetFormByFormID<RE::Actor>(a_event.formId);
			if (actor) {
				if (extended_log)
					logger::info("OnLoad event for actor {}", std::format("{:x}", actor->formID));
				if (deserialized) {
					std::thread([actor] {
						proccess_actor_after_loaded(actor);
					}).detach();
				}
			}
		}*/

		if (a_event.loaded) {
			RE::Actor* ref = GetFormByFormID<RE::Actor>(a_event.formId);
			if (global::is_qualified_race(ref)) {
				if (ref && extended_log)
					logger::info("OnLoad event Hooked LooksMenu for actor {}", std::format("{:x}", ref->formID));
				if (ref) {
					if (!IsSerializeFinished()) {
						looksmenu_hooked_queue.push(std::make_pair(a_event.formId, a_source));
					} else if (auto is_excluded = IsActorExcluded(ref); (is_excluded.has_value() && !*is_excluded) && !global::is_excluded(ref)) {
						if (!proccess_actor_after_loaded(ref))
							//SendLooksMenuLoadEvent(a_event, a_source);
							return g_OriginalReceiveEventObjectLoaded(LooksMenuInterfaces<ActorUpdateManager>::GetInterface(), const_cast<RE::TESObjectLoadedEvent*>(&a_event), a_source);
					} else  // если нет в мапе или исключен из обработки (empty preset)
						//SendLooksMenuLoadEvent(a_event, a_source);
						return g_OriginalReceiveEventObjectLoaded(LooksMenuInterfaces<ActorUpdateManager>::GetInterface(), const_cast<RE::TESObjectLoadedEvent*>(&a_event), a_source);
				}
			}
		}
		return RE::BSEventNotifyControl::kContinue;
	}

	ActorPreset* ActorsManager::get(RE::Actor* actor) const
	{
		if (!actor)
			return nullptr;

		if (auto it = actors_map.find(actor->formID); it != actors_map.end())
			return &it->second;
		else
			return nullptr;
	}

	//void ActorsManager::merge(ActorPreset* preset)
	//{
	//	if (!preset || !preset->actor) {
	//		return;  // Проверяем наличие preset и actor
	//	}

	//	if (auto m_preset = get(preset->actor); m_preset) {
	//		// Обновляем поля body, skin и hair, если они заданы
	//		if (preset->body) {
	//			m_preset->body = preset->body;
	//		}
	//		if (preset->skin) {
	//			m_preset->skin = preset->skin;
	//		}
	//		if (preset->hair) {
	//			m_preset->hair = preset->hair;
	//		}

	//		// Объединяем оверлеи
	//		if (!preset->overlays.empty()) {
	//			std::unordered_set<overlays::Collection, overlays::hashCollection, overlays::CollectionEqualByPresetId> set{};

	//			// Заполняем множество уникальными оверлеями
	//			auto fill_set = [&set](const std::vector<overlays::Collection>& col) {
	//				for (const auto& c : col) {
	//					set.emplace(c);
	//				}
	//			};

	//			fill_set(m_preset->overlays);
	//			fill_set(preset->overlays);

	//			// Обновляем оверлеи в m_preset
	//			m_preset->overlays = std::vector<overlays::Collection>(set.begin(), set.end());
	//		}
	//	} else {
	//		emplace(preset);  // Если m_preset не найден, добавляем новый
	//	}
	//}

	/*void ActorsManager::emplace(ActorPreset* preset)
	{
		if (!preset || preset->empty())
			return;

		auto l{ lock };
		actors_map.insert(preset->actor->formID, std::move(*preset));
	}*/

	bool ActorsManager::move_to_map(ActorPreset&& preset)
	{
		if (!preset.actor)
			return false;  

		// Check body preset validity
		if (preset.body && !bodymorphs::Get(*preset.body))
			preset.body.reset();

		// Check skin validity
		if (preset.skin && !skins::Get(*preset.skin))
			preset.skin.reset();

		// Check overlays validity
		if (preset.overlays) {
			preset.overlays->validate();
		}
			

		// Check if the preset is valid to move to the map
		if (preset.empty())
			return false;

		//auto l{ lock };
		//actors_map.emplace(preset.actor->formID, preset);
		actors_map.insert(std::move(std::make_pair(preset.actor->formID, preset)));
		preset.actor = nullptr;
		return true;  // Indicate success
	}

	//bool ActorsManager::apply(ActorPreset* preset)
	//{
	//	if (!preset)
	//		return false;

	//	if (!preset->actor)
	//		return false;

	//	uint32_t formID = preset->actor->GetNPC()->formID;

	//	// Уникальная блокировка для каждого formID
	//	std::unique_lock<std::mutex> lock(formIDMutexes[formID].mutex);
	//	formIDMutexes[formID].lastAccessTime = std::chrono::steady_clock::now();  // Обновляем время доступа

	//	ThreadHandler<ActorPreset>::get_thread()->enqueue([&]() {
	//
	//		preset->apply();

	//		std::thread([this]() {
	//			std::this_thread::sleep_for(std::chrono::minutes(1));  // Ждем 1 минуту
	//			clean_up_old_mutexes();                                // Вызываем очистку
	//		}).detach();                                               // Отделяем поток, чтобы он работал независимо
	//	});

	//	return true;
	//}

	bool ActorsManager::apply(ActorPreset* preset)
	{
		if (!preset)
			return false;

		if (preset->empty())
			return false;

		//if (preset->actor->GetFullyLoaded3D()) {
		if (extended_log)
			logger::info("Apply preset for {}->{}", preset->actor->GetDisplayFullName(), std::format("{:x}", preset->actor->formID));

		preset->apply();
		//}

		return true;
	}

	boost::json::array ActorsManager::gatherActorPresets()
	{
		boost::json::array jsonArray;

		for (const auto& [formId, preset] : actors_map) {
			jsonArray.push_back(preset.serialize());
		}

		return jsonArray;
	}

	void ActorsManager::Serialize(const F4SE::SerializationInterface* a_intfc)
	{
		if (extended_log)
			logger::info("Serialize started");
		auto l{ lock };  // Lock the mutex
		if (a_intfc && a_intfc->OpenRecord(ver::UID, serialization_ver)) {
			// Create a string to hold the serialized data
			std::string serializedData;
			uint32_t fullSize{};

			// Use a stream to serialize actors
			std::ostringstream ostream{};
			ostream << gatherActorPresets();  // Serialize actors
			serializedData = ostream.str();
			fullSize = serializedData.size();

			// Определяем размер сжатых данных
			uLongf compressedSize = compressBound(serializedData.size());
			std::vector<Bytef> compressedData(compressedSize);

			if (extended_log)
				logger::info("Data to serialize : {}", serializedData);

			// Сжимаем данные
			if (compress(compressedData.data(), &compressedSize, reinterpret_cast<const Bytef*>(serializedData.data()), serializedData.size()) != Z_OK) {
				logger::error("Failed to compress serialized data.");
				return;
			}

			compressedData.resize(compressedData.size() + sizeof(fullSize));
			std::memcpy(compressedData.data() + compressedData.size() - sizeof(fullSize), &fullSize, sizeof(fullSize));  // Копируем fullSize в конец

			/*//БЕЗ СЖАТИЯ

			uint32_t buf_size = static_cast<uint32_t>(serializedData.size());  // Use uint32_t

			// Create a char array to hold the data
			char* buf = new (std::nothrow) char[buf_size];
			if (!buf) {
				logger::error("Memory allocation failed for serialization buffer.");
				return;  // Return if memory allocation fails
			}

			std::copy(serializedData.begin(), serializedData.end(), buf);  // Copy data

			if (!a_intfc->WriteRecordData(buf, buf_size)) {
				logger::warn("Failed to write all serialized data to the interface.");
			}

			 Free the allocated memory
			delete[] buf;*/

			 // Записываем сжатые данные в интерфейс
			if (extended_log)
				logger::info("Try to write serialized data...");
			if (!a_intfc->WriteRecordData(compressedData.data(), compressedData.size())) {
				if (extended_log)
					logger::info("Serialize failed");
				logger::warn("Failed to write all compressed data to the interface.");
			}
		}
		if (extended_log)
			logger::info("Serialize success");

	}

	void ActorsManager::Deserialize(const F4SE::SerializationInterface* a_intfc)
	{
		if (extended_log)
			logger::info("Deserialize started");
		actors_map.clear();
		uint32_t fullSize{};
		
		auto GetNextRecord = [&a_intfc, &fullSize]() {
			uint32_t recType{}, length{}, curVersion{};
			if (!a_intfc->GetNextRecordInfo(recType, curVersion, length))
				return 0u;

			if (recType != ver::UID || curVersion != serialization_ver)
				return 0u;

			if (length > INT_MAX) {
				logger::info("buf_size is more than {}", std::to_string(INT_MAX));
				return 0u;
			}

			if (length <= sizeof(fullSize)) {
				logger::info("No serialized data.");
				return 0u;
			}

			return length;
		};
		
		auto l{ lock };  // Lock the mutex

		if (!a_intfc && a_intfc->OpenRecord(ver::UID, serialization_ver)) {
			deserialized = true;
			return;
		}

		auto length = GetNextRecord();

		if (!length) {
			deserialized = true;
			return;
		}

		/*// БЕЗ СЖАТИЯ
		// Выделяем буфер для чтения данных
		char* buf = new (std::nothrow) char[length + 1];
		if (!buf) {
			logger::error("Memory allocation failed for deserialization buffer.");
			return;  // Return if memory allocation fails
		}

		// Read data into the buffer
		auto read = a_intfc->ReadRecordData(buf, length);
		if (read != length) {
			logger::warn("Failed to read the record data");
			delete[] buf;  // Free memory in case of error
			return;
		}
		buf[length] = '\x0';
		boost::system::error_code ec{};
		boost::json::value jv = boost::json::parse(buf, ec);
		if (!ec && jv.is_array()) {
			auto arr = jv.as_array();

			for (auto& json : arr) {
				ActorsManager::move_to_map(ActorPreset(json));
			}
			update_loaded();
		} else if (ec) {
			logger::error("Deserialized array parse fail, {}", ec.message());
		} else {
			logger::warn("Deserialized is not json array");
		}
		// Free memory
		delete[] buf;*/

		// Выделяем буфер для чтения данных
		std::vector<Bytef> compressedData(length);
		auto read = a_intfc->ReadRecordData(compressedData.data(), length);
		if (read != length) {
			logger::warn("Failed to read the record data");
			deserialized = true;
			return;
		}

		// Извлекаем fullSize из конца compressedData
		if (compressedData.size() < sizeof(fullSize)) {
			logger::error("Compressed data is too small to contain fullSize.");
			deserialized = true;
			return;
		}

		std::memcpy(&fullSize, compressedData.data() + compressedData.size() - sizeof(fullSize), sizeof(fullSize));
		compressedData.resize(compressedData.size() - sizeof(fullSize));  // Уменьшаем размер на sizeof(fullSize)

		// Определяем размер для разжатых данных
		uLongf decompressedSize = fullSize;
		std::string decompressedData(decompressedSize, '\0');

		// Разжимаем данные
		if (uncompress(reinterpret_cast<Bytef*>(&decompressedData[0]), &decompressedSize, compressedData.data(), compressedData.size()) != Z_OK) {
			logger::error("Failed to decompress data.");
			deserialized = true;
			return;
		}

		if (extended_log)
			logger::info("Deserialize decompressed data : {}", decompressedData);

		// Обрабатываем разжатые данные как JSON
		boost::system::error_code ec{};
		boost::json::value jv = boost::json::parse(decompressedData.data(), ec);
		if (!ec && jv.is_array()) {
			auto arr = jv.as_array();

			for (auto& json : arr) {
				ActorsManager::move_to_map(ActorPreset(json));
			}

			std::thread
			{
				[] {
					//std::this_thread::sleep_for(std::chrono::seconds(10));
					deserialized = true;
					update_loaded();
				}
			}.detach();

			return;
		} else if (ec) {
			logger::error("Deserialized array parse fail, {}", ec.message());
			deserialized = true;
			return;
		} else {
			logger::warn("Deserialized is not json array");
			deserialized = true;
			return;
		}
		if (extended_log)
			logger::info("Deserialize function finished (but deserialization runs in thread still)");
	}

	void ActorsManager::update_loaded()
	{	
		std::this_thread::sleep_for(std::chrono::seconds(iniSettings::getInstance().getDelayTimer()));
		
		auto& q = looksmenu_hooked_queue;
		std::vector<void*> to_delete{};
		std::decay_t<decltype(looksmenu_hooked_queue)::value_type> el;
		
		while (q.try_pop(el)) {
			if (!contains_in_map(el.first)) {
				auto actor = GetFormByFormID<RE::Actor>(el.first);
				if (actor) {
					auto evn = new RE::TESObjectLoadedEvent;
					evn->formId = el.first;
					evn->loaded = 1;
					to_delete.emplace_back(evn);
					if (auto intrfc = LooksMenuInterfaces<ActorUpdateManager>::GetInterface(); intrfc /*&& g_OriginalReceiveEventObjectLoaded*/)
						//g_OriginalReceiveEventObjectLoaded(intrfc, evn, el.second);
						//HookedReceiveEventObjectLoaded(intrfc, evn, el.second);
						getInstance()->ProcessEvent(*evn, getInstance());
				}
			}
		}
		
		deserialized = true;
		for (auto& el : actors_map) {
			auto actor = el.second.actor;
			if (actor && actor->GetFullyLoaded3D()) {
				//el.second.apply();
				el.second.actor->Reset3D(false, RE::RESET_3D_FLAGS::kDiverseBodiesFlag, false, RE::RESET_3D_FLAGS::kNone);
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}

		for (auto& el : to_delete) {
			delete el;
		}

		logger::info("Deserialization finished!");
	}

	bool ActorsManager::contains_in_map(uint32_t actorId)
	{
		return actors_map.find(actorId) != actors_map.end();
	}

	ActorPreset* ActorsManager::find(RE::Actor* actor)
	{
		if (!actor)
			return nullptr;
		if (auto it = actors_map.find(actor->formID); it != actors_map.end())
			return &it->second;
		else
			return nullptr;
	}
}

bool IsSerializeFinished()
{
	return dbr_manager::ActorsManager::deserialized;
}

bool IsInActorsMap(uint32_t actorId)
{
	return dbr_manager::ActorsManager::contains_in_map(actorId);
}

bool IsInActorsMap(RE::Actor* actor)
{
	return IsInActorsMap(actor->formID);
}

std::optional<bool> IsActorExcluded(RE::Actor* actor)
{
	if (actor)
		if (auto preset = dbr_manager::ActorsManager::find(actor); preset)
			return preset->empty();
		else
			return false;
	return std::nullopt;
}

dbr_manager::ActorPreset* Find(RE::Actor* actor)
{
	if (actor)
		if (auto preset = dbr_manager::ActorsManager::find(actor); preset) {
			return preset;
		}
	return nullptr;
}
