#pragma once
#include "DiverseBodiesRedux/Parser/Diversers.h"
#include <concurrent_queue.h>
#include <concurrent_unordered_map.h>
#include <concurrent_unordered_set.h>
#include <DiverseBodiesRedux/Globals/Globals.h>
#include <DiverseBodiesRedux/Hooks/hooks.h>
#include <DiverseBodiesRedux/version.h>
#include <functional>
#include <NAFicator/ThreadPool/ThreadPool.h>
#include <ppl.h>
#include <queue>
#include <utils/RandomGenerator.h>

#include "ManagerActorPreset.h"

using namespace boost::json;

namespace TESObjectREFR_Events
{
	//TESCombatEvent		0x004420F0
	//TESDeathEvent			0x00442550
	//TESFurnitureEvent		0x00442C30
	//TESLoadGameEvent		0x00442EB0
	//TESObjectLoadedEvent	0x004431D0
	//TESInitScriptEvent	0x00442D70

	inline void RegisterForObjectLoaded(RE::BSTEventSink<RE::TESObjectLoadedEvent>* a_sink)
	{
		HMODULE module = GetModuleHandleA(NULL);
		uintptr_t baseAddr = reinterpret_cast<uintptr_t>(module);
		
		using func_t = decltype(&RegisterForObjectLoaded);
		REL::Relocation<func_t> func{ baseAddr + 0x436A40 };
		return func(a_sink);
	}

	inline void UnregisterForObjectLoaded(RE::BSTEventSink<RE::TESObjectLoadedEvent>* a_sink)
	{
		HMODULE module = GetModuleHandleA(NULL);
		uintptr_t baseAddr = reinterpret_cast<uintptr_t>(module);
		
		using func_t = decltype(&UnregisterForObjectLoaded);
		REL::Relocation<func_t> func{ baseAddr + 0x436A70 };
		return func(a_sink);
	}
};

template <class Object>
class UnorderedSet
{

private:
	std::unordered_set<Object> _set;
};

namespace papyrus
{
	extern void ApplyRandomHair(std::monostate, RE::Actor*);
	extern void ApplyBodyPresetFromFile(std::monostate, RE::Actor*);
};

namespace dbr_manager
{
	class ActorsManager :
		public RE::BSTEventSink<RE::TESObjectLoadedEvent>,
		public RE::BSTEventSource<RE::TESObjectLoadedEvent>
	{
		inline static ActorsManager* instance = nullptr;

		inline static RE::BSSpinLock lock{};

	public:
		// Deleted copy constructor and assignment operator to prevent copying
		ActorsManager(const ActorsManager&) = delete;
		ActorsManager& operator=(const ActorsManager&) = delete;

		// Static method to access the singleton instance
		inline static ActorsManager* getInstance()
		{
			if (instance == nullptr)
				instance = new ActorsManager{};
			return instance;
		}

		ActorPreset* get(RE::Actor*) const;
		static bool contains_in_map(uint32_t actorId);
		static ActorPreset* find(RE::Actor* actor);

		RE::BSEventNotifyControl ProcessEvent(const RE::TESObjectLoadedEvent& a_event, RE::BSTEventSource<RE::TESObjectLoadedEvent>* a_source) override;

		static void Serialize(const F4SE::SerializationInterface* a_intfc);
		static void Deserialize(const F4SE::SerializationInterface* a_intfc);
		inline static bool deserialized{ false };
		
		~ActorsManager()
		{
			{
				delete this;
			};
		}
		
	private:
		
		static boost::json::array gatherActorPresets();
		static bool move_to_map(ActorPreset&&);
		static void update_loaded();
		static bool apply(ActorPreset*);
		static bool proccess_actor_after_loaded(RE::Actor* actor);

		ActorsManager(){}; //private constructor

		inline static concurrency::concurrent_unordered_map<uint32_t, ActorPreset> actors_map{};
		constexpr static size_t serialization_ver = 100;

		friend void papyrus::ApplyRandomHair(std::monostate, RE::Actor* actor);
		friend void papyrus::ApplyBodyPresetFromFile(std::monostate, RE::Actor* actor);
	};	
}

bool IsSerializeFinished();
bool IsInActorsMap(uint32_t);
bool IsInActorsMap(RE::Actor*);
dbr_manager::ActorPreset* Find(RE::Actor*);
std::optional<bool> IsActorExcluded(RE::Actor* actor);
