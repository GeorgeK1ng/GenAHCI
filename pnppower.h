/*++

Copyright (C) Microsoft Corporation, 2009

Module Name:

    pnppower.h

Abstract:

    

Notes:

Revision History:

        Nathan Obr (natobr),  February 2005
        Michael Xing (xiaoxing),  December 2009
--*/

#pragma once

#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning(disable:4214) // bit field types other than int
#pragma warning(disable:4201) // nameless struct/union

// ACPI methods
#define ACPI_METHOD_GTF   ((ULONG) 'FTG_') // _GTF
#define ACPI_METHOD_SDD   ((ULONG) 'DDS_') // _SDD
#define ACPI_METHOD_PR3   ((ULONG) '3RP_') // _PR3
#define ACPI_METHOD_DSM   ((ULONG) 'MSD_') // _DSM

// ACPI _DSM method related definition
#define ACPI_METHOD_DSM_LINKPOWER_REVISION              0x01

#define ACPI_METHOD_DSM_LINKPOWER_FUNCTION_SUPPORT      0x00
#define ACPI_METHOD_DSM_LINKPOWER_FUNCTION_QUERY        0x01
#define ACPI_METHOD_DSM_LINKPOWER_FUNCTION_CONTROL      0x02

#define ACPI_METHOD_DSM_LINKPOWER_REMOVE_POWER          0x00
#define ACPI_METHOD_DSM_LINKPOWER_APPLY_POWER           0x01

//
// When waiting for the link to change power states, don't wait more
// than 100 microseconds.
//
#define AHCI_LINK_POWER_STATE_CHANGE_TIMEOUT_US   100

typedef union _AHCI_LPM_POWER_SETTINGS {
    struct {
    //LSB
    ULONG HipmEnabled: 1;
    ULONG DipmEnabled: 1;
    ULONG Reserved: 30;
    //MSB
    };

    ULONG AsUlong;

} AHCI_LPM_POWER_SETTINGS, *PAHCI_LPM_POWER_SETTINGS;

BOOLEAN 
AhciPortInitialize(
    __in PAHCI_CHANNEL_EXTENSION ChannelExtension
    );

BOOLEAN 
AhciAdapterPowerUp(
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension
    );

BOOLEAN 
AhciAdapterPowerSettingNotification(
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension,
    __in PSTOR_POWER_SETTING_INFO PowerSettingInfo
    );

BOOLEAN 
AhciAdapterPowerDown(
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension
    );

VOID 
AhciPortPowerUp(
    __in PAHCI_CHANNEL_EXTENSION ChannelExtension
    );

VOID 
AhciPortPowerDown(
    __in PAHCI_CHANNEL_EXTENSION ChannelExtension
    );

VOID 
AhciPortStop(
    __in PAHCI_CHANNEL_EXTENSION ChannelExtension
    );

VOID
AhciPortSmartCompletion(
    __in PAHCI_CHANNEL_EXTENSION ChannelExtension,
    __in PSCSI_REQUEST_BLOCK_EX Srb
  );

VOID
AhciPortNVCacheCompletion(
    __in PAHCI_CHANNEL_EXTENSION ChannelExtension,
    __in PSCSI_REQUEST_BLOCK_EX Srb
  );

VOID
AhciPortGetInitCommands(
    __in PAHCI_CHANNEL_EXTENSION ChannelExtension
  );

VOID
AhciPortEvaluateSDDMethod(
    __in PAHCI_CHANNEL_EXTENSION ChannelExtension
  );

VOID
AhciAdapterEvaluateDSMMethod(
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension
  );

VOID
AhciPortAcpiDSMControl(
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension,
    __in ULONG                   PortNumber,
    __in BOOLEAN                 Sleep
  );


VOID
IssuePreservedSettingCommands(
    __in PAHCI_CHANNEL_EXTENSION ChannelExtension,
    __in_opt PSCSI_REQUEST_BLOCK_EX Srb
  );

VOID
IssueInitCommands(
    __in PAHCI_CHANNEL_EXTENSION ChannelExtension,
    __in_opt PSCSI_REQUEST_BLOCK_EX Srb
  );

VOID
IssueSetDateAndTimeCommand(
    __in PAHCI_CHANNEL_EXTENSION ChannelExtension,
    __inout PSCSI_REQUEST_BLOCK_EX Srb,
    __in BOOLEAN SendStandBy
  );

VOID
IssueReadLogExtCommand(
    __in PAHCI_CHANNEL_EXTENSION ChannelExtension,
    __in PSCSI_REQUEST_BLOCK_EX Srb,
    __in UCHAR  LogAddress,
    __in USHORT PageNumber,
    __in USHORT BlockCount,
    __in USHORT FeatureField,
    __in PSTOR_PHYSICAL_ADDRESS PhysicalAddress,
    __in PVOID DataBuffer,
    __in_opt PSRB_COMPLETION_ROUTINE CompletionRoutine
    );

HW_TIMER_EX AhciAutoPartialToSlumber;

BOOLEAN
AhciLpmSettingsModes(
    __in PAHCI_CHANNEL_EXTENSION ChannelExtension,
    __in AHCI_LPM_POWER_SETTINGS LpmMode
    );


#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4214)
#pragma warning(default:4201)
#endif

