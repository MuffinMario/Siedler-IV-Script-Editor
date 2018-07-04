#pragma once
constexpr int IDC_SCRIPT_SET_BUTTON = 10000;
constexpr int IDC_SCRIPT_EDIT_BOX = 11000;
constexpr int IDC_MO_CHECK_NEW_WORLD = 12000;
constexpr int IDC_MO_CHECK_MAP_PREVIEW = 12001;
constexpr int IDC_OBJECTS_COMBO_BOX = 13000;

#define USE_DEBUG_CONSOLE 0


#if USE_DEBUG_CONSOLE == 1

// TAKEN FROM http://goodliffe.blogspot.com/2009/07/c-how-to-say-warning-to-visual-studio-c.html
#define STRINGIZE_HELPER(x) #x
#define STRINGIZE(x) STRINGIZE_HELPER(x)
#define WARNING(desc) message(__FILE__ "(" STRINGIZE(__LINE__) ") : Warning: " #desc)

#pragma WARNING(DEBUG CONSOLE ACTIVATED! TURN OFF ON RELEASE)
#endif

void initOriginalModule();
void initFunctions();
void freeAll();

bool setObjectOnGroundPlaceable(short objectID, bool placeable);
bool setObjectOnSandPlaceable(short objectID, bool placeable);
void overridePreviousPlaceableSettings();

enum : unsigned int{
	MMAP_CHANGE_RESOURCES = 0U,
	MMAP_CHANGE_ALLIANCES,
	MMAP_CHANGE_PLAYER_COUNT,
	MMAP_HIDE_MAP_PREVIEW,
	MMAP_CHANGE_ECONOMY_MODE,
	MMAP_ADDON,
	MMAP_CONFLICT_MODE,
	MMAP_UNKNOWN_1,
	MMAP_UNKNOWN_2,
	MMAP_NEW_WORLD,
	MMAP_UNKNOWN_3,
	MMAP_UNKNOWN_4,
	MMAP_UNKNOWN_5,
	MMAP_UNKNOWN_6,
	MMAP_UNKNOWN_7,
	MMAP_UNKNOWN_8
};