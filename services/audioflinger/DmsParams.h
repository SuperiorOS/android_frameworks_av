#ifndef DMS_PARAMS_H_
#define DMS_PARAMS_H_

#include <stdint.h>


/**
 * Param names for DMS HIDL interface APIs
 * Names and values in sequence
 * Any change of these IDs must also apply to DaxService.Param class
 */

enum {
    DMS_INTF_PARAM_DAP_ON                      = 'DAPO',    //0x4441504F
    DMS_INTF_PARAM_DECODER_CP                  = 'DCCP',    //0x44434350
    DMS_INTF_PARAM_DECODER_DV                  = 'DCDV',    //0x44434456
    DMS_INTF_PARAM_DECODER_OUTPUT_COUNT        = 'DCOC',    //0x44434F43
    DMS_INTF_PARAM_DE_AMOUNT                   = 'DEAM',    //0x4445414D
    DMS_INTF_PARAM_SPATIALIZER_ACTIVE          = 'SPAA',    //0x53504141
    DMS_INTF_PARAM_GAME_DAP_EFFECT_ACTIVE      = 'GDEA',    //0x47444541
    DMS_INTF_PARAM_LAST_CODEC_ACTIVE_TIME      = 'LCTM',    //0x4C43544D
    DMS_INTF_PARAM_LAST_CODEC_ACTIVE_TYPE      = 'LCTP',    //0x4C435450
    DMS_INTF_PARAM_RELOAD_TUNING_FILE          = 'RLTF',    //0x524C5446
    DMS_INTF_PARAM_SPEAKER_IS_MONO             = 'SPKM',    //0x53504B4D
    DMS_INTF_PARAM_SPEAKER_IS_PORTRAIT         = 'SPKP',    //0x53504B50
    DMS_INTF_PARAM_ACTIVE_DEVICE               = 'ADEV',    //0x41444456
    DMS_INTF_PARAM_CHANNEL_COUNT               = 'CHCT',    //0x43484354
    DMS_INTF_PARAM_ACTIVE_PROFILE_IS_MOBILITY  = 'APMO',    //0x44504D4F
    DMS_INTF_PARAM_VOLUME_LEVELER_AMOUNT       = 'DVLA',    //0x44564c41
    DMS_INTF_PARAM_TUNING_PARAMETER_DSA        = 'DDSA',    //0x44445341
    DMS_INTF_PARAM_TUNING_PARAMETER_DHSA       = 'DHSA',    //0x44485341
    DMS_INTF_PARAM_TUNING_PARAMETER_DFSA       = 'DFSA',    //0x44465341
    DMS_INTF_PARAM_DEF_TUNING_FMT_VERION       = 'DTFV',    //0x44544656
    DMS_INTF_PARAM_IEQ_PRESET_NAME             = 'IEQN',    //0x4945514E
    DMS_INTF_PARAM_ATMOS_GAME_SYSTEM_GAIN      = 'AGSG',    //0x41475347
};

/**
 * Type of Dolby service client.
 */
typedef int32_t DolbyClientType;

#define DOLBY_CLIENT_INVALID           -1
#define DOLBY_CLIENT_NATIVE_SERVICE     0   // Client of Native service
#define DOLBY_CLIENT_DD                 1   // Client of UDC OMX component
#define DOLBY_CLIENT_DDP                2   // Client of DDP OMX component
#define DOLBY_CLIENT_DDP_JOC            3   // Client of UDC OMX component with JOC Decoder and DAP
#define DOLBY_CLIENT_AC4DEC             4   // Client of AC4 OMX component
#define DOLBY_CLIENT_DAP_EFF            5   // Client of full feature Dap component
#define DOLBY_CLIENT_DP_EFF             6   // Clinet of Device process component
#define DOLBY_CLIENT_GAME_DAP_EFF       7   // Clinet of Game Dap component

/* dolby 3.8 */
#define DOLBY_CLIENT_DDP_2CH            8   // Client of DDP decoder with 2 channel output
#define DOLBY_CLIENT_DDP_6CH            9   // Client of DDP decoder with 6 channel output
#define DOLBY_CLIENT_DDP_8CH            10   // Client of DDP decoder with 8 channel output
#define DOLBY_CLIENT_DDP_12CH           11   // Client of DDP decoder with 12 channel output
#define DOLBY_CLIENT_SPATIALIZER        12   // Client of Spatializer component
// AUDIO END

#endif //DMS_PARAMS_H_
