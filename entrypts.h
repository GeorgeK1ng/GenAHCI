/*++

Copyright (C) Microsoft Corporation, 2009

Module Name:

    entrypts.h

Abstract:

    This header contains the declaration of functions that will be called by Storport driver.

Authors:

--*/

#pragma once

#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4201)   // nameless struct/union


#define AHCI_POOL_TAG               'ichA'  // "Ahci" - StorAHCI miniport driver

#define NUM_ACCESS_RANGES           6

#define AHCI_MAX_PORT_COUNT         32
#define AHCI_MAX_DEVICE             1       //not support Port Multiplier
#define AHCI_MAX_LUN                8       //ATAport supports this much in old implementation.
#define AHCI_MAX_NCQ_REQUEST_COUNT  32

#define KB                          (1024)
#define AHCI_MAX_TRANSFER_LENGTH    (128 * KB)
#define MAX_SETTINGS_PRESERVED      32
#define MAX_CRB_LOG_INDEX           64

#define INQUIRYDATABUFFERSIZE       36

// timeout values and counters for channel start
// some big HDDs (e.g. 2TB) takes ~ 20 seconds to spin up.  We hit timeout during S3 resume for now,
// we reset after 1/3 * AHCI_PORT_START_TIMEOUT_IN_SECONDS Busy, and give up after AHCI_PORT_START_TIMEOUT_IN_SECONDS seconds

#define AHCI_PORT_START_TIMEOUT_IN_SECONDS  60      // 60 sec.

#define AHCI_PORT_WAIT_ON_DET_COUNT         3       // in unit of 10ms, default 30ms.


// port start states
#define WaitOnDET       0x11
#define WaitWhileDET1   0x12
#define WaitOnDET3      0x13
#define WaitOnFRE       0x14
#define WaitOnBSYDRQ    0x15
#define StartComplete   0x16
#define Stopped         0x20
#define StartFailed     0xff

//
// Bit field definitions for PortProperties field in CHANNEL_EXTENSION
//

// Clear: Internal port
// Set:   External port
#define PORT_PROPERTIES_EXTERNAL_PORT  ( 1 << 0 )

// registry flags apply to port or device
typedef struct _CHANNEL_REGISTRY_FLAGS {


    ULONG Reserved : 20;


} CHANNEL_REGISTRY_FLAGS, *PCHANNEL_REGISTRY_FLAGS;

// registry flags apply to the whole adapter
typedef struct _ADAPTER_REGISTRY_FLAGS {

    ULONG Reserved2 : 16;


} ADAPTER_REGISTRY_FLAGS, *PADAPTER_REGISTRY_FLAGS;

typedef struct _CHANNEL_STATE_FLAGS {

    ULONG Initialized : 1;
    ULONG NoMoreIO : 1;
    ULONG QueuePaused : 1;
    ULONG ReservedSlotInUse : 1;    //NOTE: this field is accessed in InterlockedBitTestAndReset, bit position (currently: 3) is used there.

    ULONG NCQ_Activated : 1;
    ULONG NCQ_Succeeded : 1;
    ULONG RestorePreservedSettingsActiveReferenced : 1; // this bit will be set if active reference is acquired for RestorePreservedSettings process
    ULONG HybridInfoEnabledOnHiberFile : 1;             // this bit indicates if priority level should be put for NCQ Write by StorAHCI when writes a hiber file.

    ULONG CallAhciReset : 1;
    ULONG CallAhciNonQueuedErrorRecovery : 1;
    ULONG CallAhciReportBusChange : 1;
    ULONG IgnoreHotplugInterrupt : 1;

    ULONG PowerDown : 1;            //NOTE: this field is accessed in InterlockedBitTestAndReset, bit position (currently: 12) is used there.
    ULONG PoFxEnabled : 1;
    ULONG PoFxActive : 1;
    ULONG D3ColdEnabled : 1;

    ULONG Reserved1;
} CHANNEL_STATE_FLAGS, *PCHANNEL_STATE_FLAGS;

typedef struct _ATA_IO_RECORD {

    ULONG   SuccessCount;

    ULONG   CrcErrorCount;
    ULONG   MediaErrorCount;
    ULONG   EndofMediaCount;
    ULONG   IllegalCommandCount;
    ULONG   AbortedCommandCount;

    ULONG   OtherErrorCount;

    ULONG   PortDriverResetCount;
    ULONG   TotalResetCount;

} ATA_IO_RECORD, *PATA_IO_RECORD;

//
// Data structure to retrieve a Log Page 
//
typedef struct _ATA_GPL_PAGE {
    BOOLEAN  Query;
    UCHAR    LogAddress;
    USHORT   PageNumber;
    USHORT   BlockCount;
    USHORT   FeatureField;
} ATA_GPL_PAGE, *PATA_GPL_PAGE;

#define ATA_GPL_PAGES_QUERY_COUNT       0x10
#define ATA_GPL_PAGES_INVALID_INDEX     0xFFFF

typedef struct _ATA_GPL_PAGES_TO_QUERY {
    USHORT          TotalPageCount;
    USHORT          CurrentPageIndex;

    ATA_GPL_PAGE    LogPage[ATA_GPL_PAGES_QUERY_COUNT];
} ATA_GPL_PAGES_TO_QUERY, *PATA_GPL_PAGES_TO_QUERY;

//
// This data structure defines supportive information of Log Address and Log Page
// If a Log Address contains more than one different Log Pages, there normally is Log Page 0x00 to get supported log pages.
//
typedef struct _ATA_SUPPORTED_GPL_PAGES {

    struct {
        ULONG  LogAddressSupported : 1;     // Log Address 0x04
        ULONG  GeneralStatistics : 1;           // Log Page 0x01
        ULONG  Reserved : 30;
    } DeviceStatistics;

    struct {
        ULONG  LogAddressSupported : 1;     // Log Address 0x30
        ULONG  SATA : 1;                        // Log Page 0x08
        ULONG  Reserved : 30;
    } IdentifyDeviceData;

    struct {
        ULONG  NcqCommandError                  : 1;         // Log Address 0x10
        ULONG  NcqNonData                       : 1;         // Log Address 0x12
        ULONG  NcqSendReceive                   : 1;         // Log Address 0x13
        ULONG  HybridInfo                       : 1;         // Log Address 0x14
        ULONG  CurrentDeviceInternalStatusData  : 1;         // Log Address 0x24
        ULONG  SavedDeviceInternalStatusData    : 1;         // Log Address 0x25

        ULONG  Reserved : 26;
    } SinglePage;

} ATA_SUPPORTED_GPL_PAGES, *PATA_SUPPORTED_GPL_PAGES;

typedef struct _ATA_COMMAND_SUPPORTED {

    ULONG  HybridDemoteBySize       : 1;
    ULONG  HybridChangeByLbaRange   : 1;
    ULONG  HybridControl            : 1;
    ULONG  HybridEvict              : 1;

    ULONG  SetDateAndTime           : 1;

    ULONG  Reserved                 : 27;

} ATA_COMMAND_SUPPORTED, *PATA_COMMAND_SUPPORTED;


typedef struct _AHCI_DEVICE_EXTENSION {
    STOR_ADDR_BTL8          DeviceAddress;
    ATA_DEVICE_PARAMETERS   DeviceParameters;
    ATA_IO_RECORD           IoRecord;       //IO counts completed by device, the count only change if an IO reaches hardware

    union {
      PIDENTIFY_DEVICE_DATA IdentifyDeviceData;
      PIDENTIFY_PACKET_DATA IdentifyPacketData;
    };
    STOR_PHYSICAL_ADDRESS   IdentifyDataPhysicalAddress;

    PUCHAR                  InquiryData;    //for ATAPI device, size:INQUIRYDATABUFFERSIZE
    STOR_PHYSICAL_ADDRESS   InquiryDataPhysicalAddress;

    ATA_GPL_PAGES_TO_QUERY  QueryLogPages;

    ATA_SUPPORTED_GPL_PAGES SupportedGPLPages;
    ATA_COMMAND_SUPPORTED   SupportedCommands;
    GP_LOG_HYBRID_INFORMATION_HEADER    HybridInfo;
    LONG                                HybridCachingMediumEnableRefs;

    PUSHORT                 ReadLogExtPageData;
    STOR_PHYSICAL_ADDRESS   ReadLogExtPageDataPhysicalAddress;

} AHCI_DEVICE_EXTENSION, *PAHCI_DEVICE_EXTENSION;

typedef struct _COMMAND_HISTORY {
    union {                              //0x10 bytes
        ATA_TASK_FILE InitialTaskFile;
        UCHAR         Cdb[16];
    };
    ULONG InitialPx[0x10];               //0x40 bytes
    ULONG CompletionPx[0x10];

    AHCI_D2H_REGISTER_FIS CompletionFIS; //0x14 bytes
    ULONG Tag;
    ULONG Function;
    ULONG SrbStatus;
} COMMAND_HISTORY, *PCOMMAND_HISTORY;

typedef struct _SLOT_MANAGER {
    ULONG HighPriorityAttribute;

    ULONG NCQueueSlice;
    ULONG NormalQueueSlice;
    ULONG SingleIoSlice;

    ULONG CommandsIssued;
    ULONG CommandsToComplete;
} SLOT_MANAGER, *PSLOT_MANAGER;

typedef struct _EXECUTION_HISTORY {
    ULONG        Function;
    ULONG        IS;
    SLOT_MANAGER SlotManager;   //SLOT_MANAGER from _AHCI_CHANNEL_EXTENSION
    ULONG        Px[0x10];      //Px registers value, end to AHCI_SNOTIFICATION -- SNTF
} EXECUTION_HISTORY, *PEXECUTION_HISTORY;

typedef struct _SLOT_STATE_FLAGS {
    UCHAR FUA :1;
    UCHAR Reserved :7;
} SLOT_STATE_FLAGS, *PSLOT_STATE_FLAGS;


typedef struct _SLOT_CONTENT {
    UCHAR                   CommandHistoryIndex;
    SLOT_STATE_FLAGS        StateFlags;
    UCHAR                   Reserved0[2];
    PSTORAGE_REQUEST_BLOCK  Srb;
    PAHCI_COMMAND_HEADER    CmdHeader;
    PVOID                   Reserved;
} SLOT_CONTENT, *PSLOT_CONTENT;

#pragma pack(1)
typedef struct _ACPI_GTF_IDE_REGISTERS {
    UCHAR    FeaturesReg;
    UCHAR    SectorCountReg;
    UCHAR    SectorNumberReg;
    UCHAR    CylLowReg;
    UCHAR    CylHighReg;
    UCHAR    DriveHeadReg;
    UCHAR    CommandReg;
} ACPI_GTF_IDE_REGISTERS, *PACPI_GTF_IDE_REGISTERS;
#pragma pack()

typedef struct _AHCI_DEVICE_INIT_COMMANDS {
    UCHAR           CommandCount;       // count of total commands, mainly used to allocate memory for CommandTaskFile
    UCHAR           ValidCommandCount;  // count of valid commands
    UCHAR           CommandToSend;      // command index indicating which command is to be sent
    PATA_TASK_FILE  CommandTaskFile;    // commands
} AHCI_DEVICE_INIT_COMMANDS, *PAHCI_DEVICE_INIT_COMMANDS;


typedef struct _SET_FEATURE_PARAMS {
    UCHAR Features;
    UCHAR SectorCount;
} SET_FEATURE_PARAMS, *PSET_FEATURE_PARAMS;

typedef struct _PERSISTENT_SETTINGS {
    ULONG Slots;
    ULONG SlotsToSend;
    SET_FEATURE_PARAMS CommandParams[MAX_SETTINGS_PRESERVED];
} PERSISTENT_SETTINGS, *PPERSISTENT_SETTINGS;

typedef struct _CHANNEL_START_STATE {
    UCHAR  ChannelNextStartState;
    UCHAR  ChannelStateDETCount;
    UCHAR  ChannelStateDET1Count;
    UCHAR  ChannelStateDET3Count;
    UCHAR  ChannelStateFRECount;
    UCHAR  AtDIRQL : 1;
    UCHAR  DirectStartInProcess : 1;
    UCHAR  Reserved : 6;
    USHORT ChannelStateBSYDRQCount;
} CHANNEL_START_STATE, *PCHANNEL_START_STATE;

typedef VOID
(*PSRB_COMPLETION_ROUTINE) (
    _In_ PAHCI_CHANNEL_EXTENSION    ChannelExtension,
    _In_opt_ PSTORAGE_REQUEST_BLOCK Srb
    );

typedef struct _LOCAL_SCATTER_GATHER_LIST {
    ULONG                       NumberOfElements;
    ULONG_PTR                   Reserved;
    _Field_size_(NumberOfElements)
    STOR_SCATTER_GATHER_ELEMENT List[33];
} LOCAL_SCATTER_GATHER_LIST, *PLOCAL_SCATTER_GATHER_LIST;

typedef struct _AHCI_SRB_EXTENSION {
    AHCI_COMMAND_TABLE CommandTable;        // this field MUST to be the first one as it's asked to be 128 aligned
    USHORT             AtaFunction;         // if this field is 0, it means the command does not need to be sent to device
    UCHAR              AtaStatus;
    UCHAR              AtaError;
    ULONG              Flags;
    union {
        ATA_TASK_FILE           TaskFile;       // for ATA device
        CDB                     Cdb;            // for ATAPI device
        AHCI_H2D_REGISTER_FIS   Cfis;           // info related to Host to Device FIS (0x27) 
    };
    PVOID              DataBuffer;          // go with Cdb field when needed.
    ULONG              DataTransferLength;  // go with Cdb field when needed.
    PLOCAL_SCATTER_GATHER_LIST  Sgl;        // pointer to the local or port provided SGL
    LOCAL_SCATTER_GATHER_LIST   LocalSgl;   // local SGL
    PSRB_COMPLETION_ROUTINE CompletionRoutine;   // go with Cdb field when needed.
    PVOID                   CompletionContext;   // context information for completionRoutine
    UCHAR              QueueTag;            // for AHCI controller slots
    UCHAR              RetryCount;          // how many times the command has been retired
    ULONGLONG          StartTime;

    PVOID               ResultBuffer;       // for requests marked with ATA_FLAGS_RETURN_RESULTS
    ULONG               ResultBufferLength;
} AHCI_SRB_EXTENSION, *PAHCI_SRB_EXTENSION;

typedef struct _LOCAL_COMMAND {
    SCSI_REQUEST_BLOCK      Srb;
    PAHCI_SRB_EXTENSION     SrbExtension;
    STOR_PHYSICAL_ADDRESS   SrbExtensionPhysicalAddress;
} LOCAL_COMMAND, *PLOCAL_COMMAND;

typedef enum {
    CallbackTypeNone = 0,       //not used
    CallbackTypeStartPort,
    CallbackTypeLPM,
    CallbackTypeMax             //count
} TIMER_CALLBACK_TYPE;

typedef
BOOLEAN
(*PAHCI_TIMER_CALLBACK) (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PVOID Context
    );

typedef struct _STORAHCI_QUEUE {
    PVOID Head;
    PVOID Tail;
    ULONG CurrentDepth;
    ULONG DeepestDepth;
    ULONG DepthHistoryIndex;
    ULONG DepthHistory[100];
} STORAHCI_QUEUE, *PSTORAHCI_QUEUE;

typedef struct _AHCI_ADAPTER_EXTENSION  AHCI_ADAPTER_EXTENSION, *PAHCI_ADAPTER_EXTENSION;

typedef struct _AHCI_CHANNEL_EXTENSION {
//Adapter Characteristics
    PAHCI_ADAPTER_EXTENSION AdapterExtension;

//Device Characteristics
    AHCI_DEVICE_EXTENSION   DeviceExtension[1];

//Channel Characteristics
    ULONG                   PortNumber;
    ULONG                   PortProperties;               // See PORT_PROPERTIES_XYZ definitions above in this file.

//Channel State
    CHANNEL_START_STATE     StartState;
    CHANNEL_STATE_FLAGS     StateFlags;
    CHANNEL_REGISTRY_FLAGS  RegistryFlags;

    UCHAR                   MaxPortQueueDepth;
    UCHAR                   CurrentCommandSlot;           // miniport driver use to remember which slot to put command in.
    UCHAR                   LastActiveSlot;
    UCHAR                   LastUserLpmPowerSetting;      // bit 0: HIPM; bit 1: DIPM

    ULONG                   AutoPartialToSlumberInterval; // in milliSeconds, max: 300,000 (5 minutes)

    AHCI_TASK_FILE_DATA     TaskFileData;

// Port runtime power management
    PSTOR_POFX_DEVICE       PoFxDevice;
    UCHAR                   PoFxFState;         // Current F-State of the unit
    STOR_DEVICE_POWER_STATE DevicePowerState;   // Current D-State of the unit.
    
    //
    // AutoPartialToSlumber statistics to aid in debugging.
    //
    struct {
        ULONG InterfaceNotReady;    // Number of times the interface was no ready (CMD.ICC != 0)
        ULONG InterfaceReady;       // Number of times the interface was ready (CMD.ICC == 0)
        ULONG ActiveFailCount;      // Number of times we failed to go to Active
        ULONG ActiveSuccessCount;   // Number of times we succeeded in going to Active
        ULONG SlumberFailCount;     // Number of times we failed to go to Slumber
        ULONG SlumberSuccessCount;  // Number of times we succeeded in going to Slumber
    } AutoPartialToSlumberDbgStats;


    struct {
        ULONG RestorePreservedSettings :1;  //NOTE: this field is accessed in InterlockedBitTestAndReset, bit position (currently: 0) is used there.
        ULONG BusChange :1;                 //NOTE: this field is accessed in InterlockedBitTestAndReset, bit position (currently: 1) is used there.

        ULONG Reserved: 30;
    } PoFxPendingWork;

//IO
    SLOT_MANAGER            SlotManager;
    SLOT_CONTENT            Slot[AHCI_MAX_NCQ_REQUEST_COUNT];

//Port IO Queue
    STORAHCI_QUEUE          SrbQueue;

//IO Completion Queue and DPC
    STORAHCI_QUEUE          CompletionQueue;
    STOR_DPC                CompletionDpc;

//DPC to handle hotplug notification
    STOR_DPC                BusChangeDpc;

//AHCI defined register interface structures
    PAHCI_PORT              Px;
    PAHCI_COMMAND_HEADER    CommandList;
    PAHCI_RECEIVED_FIS      ReceivedFIS;
    STOR_PHYSICAL_ADDRESS   CommandListPhysicalAddress;
    STOR_PHYSICAL_ADDRESS   ReceivedFisPhysicalAddress;

//Local Command Structures
    LOCAL_COMMAND               Local;
    LOCAL_COMMAND               Sense;              // used to retrieve Sense Data from device

    AHCI_DEVICE_INIT_COMMANDS   DeviceInitCommands;
    PERSISTENT_SETTINGS         PersistentSettings;

//Timer
    PVOID                   StartPortTimer;         // used for the Port Starting process
    PVOID                   WorkerTimer;            // used for LPM management for now

//Logging
    UCHAR                   CommandHistoryNextAvailableIndex;
    COMMAND_HISTORY         CommandHistory[64];
    UCHAR                   ExecutionHistoryNextAvailableIndex;
    EXECUTION_HISTORY       ExecutionHistory[100];

} AHCI_CHANNEL_EXTENSION, *PAHCI_CHANNEL_EXTENSION;

typedef struct _ADAPTER_STATE_FLAGS {

    ULONG StoppedState : 1;
    ULONG PowerDown : 1;
    ULONG PoFxEnabled : 1;
    ULONG PoFxActive : 1;

    ULONG SupportsAcpiDSM : 1;      // indicates if the system has _DSM method implemented to control port/device power. when the value is 1, the _DSM method at least supports powering on all connected devices.
    ULONG Removed : 1;

    ULONG Reserved : 25;

} ADAPTER_STATE_FLAGS, *PADAPTER_STATE_FLAGS;

typedef union _ADAPTER_LOG_FLAGS {

    struct {
        ULONG ExecutionDetail :1;
        ULONG CommandHistory :1;

        ULONG Reserved: 30;
    };

    ULONG AsUlong;

} ADAPTER_LOG_FLAGS, *PADAPTER_LOG_FLAGS;

#define LogCommand(flags)           (flags.CommandHistory != 0)
#define LogExecuteFullDetail(flags) (flags.ExecutionDetail != 0)


typedef struct _AHCI_ADAPTER_EXTENSION {
    ULONG                   AdapterNumber;
    ULONG                   SystemIoBusNumber;
    ULONG                   SlotNumber;
    ULONG                   AhciBaseAddress;    // AHCI Base Address from bus. for example: BAR5 value on PCI bus

    USHORT                  VendorID;
    USHORT                  DeviceID;
    UCHAR                   RevisionID;

// Adapter runtime power management
    PSTOR_POFX_DEVICE_V2    PoFxDevice;
    RAID_SYSTEM_POWER       SystemPowerHintState;
    ULONG                   SystemPowerResumeLatencyMSec;

    struct {
        ULONG AdapterStop :1;    //NOTE: this field is accessed in InterlockedBitTestAndReset, bit position (currently: 0) is used there.

        ULONG Reserved: 31;
    } PoFxPendingWork;

//Flags
    ADAPTER_STATE_FLAGS     StateFlags;
    ULONG                   ErrorFlags;     // save adapter errors
    ADAPTER_LOG_FLAGS       LogFlags;       // internal log levels
    ADAPTER_REGISTRY_FLAGS  RegistryFlags;

//adapter attributes
    ULONG                   PortImplemented;
    ULONG                   HighestPort;
    ULONG                   LastInterruptedPort;

    UCHAR                   DumpMode;
    BOOLEAN                 InRunningPortsProcess;  //in process of starting every implemented ports
    BOOLEAN                 TracingEnabled;

//Memory structures
    PAHCI_MEMORY_REGISTERS  ABAR_Address;           //mapped AHCI Base Address. StorAHCI uses this field to control the adapter and ports.
    AHCI_VERSION            Version;

    PULONG                  IS;
    AHCI_HBA_CAPABILITIES   CAP;
    AHCI_HBA_CAPABILITIES2  CAP2;

//Channel Extensions
    PAHCI_CHANNEL_EXTENSION PortExtension[AHCI_MAX_PORT_COUNT];

//nonCacheExtension
    PVOID                   NonCachedExtension;

} AHCI_ADAPTER_EXTENSION, *PAHCI_ADAPTER_EXTENSION;

// information that will be transferred to dump/hibernate environment
typedef struct _AHCI_DUMP_CONTEXT {
    // adapter information
    USHORT                  VendorID;
    USHORT                  DeviceID;
    UCHAR                   RevisionID;
    UCHAR                   Reserved[3];

    ULONG                   AhciBaseAddress;

    ADAPTER_LOG_FLAGS       LogFlags;       // internal log levels
    ADAPTER_REGISTRY_FLAGS  AdapterRegistryFlags;

    // port information
    ULONG                   DumpPortNumber;
    CHANNEL_REGISTRY_FLAGS  PortRegistryFlags;

    //
    // Device telemetry descriptors for the boot device
    //
    ULONG                   PublicGPLogTableAddresses[TC_PUBLIC_DEVICEDUMP_CONTENT_GPLOG_MAX];
    ULONG                   PrivateGPLogPageAddress;

    //
    // Hybrid Disk Information Log
    //
    GP_LOG_HYBRID_INFORMATION_HEADER    HybridInfo;

} AHCI_DUMP_CONTEXT, *PAHCI_DUMP_CONTEXT;


// Storport miniport driver entry routines, with prefix: "AhciHw"
sp_DRIVER_INITIALIZE DriverEntry;

HW_FIND_ADAPTER AhciHwFindAdapter;

HW_INITIALIZE AhciHwInitialize;

HW_PASSIVE_INITIALIZE_ROUTINE AhciHwPassiveInitialize;

HW_STARTIO AhciHwStartIo;

HW_BUILDIO AhciHwBuildIo;

HW_INTERRUPT AhciHwInterrupt;

HW_RESET_BUS AhciHwResetBus;

HW_ADAPTER_CONTROL AhciHwAdapterControl;

HW_TRACING_ENABLED AhciHwTracingEnabled;

HW_UNIT_CONTROL AhciHwUnitControl;


VOID
AhciGetNextIos (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ BOOLEAN AtDIRQL
    );

VOID
AhciDeviceStart (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    );

//
// References to globals
//
//
// Device telemetry descriptors for the boot device
//
extern ULONG AhciPublicGPLogTableAddresses[TC_PUBLIC_DEVICEDUMP_CONTENT_GPLOG_MAX];
extern ULONG AhciGPLogPageIntoPrivate;



#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4214)
#pragma warning(default:4201)
#endif


