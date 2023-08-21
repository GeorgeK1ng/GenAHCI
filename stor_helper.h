
/*
 * File: stor_helper.h
 */

#ifndef __STOR_HELPER_H__
#define __STOR_HELPER_H__

#define IDE_GP_SUMMARY_SMART_ERROR                  0x01    // Access: SMART Logging
#define IDE_GP_COMPREHENSIVE_SMART_ERROR            0x02    // Access: SMART Logging
#define IDE_GP_EXTENDED_COMPREHENSIVE_SMART_ERROR   0x03
#define IDE_GP_SMART_SELF_TEST                      0x06    // Access: SMART Logging
#define IDE_GP_EXTENDED_SMART_SELF_TEST             0x07
#define IDE_GP_LOG_POWER_CONDITIONS                 0x08
#define IDE_GP_SELECTIVE_SELF_TEST                  0x09    // Access: SMART Logging
#define IDE_GP_DEVICE_STATISTICS_NOTIFICATION       0x0A
#define IDE_GP_PENDING_DEFECTS                      0x0C
#define IDE_GP_LPS_MISALIGNMENT                     0x0D

#define IDE_GP_LOG_PHY_EVENT_COUNTER_ADDRESS        0x11
#define IDE_GP_LOG_NCQ_NON_DATA_ADDRESS             0x12
#define IDE_GP_LOG_NCQ_SEND_RECEIVE_ADDRESS         0x13
#define IDE_GP_LOG_HYBRID_INFO_ADDRESS              0x14
#define IDE_GP_LOG_REBUILD_ASSIST                   0x15
#define IDE_GP_LOG_LBA_STATUS                       0x19

#define IDE_GP_LOG_WRITE_STREAM_ERROR               0x21
#define IDE_GP_LOG_READ_STREAM_ERROR                0x22

#define IDE_GP_LOG_SCT_COMMAND_STATUS               0xE0
#define IDE_GP_LOG_SCT_DATA_TRANSFER                0xE1

#define STOR_FEATURE_EXTRA_IO_INFORMATION                   0x00000080  // Indicating that miniport driver wants SRBEX_DATA_IO_INFO in a SRBEX if available
#define STOR_FEATURE_ADAPTER_CONTROL_PRE_FINDADAPTER        0x00000100  // Indicating that miniport driver can safely process AdapterControl call from Storport before receiving HwFindAdapter.
#define STOR_FEATURE_ADAPTER_NOT_REQUIRE_IO_PORT            0x00000200  // Indicating that miniport driver doesn't require IO Port resource for its adapter.

#define REQUEST_INFO_VALID_CACHEPRIORITY_FLAG       0x80000000
#define REQUEST_INFO_HYBRID_WRITE_THROUGH_FLAG      0x00000020

//
// "Hybrid Information log page" strucutre definition
//
#define HYBRID_INFORMATION_DISABLED             0x00
#define HYBRID_INFORMATION_DISABLE_IN_PROCESS   0x80
#define HYBRID_INFORMATION_ENABLED              0xFF

#define HYBRID_REQUEST_INFO_STRUCTURE_VERSION           0x1


//
// HYBRID IOCTL status
//
#define HYBRID_STATUS_SUCCESS                             0x0
#define HYBRID_STATUS_ILLEGAL_REQUEST                     0x1
#define HYBRID_STATUS_INVALID_PARAMETER                   0x2
#define HYBRID_STATUS_OUTPUT_BUFFER_TOO_SMALL             0x3

#define IDE_SATA_FEATURE_HYBRID_INFORMATION                 0xa

#define HYBRID_STATUS_ENABLE_REFCOUNT_HOLD                0x10


// SubCommand of IDE_COMMAND_NCQ_NON_DATA
#define IDE_NCQ_NON_DATA_ABORT_NCQ_QUEUE                0x00
#define IDE_NCQ_NON_DATA_DEADLINE_HANDLING              0x01
#define IDE_NCQ_NON_DATA_HYBRID_CHANGE_BY_SIZE          0x02    // this subCommand has been renamed to Hybrid Demote by Size.
#define IDE_NCQ_NON_DATA_HYBRID_DEMOTE_BY_SIZE          0x02
#define IDE_NCQ_NON_DATA_HYBRID_CHANGE_BY_LBA_RANGE     0x03
#define IDE_NCQ_NON_DATA_HYBRID_CONTROL                 0x04



//
// Data structure and definitions related to IOCTL_SCSI_MINIPORT_HYBRID
//

#define HYBRID_FUNCTION_GET_INFO                            0x01

#define HYBRID_FUNCTION_DISABLE_CACHING_MEDIUM              0x10
#define HYBRID_FUNCTION_ENABLE_CACHING_MEDIUM               0x11
#define HYBRID_FUNCTION_SET_DIRTY_THRESHOLD                 0x12
#define HYBRID_FUNCTION_DEMOTE_BY_SIZE                      0x13


#define DeviceDsmAction_NvCache_Change_Priority (0x0000000Eu | DeviceDsmActionFlag_NonDestructive)
#define DeviceDsmAction_NvCache_Evict           (0x0000000Fu | DeviceDsmActionFlag_NonDestructive)


//
// SCSI I/O Request Block
//

typedef struct _SCSI_REQUEST_BLOCK_EX {
    USHORT Length;                  // offset 0
    UCHAR Function;                 // offset 2
    UCHAR SrbStatus;                // offset 3
    UCHAR ScsiStatus;               // offset 4
    UCHAR PathId;                   // offset 5
    UCHAR TargetId;                 // offset 6
    UCHAR Lun;                      // offset 7
    UCHAR QueueTag;                 // offset 8
    UCHAR QueueAction;              // offset 9
    UCHAR CdbLength;                // offset a
    UCHAR SenseInfoBufferLength;    // offset b
    ULONG SrbFlags;                 // offset c
    ULONG DataTransferLength;       // offset 10
    ULONG TimeOutValue;             // offset 14
    __field_bcount(DataTransferLength) \
    PVOID DataBuffer;               // offset 18
    PVOID SenseInfoBuffer;          // offset 1c
    struct _SCSI_REQUEST_BLOCK_EX *NextSrb; // offset 20
    PVOID OriginalRequest;          // offset 24
    PVOID SrbExtension;             // offset 28
    union {
        ULONG InternalStatus;       // offset 2c
        ULONG QueueSortKey;         // offset 2c
        ULONG LinkTimeoutValue;     // offset 2c
    };
	//ULONG                                ReservedUlong1;
	//ULONG                                Signature;
	//ULONG                                Version;
	//ULONG                                SrbLength;
	//ULONG                                SrbFunction;
	//ULONG                                ReservedUlong2;
	//ULONG                                RequestTag;
	//USHORT                               RequestPriority;
	//USHORT                               RequestAttribute;
	//ULONG                                SystemStatus;
	//ULONG                                ZeroGuard1;
	//ULONG                                AddressOffset;
	//ULONG                                NumSrbExData;
	//PVOID POINTER_ALIGN                  ZeroGuard2;
	//PVOID POINTER_ALIGN                  ClassContext;
	//PVOID POINTER_ALIGN                  PortContext;
	//PVOID POINTER_ALIGN                  MiniportContext;
	//ULONG                                SrbExDataOffset[ANYSIZE_ARRAY];
	
#if defined(_WIN64)

    //
    // Force PVOID alignment of Cdb
    //

    ULONG Reserved;

#endif

    UCHAR Cdb[16];                  // offset 30
} SCSI_REQUEST_BLOCK_EX, *PSCSI_REQUEST_BLOCK_EX;




__drv_maxIRQL(DISPATCH_LEVEL)
ULONG
FORCEINLINE
StorPortPoFxActivateComponentEx(
    __in PVOID HwDeviceExtension,
    __in_opt PSTOR_ADDRESS Address,
    __in_opt PSCSI_REQUEST_BLOCK_EX Srb,
    __in ULONG Component,
    __in ULONG Flags
)
/*
Description:
    A miniport should call this function to indicate it needs to access a
    component's hardware.  This call must be paired to a subsequent call to
    StorPortPoFxIdleComponent().

    If this call causes the component to go from the idle to the active state,
    by default this function will perform the work to make the component active
    asynchronously. However, the miniport can set a flag to indicate if the
    active work should be performed synchronously or asynchronously.

Parameters:
    HwDeviceExtension - The miniport's device extension.
    Address - NULL if the device being registered is an adapter, otherwise the
        address specifies the unit object.
    Srb - NULL if for a miniport internally generated request, otherwise the
        SRB.
    Component - The index of the component.
    Flags - For future use.  Must be 0.

Returns:
    A STOR_STATUS code.
*/
{

    return StorPortExtendedFunction(ExtFunctionPoFxActivateComponent,
                                    HwDeviceExtension,
                                    Address,
                                    Srb,
                                    Component,
                                    Flags);
}


ULONG
FORCEINLINE
StorPortPoFxIdleComponentEx(
    __in PVOID HwDeviceExtension,
    __in_opt PSTOR_ADDRESS Address,
    __in_opt PSCSI_REQUEST_BLOCK_EX Srb,
    __in ULONG Component,
    __in ULONG Flags
)
/*
Description:
    A miniport should call this function to indicate it no longer needs to
    access a component's hardware.  This call must be paired with an earlier
    call to StorPortPoFxActivateComponent().

    If this call causes the component to go from the active to the idle state,
    by default this function will perform the work to make the component idle
    asynchronously. However, the miniport can set a flag to indicate if the
    idle work should be performed synchronously or asynchronously.

Parameters:
    HwDeviceExtension - The miniport's device extension.
    Address - NULL if the device being registered is an adapter, otherwise the
        address specifies the unit object.
    Srb - NULL if for a miniport internally generated request, otherwise the
        SRB.
    Component - The index of the component.
    Flags - For future use.  Must be 0.

Returns:
    A STOR_STATUS code.
*/
{

    return StorPortExtendedFunction(ExtFunctionPoFxIdleComponent,
                                    HwDeviceExtension,
                                    Address,
                                    Srb,
                                    Component,
                                    Flags);
}





typedef
BOOLEAN
HW_STARTIO_EX (
    __in PVOID DeviceExtension,
    __in PSCSI_REQUEST_BLOCK_EX Srb
    );
typedef HW_STARTIO_EX *PHW_STARTIO_EX;

typedef
BOOLEAN
HW_BUILDIO_EX (
    __in PVOID DeviceExtension,
    __in PSCSI_REQUEST_BLOCK_EX Srb
    );
typedef HW_BUILDIO_EX *PHW_BUILDIO_EX;



//
// Structure passed between miniport initialization
// and SCSI port initialization
//



typedef struct _HW_INITIALIZATION_DATA_EX {

  ULONG                       HwInitializationDataSize;
  INTERFACE_TYPE              AdapterInterfaceType;
  PHW_INITIALIZE              HwInitialize;
  PHW_STARTIO_EX                 HwStartIo;
  PHW_INTERRUPT               HwInterrupt;

  //
  // If miniport is physical, HwFindAdapter shall be of type PHW_FIND_ADAPTER.
  // If miniport is virtual, HwFindAdapter shall be of type
  // PVIRTUAL_HW_FIND_ADAPTER.
  //
  PVOID                       HwFindAdapter;
  PHW_RESET_BUS               HwResetBus;
  PHW_DMA_STARTED             HwDmaStarted;
  PHW_ADAPTER_STATE           HwAdapterState;
  ULONG                       DeviceExtensionSize;
  ULONG                       SpecificLuExtensionSize;
  ULONG                       SrbExtensionSize;
  ULONG                       NumberOfAccessRanges;
  PVOID                       Reserved;
  UCHAR                       MapBuffers;
  BOOLEAN                     NeedPhysicalAddresses;
  BOOLEAN                     TaggedQueuing;
  BOOLEAN                     AutoRequestSense;
  BOOLEAN                     MultipleRequestPerLu;
  BOOLEAN                     ReceiveEvent;
  USHORT                      VendorIdLength;
  PVOID                       VendorId;
  union {
    USHORT ReservedUshort;
    USHORT PortVersionFlags;
  } ;
  USHORT                      DeviceIdLength;
  PVOID                       DeviceId;
  PHW_ADAPTER_CONTROL         HwAdapterControl;
  PHW_BUILDIO_EX                 HwBuildIo;

  //
  // Following 5 callbacks are virtual miniport specific and should not be set
  // by physical miniports.
  //
  PHW_FREE_ADAPTER_RESOURCES  HwFreeAdapterResources;
  PHW_PROCESS_SERVICE_REQUEST HwProcessServiceRequest;
  PHW_COMPLETE_SERVICE_IRP    HwCompleteServiceIrp;
  PHW_INITIALIZE_TRACING      HwInitializeTracing;
  PHW_CLEANUP_TRACING         HwCleanupTracing;

  //
  // New fields should only be added after this one as the previous fields match
  // the HW_INITIALIZATION definition for pre-Win8.
  //
  PHW_TRACING_ENABLED         HwTracingEnabled;

  // Used to indicate features supported by miniport such as virtual miniports
  ULONG             FeatureSupport;

  // New fields added to support extended SRB format(s)
  ULONG             SrbTypeFlags;
  ULONG             AddressTypeFlags;
  ULONG             Reserved1;

  //
  // Unit control callback
  //
  PHW_UNIT_CONTROL  HwUnitControl;

} HW_INITIALIZATION_DATA_EX, *PHW_INITIALIZATION_DATA_EX;

typedef struct _GP_LOG_HYBRID_INFORMATION_HEADER {
    USHORT  HybridInfoDescrCount        : 4;
    USHORT  Reserved0                   : 12;

    UCHAR   Enabled;                //Can be 0x00, 0x80 or 0xFF
    UCHAR   HybridHealth;
    UCHAR   DirtyLowThreshold;
    UCHAR   DirtyHighThreshold;
    UCHAR   OptimalWriteGranularity;

    UCHAR   MaximumHybridPriorityLevel  : 4;
    UCHAR   Reserved1                   : 4;

    UCHAR   PowerCondidtion;
    UCHAR   CachingMediumEnabled ;

    struct {
        UCHAR   MaximumPriorityBehavior : 1;
        UCHAR   SupportCacheBehavior    : 1;
        UCHAR   Reserved                : 6;
    } SupportedOptions;

    UCHAR   Reserved2;              // byte 11

    ULONG       TimeSinceEnabled;
    ULONGLONG   NVMSize;
    ULONGLONG   EnableCount;

    USHORT  MaximumEvictionCommands : 5;
    USHORT  Reserved3               : 11;

    USHORT  MaximumEvictionDataBlocks;  // how many data blocks (one block is 512 bytes) an Evict command can carry with.

    UCHAR   Reserved[28];

} GP_LOG_HYBRID_INFORMATION_HEADER, *PGP_LOG_HYBRID_INFORMATION_HEADER;

typedef struct _GP_LOG_NCQ_NON_DATA {

    struct {
        ULONG   AbortNcq            : 1;
        ULONG   AbortAll            : 1;
        ULONG   AbortStreaming      : 1;
        ULONG   AbortNonStreaming   : 1;
        ULONG   AbortSelectedTTag   : 1;
        ULONG   Reserved            : 27;
    } SubCmd0;

    struct {
        ULONG   DeadlineHandling        : 1;
        ULONG   WriteDataNotContinue    : 1;
        ULONG   ReadDataNotContinue     : 1;
        ULONG   Reserved                : 29;
    } SubCmd1;

    struct {
        ULONG   HybridDemoteBySize  : 1;
        ULONG   Reserved            : 31;
    } SubCmd2;

    struct {
        ULONG   HybridChangeByLbaRange  : 1;
        ULONG   Reserved                : 31;
    } SubCmd3;

    struct {
        ULONG   HybridControl   : 1;
        ULONG   Reserved        : 31;
    } SubCmd4;

    struct {
        ULONG   Reserved        : 32;
    } SubCmd5;

    struct {
        ULONG   Reserved        : 32;
    } SubCmd6;

    struct {
        ULONG   Reserved        : 32;
    } SubCmd7;

    struct {
        ULONG   Reserved        : 32;
    } SubCmd8;

    struct {
        ULONG   Reserved        : 32;
    } SubCmd9;

    struct {
        ULONG   Reserved        : 32;
    } SubCmdA;

    struct {
        ULONG   Reserved        : 32;
    } SubCmdB;

    struct {
        ULONG   Reserved        : 32;
    } SubCmdC;

    struct {
        ULONG   Reserved        : 32;
    } SubCmdD;

    struct {
        ULONG   Reserved        : 32;
    } SubCmdE;

    struct {
        ULONG   Reserved        : 32;
    } SubCmdF;

    ULONG   Reserved[112];

} GP_LOG_NCQ_NON_DATA, *PGP_LOG_NCQ_NON_DATA;


typedef struct _GP_LOG_NCQ_SEND_RECEIVE {

    struct {
        ULONG   DataSetManagement   : 1;
        ULONG   HybridEvict         : 1;
        ULONG   Reserved            : 30;
    } SubCmd;

    struct {
        ULONG   Trim        : 1;
        ULONG   Reserved    : 31;
    } DataSetManagement;

    ULONG   Reserved[126];

} GP_LOG_NCQ_SEND_RECEIVE, *PGP_LOG_NCQ_SEND_RECEIVE;

/* Extended storport.h instead 

typedef enum _STORPORT_FUNCTION_CODE_EX {
    ExtFunctionAllocatePool,
    ExtFunctionFreePool,
    ExtFunctionAllocateMdl,
    ExtFunctionFreeMdl,
    ExtFunctionBuildMdlForNonPagedPool,
    ExtFunctionGetSystemAddress,
    ExtFunctionGetOriginalMdl,
    ExtFunctionCompleteServiceIrp,
    ExtFunctionGetDeviceObjects,
    ExtFunctionBuildScatterGatherList,
    ExtFunctionPutScatterGatherList,
    ExtFunctionAcquireMSISpinLock,
    ExtFunctionReleaseMSISpinLock,
    ExtFunctionGetMessageInterruptInformation,
    ExtFunctionInitializePerformanceOptimizations,
    ExtFunctionGetStartIoPerformanceParameters,
    ExtFunctionLogSystemEvent,
    ExtFunctionGetCurrentProcessorNumber,
    ExtFunctionGetActiveGroupCount,
    ExtFunctionGetGroupAffinity,
    ExtFunctionGetActiveNodeCount,
    ExtFunctionGetNodeAffinity,
    ExtFunctionGetHighestNodeNumber,
    ExtFunctionGetLogicalProcessorRelationship,
    ExtFunctionAllocateContiguousMemorySpecifyCacheNode,
    ExtFunctionFreeContiguousMemorySpecifyCache,
    ExtFunctionSetPowerSettingNotificationGuids,
    ExtFunctionInvokeAcpiMethod,
    ExtFunctionGetRequestInfo,
    ExtFunctionInitializeWorker,
    ExtFunctionQueueWorkItem,
    ExtFunctionFreeWorker,
    ExtFunctionInitializeTimer,
    ExtFunctionRequestTimer,
    ExtFunctionFreeTimer,
    ExtFunctionInitializeSListHead,
    ExtFunctionInterlockedFlushSList,
    ExtFunctionInterlockedPopEntrySList,
    ExtFunctionInterlockedPushEntrySList,
    ExtFunctionQueryDepthSList,
    ExtFunctionGetActivityId,
    ExtFunctionGetSystemPortNumber,
    ExtFunctionGetDataInBufferMdl,
    ExtFunctionGetDataInBufferSystemAddress,
    ExtFunctionGetDataInBufferScatterGatherList,
    ExtFunctionMarkDumpMemory,
    ExtFunctionSetUnitAttributes,
    ExtFunctionQueryPerformanceCounter,
    ExtFunctionInitializePoFxPower,
    ExtFunctionPoFxActivateComponent,
    ExtFunctionPoFxIdleComponent,
    ExtFunctionPoFxSetComponentLatency,
    ExtFunctionPoFxSetComponentResidency,
    ExtFunctionPoFxPowerControl,
    ExtFunctionFlushDataBufferMdl,
    ExtFunctionDeviceOperationAllowed,
    ExtFunctionGetProcessorIndexFromNumber,
    ExtFunctionPoFxSetIdleTimeout,
    ExtFunctionMiniportEtwEvent2,
    ExtFunctionMiniportEtwEvent4,
    ExtFunctionMiniportEtwEvent8,
    ExtFunctionCurrentOsInstallationUpgrade,
    ExtFunctionRegistryReadAdapterKey,
    ExtFunctionRegistryWriteAdapterKey,
    ExtFunctionSetAdapterBusType,
    ExtFunctionPoFxRegisterPerfStates,
    ExtFunctionPoFxSetPerfState,
    ExtFunctionGetD3ColdSupport,
    ExtFunctionInitializeRpmb,
    ExtFunctionAllocateHmb,
    ExtFunctionFreeHmb,
    ExtFunctionPropagateIrpExtension
} STORPORT_FUNCTION_CODE_EX, *PSTORPORT_FUNCTION_CODE_EX;

*/


//
// STOR_FX_COMPONENT_V2 defines a component of a STOR_POFX_DEVICE(_V2).
//
typedef struct _STOR_POFX_COMPONENT_V2 {
    ULONG Version;
    ULONG Size;
    ULONG FStateCount;
    ULONG DeepestWakeableFState;
    GUID Id;

    //
    // Indicates the deepest F-State that this component can be in where the
    // adapter still requires power.  Only relevant for components whose Id
    // is STORPORT_POFX_LUN_GUID.
    //
    ULONG DeepestAdapterPowerRequiredFState;

    //
    // The deepest F-State that the component can be in where the miniport can
    // power up the component should a crash occur and a crash dump needs to be
    // written.
    // NOTE: In this case, the STOR_POFX_DEVICE_FLAG_NO_DUMP_ACTIVE flag does
    // not apply to the Idle Condition; it will only apply to D3Cold.
    //
    ULONG DeepestCrashDumpReadyFState;

    __field_ecount_full(FStateCount) STOR_POFX_COMPONENT_IDLE_STATE FStates[ANYSIZE_ARRAY];
} STOR_POFX_COMPONENT_V2, *PSTOR_POFX_COMPONENT_V2;

#define STOR_POFX_COMPONENT_V2_SIZE ((ULONG)FIELD_OFFSET(STOR_POFX_COMPONENT_V2, FStates))
#define STOR_POFX_COMPONENT_VERSION_V2 2



//
// Storport interfaces to allow miniports to log ETW events.
//
typedef enum _STORPORT_ETW_LEVEL {
    StorportEtwLevelLogAlways = 0,
    StorportEtwLevelCritical = 1,
    StorportEtwLevelError = 2,
    StorportEtwLevelWarning = 3,
    StorportEtwLevelInformational = 4,
    StorportEtwLevelVerbose = 5,
    StorportEtwLevelMax = StorportEtwLevelVerbose
} STORPORT_ETW_LEVEL, *PSTORPORT_ETW_LEVEL;

//
// These keyword bits can be OR'd together to specify more than one keyword.
//
#define STORPORT_ETW_EVENT_KEYWORD_IO           0x01
#define STORPORT_ETW_EVENT_KEYWORD_PERFORMANCE  0x02
#define STORPORT_ETW_EVENT_KEYWORD_POWER        0x04
#define STORPORT_ETW_EVENT_KEYWORD_ENUMERATION  0x08

typedef enum _STORPORT_ETW_EVENT_OPCODE {
    StorportEtwEventOpcodeInfo = 0,
    StorportEtwEventOpcodeStart = 1,
    StorportEtwEventOpcodeStop = 2,
    StorportEtwEventOpcodeDC_Start = 3,
    StorportEtwEventOpcodeDC_Stop = 4,
    StorportEtwEventOpcodeExtension = 5,
    StorportEtwEventOpcodeReply = 6,
    StorportEtwEventOpcodeResume = 7,
    StorportEtwEventOpcodeSuspend = 8,
    StorportEtwEventOpcodeSend = 9,
    StorportEtwEventOpcodeReceive = 240
} STORPORT_ETW_EVENT_OPCODE, *PSTORPORT_ETW_EVENT_OPCODE;



//
// The event description must not exceed 32 characters and the parameter
// names must not exceed 16 characters (not including the NULL terminator).
//
#define STORPORT_ETW_MAX_DESCRIPTION_LENGTH 32
#define STORPORT_ETW_MAX_PARAM_NAME_LENGTH 16

ULONG
FORCEINLINE
StorPortEtwEvent2(
    __in PVOID HwDeviceExtension,
    __in_opt PSTOR_ADDRESS Address,
    __in ULONG EventId,
    __in PWSTR EventDescription,  //_In_reads_or_z_(STORPORT_ETW_MAX_DESCRIPTION_LENGTH) PWSTR EventDescription,
    __in ULONGLONG EventKeywords,
    __in STORPORT_ETW_LEVEL EventLevel,
    __in STORPORT_ETW_EVENT_OPCODE EventOpcode,
    __in_opt PSCSI_REQUEST_BLOCK Srb,
    __in PWSTR Parameter1Name,    //_In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter1Name,
    __in ULONGLONG Parameter1Value,
    __in PWSTR Parameter2Name,    //_In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter2Name,
    __in ULONGLONG Parameter2Value
)
/*
Description:
    A miniport can call this function to log an ETW event with two extra
    general purpose parameters (expressed as name-value pairs).

Parameters:
    HwDeviceExtension - The miniport's device extension.
    Address - NULL if the device is an adapter, otherwise the address specifies
        the unit object.
    EventId - A miniport-specific event ID to uniquely identify the type of event.
    EventDescription - Required.  A short string describing the event.  Must
        not be longer than 16 characters, not including the NULL terminator.
    EventKeywords - Bitmask of STORPORT_ETW_EVENT_KEYWORD_* values to further
        characterize the event.  Can be 0 if no keywords are desired.
    EventLevel - The level of the event (e.g. Informational, Error, etc.).
    EventOpcode - The opcode of the event (e.g. Info, Start, Stop, etc.).
    Srb - Optional pointer to an SRB.  If specified, the SRB pointer and the
        pointer of the associated IRP will be logged.
    Parameter<N>Name - A short string that gives meaning to parameter N's value.
        If NULL or an empty string, parameter N will be ignored.  Must not be
        longer than 16 characters, not including the NULL terminator.
    Parameter<N>Value - Value of parameter N.  If the associated parameter N
        name is NULL or empty, the value will be logged as 0.

Returns:
    STOR_STATUS_SUCCESS if the ETW event was successuflly logged.
    STOR_STATUS_INVALID_PARAMETER if there is an invalid parameter. This is
        typically returned if a passed-in string has too many characters.
    STOR_STATUS_UNSUCCESSFUL may also be returned for other, internal reasons.

*/
{
    ULONG status = STOR_STATUS_NOT_IMPLEMENTED;

#if (NTDDI_VERSION >= NTDDI_WINBLUE)

    status = StorPortExtendedFunction(ExtFunctionMiniportEtwEvent2,
                                        HwDeviceExtension,
                                        Address,
                                        EventId,
                                        EventDescription,
                                        EventKeywords,
                                        EventLevel,
                                        EventOpcode,
                                        Srb,
                                        Parameter1Name,
                                        Parameter1Value,
                                        Parameter2Name,
                                        Parameter2Value);
#else
    UNREFERENCED_PARAMETER(HwDeviceExtension);
    UNREFERENCED_PARAMETER(Address);
    UNREFERENCED_PARAMETER(EventId);
    UNREFERENCED_PARAMETER(EventDescription);
    UNREFERENCED_PARAMETER(EventKeywords);
    UNREFERENCED_PARAMETER(EventLevel);
    UNREFERENCED_PARAMETER(EventOpcode);
    UNREFERENCED_PARAMETER(Srb);
    UNREFERENCED_PARAMETER(Parameter1Name);
    UNREFERENCED_PARAMETER(Parameter1Value);
    UNREFERENCED_PARAMETER(Parameter2Name);
    UNREFERENCED_PARAMETER(Parameter2Value);
#endif

    return status;
}


typedef union _ATA_HYBRID_INFO_FIELDS {

    struct {
        UCHAR   HybridPriority  : 4;
        UCHAR   Reserved0       : 1;
        UCHAR   InfoValid       : 1;
        UCHAR   Reserved1       : 2;
    };

    UCHAR   AsUchar;

} ATA_HYBRID_INFO_FIELDS, *PATA_HYBRID_INFO_FIELDS;



typedef struct SRB_ALIGN _SRBEX_DATA_IO_INFO_EX {
    __field_ecount(SrbExDataTypeIoInfo)
    SRBEXDATATYPE Type;
    __field_ecount(SRBEX_DATA_IO_INFO_LENGTH)
    ULONG Length;
    ULONG Flags;
    ULONG Key;
    ULONG RWLength;
    BOOLEAN IsWriteRequest;
    UCHAR CachePriority;
    UCHAR Reserved[2];
    ULONG Reserved1[2];
} SRBEX_DATA_IO_INFO_EX, *PSRBEX_DATA_IO_INFO_EX;


typedef struct _HYBRID_REQUEST_BLOCK {
    ULONG   Version;            // HYBRID_REQUEST_BLOCK_STRUCTURE_VERSION
    ULONG   Size;               // Size of the data structure.
    ULONG   Function;           // Function code
    ULONG   Flags;

    ULONG   DataBufferOffset;   // the offset is from the beginning of buffer. e.g. from beginning of SRB_IO_CONTROL. The value should be multiple of sizeof(PVOID); Value 0 means that there is no data buffer.
    ULONG   DataBufferLength;   // length of the buffer
} HYBRID_REQUEST_BLOCK, *PHYBRID_REQUEST_BLOCK;


typedef enum _NVCACHE_STATUS {
    NvCacheStatusUnknown     = 0,   // Driver can't report non-volatile cache status
    NvCacheStatusDisabling   = 1,   // non-volatile cache is in process of being disabled.
    NvCacheStatusDisabled    = 2,   // non-volatile cache has been disabled.
    NvCacheStatusEnabled     = 3    // non-volatile cache has been enabled.
} NVCACHE_STATUS;

//
// Parameter for HYBRID_FUNCTION_GET_INFO
// Input buffer should contain SRB_IO_CONTROL and HYBRID_REQUEST_BLOCK data structures.
// Field "DataBufferOffset" of HYBRID_REQUEST_BLOCK points to output data buffer and HYBRID_INFORMATION should be returned.
//

//
// Output parameter for HYBRID_FUNCTION_GET_INFO
//

typedef struct _NVCACHE_PRIORITY_LEVEL_DESCRIPTOR {
    UCHAR   PriorityLevel;
    UCHAR   Reserved0[3];
    ULONG   ConsumedNVMSizeFraction;
    ULONG   ConsumedMappingResourcesFraction;
    ULONG   ConsumedNVMSizeForDirtyDataFraction;
    ULONG   ConsumedMappingResourcesForDirtyDataFraction;
    ULONG   Reserved1;
} NVCACHE_PRIORITY_LEVEL_DESCRIPTOR, *PNVCACHE_PRIORITY_LEVEL_DESCRIPTOR;


typedef enum _NVCACHE_TYPE {
    NvCacheTypeUnknown        = 0,  // Driver can't report the type of the nvcache
    NvCacheTypeNone           = 1,  // Device doesn't support non-volatile cache
    NvCacheTypeWriteBack      = 2,  // Device supports write back caching
    NvCacheTypeWriteThrough   = 3   // Device supports write through caching
} NVCACHE_TYPE;


typedef struct _HYBRID_INFORMATION {
    ULONG           Version;                // HYBRID_REQUEST_INFO_STRUCTURE_VERSION
    ULONG           Size;                   // sizeof(HYBRID_INFORMATION)

    BOOLEAN         HybridSupported;
    NVCACHE_STATUS  Status;                 // for hybrid disk, expect values can be: NvCacheStatusDisabling, NvCacheStatusDisabled or NvCacheStatusEnabled
    NVCACHE_TYPE    CacheTypeEffective;     // for hybrid disk, expect value will be: NvCacheTypeWriteBack
    NVCACHE_TYPE    CacheTypeDefault;       // for hybrid disk, expect values can be: NvCacheTypeWriteBack

    ULONG           FractionBase;           // Base value of all fraction type of fields in the data structure. For hybrid disk, value of this field will be 255.

    ULONGLONG       CacheSize;              // total size of NVCache. unit: LBA count

    struct {
        ULONG   WriteCacheChangeable    : 1;    // Does the device respect change in write caching policy
        ULONG   WriteThroughIoSupported : 1;    // Does the device support WriteThrough semantics for the NVCache on individual Writes.
        ULONG   FlushCacheSupported     : 1;    // Does the device support flushing of the NVCache
        ULONG   Removable               : 1;    // Does the nvcache can be removed.
        ULONG   ReservedBits            : 28;
    } Attributes;

    struct {
        UCHAR     PriorityLevelCount;           // A non-zero value indicates the non-volatile cache supports priority levels.
        BOOLEAN   MaxPriorityBehavior;          // If set to TRUE, the disk may fail IO sent with max priority level when it cannot find space for the IO in caching medium.
        UCHAR     OptimalWriteGranularity;      // In LBAs. Value is the power value (of 2). Value 0xFF means that Optimal Write Granularity is not indicated. 
                                                //          For example: value 0 indicates 2^0 = 1 logical sector, 1 indicates 2^1 = 2 logical sectors
        UCHAR     Reserved;

        ULONG     DirtyThresholdLow;            // fraction type of value, with base "FractionBase".
        ULONG     DirtyThresholdHigh;           // fraction type of value, with base "FractionBase".

        struct {
            ULONG   CacheDisable                : 1;    // support of disabling the caching medium
            ULONG   SetDirtyThreshold           : 1;    // support of Setting dirty threshold for the entire caching medium
            ULONG   PriorityDemoteBySize        : 1;    // support of demote by size command
            ULONG   PriorityChangeByLbaRange    : 1;    // support of change by lba command
            ULONG   Evict                       : 1;    // support of evict command
            ULONG   ReservedBits                : 27;

            ULONG   MaxEvictCommands;                   // Max outstanding Evict commands concurrently. Only value when "Evict" value is 1.

            ULONG   MaxLbaRangeCountForEvict;           // Count of LBA ranges can be associated with evict command. Only value when "Evict" value is 1.
            ULONG   MaxLbaRangeCountForChangeLba;       // Count of LBA ranges associated with PriorityChangeByLbaRange command. Only value when "PriorityChangeByLbaRange" value is 1.
        } SupportedCommands;

        NVCACHE_PRIORITY_LEVEL_DESCRIPTOR   Priority[ANYSIZE_ARRAY]; // Priority[0];

    } Priorities;

} HYBRID_INFORMATION, *PHYBRID_INFORMATION;




typedef struct _GP_LOG_HYBRID_INFORMATION_DESCRIPTOR {

    UCHAR   HybridPriority;
    UCHAR   ConsumedNVMSizeFraction;
    UCHAR   ConsumedMappingResourcesFraction;
    UCHAR   ConsumedNVMSizeForDirtyDataFraction;
    UCHAR   ConsumedMappingResourcesForDirtyDataFraction;

    UCHAR   Reserved[11];

} GP_LOG_HYBRID_INFORMATION_DESCRIPTOR, *PGP_LOG_HYBRID_INFORMATION_DESCRIPTOR;


typedef struct _GP_LOG_HYBRID_INFORMATION {

    GP_LOG_HYBRID_INFORMATION_HEADER        Header;

    GP_LOG_HYBRID_INFORMATION_DESCRIPTOR    Descriptor[ANYSIZE_ARRAY]; // Descriptor[0];

} GP_LOG_HYBRID_INFORMATION, *PGP_LOG_HYBRID_INFORMATION;


//
// Parameter for HYBRID_FUNCTION_SET_DIRTY_THRESHOLD
// Input buffer should contain SRB_IO_CONTROL, HYBRID_REQUEST_BLOCK and HYBRID_DIRTY_THRESHOLDS data structures.
// Field "DataBufferOffset" of HYBRID_REQUEST_BLOCK should be set to the starting offset of HYBRID_DIRTY_THRESHOLDS from beginning of buffer.
// NOTE that these functions don't have output parameter.
//
typedef struct _HYBRID_DIRTY_THRESHOLDS {
    ULONG   Version;
    ULONG   Size;               // sizeof(HYBRID_DIRTY_THRESHOLDS)

    ULONG   DirtyLowThreshold;  //
    ULONG   DirtyHighThreshold; // >= DirtyLowThreshold 
} HYBRID_DIRTY_THRESHOLDS, *PHYBRID_DIRTY_THRESHOLDS;

//
// Parameter for HYBRID_FUNCTION_DEMOTE_BY_SIZE
// Input buffer should contain SRB_IO_CONTROL, HYBRID_REQUEST_BLOCK and HYBRID_DEMOTE_BY_SIZE data structures.
// Field "DataBufferOffset" of HYBRID_REQUEST_BLOCK should be set to the starting offset of HYBRID_DEMOTE_BY_SIZE from beginning of buffer.
// NOTE that these functions don't have output parameter.
//
typedef struct _HYBRID_DEMOTE_BY_SIZE {
    ULONG       Version;
    ULONG       Size;               // sizeof(HYBRID_DEMOTE_BY_SIZE)

    UCHAR       SourcePriority;     // 1 ~ max priority
    UCHAR       TargetPriority;     // < SourcePriority
    USHORT      Reserved0;
    ULONG       Reserved1;
    ULONGLONG   LbaCount;           // How many LBAs should be demoted 
} HYBRID_DEMOTE_BY_SIZE, *PHYBRID_DEMOTE_BY_SIZE;


typedef struct _DEVICE_DSM_NVCACHE_CHANGE_PRIORITY_PARAMETERS {

    ULONG Size;

    UCHAR TargetPriority;
    UCHAR Reserved[3];

} DEVICE_DSM_NVCACHE_CHANGE_PRIORITY_PARAMETERS, *PDEVICE_DSM_NVCACHE_CHANGE_PRIORITY_PARAMETERS;


//
// Configuration information structure.  Contains the information necessary
// to initialize the adapter. NOTE: This structure must be a multiple of
// quadwords.
//

typedef struct _PORT_CONFIGURATION_INFORMATION_EX {

    //
    // Length of port configuation information strucuture.
    //

    ULONG Length;

    //
    // IO bus number (0 for machines that have only 1 IO bus
    //

    ULONG SystemIoBusNumber;

    //
    // EISA, MCA or ISA
    //

    INTERFACE_TYPE  AdapterInterfaceType;

    //
    // Interrupt request level for device
    //

    ULONG BusInterruptLevel;

    //
    // Bus interrupt vector used with hardware buses which use as vector as
    // well as level, such as internal buses.
    //

    ULONG BusInterruptVector;

    //
    // Interrupt mode (level-sensitive or edge-triggered)
    //

    KINTERRUPT_MODE InterruptMode;

    //
    // Maximum number of bytes that can be transferred in a single SRB
    //

    ULONG MaximumTransferLength;

    //
    // Number of contiguous blocks of physical memory
    //

    ULONG NumberOfPhysicalBreaks;

    //
    // DMA channel for devices using system DMA
    //

    ULONG DmaChannel;
    ULONG DmaPort;
    DMA_WIDTH DmaWidth;
    DMA_SPEED DmaSpeed;

    //
    // Alignment masked required by the adapter for data transfers.
    //

    ULONG AlignmentMask;

    //
    // Number of access range elements which have been allocated.
    //

    ULONG NumberOfAccessRanges;

    //
    // Pointer to array of access range elements.
    //

    ACCESS_RANGE (*AccessRanges)[];


    //
    // Miniport context used during dump or hibernation
    //

    PVOID MiniportDumpData;

    //
    // Number of SCSI buses attached to the adapter.
    //

    UCHAR NumberOfBuses;

    //
    // SCSI bus ID for adapter
    //

    CCHAR InitiatorBusId[8];

    //
    // Indicates that the adapter does scatter/gather
    //

    BOOLEAN ScatterGather;

    //
    // Indicates that the adapter is a bus master
    //

    BOOLEAN Master;

    //
    // Host caches data or state.
    //

    BOOLEAN CachesData;

    //
    // Host adapter scans down for bios devices.
    //

    BOOLEAN AdapterScansDown;

    //
    // Primary at disk address (0x1F0) claimed.
    //

    BOOLEAN AtdiskPrimaryClaimed;

    //
    // Secondary at disk address (0x170) claimed.
    //

    BOOLEAN AtdiskSecondaryClaimed;

    //
    // The master uses 32-bit DMA addresses.
    //

    BOOLEAN Dma32BitAddresses;

    //
    // Use Demand Mode DMA rather than Single Request.
    //

    BOOLEAN DemandMode;

    //
    // Data buffers must be mapped into virtual address space.
    //

    UCHAR MapBuffers;

    //
    // The driver will need to tranlate virtual to physical addresses.
    //

    BOOLEAN NeedPhysicalAddresses;

    //
    // Supports tagged queuing
    //

    BOOLEAN TaggedQueuing;

    //
    // Supports auto request sense.
    //

    BOOLEAN AutoRequestSense;

    //
    // Supports multiple requests per logical unit.
    //

    BOOLEAN MultipleRequestPerLu;

    //
    // Support receive event function.
    //

    BOOLEAN ReceiveEvent;

    //
    // Indicates the real-mode driver has initialized the card.
    //

    BOOLEAN RealModeInitialized;

    //
    // Indicate that the miniport will not touch the data buffers directly.
    //

    BOOLEAN BufferAccessScsiPortControlled;

    //
    // Indicator for wide scsi.
    //

    UCHAR   MaximumNumberOfTargets;

    // Use 2 reserved UCHARs to pass info on SRB and address type used.
    UCHAR   SrbType;
    UCHAR   AddressType;

    //
    // Adapter slot number
    //

    ULONG SlotNumber;

    //
    // Interrupt information for a second IRQ.
    //

    ULONG BusInterruptLevel2;
    ULONG BusInterruptVector2;
    KINTERRUPT_MODE InterruptMode2;

    //
    // DMA information for a second channel.
    //

    ULONG DmaChannel2;
    ULONG DmaPort2;
    DMA_WIDTH DmaWidth2;
    DMA_SPEED DmaSpeed2;

    //
    // Fields added to allow for the miniport
    // to update these sizes based on requirements
    // for large transfers ( > 64K);
    //

    ULONG DeviceExtensionSize;
    ULONG SpecificLuExtensionSize;
    ULONG SrbExtensionSize;

    //
    // Used to determine whether the system and/or the miniport support
    // 64-bit physical addresses.  See SCSI_DMA64_* flags below.
    //

    UCHAR  Dma64BitAddresses;        /* New */

    //
    // Indicates that the miniport can accept a SRB_FUNCTION_RESET_DEVICE
    // to clear all requests to a particular LUN.
    //

    BOOLEAN ResetTargetSupported;       /* New */

    //
    // Indicates that the miniport can support more than 8 logical units per
    // target (maximum LUN number is one less than this field).
    //

    UCHAR MaximumNumberOfLogicalUnits;  /* New */

    //
    // Supports WMI?
    //

    BOOLEAN WmiDataProvider;

    //
    // STORPORT synchronization model, either half or full duplex
    // depending on whether the driver supports async-with-interrupt
    // model or not.
    //

    STOR_SYNCHRONIZATION_MODEL SynchronizationModel;    // STORPORT New

    PHW_MESSAGE_SIGNALED_INTERRUPT_ROUTINE HwMSInterruptRoutine;

    INTERRUPT_SYNCHRONIZATION_MODE InterruptSynchronizationMode;

    MEMORY_REGION DumpRegion;

    ULONG         RequestedDumpBufferSize;

    BOOLEAN       VirtualDevice;

    UCHAR         DumpMode;
    // 2 pad bytes unused
//    ULONG         MaxNumberOfIO;
    ULONG         MaxIOsPerLun;
    ULONG         InitialLunQueueDepth;
    ULONG         BusResetHoldTime;         // in usec
    ULONG         FeatureSupport;


} PORT_CONFIGURATION_INFORMATION_EX, *PPORT_CONFIGURATION_INFORMATION_EX;


typedef
ULONG
HW_FIND_ADAPTER_EX (
    __in PVOID DeviceExtension,
    __in PVOID HwContext,
    __in PVOID BusInformation,
    __in_z PCHAR ArgumentString,
    __inout PPORT_CONFIGURATION_INFORMATION_EX ConfigInfo,
    __in PBOOLEAN Reserved3
    );
typedef HW_FIND_ADAPTER_EX *PHW_FIND_ADAPTER_EX;


/*
VOID
StorPortGetUncachedExtensionEx(
    __in PVOID HwDeviceExtension,
    __in PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    __in ULONG NumberOfBytes
    )*/
/*++

Routine Description:

    This function allocates a common buffer to be used as the uncached device
    extension for the miniport driver. 

Arguments:

    DeviceExtension - Supplies a pointer to the miniports device extension.

    ConfigInfo - Supplies a pointer to the partially initialized configuraiton
        information.  This is used to get an DMA adapter object.

    NumberOfBytes - Supplies the size of the extension which needs to be
        allocated

Return Value:

    A pointer to the uncached device extension or NULL if the extension could
    not be allocated or was previously allocated.

--*/
/*
{
    NTSTATUS Status;
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_ADAPTER_PARAMETERS Parameters;

    //
    // SCSIPORT also allocates the SRB extension from here. Wonder if
    // that's necessary at this point.
    //

    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return NULL;
    }

    //
    // The noncached extension has not been allocated. Allocate it.
    //

    if (!RaidIsRegionInitialized (&Adapter->UncachedExtension)) {

        //
        // The DMA Adapter may not have been initialized at this point. If
        // not, initialize it.
        //

        if (!RaidIsDmaInitialized (&Adapter->Dma)) {

            Status = RaidInitializeDma (&Adapter->Dma,
                                        Adapter->PhysicalDeviceObject,
                                        &Adapter->Miniport.PortConfiguration);

            if (!NT_SUCCESS (Status)) {
                return NULL;
            }
        }

        Parameters = &Adapter->Parameters;
        Status = RaidDmaAllocateUncachedExtension (&Adapter->Dma,
                                                   NumberOfBytes,
                                                   Parameters->MinimumUncachedAddress,
                                                   Parameters->MaximumUncachedAddress,
                                                   Parameters->UncachedExtAlignment,
                                                   &Adapter->UncachedExtension);

        //
        // Failed to allocate uncached extension; bail.
        //
        
        if (!NT_SUCCESS (Status)) {
            return NULL;
        }
    }

    //
    // Return the base virtual address of the region.
    //
    
    return RaidRegionGetVirtualBase (&Adapter->UncachedExtension);
}

*/

#endif /* __STOR_HELPER_H__ */

