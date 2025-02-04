#pragma once
#include <algorithm>
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <pugixml/src/pugixml.hpp>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

#include "F4SE/F4SE.h"
#include "RE/Fallout.h"
#include <spdlog/sinks/basic_file_sink.h>

#include "DiverseBodiesRedux/Parser/Conditions.h"
#include "DiverseBodiesRedux/Globals/Globals.h"
#include "DiverseBodiesRedux/Hooks/LooksMenuInterfaces.h"
#include <DiverseBodiesRedux/Parser/HairFilter.h>
#include <NAFicator/ThreadPool/ThreadPool.h>
#include <utils/RandomGenerator.h>
#include <utils/utility.h>

#include <boost/json.hpp>

namespace bodymorphs
{
	class Preset
	{
	public:
		enum class Type : char
		{
			NONE,
			FAT,
			SLIM,
			SEXY,
			ATHL
		};

	private:
		std::optional<std::string> filename;
		Type type;
		RE::Actor::Sex sex;
		std::unordered_map<std::string, float> morphs;

		inline static std::map<std::string, Preset> MAP{};

	public:
		inline static std::mutex processing_lock;
		inline static std::mutex process_counter_lock;
		inline static std::condition_variable processingCondition;

		Preset() = delete;
		Preset(const Preset&) = delete;
		Preset& operator=(const Preset&) = delete;
		Preset(const std::filesystem::path&);
		Preset(Preset&& other) noexcept;

		Preset& operator=(Preset&& other) noexcept;
		bool operator<(const Preset&) const;

		bool empty() const;
		const std::string& name() const;
		RE::Actor::Sex get_sex() const;
		Type get_bodytype() const;
		bool apply(RE::Actor* actor) const;
	//friends
		friend void Parse();
		friend std::vector<Preset*> ApplyFilter(RE::Actor::Sex, Preset::Type);
		friend Preset* Get(const std::string&);
	};

	inline Preset::Type GetTypeFromInt(int t) { return (t > 0 && t < 5) ? static_cast<Preset::Type>(t) : Preset::Type::NONE; }
	std::vector<Preset*> ApplyFilter(RE::Actor::Sex sex, Preset::Type bodyType = Preset::Type::NONE);

	void Parse();

	Preset* Get(const std::string&);
	void Remove(RE::Actor* actor);
	Preset* GetRandom(RE::Actor*);
	Preset* GetRandom(bool isFemale, Preset::Type bodyType = Preset::Type::NONE);
}

namespace overlays
{	
	struct Settings
	{
		RE::NiColorA tint{ 0.0f, 0.0f, 0.0f, 1.0f };
		RE::NiPoint2 offsetUV{ 0.0, 0.0 };
		RE::NiPoint2 scaleUV{ 1.0, 1.0 };
		int priority{};

		Settings() = default;
		Settings(boost::json::value& item);

		boost::json::object serialize() const;
		Settings& deserialize(const boost::json::value& item);
	};

	struct Data
	{
		std::string id{};
		Settings settings{};

		Data() = default;
		Data(boost::json::value& item);

		bool empty() const;
		bool apply(RE::Actor* actor) const;
		bool is_valid() const;

		bool operator==(const Data&) const;

		struct Hash
		{
			std::size_t operator()(const Data& overlayData) const;
		};

		struct Equal
		{
			bool operator()(const Data& lhs, const Data& rhs) const;
		};

		boost::json::object serialize() const;
		Data& deserialize(const boost::json::value& item);

	};

	class Preset
	{
	public:
		struct Overlay
		{
			std::string id{};
			std::unordered_set<Data, Data::Hash, Data::Equal> overlays;
			ConditionSettings conditions{};
			std::unordered_set<std::string> remove;

			Overlay() = default;
			Overlay(boost::json::value& item);

			bool empty() const;
			bool apply(RE::Actor* actor) const;

			bool operator==(const Overlay&) const;
			bool operator==(const std::string&) const;

			bool check(const RE::Actor*) const;
			bool is_condition_random() const;
			const auto& get_remove() const;
			const auto& get_overlays() const;

			struct Hash
			{
				std::size_t operator()(const Overlay& overlaysArr) const;
			};

			struct Equal
			{
				bool operator()(const Overlay& lhs, const Overlay& rhs) const;
			};
		};

	private:

		inline static std::set<std::string> ValidOverlays{};
		inline static std::map<std::string, Preset> MAP{};

		std::string id = std::string{};
		std::unordered_set<Overlay, Overlay::Hash, Overlay::Equal> details;
		std::unordered_set<std::string> remove;
		ConditionSettings conditions{};

	public:
		Preset() = delete;
		Preset(const Preset&) = delete;
		Preset& operator=(const Preset&) = delete;
		Preset(Preset&& other) noexcept = default;

		Preset(boost::json::value& obj);
		bool operator==(const Preset&) const;
		bool operator==(const std::string&) const;

		const std::string& name() const;
		bool empty() const;
		bool apply(RE::Actor* actor) const;
		const Overlay* const find(const std::string& overlay_id);
		bool check(const RE::Actor* actor) const;

		static const auto& get_map();
		const auto& get_remove() const;
		const auto& get_details() const;
		bool is_condition_random() const;
		static bool is_valid(const std::string&);

	//friends
		friend Data;
		friend void Parse();
	};

	struct Collection  //need this for serialization
	{
		std::unordered_set<Data, Data::Hash, Data::Equal> overlays{};
		std::unordered_set<std::string> remove{};

		Collection() = default;
		Collection(const RE::Actor* actor);
		Collection(const Collection&) = default;
		Collection(Collection&&) noexcept = default;
		Collection& operator=(const Collection&) = default;
		Collection& operator=(Collection&&) noexcept = default;


		bool apply(RE::Actor*) const;
		void validate();
		bool empty() const;
		boost::json::object serialize() const;
		Collection& deserialize(const boost::json::value& item);

	private:
		Collection& create_new_collection_for_actor(const RE::Actor*);
		void handle_overlays(const RE::Actor*, const Preset::Overlay& overlay);
		void handle_details(const RE::Actor*, const std::unordered_set<Preset::Overlay, Preset::Overlay::Hash, Preset::Overlay::Equal>& details);
		void handle_detail(const RE::Actor*, const Preset::Overlay& detail);
	};

	const std::set<std::string>& GetValid();
	void Parse();

	Preset* GetOverlay(const std::string&);
	void Remove(RE::Actor* actor, const std::string& id);
	void Remove(RE::Actor* actor);
}

namespace skins
{
	class Preset
	{
		std::string id{};
		ConditionSettings conditions{};

		inline static std::map<std::string, Preset> MAP{};

	public:
		Preset() = delete;
		Preset(const Preset&) = delete;
		Preset& operator=(const Preset&) = delete;
		Preset(Preset&& other) noexcept = default;

		Preset(boost::json::value& obj);
		
		const std::string& name() const;
		bool empty() const;
		bool apply(RE::Actor* actor) const;
		
	//friends
		friend void Parse();
		friend std::vector<Preset*> ApplyFilter(RE::Actor*);
		friend Preset* Get(const std::string&);
	};

	void Remove(RE::Actor* actor);
	void Parse();
	std::vector<Preset*> ApplyFilter(RE::Actor*);
	Preset* GetRandom(RE::Actor*);
	Preset* Get(const std::string&);
	
}

namespace hairs
{	
	class Preset
	{
		std::string id;  //formid
		RE::BGSHeadPart* hair;

		inline static std::map<std::string, Preset> MAP{};

	public:
		Preset(RE::BGSHeadPart* hair);

		bool is_vanilla() const;
		const std::string& name() const;
		bool apply(RE::Actor*) const;
		static bool apply(RE::TESNPC* npc, RE::BGSHeadPart* hair);
		RE::BGSHeadPart* hpart() const;

	//friends
		friend void Parse();
		friend std::vector<Preset*> ApplyFilter(RE::Actor* actor);
	};

	void Parse();
	std::vector<Preset*> ApplyFilter(RE::Actor*);
	Preset* GetRandom(RE::Actor*);
}

inline std::string get_json(const std::filesystem::path& filepath)
{
	std::ifstream file(filepath);
	if (!file) {
		throw std::runtime_error("Could not open file: " + filepath.string());
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

inline void remove_chargen(RE::Actor* actor)
{	
	if (!actor)
		return;

	auto npc = get_leveled_TESNPC(actor->GetNPC());

	if (npc && npc->formType == RE::ENUM_FORM_ID::kNPC_) {
		npc->actorData.actorBaseFlags |= RE::TESActorBase::ACTOR_BASE_DATA_FLAGS::kFlagIsCharGenFacePreset;
	}
	/*auto face = npc->GetRootFaceNPC();
	if (!face)
		return;
	face->actorData.actorBaseFlags |= RE::TESActorBase::ACTOR_BASE_DATA_FLAGS::kFlagIsCharGenFacePreset;*/

	/*auto hparts = npc->GetHeadParts();
	if (hparts.empty())
		return;
	for (auto& hp : hparts) {
		hp->chargenConditions.head = nullptr;
	}*/
}

inline void remove_chargen(RE::TESNPC* npc)
{
	if (!npc)
		return;

	npc = get_leveled_TESNPC(npc);

	if (npc && npc->formType == RE::ENUM_FORM_ID::kNPC_) {
		npc->actorData.actorBaseFlags |= RE::TESActorBase::ACTOR_BASE_DATA_FLAGS::kFlagIsCharGenFacePreset;
	}
}
