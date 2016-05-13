/********************************************/
/*LVM_Equaliser.c*/
/********************************************/
#include <malloc.h>
#include <string.h>
#include "LVEQNB_Private.h"
#include "LVEQNB.h"
#include "LVM_Equaliser.h"

#define FIVEBAND_NUMBANDS          5
#define MAX_NUM_BANDS              5
#define LVM_EQ_NBANDS              5    /* Number of bands for equalizer */
#define ALIGN(x, n)    (((x)+(n)-1)&~(n-1))
#define MAX_BLOCK_SIZE 4096

/* N-Band Equaliser operating mode */
typedef enum
{
    LVM_EQNB_OFF   = 0,
    LVM_EQNB_ON    = 1,
    LVM_EQNB_DUMMY = LVM_MAXENUM
} LVM_EQNB_Mode_en;

/* N-Band equaliser band definition */
typedef struct
{
    LVM_INT16                   Gain;                   /* Band gain in dB */
    LVM_UINT16                  Frequency;              /* Band centre frequency in Hz */
    LVM_UINT16                  QFactor;                /* Band quality factor (x100) */
} LVM_EQNB_BandDef_t;

/* Control Parameter structure */
typedef struct
{
    /* General parameters */
    LVM_Mode_en                 OperatingMode;          /* Bundle operating mode On/Bypass */
    LVM_Fs_en                   SampleRate;             /* Sample rate */
    LVM_Format_en               SourceFormat;           /* Input data format */

    /* N-Band Equaliser parameters */
    LVM_EQNB_Mode_en            EQNB_OperatingMode;     /* N-Band Equaliser operating mode */
    LVM_UINT16                  EQNB_NBands;            /* Number of bands */
    LVM_EQNB_BandDef_t          *pEQNB_BandDefinition;  /* Pointer to equaliser definitions */

} Local_ControlParams_t;

static const uint32_t bandFreqRange[FIVEBAND_NUMBANDS][2] = {
                                       {30000, 120000},
                                       {120001, 460000},
                                       {460001, 1800000},
                                       {1800001, 7000000},
                                       {7000001, 1}};

//Note: If these frequencies change, please update LimitLevel values accordingly.
static const LVM_UINT16  EQNB_5BandPresetsFrequencies[] = {
                                       60,           /* Frequencies in Hz */
                                       230,
                                       910,
                                       3600,
                                       14000};

static const LVM_UINT16 EQNB_5BandPresetsQFactors[] = {
                                       96,               /* Q factor multiplied by 100 */
                                       96,
                                       96,
                                       96,
                                       96};

static const LVM_INT16 EQNB_5BandNormalPresets[] = {
                                       3, 0, 0, 0, 3,       /* Normal Preset */
                                       8, 5, -3, 5, 6,      /* Classical Preset */
                                       15, -6, 7, 13, 10,   /* Dance Preset */
                                       0, 0, 0, 0, 0,       /* Flat Preset */
                                       6, -2, -2, 6, -3,    /* Folk Preset */
                                       8, -8, 13, -1, -4,   /* Heavy Metal Preset */
                                       10, 6, -4, 5, 8,     /* Hip Hop Preset */
                                       8, 5, -4, 5, 9,      /* Jazz Preset */
                                      -6, 4, 9, 4, -5,      /* Pop Preset */
                                       10, 6, -1, 8, 10};   /* Rock Preset */

static const LVM_INT16 EQNB_5BandSoftPresets[] = {
                                        3, 0, 0, 0, 3,      /* Normal Preset */
                                        5, 3, -2, 4, 4,     /* Classical Preset */
                                        6, 0, 2, 4, 1,      /* Dance Preset */
                                        0, 0, 0, 0, 0,      /* Flat Preset */
                                        3, 0, 0, 2, -1,     /* Folk Preset */
                                        4, 1, 9, 3, 0,      /* Heavy Metal Preset */
                                        5, 3, 0, 1, 3,      /* Hip Hop Preset */
                                        4, 2, -2, 2, 5,     /* Jazz Preset */
                                       -1, 2, 5, 1, -2,     /* Pop Preset */
                                        5, 3, -1, 3, 5};    /* Rock Preset */
static const char* gEqualizerPresets[] = {
                                        "Normal",
                                        "Classical",
                                        "Dance",
                                        "Flat",
                                        "Folk",
                                        "Heavy Metal",
                                        "Hip Hop",
                                        "Jazz",
                                        "Pop",
                                        "Rock"};

/* Instance parameters */
static Local_ControlParams_t LocalParams;
static LVM_EQNB_BandDef_t BandDefs[MAX_NUM_BANDS];
static LVEQNB_Instance_t EQNB_Instance;
static LVEQNB_MemTab_t   EQNB_MemTab;            /* For N-Band Equaliser */
static LVEQNB_Handle_t gLVEQNB_Handle = LVM_NULL;


static int32_t EqualizerGetNumPresets(){
    return sizeof(gEqualizerPresets) / sizeof(char*);
}

LVEQNB_Handle_t LVM_EQ_CreateEQNBInstance()
{
    static char CreateFlag = 0;
    int i;

    if(1 == CreateFlag)
    {
        if(NULL != gLVEQNB_Handle)
            return gLVEQNB_Handle;
        else
            return LVM_NULL;
    }

    memset((void*)&EQNB_Instance, 0, sizeof(LVEQNB_Instance_t));
    /*
     * N-Band equaliser requirements
     */
    LVEQNB_Capabilities_t   EQNB_Capabilities;
    memset((void*)&EQNB_Capabilities, 0, sizeof(LVEQNB_Capabilities_t));

    /*
     * Set the capabilities
     */
    EQNB_Capabilities.SampleRate   = LVEQNB_CAP_FS_8000  |
                                     LVEQNB_CAP_FS_11025 |
                                     LVEQNB_CAP_FS_12000 |
                                     LVEQNB_CAP_FS_16000 |
                                     LVEQNB_CAP_FS_22050 |
                                     LVEQNB_CAP_FS_24000 |
                                     LVEQNB_CAP_FS_32000 |
                                     LVEQNB_CAP_FS_44100 |
                                     LVEQNB_CAP_FS_48000;
    EQNB_Capabilities.SourceFormat = LVEQNB_CAP_STEREO   |
                                     LVEQNB_CAP_MONOINSTEREO;
    EQNB_Capabilities.MaxBlockSize = MAX_BLOCK_SIZE; // InternalBlockSize;
    EQNB_Capabilities.MaxBands     = MAX_NUM_BANDS;  // pInstParams->EQNB_NumBands;

    /*
     * Get the memory requirements
     */
    LVEQNB_Memory(LVM_NULL,
                  &EQNB_MemTab,
                  &EQNB_Capabilities);

    for (i = 0; i < LVEQNB_NR_MEMORY_REGIONS; i++)
    {
        LVEQNB_MemoryRegion_t *region;
        region = &EQNB_MemTab.Region[i];
        //printf("region[%d] %d %d %d %x\n", i, region->Size, region->Alignment, region->Type, region->pBaseAddress);
        if (i != LVEQNB_MEMREGION_INSTANCE)
            region->pBaseAddress = malloc(ALIGN(region->Size, region->Alignment));
    }

    EQNB_MemTab.Region[LVEQNB_MEMREGION_INSTANCE].pBaseAddress = &EQNB_Instance;

    /*
     *          * Initialise the Dynamic Bass Enhancement instance and save the instance handle
     *                   */
    LVEQNB_Handle_t hEQNBInstance = LVM_NULL;
    LVEQNB_ReturnStatus_en LVEQNB_Status = LVEQNB_Init(&hEQNBInstance,         /* Initiailse */
                                                       &EQNB_MemTab,
                                                       &EQNB_Capabilities);
    if (LVEQNB_Status != LVEQNB_SUCCESS)
        return LVM_NULL;

    gLVEQNB_Handle = hEQNBInstance;
    return hEQNBInstance;
}

LVEQNB_Handle_t LVM_EQ_GetEQNBHandle()
{
    return gLVEQNB_Handle;
}

void LVM_EQ_ReleaseHandle(LVEQNB_Handle_t hHandle)
{
    int i;
    if (hHandle == LVM_NULL)
        return;

    for (i = LVEQNB_MEMREGION_PERSISTENT_DATA; i < LVEQNB_NR_MEMORY_REGIONS; i++) {
        if (EQNB_MemTab.Region[i].pBaseAddress) {
            free(EQNB_MemTab.Region[i].pBaseAddress);
            EQNB_MemTab.Region[i].pBaseAddress = LVM_NULL;
        }
    }

    gLVEQNB_Handle = LVM_NULL;
}

void LVM_EQ_InitParams()
{
    int i;
    memset((void*)&LocalParams, 0, sizeof(Local_ControlParams_t));
    LocalParams.OperatingMode = LVM_MODE_ON;
    LocalParams.SampleRate = LVM_FS_44100;
    LocalParams.SourceFormat = LVM_STEREO;
    LocalParams.EQNB_OperatingMode = LVM_EQNB_ON;
    LocalParams.EQNB_NBands = LVM_EQ_NBANDS;
    LocalParams.pEQNB_BandDefinition = &BandDefs[0];

    for (i=0; i<FIVEBAND_NUMBANDS; i++)
    {
        BandDefs[i].Frequency = EQNB_5BandPresetsFrequencies[i];
        BandDefs[i].QFactor   = EQNB_5BandPresetsQFactors[i];
        BandDefs[i].Gain      = EQNB_5BandSoftPresets[i];
    }
}

LVM_ReturnStatus_en LVM_EQ_ApplyNewSetting(LVEQNB_Handle_t    hEQNBInstance)
{
    LVEQNB_ReturnStatus_en      EQNB_Status;
    LVEQNB_Params_t             EQNB_Params;


    if(NULL == hEQNBInstance)
        return LVEQNB_NULLADDRESS;
    /*
    * Set the new parameters
    */

    if(LocalParams.OperatingMode == LVM_MODE_OFF)
    {
        EQNB_Params.OperatingMode = LVEQNB_BYPASS;
    }
    else
    {
        EQNB_Params.OperatingMode  = (LVEQNB_Mode_en)LocalParams.EQNB_OperatingMode;
    }

    EQNB_Params.SampleRate       = (LVEQNB_Fs_en)LocalParams.SampleRate;
    EQNB_Params.NBands           = LocalParams.EQNB_NBands;
    EQNB_Params.pBandDefinition  = (LVEQNB_BandDef_t *)LocalParams.pEQNB_BandDefinition;
    if (LocalParams.SourceFormat == LVM_STEREO)    /* Mono format not supported */
    {
        EQNB_Params.SourceFormat = LVEQNB_STEREO;
    }
    else
    {
        EQNB_Params.SourceFormat = LVEQNB_MONOINSTEREO;     /* Force to Mono-in-Stereo mode */
    }


    /*
     * Set the control flag
     */
    if ((LocalParams.OperatingMode == LVM_MODE_ON) &&
            (LocalParams.EQNB_OperatingMode == LVM_EQNB_ON))
    {
        //pInstance->EQNB_Active = LVM_TRUE;
    }
    else
    {
        EQNB_Params.OperatingMode = LVEQNB_BYPASS;
    }

    /*
     * Make the changes
     */
    EQNB_Status = LVEQNB_Control(hEQNBInstance,
            &EQNB_Params);


    /*
     * Quit if the changes were not accepted
     */
    if (EQNB_Status != LVEQNB_SUCCESS)
    {
        return((LVM_ReturnStatus_en)EQNB_Status);
    }
    else
    {
        return ((LVM_ReturnStatus_en)LVEQNB_SUCCESS);
    }
}

LVM_ReturnStatus_en LVM_EQ_Process(LVEQNB_Handle_t       hInstance,
        const LVM_INT16       *pInData,
        LVM_INT16             *pOutData,
        LVM_UINT16            NumSamples)
{
    /*
     * Check if the number of samples is zero
     */
    if (NumSamples == 0)
    {
        return(LVM_SUCCESS);
    }

    /*
     * Check valid points have been given
     */
    if ((hInstance == LVM_NULL) || (pInData == LVM_NULL) || (pOutData == LVM_NULL))
    {
        return (LVM_NULLADDRESS);
    }

    LVEQNB_Process(hInstance,        /* N-Band equaliser instance handle */
            pInData, pOutData, NumSamples);

    return LVM_SUCCESS;
}

//----------------------------------------------------------------------------
// LVM_EQ_SetParameter()
//----------------------------------------------------------------------------
// Purpose:
// Set a Equalizer parameter
//
// Inputs:
//  pParam        - pointer to parameter
//  pValue        - pointer to value
//
// Outputs:
//
//----------------------------------------------------------------------------
int LVM_EQ_SetParams (void *pParam, void *pValue){
    int status = 0;
    int32_t preset;
    int32_t band;
    int32_t level;
    int32_t *pParamTemp = (int32_t *)pParam;
    int32_t param = *pParamTemp++;
    int i;
    int gainRounded;


    if((NULL == pParam) || (NULL == pValue))
        return -EINVAL;

    printf("\tEqualizer_setParameter start\n");
    switch (param) {
    case EQ_PARAM_CUR_PRESET:
        preset = (int32_t)(*(uint16_t *)pValue);

        printf("\tEqualizer_setParameter() EQ_PARAM_CUR_PRESET %d\n", preset);
        if ((preset >= EqualizerGetNumPresets())||(preset < 0)) {
            status = -EINVAL;
            break;
        }

        for (i=0; i<FIVEBAND_NUMBANDS; i++)
        {
            BandDefs[i].Gain = EQNB_5BandSoftPresets[i + preset * FIVEBAND_NUMBANDS];
        }

        break;
    case EQ_PARAM_BAND_LEVEL:
        band =  *pParamTemp;
        level = (int32_t)(*(int16_t *)pValue);
        printf("\tEqualizer_setParameter() EQ_PARAM_BAND_LEVEL band %d, level %d\n", band, level);
        if (band >= FIVEBAND_NUMBANDS) {
            status = -EINVAL;
            break;
        }
        if(level > 0){
            gainRounded = (int)((level+50)/100);
        }else{
            gainRounded = (int)((level-50)/100);
        }
        //EqualizerSetBandLevel(pContext, band, level);
        BandDefs[band].Gain = gainRounded;
        break;
    case EQ_PARAM_PROPERTIES: {
        printf("\tEqualizer_setParameter() EQ_PARAM_PROPERTIES\n");
        int16_t *p = (int16_t *)pValue;
        if ((int)p[0] >= EqualizerGetNumPresets()) {
            status = -EINVAL;
            break;
        }
        if (p[0] >= 0) {
            //EqualizerSetPreset(pContext, (int)p[0]); 
            for (i=0; i<FIVEBAND_NUMBANDS; i++)
            {
                BandDefs[i].Gain =   EQNB_5BandSoftPresets[i + p[0] * FIVEBAND_NUMBANDS];
            }
        } else {
            if ((int)p[1] != FIVEBAND_NUMBANDS) {
                status = -EINVAL;
                break;
            }
            for (i = 0; i < FIVEBAND_NUMBANDS; i++) {
                //EqualizerSetBandLevel(pContext, i, (int)p[2 + i]);
                level = (int)p[2 + i];
                if(level > 0){
                    gainRounded = (int)((level+50)/100);
                }else{
                    gainRounded = (int)((level-50)/100);
                }
                //EqualizerSetBandLevel(pContext, band, level);
                BandDefs[i].Gain = gainRounded;
            }
        }
    } break;
    default:
        printf("\tLVM_ERROR : Equalizer_setParameter() invalid param %d\n", param);
        status = -EINVAL;
        break;
    }

    printf("\tEqualizer_setParameter end\n");
    return status;
} /* end Equalizer_setParameter */
