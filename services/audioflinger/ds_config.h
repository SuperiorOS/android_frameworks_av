#ifndef DOLBY_DS_CONFIG_H_
#define DOLBY_DS_CONFIG_H_

#include <system/audio.h>
#include <hardware/audio_effect.h>
#include "DmsParams.h"

/*
    Devices that require software DAP on platform that has DSP implementation.
    NOTE: This definition can be chipset dependent, check the chipset and re-define
          it if necessary
*/
#define NO_OFFLOAD_DEVICES (AUDIO_DEVICE_OUT_REMOTE_SUBMIX)

constexpr char C2_PARAMKEY_DOLBY_DAX_STATE[] = "dolby.dax-state";
constexpr char C2_PARAMKEY_DOLBY_PROFILE_INDEX[] = "dolby.profile-index";
constexpr char C2_PARAMKEY_DOLBY_ACTIVE_DEVICE[] = "dolby.active-device";
constexpr char C2_PARAMKEY_DOLBY_OUTPUT_LEVEL[] = "dolby.output-level";
constexpr char C2_PARAMKEY_DOLBY_DIALOG_ENHANCEMENT_GAIN[] = "dolby.dialog-enhancement-gain";

static const uint32_t kParamIndexDolbyDaxState = 0xB201;
static const uint32_t kParamIndexDolbyProfileIndex = 0xB202;
static const uint32_t kParamIndexDolbyActiveDevice = 0xB203;
static const uint32_t kParamIndexDolbyOutputLevel = 0xB204;
static const uint32_t kParamIndexDolbyDialogEnhancementGain = 0xB205;

/*
    Define the Dolby parameters cName and key for Dolby OMX component
*/
#define DOLBY_PARAM_CNAME "dolby"
#define DOLBY_PARAM_KEY_DAX_STATE "dax-state"
#define DOLBY_PARAM_KEY_PROFILE_INDEX "profile-index"
#define DOLBY_PARAM_KEY_ACTIVE_DEVICE "active-device"
#define DOLBY_PARAM_KEY_OUTPUT_LEVEL "output-level"
#define DOLBY_PARAM_KEY_DIALOG_ENHANCEMENT_GAIN "dialog-enhancement-gain"

/*
    Define the Dolby process effect name
*/
#define DAPNAME_OFFLOAD "DAP_offload"
#define DAPNAME "DAP"

#ifdef DOLBY_SPATIALIZER
#define DAPNAME_SPATIALIZER "DAP_Spatializer"
#endif // DOLBY_SPATIALIZER

/*
    Define the Dolby Game Dap effect name
*/
#define DAPNAME_GAME "DAP_game"

/*
    Define the Dolby voice effect name
*/
#define VQENAME "VQE"

/*
    Type-UUID and UUID for DAP Effect.
    IMPORTANT NOTES: Do not change these values without updating their counterparts in DolbyAudioEffect.java
*/
static const effect_uuid_t EFFECT_SL_IID_DAP = // 46d279d9-9be7-453d-9d7c-ef937f675587
{ 0x46d279d9, 0x9be7, 0x453d, 0x9d7c, {0xef, 0x93, 0x7f, 0x67, 0x55, 0x87} };
static const effect_uuid_t EFFECT_UUID_DAP = // 9d4921da-8225-4f29-aefa-39537a04bcaa
{ 0x9d4921da, 0x8225, 0x4f29, 0xaefa, {0x39, 0x53, 0x7a, 0x04, 0xbc, 0xaa} };
static const effect_uuid_t EFFECT_UUID_DAP_SW = // 6ab06da4-c516-4611-8166-452799218539
{ 0x6ab06da4, 0xc516, 0x4611, 0x8166, {0x45, 0x27, 0x99, 0x21, 0x85, 0x39} };
static const effect_uuid_t EFFECT_UUID_DAP_HW = // a0c30891-8246-4aef-b8ad-d53e26da0253
{ 0x9d4921da, 0x8225, 0x4f29, 0xaefa, {0x39, 0x53, 0x7a, 0x04, 0xbc, 0xaa} };

/*
    Type-UUID and UUID for Game DAP Effect.
*/
static const effect_uuid_t EFFECT_SL_IID_GAME_DAP = // 12ea851f-0e12-4bde-a5e5-71f5651e9f96
{ 0x12ea851f, 0x0e12, 0x4bde, 0xa5e5, {0x71, 0xf5, 0x65, 0x1e, 0x9f, 0x96} };
static const effect_uuid_t EFFECT_UUID_GAME_DAP = // 3783c334-d3a0-4d13-874f-0032e5fb80e2
{ 0x3783c334, 0xd3a0, 0x4d13, 0x874f, {0x00, 0x32, 0xe5, 0xfb, 0x80, 0xe2} };
static const effect_uuid_t EFFECT_UUID_GAME_DAP_SW = // f62d3a5b-e3fd-40e0-8ee4-4f1ea90b5ed9
{ 0xf62d3a5b, 0xe3fd, 0x40e0, 0x8ee4, {0x4f, 0x1e, 0xa9, 0x0b, 0x5e, 0xd9} };
static const effect_uuid_t EFFECT_UUID_GAME_DAP_HW = // e653a3d1-0856-4810-8d2b-3f37c29a5a17
{ 0xe653a3d1, 0x0856, 0x4810, 0x8d2b, {0x3f, 0x37, 0xc2, 0x9a, 0x5a, 0x17} };

/*
    Type-UUID and UUID for VQE Effect.
    IMPORTANT NOTES: Do not change these values without updating their counterparts in DolbyVoiceEffect.java
*/
static const effect_uuid_t EFFECT_SL_IID_VQE = // 733fefa3-667e-44e4-a1a5-4a3d4c980f50
{ 0x733fefa3, 0x667e, 0x44e4, 0xa1a5, {0x4a, 0x3d, 0x4c, 0x98, 0x0f, 0x50} };
static const effect_uuid_t EFFECT_UUID_VQE = // 64a0f614-7fa4-48b8-b081-d59dc954616f
{ 0x64a0f614, 0x7fa4, 0x48b8, 0xb081, {0xd5, 0x9d, 0xc9, 0x54, 0x61, 0x6f} };
static const effect_uuid_t EFFECT_UUID_VQE_SW = // e5f7f902-8b93-4aa2-a55e-d9acd7348652
{ 0xe5f7f902, 0x8b93, 0x4aa2, 0xa55e, {0xd9, 0xac, 0xd7, 0x34, 0x86, 0x52} };
static const effect_uuid_t EFFECT_UUID_VQE_HW = // 8dacff1e-4c9d-4bd6-abb1-27b386ebcc7b
{ 0x8dacff1e, 0x4c9d, 0x4bd6, 0xabb1, {0x27, 0xb3, 0x86, 0xeb, 0xcc, 0x7b} };

/*
   When playing Dolby Content, the gloabl DAP effect will provide 4dB system gain
*/
#define DOLBY_CONTENT_SYSTEM_BOOST 4

/*
   Dolby spatializer parameters
*/
#define DOLBY_SPATIALIZER_PARAM_PROFILE_ID              (0x100)
#define DOLBY_SPATIALIZER_PARAM_UNMUTED_IN_CHANNEL_MASK (0x110)
#define DOLBY_SPATIALIZER_PARAM_DP_ENABLE               (0x200) // MIUI ADD

/**
    DS effect parameter identifiers
*/
enum DolbyEffectParams {
    EFFECT_PARAM_SET_VALUES = 0,
    EFFECT_PARAM_TUNING = 1,
    EFFECT_PARAM_TUNING_LEN = 2,
    EFFECT_PARAM_VERSION = 3,
    EFFECT_PARAM_SELECTED_TUNING = 4,
    EFFECT_PARAM_CPDP_VALUES = 5, // Set/Get parameter(s) through Effect API
    EFFECT_PARAM_PROFILE_NAME = 6,
    EFFECT_PARAM_TUNING_VERSION = 7,
    EFFECT_PARAM_IEQ_PRESET_NAME = 8,
    EFFECT_PARAM_VQE_VALUES = 10, // Set/Get VQE parameter(s) through Effect API
    // Here internal effect params start.
    EFFECT_PARAM_SET_PREGAIN = 0x10,
    EFFECT_PARAM_SET_POSTGAIN = 0x11,
    EFFECT_PARAM_SET_BYPASS = 0x12,
    EFFECT_PARAM_SKIP_HARD_BYPASS = 0x13,
    EFFECT_PARAM_IO_HANDLE = 0x14,
    EFFECT_PARAM_SET_AUDIO_FLAG = 0x15,
    // Here VQE effect params start.
    EFFECT_PARAM_RESET_NOISE_ESTIMATION = 0x0100,

    // Here head tracking params start
    EFFECT_PARAM_HT_EN = 0x0200,
    EFFECT_PARAM_IHT_YPR,
    EFFECT_PARAM_IHT_MAN_REC_TRG,
};

// This is event ID for VQE parameter. Use maximum significent byte to distinguish with different
// events. Need to align with DaxVoiceParams.java.
enum DolbyVoiceEffectEvent {
    EFFECT_PARAM_VQE_ENABLE = 0x00000000,
    // EFFECT_PARAM_VQE_XXX = 0x01000000
};

#define ATMOS_GAME_512_ACTIVE -1
#define ATMOS_GAME_INACTIVE 0

#define MAX_PROFILE_NAME_LEN 32

#ifdef DOLBY_DAP_BYPASS_SMALL_VOLUME
// This gate value can be updated device by device
static const int32_t kGainGate = -830;
#endif

#endif//DOLBY_DS_CONFIG_H_
