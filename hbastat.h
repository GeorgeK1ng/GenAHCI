/*++

Copyright (C) Microsoft Corporation, 2009

Module Name:

    hbastat.h

Abstract:

    

Notes:

Revision History:

        Nathan Obr (natobr),  February 2005
        Michael Xing (xiaoxing),  December 2009
--*/

#pragma once

BOOLEAN
AhciAdapterReset( 
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension
    );

VOID
AhciCOMRESET(
    PAHCI_CHANNEL_EXTENSION ChannelExtension,
    PAHCI_PORT Px
    );

BOOLEAN
P_NotRunning(
    PAHCI_CHANNEL_EXTENSION ChannelExtension,
    PAHCI_PORT Px
    );

VOID
AhciAdapterRunAllPorts(
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension
    );

VOID
RunNextPort(
    __in PAHCI_CHANNEL_EXTENSION ChannelExtension,
    __in BOOLEAN AtDIRQL
    );

VOID
P_Running_StartAttempt(
    __in PAHCI_CHANNEL_EXTENSION ChannelExtension,
    __in BOOLEAN AtDIRQL
    );

BOOLEAN
P_Running(
    __in PAHCI_CHANNEL_EXTENSION ChannelExtension,
    __in BOOLEAN TimerCallbackProcess
    );

HW_TIMER_EX P_Running_Callback;

VOID
P_Running_WaitOnDET(
    __in PAHCI_CHANNEL_EXTENSION ChannelExtension,
    __in BOOLEAN TimerCallbackProcess
    );

VOID
P_Running_WaitWhileDET1(
    __in PAHCI_CHANNEL_EXTENSION ChannelExtension,
    __in BOOLEAN TimerCallbackProcess
    );

VOID
P_Running_WaitOnDET3(
    __in PAHCI_CHANNEL_EXTENSION ChannelExtension,
    __in BOOLEAN TimerCallbackProcess
    );

VOID
P_Running_WaitOnFRE(
    __in PAHCI_CHANNEL_EXTENSION ChannelExtension,
    __in BOOLEAN TimerCallbackProcess
    );

VOID
P_Running_WaitOnBSYDRQ(
    __in PAHCI_CHANNEL_EXTENSION ChannelExtension,
    __in BOOLEAN TimerCallbackProcess
    );

VOID
P_Running_StartFailed(
    __in PAHCI_CHANNEL_EXTENSION ChannelExtension,
    __in BOOLEAN TimerCallbackProcess
    );

BOOLEAN
AhciPortReset (
    __in PAHCI_CHANNEL_EXTENSION ChannelExtension,
    __in BOOLEAN CompleteAllRequests
    );

VOID
AhciPortErrorRecovery(
    __in PAHCI_CHANNEL_EXTENSION ChannelExtension
    );


