/************************************************************************************/
/*                                                                                  */
/*  Includes                                                                        */
/*                                                                                  */
/************************************************************************************/

#include <stdlib.h>
#include <stdio.h>

#include "LVM_Equaliser.h"

#define INPUT_FILE_PATCH "../data/input.pcm"
#define OUTPUT_FILE_PATCH "../data/output.pcm"
#define IO_BUFF_SIZE 4096

LVM_INT16 INPUT_Buffer[IO_BUFF_SIZE];
LVM_INT16 OUTPUT_Buffer[IO_BUFF_SIZE];

/*
*
*/

int main(int argc, char *argv[])
{
    FILE *InputFilePtr;
    FILE *OutputFilePtr;
    int ReadDataNum;
    int32_t EQ_CMD[10];
    uint16_t Preset;
    //LVEQNB_Capabilities_t capabilities;


    if(NULL == (InputFilePtr=fopen(INPUT_FILE_PATCH,"r+b")))
    {
        printf("Error!! Cann't open input file.\n");
        return -1;
    }
    else
    {
        printf("open input file: INPUT_FILE_PATCH.\n");
    }
    if(NULL == (OutputFilePtr=fopen(OUTPUT_FILE_PATCH,"w+")))
    {
        printf("Error!! Cann't open output file.\n");
        return -1;
    }
    else
    {
        printf("open output file: OUTPUT_FILE_PATCH.\n");
    }



    //fread((void*)INPUT_Buffer, sizeof(LVM_INT16), 2048, InputFilePtr);
    memset((void*)OUTPUT_Buffer, 0, sizeof(LVM_INT16)*4096);
    LVM_EQ_CreateEQNBInstance();

    LVEQNB_Handle_t hEQNBHandle = LVM_EQ_GetEQNBHandle();

    if(LVM_NULL == hEQNBHandle)
    {
        LVM_EQ_ReleaseHandle(hEQNBHandle);
        printf("Error!! Get EQNB Handle Failed.\n");
        return -1;
    }
    else
    {
        printf("Get EQNB Handle SUCCESS.\n");
    }

#if 0
    LVEQNB_GetCapabilities(hEQNBHandle, &capabilities);
    printf("EQNB Cap: %x %x %x %x %x %x\n",
            capabilities.SampleRate,
            capabilities.SourceFormat,
            capabilities.MaxBlockSize,
            capabilities.MaxBands,
            capabilities.CallBack,
            capabilities.pBundleInstance);
#endif

    LVM_EQ_InitParams();
    memset((void*)EQ_CMD, 0 , sizeof(int32_t)*10);
    EQ_CMD[0] = EQ_PARAM_CUR_PRESET;
    printf("Select Eq Type.\n");
    printf("0: Normal \n");
    printf("1: Classical \n");
    printf("2: Dance \n");
    printf("3: Flat \n");
    printf("4: Folk \n");
    printf("5: Heavy Metal \n");
    printf("6: Hip Hop \n");
    printf("7: Jazz \n");
    printf("8: Pop \n");
    printf("9: Rock \n");
    printf("Eq Type = ");
    scanf("%d",&Preset);
    LVM_EQ_SetParams((void*)EQ_CMD, (void*)&Preset);

    printf("Apply New Setting.\n");
    LVM_EQ_ApplyNewSetting(hEQNBHandle);

    printf("To process data.\n");
    printf("Waiting.....\n");
    do
    {
        ReadDataNum = fread((void*)INPUT_Buffer, sizeof(LVM_INT16), IO_BUFF_SIZE, InputFilePtr);
        if(ReadDataNum >= 4 && ReadDataNum <= IO_BUFF_SIZE)
        {
            LVM_EQ_Process(hEQNBHandle,        /* N-Band equaliser instance handle */
                          (const LVM_INT16 *)INPUT_Buffer,
                           OUTPUT_Buffer,
                           ReadDataNum/2);

            fwrite((void*)OUTPUT_Buffer, sizeof(LVM_INT16), ReadDataNum, OutputFilePtr);
        }
    }while(ReadDataNum == IO_BUFF_SIZE);

    printf("Process Finish!\n");
    LVM_EQ_ReleaseHandle(hEQNBHandle);

    printf("Release EQ Handle!\n");

    fclose(InputFilePtr);
    fclose(OutputFilePtr);

    return 0;
}
