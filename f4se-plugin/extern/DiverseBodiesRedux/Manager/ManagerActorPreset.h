#pragma once
#include "DiverseBodiesRedux/Parser/Diversers.h"
#include "DiverseBodiesRedux/Globals/Globals.h"

namespace logger = F4SE::log;

namespace dbr_manager
{
	struct ActorPreset
	{
		inline static std::mutex m{};
		RE::Actor* actor = nullptr;
		RE::TESNPC* base = nullptr;
		std::optional<std::string> body{};
		std::optional<std::string> skin{};
		std::optional<RE::BGSHeadPart*> hair{};
		std::optional<overlays::Collection> overlays{};

		ActorPreset(RE::Actor* a);
		ActorPreset(RE::Actor* a, int); //int для создания пустого пресета, нужен для ручного применения пресета, игнорирует excluded
		ActorPreset(boost::json::value&);
		ActorPreset() = default;
		ActorPreset(const ActorPreset&) = default;
		ActorPreset& operator=(const ActorPreset&) = default;
		ActorPreset(ActorPreset&&) noexcept = default;

		ActorPreset& operator=(ActorPreset&&) noexcept = default;

		bool empty() const;
		bool update() const;
		bool apply(bool reset = false, bool a_reloadAll = false, RE::RESET_3D_FLAGS a_additionalFlags = f3D::kNone, bool a_queueReset = false, RE::RESET_3D_FLAGS a_excludeFlags = f3D::kNone) const;
		f3D get_flags() const;
		f3D get_rflags() const;
		f3D update_head_and_return_flags() const;
		f3D update_and_return_flags() const;

		boost::json::object serialize() const;
		ActorPreset& deserialize(boost::json::object&);
	};
}
