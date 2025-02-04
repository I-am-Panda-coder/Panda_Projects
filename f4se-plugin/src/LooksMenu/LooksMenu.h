#pragma once
#include <fstream>
#include <DiverseBodiesRedux/Hooks/FixedString.h>
#pragma warning(push)
#pragma warning(disable: 4100)

namespace LooksMenu
{
	//NAFBRIDGE for f4ee removeOverlay fix. https://www.nexusmods.com/fallout4/mods/91235
	typedef uint32_t UniqueID;

	typedef std::shared_ptr<F4EEFixedString> StringTableItem;
	typedef std::weak_ptr<F4EEFixedString> WeakTableItem;

	class OverlayInterface : public RE::BSTEventSink<RE::TESObjectLoadedEvent>, public RE::BSTEventSink<RE::TESLoadGameEvent>
	{
	public:
		OverlayInterface() :
			m_highestUID(0) {}

		typedef uint32_t UniqueID;

		enum
		{
			kVersion1 = 1,
			kVersion2 = 2,  // Version 2 now only saves uint32_t FormID instead of UInt64 Handle
			kSerializationVersion = kVersion2,
		};

		class OverlayData
		{
		public:
			enum Flags
			{
				kHasTintColor = (1 << 0),
				kHasOffsetUV = (1 << 1),
				kHasScaleUV = (1 << 2),
				kHasRemapIndex = (1 << 3)
			};

			OverlayData()
			{
				uid = 0;
				flags = 0;
				tintColor.r = 0.0f;
				tintColor.g = 0.0f;
				tintColor.b = 0.0f;
				tintColor.a = 0.0f;
				offsetUV.x = 0.0f;
				offsetUV.y = 0.0f;
				scaleUV.x = 1.0f;
				scaleUV.y = 1.0f;
				remapIndex = 0.50196f;
			}

			UniqueID uid;
			uint32_t flags;
			StringTableItem templateName;
			RE::NiColorA tintColor;
			RE::NiPoint2 offsetUV;
			RE::NiPoint2 scaleUV;
			float remapIndex;

			void UpdateFlags();

			void Save(const F4SE::SerializationInterface* intfc, uint32_t kVersion);
			bool Load(const F4SE::SerializationInterface* intfc, uint32_t kVersion, const std::unordered_map<uint32_t, StringTableItem>& stringTable);
		};
		typedef std::shared_ptr<OverlayData> OverlayDataPtr;

		class PriorityMap : public std::multimap<int32_t, OverlayDataPtr>
		{
		public:
			void Save(const F4SE::SerializationInterface* intfc, uint32_t kVersion);
			bool Load(const F4SE::SerializationInterface* intfc, bool isFemale, uint32_t kVersion, const std::unordered_map<uint32_t, StringTableItem>& stringTable);
		};
		typedef std::shared_ptr<PriorityMap> PriorityMapPtr;

		class OverlayMap : public std::unordered_map<uint32_t, PriorityMapPtr>
		{
		public:
			void Save(const F4SE::SerializationInterface* intfc, uint32_t kVersion);
			bool Load(const F4SE::SerializationInterface* intfc, bool isFemale, uint32_t kVersion, const std::unordered_map<uint32_t, StringTableItem>& stringTable);
		};

		class OverlayTemplate
		{
		public:
			OverlayTemplate() :
				playable(false), sort(0), transformable(false), tintable(false) {}

			typedef std::unordered_map<uint32_t, std::pair<F4EEFixedString, bool>> MaterialMap;

			F4EEFixedString displayName;
			MaterialMap slotMaterial;
			bool playable;
			bool transformable;
			bool tintable;
			int32_t sort;
		};
		typedef std::shared_ptr<OverlayTemplate> OverlayTemplatePtr;

		virtual void Save(const F4SE::SerializationInterface* intfc, uint32_t kVersion);
		virtual bool Load(const F4SE::SerializationInterface* intfc, uint32_t kVersion, const std::unordered_map<uint32_t, StringTableItem>& stringTable);
		virtual void Revert();

		virtual void LoadOverlayMods();
		virtual void ClearMods();
		virtual bool LoadOverlayTemplates(const std::string& filePath);

		virtual UniqueID AddOverlay(RE::Actor* actor, bool isFemale, int32_t priority, const F4EEFixedString& templateName, const RE::NiColorA& tintColor, const RE::NiPoint2& offsetUV, const RE::NiPoint2& scaleUV);
		virtual bool RemoveOverlay(RE::Actor* actor, bool isFemale, UniqueID uid);
		virtual bool RemoveAll(RE::Actor* actor, bool isFemale);
		virtual bool ReorderOverlay(RE::Actor* actor, bool isFemale, UniqueID uid, int32_t newPriority);

		virtual bool ForEachOverlay(RE::Actor* actor, bool isFemale, std::function<void(int32_t, const OverlayDataPtr&)> functor);
		virtual bool ForEachOverlayBySlot(RE::Actor* actor, bool isFemale, uint32_t slotIndex, std::function<void(int32_t, const OverlayDataPtr&, const F4EEFixedString&, bool)> functor);

		virtual void ForEachOverlayTemplate(bool isFemale, std::function<void(const F4EEFixedString&, const OverlayTemplatePtr&)> functor);

		virtual bool UpdateOverlays(RE::Actor* actor);
		virtual bool UpdateOverlay(RE::Actor* actor, uint32_t slotIndex);

		virtual void CloneOverlays(RE::Actor* source, RE::Actor* target);

		virtual UniqueID GetNextUID();

		virtual RE::NiNode* GetOverlayRoot(RE::Actor* actor, RE::NiNode* rootNode, bool createIfNecessary = true);

		virtual const OverlayTemplatePtr GetTemplateByName(bool isFemale, const F4EEFixedString& name);
		virtual const OverlayDataPtr GetOverlayByUID(UniqueID uid);

		std::pair<int32_t, OverlayDataPtr> GetActorOverlayByUID(RE::Actor* actor, bool isFemale, UniqueID uid);

		bool HasSkinChildren(RE::NiAVObject* slot);
		void LoadMaterialData(RE::TESNPC* npc, RE::BSTriShape* shape, const F4EEFixedString& material, bool effect, const OverlayDataPtr& overlayData);

		void DestroyOverlaySlot(RE::Actor* actor, RE::NiNode* overlayHolder, uint32_t slotIndex);
		bool UpdateOverlays(RE::Actor* actor, RE::NiNode* rootNode, RE::NiAVObject* object, uint32_t slotIndex);

	protected:
		friend class OverlayTemplate;
		friend class PriorityMap;
		friend class OverlayData;

		RE::BSSpinLock m_overlayLock;
		OverlayMap m_overlays[2];
		std::vector<UniqueID> m_freeIndices;
		std::unordered_map<UniqueID, OverlayDataPtr> m_dataMap;
		UniqueID m_highestUID;
		std::unordered_map<F4EEFixedString, OverlayTemplatePtr> m_overlayTemplates[2];
		friend bool HookedRemoveOverlay(OverlayInterface* OverlayInterface, RE::Actor* actor, bool isFemale, UniqueID uid);
	};

	bool HookedRemoveOverlay(OverlayInterface* overlayInterface, RE::Actor* actor, bool isFemale, UniqueID uid)
	{
		RE::BSSpinLock locker(overlayInterface->m_overlayLock);
		auto hit = overlayInterface->m_overlays[isFemale ? 1 : 0].find(actor->formID);
		if (hit == overlayInterface->m_overlays[isFemale ? 1 : 0].end())
			return false;

		OverlayInterface::PriorityMapPtr priorityMap = hit->second;
		if (!priorityMap)
			return false;

		for (auto it = priorityMap->begin(); it != priorityMap->end(); ++it) {
			OverlayInterface::OverlayDataPtr overlayPtr = it->second;
			if (!overlayPtr)
				continue;

			if (overlayPtr->uid == uid) {
				overlayInterface->m_dataMap.erase(overlayPtr->uid);
				overlayInterface->m_freeIndices.push_back(overlayPtr->uid);
				priorityMap->erase(it);
				return true;
			}
		}

		return false;
	}
	
	//NAFBRIDGE END
	enum LMVersion
	{
		k1_6_20,
		k1_6_18
	};

	class BodyMorphInterface
	{
	public:
		virtual void Save(const F4SE::SerializationInterface*, uint32_t) {}
		virtual bool Load(const F4SE::SerializationInterface*, bool, uint32_t, const std::unordered_map<uint32_t, std::string>&) { return false; }
		virtual void Revert() {}

		virtual void LoadBodyGenSliderMods() {}
		virtual void ClearBodyGenSliders() {}

		virtual bool LoadBodyGenSliders(const std::string& filePath) { return false; }

		virtual void ForEachSlider(uint8_t gender, std::function<void(const std::shared_ptr<uint8_t>& slider)> func) {}

		virtual std::shared_ptr<uint8_t> GetTrishapeMap(const char* relativePath) { return nullptr; }
		virtual std::shared_ptr<uint8_t> GetMorphMap(RE::Actor* actor, bool isFemale) { return nullptr; }

		virtual void SetMorph(RE::Actor* actor, bool isFemale, const RE::BSFixedString& morph, RE::BGSKeyword* keyword, float value) {}
		virtual float GetMorph(RE::Actor* actor, bool isFemale, const RE::BSFixedString& morph, RE::BGSKeyword* keyword) { return 0.0f; }

		virtual void GetKeywords(RE::Actor* actor, bool isFemale, const RE::BSFixedString& morph, std::vector<RE::BGSKeyword*>& keywords) {}
		virtual void GetMorphs(RE::Actor* actor, bool isFemale, std::vector<RE::BSFixedString>& morphs) {}
		virtual void RemoveMorphsByName(RE::Actor* actor, bool isFemale, const RE::BSFixedString& morph) {}
		virtual void RemoveMorphsByKeyword(RE::Actor* actor, bool isFemale, RE::BGSKeyword* keyword) {}
		virtual void ClearMorphs(RE::Actor* actor, bool isFemale) {}
		virtual void CloneMorphs(RE::Actor* source, RE::Actor* target) {}

		virtual void GetMorphableShapes(RE::NiAVObject* node, std::vector<std::shared_ptr<uint8_t>>& shapes) {}
		virtual bool ApplyMorphsToShapes(RE::Actor* actor, RE::NiAVObject* slotNode) { return false; }
		virtual bool ApplyMorphsToShape(RE::Actor* actor, const std::shared_ptr<uint8_t>& morphableShape) { return false; }
		virtual bool UpdateMorphs(RE::Actor* actor) { return false; }
	};

	uint64_t checksum(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::binary);
		if (!file) {
			logger::warn("Error opening file: {}", filename);
			return 0;
		}

		std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(file), {});

		uint64_t sum = 0;
		for (uint8_t byte : buffer) {
			sum += byte;
		}

		return sum;
	}

	bool isInstalled = false;
	LMVersion version;

	namespace detail
	{
		typedef bool(ApplyMorphsToShapes)(BodyMorphInterface*, RE::Actor*, RE::NiAVObject*);
		typedef void(LoadBodyGenSliderMods)(BodyMorphInterface*);
		using RemoveOverlayHandler = bool (*)(OverlayInterface*, bool, UniqueID); //NAF Bridge F4EE Remove Overlay fix

		DetourXS applyMorphsHook;
		DetourXS applyRemoveOverlayHook; //NAF Bridge F4EE Remove Overlay fix
		ApplyMorphsToShapes* originalApply;

		BodyMorphInterface* g_bodyMorphInterface = nullptr;
		RemoveOverlayHandler originalRemoveOverlay = nullptr;

		uint64_t GetVersionOffset(uint64_t off1_6_18, uint64_t off1_6_20) {
			switch (version) {
			case k1_6_18:
				return off1_6_18;
			case k1_6_20:
				return off1_6_20;
			default:
				return 0;
			}
		}

		bool HookedApplyMorphs(BodyMorphInterface* a1, RE::Actor* a2, RE::NiAVObject* a3) {
			std::this_thread::sleep_for(std::chrono::microseconds(1));
			return originalApply(a1, a2, a3);
		}

		void RegisterHook() {
			uintptr_t baseAddr = reinterpret_cast<uintptr_t>(GetModuleHandleA("f4ee.dll"));

			if (!applyMorphsHook.Create(reinterpret_cast<LPVOID>(baseAddr + GetVersionOffset(0x10040, 0xFFCC)), &HookedApplyMorphs)) {
				logger::warn("Failed to create ApplyMorphsToShapes hook!");
			} else {
				logger::info("Create ApplyMorphsToShapes hook success!");
				originalApply = reinterpret_cast<ApplyMorphsToShapes*>(applyMorphsHook.GetTrampoline());
			}

			g_bodyMorphInterface = reinterpret_cast<BodyMorphInterface*>(baseAddr + GetVersionOffset(0xFB440, 0xF4DF0));

			//NAF Bridge F4EE Remove Overlay fix
			if (!applyRemoveOverlayHook.Create(reinterpret_cast<LPVOID>(baseAddr + 0x420D0), &HookedRemoveOverlay)) {
				logger::warn("Failed to create applyRemoveOverlayHook hook!");
			} else {
				logger::info("Create applyRemoveOverlayHook hook success!");
				originalRemoveOverlay = reinterpret_cast<RemoveOverlayHandler>(applyRemoveOverlayHook.GetTrampoline());
			}
		}
	}

	void Init() {
		if (F4SE::GetPluginInfo("F4EE").has_value()) {
			// Unfortunately expired has never changed F4EE's plugin version, so we gotta do this the old fashioned way.
			if (auto sum = checksum("Data/F4SE/Plugins/f4ee.dll"); sum > 0) {
				isInstalled = true;
				std::string verStr = "";

				switch (sum) {
				case LOOKSMENU_1_6_18_CHECKSUM:
					version = k1_6_18;
					verStr = "1.6.18";
					break;
				case LOOKSMENU_1_6_20_CHECKSUM:
					version = k1_6_20;
					verStr = "1.6.20";
					break;
				default:
					isInstalled = false;
				}

				if (isInstalled) {
					logger::info("LooksMenu version {} detected.", verStr);
					detail::RegisterHook();
				} else {
					logger::info("Unsupported LooksMenu version. Checksum: {:X}", sum);
				}

			} else {
				logger::warn("Failed to get LooksMenu version, support disabled.");
			}
		} else {
			logger::info("LooksMenu not installed, support disabled.");
		}
	}

	void SetMorph(RE::Actor* actor, const RE::BSFixedString& morph, RE::BGSKeyword* keyword, float value)
	{
		if (isInstalled && actor && detail::g_bodyMorphInterface) {
			bool isFemale = (actor->GetSex() == 1);
			detail::g_bodyMorphInterface->SetMorph(actor, isFemale, morph, keyword, value);
		}
	}

	void UpdateMorphs(RE::Actor* actor)
	{
		if (isInstalled && actor && detail::g_bodyMorphInterface) {
			detail::g_bodyMorphInterface->UpdateMorphs(actor);
		}
	}

	void RemoveMorphsByName(RE::Actor* actor, const RE::BSFixedString& morph) {
		if (isInstalled && actor && detail::g_bodyMorphInterface) {
			bool isFemale = (actor->GetSex() == 1);
			detail::g_bodyMorphInterface->RemoveMorphsByName(actor, isFemale, morph);
		}
	}
}

#pragma warning(pop)
