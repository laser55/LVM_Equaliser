/********************************************/
/*LVM_Equaliser.h*/
/********************************************/

#include <LVM_Types.h>

#define EQ_PARAM_CUR_PRESET (1)
#define EQ_PARAM_BAND_LEVEL (2)
#define EQ_PARAM_PROPERTIES (3)
#define EINVAL (1)

typedef void *LVEQNB_Handle_t;

/* Status return values */  /*LVM.h*/
typedef enum
{
    LVM_SUCCESS            = 0,                     /* Successful return from a routine */
    LVM_ALIGNMENTERROR     = 1,                     /* Memory alignment error */
    LVM_NULLADDRESS        = 2,                     /* NULL allocation address */
    LVM_OUTOFRANGE         = 3,                     /* Out of range control parameter */
    LVM_INVALIDNUMSAMPLES  = 4,                     /* Invalid number of samples */
    LVM_WRONGAUDIOTIME     = 5,                     /* Wrong time value for audio time*/
    LVM_ALGORITHMDISABLED  = 6,                     /* Algorithm is disabled*/
    LVM_ALGORITHMPSA       = 7,                     /* Algorithm PSA returns an error */
    LVM_RETURNSTATUS_DUMMY = LVM_MAXENUM
}LVM_ReturnStatus_en;


LVEQNB_Handle_t LVM_EQ_CreateEQNBInstance();
LVEQNB_Handle_t LVM_EQ_GetEQNBHandle();
void LVM_EQ_ReleaseHandle(LVEQNB_Handle_t hHandle);
void LVM_EQ_InitParams();
void LVM_EQ_Set_Params();
LVM_ReturnStatus_en LVM_EQ_ApplyNewSetting(LVEQNB_Handle_t    hEQNBInstance);
LVM_ReturnStatus_en LVM_EQ_Process(LVEQNB_Handle_t       hInstance,
                                   const LVM_INT16       *pInData,
                                   LVM_INT16             *pOutData,
                                   LVM_UINT16            NumSamples);
int LVM_EQ_SetParams (void *pParam, void *pValue);
