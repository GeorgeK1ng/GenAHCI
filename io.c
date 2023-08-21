/*++

Copyright (C) Microsoft Corporation, 2009

Module Name:

    io.c

Abstract:
    This file contains the core logic for programming and completing IO commands into an AHCI controller

Notes:

Revision History:

--*/

#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning(disable:4214) // bit field types other than int
#pragma warning(disable:4201) // nameless struct/union

#include "generic.h"


ULONG
GetSlotToActivate(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ ULONG                   TargetSlots
    )
/*++
    Select Slot to activate based on maximum slot number allowed

Called by:
    Activate Queue

It performs:

Affected Variables/Registers:
    none

Return Value:
--*/
{

    UCHAR activeCount = 0;
    UCHAR emptyCount;
    UCHAR requestCount;
    UCHAR lastActiveSlot;
    ULONG slotToActivate = 0;
    UCHAR i;

  //1 Device's queue depth is smaller than Controller's

    NT_ASSERT(ChannelExtension->DeviceExtension[0].DeviceParameters.MaxDeviceQueueDepth <= ChannelExtension->AdapterExtension->CAP.NCS);

    //count the number of slots already in use
    if (ChannelExtension->SlotManager.CommandsIssued > 0) {
        activeCount = NumberOfSetBits(ChannelExtension->SlotManager.CommandsIssued);
    }
    //1.1 Check if all slots are active.
    if (activeCount >= ChannelExtension->DeviceExtension[0].DeviceParameters.MaxDeviceQueueDepth) {
        //if all possible slots are full, no matter what, return no work (0)
        return 0;
    }

  //2 Look for any entry from last active slot
    requestCount = NumberOfSetBits(TargetSlots);
    lastActiveSlot = ChannelExtension->LastActiveSlot;
    emptyCount = ChannelExtension->DeviceExtension[0].DeviceParameters.MaxDeviceQueueDepth - activeCount;

  //3.1 Look for any entry from last active slot
    for (i = lastActiveSlot; i <= ChannelExtension->AdapterExtension->CAP.NCS; i++) {
        if ((TargetSlots & (1 << i)) > 0) {
            slotToActivate |= (1 << i);
            emptyCount--;
            requestCount--;
            if (emptyCount == 0 || requestCount == 0) {
                ChannelExtension->LastActiveSlot = i;
                return slotToActivate;
            }
        }
    }

  //3.2 Look for any entry from beginning to last active slot
  //Slot 0 is reserved for internal command
    for (i = 1 ; i <= lastActiveSlot; i++) {
        if ((TargetSlots & (1 << i)) > 0) {
            slotToActivate |= (1 << i);
            emptyCount--;
            requestCount--;
            if (emptyCount == 0 || requestCount == 0) {
                ChannelExtension->LastActiveSlot = i;
                return slotToActivate;
            }
        }
    }

    NT_ASSERT(FALSE);
    return slotToActivate;
}


UCHAR
GetSingleIo(
    PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
/*++
    Selects an IO circularly from the programmed Single IO slice starting with the slot after the most recently programmed slot
    This ensures the issuing of the SingleIO is FIFO

It assumes:
    Single IO are never high priority

Called by:
    Activate Queue

It performs:
    (overview)
    1 Initialize
    2 Chose the slot
    (details)
    1.1 Initialize variables
    2.1 Chose the slot circularly starting with CCS

Affected Variables/Registers:
    none

Return Value:
    Slot number of the oldest programmed Single IO Slice slot
    If no slots are programmed the tag returned is 0xFF
--*/
{
    UCHAR limit;
    UCHAR i;

    PAHCI_ADAPTER_EXTENSION adapterExtension = ChannelExtension->AdapterExtension;

  //1.1 Initialize variables
    limit = ChannelExtension->CurrentCommandSlot;

  // if there is internal request pending, always get it first.
    if ( (ChannelExtension->SlotManager.SingleIoSlice & 1) > 0 ) {
        return 0;
    }

  //2.1 Chose the slot circularly starting with CCS
    for (i = limit; i <= adapterExtension->CAP.NCS; i++) {
        if ( (ChannelExtension->SlotManager.SingleIoSlice & (1 << i)) > 0){
            return i;
        }
    }

    for (i = 0; i < limit; i++) {
        if ( (ChannelExtension->SlotManager.SingleIoSlice & (1 << i)) > 0){
            return i;
        }
    }

    return 0xff;
}

VOID
AddQueue (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _Inout_ PSTORAHCI_QUEUE Queue,
    _In_ PSTORAGE_REQUEST_BLOCK Srb,
    _In_ ULONG Signature,
    _In_ UCHAR Tag
    )
/*
Input Parameters:
    ChannelExtension
    Queue
    Srb
    Signature:  used to mark the location of last Srb in history log
    Tag:        bit 31 ~ 24, Queue->CurrentDepth: bit 23 ~ 0
*/
{
    PVOID tempTail, foundSrb;
    ULONG srbsFound;

    if (Queue->Tail == NULL) {
        Queue->Tail = (PVOID) Srb;
        Queue->Head = (PVOID) Srb;
    } else {
        tempTail = Queue->Tail;
        if (SrbGetNextSrb(tempTail) == NULL) {    //Verify SRBs are not about to be severed from the Queue
            SrbSetNextSrb(tempTail, (PVOID)Srb);
        } else {
            //intentional bugcheck to check queuing errors. Suppress null pointer dereferencing warnings
            #pragma warning (suppress: 6011)
            NT_ASSERT(FALSE); ChannelExtension = NULL; ChannelExtension->PortNumber++;
        }
        Queue->Tail = (PVOID)Srb;
        if (SrbGetNextSrb(Srb) != NULL) {         //Verify NextSrb is not in use by anyone
            //intentional bugcheck to check queuing errors. Suppress null pointer dereferencing warnings
            #pragma warning (suppress: 6011)
            NT_ASSERT(FALSE); ChannelExtension = NULL; ChannelExtension->PortNumber++;
        }
    }
    Queue->CurrentDepth++;
    if (Queue->Head == NULL) {              //It is impossible for Head to be NULL here
        //intentional bugcheck to check queuing errors. Suppress null pointer dereferencing warnings
        #pragma warning (suppress: 6011)
        NT_ASSERT(FALSE); ChannelExtension = NULL; ChannelExtension->PortNumber++;
        srbsFound = 0;
    } else {
        foundSrb = Queue->Head;
        srbsFound = 1;
        foundSrb = SrbGetNextSrb(foundSrb);
        while (foundSrb) {
            srbsFound++;
            foundSrb = SrbGetNextSrb(foundSrb);
        }
    }
    Queue->DepthHistory[Queue->DepthHistoryIndex] = ( (Tag << 24) | Queue->CurrentDepth );
    Queue->DepthHistoryIndex++;
    Queue->DepthHistoryIndex %= 100;
    Queue->DepthHistory[Queue->DepthHistoryIndex] = Signature;
    if (Queue->CurrentDepth != srbsFound ) {
        //intentional bugcheck to check queuing errors. Suppress null pointer dereferencing warnings
        #pragma warning (suppress: 6011)
        NT_ASSERT(FALSE); ChannelExtension = NULL; ChannelExtension->PortNumber++;
    }
    if (Queue->CurrentDepth > Queue->DeepestDepth) {
        Queue->DeepestDepth = Queue->CurrentDepth;
    }
}

PSTORAGE_REQUEST_BLOCK
RemoveQueue (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _Inout_ PSTORAHCI_QUEUE Queue,
    _In_ ULONG Signature,
    _In_ UCHAR Tag
    )
{
    PVOID nextSrb, foundSrb;
    ULONG srbsFound;

    //Check to see if the queue is empty
    if (Queue->Head == NULL) {
        return NULL;
    }

    //if it is not empty, pop
    nextSrb = Queue->Head;
    Queue->Head = SrbGetNextSrb(nextSrb);
    SrbSetNextSrb(nextSrb, NULL);                //This should have been NULL to start with, return it to it's NULL state

    if (Queue->Head == NULL) {
        Queue->Tail = NULL;
    }
    Queue->CurrentDepth--;
    if (Queue->Head == NULL) {
        srbsFound = 0;
    } else {
        foundSrb = Queue->Head;
        srbsFound = 1;
        foundSrb = SrbGetNextSrb(foundSrb);
        while (foundSrb) {
            srbsFound++;
            foundSrb = SrbGetNextSrb(foundSrb);
        }
    }
    Queue->DepthHistory[Queue->DepthHistoryIndex] = ( (Tag << 24) | Queue->CurrentDepth );
    Queue->DepthHistoryIndex++;
    Queue->DepthHistoryIndex %= 100;
    Queue->DepthHistory[Queue->DepthHistoryIndex] = Signature;
    if (Queue->CurrentDepth != srbsFound ) {
        //intentional bugcheck to check queuing errors. Suppress null pointer dereferencing warnings
        #pragma warning (suppress: 6011)
        NT_ASSERT(FALSE); ChannelExtension = NULL; ChannelExtension->PortNumber++;
    }
    if (Queue->CurrentDepth > Queue->DeepestDepth) {
        Queue->DeepestDepth = Queue->CurrentDepth;
    }
    return (PSTORAGE_REQUEST_BLOCK)nextSrb;
}


BOOLEAN
ActivateQueue(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ BOOLEAN AtDIRQL
    )
/*++

It assumes:

Called by:

It performs:
    (overview)
    1. Ensures the channel is ready for programming
    2. Program the next IO according to the priority scheme
    (details)
    1.1  Initialize variables
    1.2 If the programming should not happen now, leave, ActivateQueue will be called again when these conditions are changed
    2.1 Choose the Queue with which to program the controller
        Algorithm:
            2.1.1 Single IO SRBs (including Request Sense and non data control commands) have highest priority.
            2.1.2 When there are no Single IO commands, Normal IO get the next highest priority
            2.1.3 When there are no Single or Normal commands, NCQ commands get the next highest priority
            2.1.4 In the case that no IO is present in any Slices, program nothing
    2.2 Program all the IO from the chosen queue into the controller

Affected Variables/Registers:
    channelExtension
    CI
    SACT
Return Values:
--*/
{
    AHCI_COMMAND    cmd;
    ULONG           sact;
    ULONG           ci;
    ULONG           slotsToActivate;
    BOOLEAN         activateNcq;
    int             i;

    PAHCI_ADAPTER_EXTENSION adapterExtension = ChannelExtension->AdapterExtension;

    UNREFERENCED_PARAMETER(AtDIRQL);

  //1.1 Initialize variables
    if (LogExecuteFullDetail(adapterExtension->LogFlags)) {
        RecordExecutionHistory(ChannelExtension, 0x00000022);//ActivateQueue
    }

    slotsToActivate = 0;
    activateNcq = FALSE;

  //1.1.1 If there is no command to program, leave
    if ( (ChannelExtension->SlotManager.SingleIoSlice == 0) &&
         (ChannelExtension->SlotManager.NormalQueueSlice == 0) &&
         (ChannelExtension->SlotManager.NCQueueSlice == 0) ) {
        //
        if (LogExecuteFullDetail(adapterExtension->LogFlags)) {
            RecordExecutionHistory(ChannelExtension, 0x10040022);//ActivateQueue, No Commands to program
        }
        return FALSE;
    }

  //1.2 If the programming should not happen now, leave
    if ( !IsPortStartCapable(ChannelExtension) ) {
        RecordExecutionHistory(ChannelExtension, 0x10030022);//ActivateQueue, Channel Not Start Capable
        return FALSE;
    }

    if ( ErrorRecoveryIsPending(ChannelExtension) ) {
        RecordExecutionHistory(ChannelExtension, 0x10070022);//ActivateQueue, Error Recovery is pending.
        return FALSE;
    }

    if (ChannelExtension->StateFlags.QueuePaused == TRUE) {
        RecordExecutionHistory(ChannelExtension, 0x10020022);//ActivateQueue, Channel Queue Paused
        return FALSE;
    }

    cmd.AsUlong = StorPortReadRegisterUlong(adapterExtension, &ChannelExtension->Px->CMD.AsUlong);
    if (cmd.ST == 0) {
        RecordExecutionHistory(ChannelExtension, 0x10010022);//ActivateQueue, Channel Not Yet Started
        return FALSE;
    }

  //2.1 Choose the Queue with which to program the controller
    sact = StorPortReadRegisterUlong(adapterExtension, &ChannelExtension->Px->SACT);
    ci = StorPortReadRegisterUlong(adapterExtension, &ChannelExtension->Px->CI);
  //2.1.2 Single IO SRBs have highest priority.
    if(ChannelExtension->SlotManager.SingleIoSlice != 0) {
        if ( ( sact == 0 ) && ( ci == 0 ) ) {
            //Safely get Single IO in round robin fashion
            i = GetSingleIo(ChannelExtension);
            if (i != 0xff) {
                slotsToActivate = (1 << i);
                ChannelExtension->SlotManager.SingleIoSlice &= ~slotsToActivate;
                ChannelExtension->StateFlags.QueuePaused = TRUE;            //and pause the queue so no other IO get programmed
            }
        }
  //2.1.2 When there are no Single IO commands, Normal IO get the next highest priority
    } else if (ChannelExtension->SlotManager.NormalQueueSlice != 0) {
        // Normal commands can not be sent when NCQ commands are outstanding.  When the NCQ commands complete ActivateQueue will get called again.
        if (sact == 0) {
            //Grab the High Priority Normal IO before the Low Priority Normal IO
            slotsToActivate = ChannelExtension->SlotManager.HighPriorityAttribute & ChannelExtension->SlotManager.NormalQueueSlice;
            //If there aren't any High Priority, grab everything else
            if (slotsToActivate > 0) {
                ChannelExtension->SlotManager.NormalQueueSlice &= ~slotsToActivate;
            } else {
                slotsToActivate = ChannelExtension->SlotManager.NormalQueueSlice;
                ChannelExtension->SlotManager.NormalQueueSlice = 0;
            }
        }
  //2.1.3 When there are no Single or Normal commands, NCQ commands get the next highest priority
    } else if (ChannelExtension->SlotManager.NCQueueSlice != 0) {
        // NCQ commands can not be sent when Normal commands are outstanding.  When the Normal commands complete, Activate Queue will get called again.
        if ( ( ci != 0 ) && (sact == 0) ) {
            slotsToActivate = 0;
        } else {
            //Grab the High Priority NCQ IO before the Low Priority NCQ IO
            slotsToActivate = ChannelExtension->SlotManager.HighPriorityAttribute & ChannelExtension->SlotManager.NCQueueSlice;
            //If there aren't any High Priority, grab everything else
            if (slotsToActivate == 0) {
                slotsToActivate = ChannelExtension->SlotManager.NCQueueSlice;
            }
            //and apply any device outstanding IO limits to filter down which IO to activate
            if (slotsToActivate > 0) {
                if (ChannelExtension->DeviceExtension[0].DeviceParameters.MaxDeviceQueueDepth < ChannelExtension->MaxPortQueueDepth ) {
                    // get allowed slots if the device queue depth is less than the port can support.
                    slotsToActivate = GetSlotToActivate(ChannelExtension, slotsToActivate);
                }
                if (slotsToActivate > 0) {
                    //and if there are any IO still selected, clear them from the NCQueue
                    activateNcq = TRUE;     //Remember to program SACT for these commands
                    ChannelExtension->SlotManager.NCQueueSlice &= ~slotsToActivate;
                    //the selected IO will be activated at the end of this function
                }
            }
        }
  //2.1.4 In the case that no IO is present in any Slices, program nothing
    }

  //2.1 Program all the IO from the chosen queue into the controller
    if (slotsToActivate != 0) {
        //2.2 Get command start time
        if(adapterExtension->TracingEnabled) {
            LARGE_INTEGER perfCounter = {0};
            ULONG pendingProgrammingCommands = slotsToActivate;

            i = 0;

            StorPortQueryPerformanceCounter((PVOID)adapterExtension, NULL, &perfCounter);

            while (pendingProgrammingCommands) {
                if (pendingProgrammingCommands & 1) {
                    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(ChannelExtension->Slot[i].Srb);
                    srbExtension->StartTime = perfCounter.QuadPart;
                }

                i++;
                pendingProgrammingCommands >>= 1;
            }
        }

        ChannelExtension->SlotManager.CommandsIssued |= slotsToActivate;

        // program registers
        if (activateNcq) {
            StorPortWriteRegisterUlong(adapterExtension, &ChannelExtension->Px->SACT, slotsToActivate);
        }
        StorPortWriteRegisterUlong(adapterExtension, &ChannelExtension->Px->CI, slotsToActivate);

    }

    if (LogExecuteFullDetail(adapterExtension->LogFlags)) {
        RecordInterruptHistory(ChannelExtension, slotsToActivate, 0, 0, ci, sact, 0x10060022); //ActivateQueue, after getting IO slices,
        RecordExecutionHistory(ChannelExtension, 0x10050022);//Exit ActivateQueue
    }

    return TRUE;
}

__inline
ULONGLONG
CalculateTimeDurationIn100ns (
    _In_ ULONGLONG TimeDuration,
    _In_ ULONGLONG CounterFrequency
    )
{
    ULONGLONG timeIn100ns = 0;

    // as the counter can hold more than 20,000 years for the same power cylce,
    // the situation about the counter start over again is considered an error case of inputs.
    if (CounterFrequency > 0) {
        // difference between performance counters, needs to convert to 100ns.
        ULONGLONG countersDiff = TimeDuration;

        // get seconds
        timeIn100ns = countersDiff / CounterFrequency;

        // get milliseconds
        countersDiff = (countersDiff % CounterFrequency) * 1000;
        timeIn100ns *= 1000;
        timeIn100ns += countersDiff / CounterFrequency;

        // get 100 nanoseconds
        countersDiff = (countersDiff % CounterFrequency) * 10000;
        timeIn100ns *= 10000;
        timeIn100ns += countersDiff / CounterFrequency;
    }

    return timeIn100ns;
}

VOID
AhciCompleteIssuedSRBs(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ UCHAR SrbStatus,
    _In_ BOOLEAN AtDIRQL
  )
/*++
This function completes a command that has been programmed into a Slot

It assumes:
    The command in not running on the AHCI controller
    ATAPI commands will be sent 1 at a time

Called by:
    AhciHwInterrupt
    AhciPortReset
    AhciNonQueuedErrorRecovery

It performs:
    (overview)
    1 Initialize
    2 Process all commands in CommandsToComplete
    3 Start the next batch of commands
    (details)
    1.1 Initialize variables
    2.1 For every command marked as completed
    2.2 Set the status
    2.3 Monitor to see that any NCQ commands are completing
    2.4 Give the slot back
    3.1 Start the next IO(s) if any

Affected Variables/Registers:
    ChannelExtension->Slot[srbExtension->QueueTag].Srb
    ChannelExtension->SlotManager.CommandsToComplete

--*/
{
    PSLOT_CONTENT       slotContent;
    UCHAR               i;

    PAHCI_ADAPTER_EXTENSION adapterExtension;
    PAHCI_SRB_EXTENSION     srbExtension;

    LARGE_INTEGER           perfCounter = {0};
    LARGE_INTEGER           perfFrequency = {0};


  //1.1 Initialize variables
    adapterExtension = ChannelExtension->AdapterExtension;
    if (LogExecuteFullDetail(adapterExtension->LogFlags)) {
        RecordExecutionHistory(ChannelExtension, 0x00000046);//AhciCompleteIssuedSRBs
    }

    if( adapterExtension->TracingEnabled && (ChannelExtension->SlotManager.CommandsToComplete) ) {
        StorPortQueryPerformanceCounter((PVOID)adapterExtension, &perfFrequency, &perfCounter);
    }

  //2.1 For every command marked as completed
    for (i = 0; i <= (adapterExtension->CAP.NCS); i++) {
        if( ( ChannelExtension->SlotManager.CommandsToComplete & (1 << i) ) > 0) {
            slotContent = &ChannelExtension->Slot[i];
            srbExtension = GetSrbExtension(slotContent->Srb);

            if (slotContent->Srb == NULL) {
                //This shall never happen.
                //The completed slot has no SRB so it can not be completed back to Storport.
                //Give back the empty slot
                NT_ASSERT(FALSE);
                ChannelExtension->SlotManager.CommandsToComplete &= ~(1 << i);
                ChannelExtension->SlotManager.HighPriorityAttribute &= ~(1 << i);
                continue;
            }

            if (slotContent->CmdHeader == NULL) {
                //This shall never happen.
                //Give back the empty slot
                NT_ASSERT(FALSE);
                ChannelExtension->SlotManager.CommandsToComplete &= ~(1 << i);
                ChannelExtension->SlotManager.HighPriorityAttribute &= ~(1 << i);
                //It is now impossible to determine if a data transfer completed correctly
                slotContent->Srb->SrbStatus = SRB_STATUS_ABORTED;
                AhciCompleteRequest(ChannelExtension, slotContent->Srb, AtDIRQL);
                continue;
            }

          //2.1.2 Log command execution time, if it's allowed
            if ( adapterExtension->TracingEnabled &&
                 (srbExtension->StartTime != 0) &&
                 (perfCounter.QuadPart != 0) &&
                 !IsMiniportInternalSrb(ChannelExtension, slotContent->Srb) ) {

                ULONGLONG durationTime = CalculateTimeDurationIn100ns((perfCounter.QuadPart - srbExtension->StartTime), perfFrequency.QuadPart);
                StorPortNotification(IoTargetRequestServiceTime, (PVOID)adapterExtension, durationTime, slotContent->Srb);
            }

          //2.2 Set the status
            if( (SrbStatus == SRB_STATUS_SUCCESS) &&
                (!IsRequestSenseSrb(srbExtension->AtaFunction)) &&
                (srbExtension->AtaFunction != ATA_FUNCTION_ATA_SMART) &&
                (!IsNCQCommand(srbExtension)) &&
                (slotContent->CmdHeader->PRDBC != RequestGetDataTransferLength(slotContent->Srb)) ) {
                //
                if (slotContent->CmdHeader->PRDBC < RequestGetDataTransferLength(slotContent->Srb)) {
                    //buffer underrun,
                    RequestSetDataTransferLength(slotContent->Srb, slotContent->CmdHeader->PRDBC);
                    slotContent->Srb->SrbStatus = SrbStatus;
                } else {
                    //buffer overrun, return error
                    NT_ASSERT(FALSE);
                    slotContent->Srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;
                }
            } else {
                //If anything has set a STATUS on this SRB, honor that over the one passed in
                if (slotContent->Srb->SrbStatus == SRB_STATUS_PENDING) {
                    slotContent->Srb->SrbStatus = SrbStatus;
                }
            }

          //2.3 Monitor to see that any NCQ commands are completing
            if ( (slotContent->Srb->SrbStatus == SRB_STATUS_SUCCESS) &&
                 (IsNCQCommand(srbExtension)) ) {
                ChannelExtension->StateFlags.NCQ_Succeeded = TRUE;
            }

          //2.4 Give the slot back
            ReleaseSlottedCommand(ChannelExtension, i, AtDIRQL); // Request sense is handled here.

            if (LogExecuteFullDetail(adapterExtension->LogFlags)) {
                RecordExecutionHistory(ChannelExtension, 0x10000046);//Completed one SRB
            }
        }
    }

  //3.1 Start the next IO(s) if any
    AhciGetNextIos(ChannelExtension, AtDIRQL);

    return;
}

VOID
SRBtoATA_CFIS(
    PAHCI_CHANNEL_EXTENSION ChannelExtension,
    PSLOT_CONTENT SlotContent
  )
/*++
    Populates CFIS structure with an ATA command in the given slot
It assumes:
    The slot is empty and not being used
Called by:
    AhciHwStartIo

It performs:
    (overview)
    1 Fills in the CFIS structure
    (details)
    1.1 Map SRB fields to CFIS fields
    1.2 Specail case mapping of NCQ

Affected Variables/Registers:
    Command Table
--*/
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(SlotContent->Srb);
    PAHCI_COMMAND_TABLE cmdTable = (PAHCI_COMMAND_TABLE)srbExtension;

    UNREFERENCED_PARAMETER(ChannelExtension);

  //1.1 Map SRB fields to CFIS fields
    cmdTable->CFIS.FisType = 0x27;
    cmdTable->CFIS.PMPort = 0;      // StorAHCI doesn't support Port Multiplier
    cmdTable->CFIS.Reserved1 = 0;
    cmdTable->CFIS.C = 1;
    cmdTable->CFIS.Command = srbExtension->TaskFile.Current.bCommandReg;
  //1.2 Specail case mapping of NCQ
    if( IsNCQCommand(srbExtension) ){
        cmdTable->CFIS.Features = srbExtension->TaskFile.Current.bSectorCountReg;
        cmdTable->CFIS.Features_Exp = srbExtension->TaskFile.Previous.bSectorCountReg;
        cmdTable->CFIS.SectorCount = (srbExtension->QueueTag << 3);
        cmdTable->CFIS.Dev_Head = 0xF & srbExtension->TaskFile.Current.bDriveHeadReg;
        cmdTable->CFIS.Dev_Head |= (1 << 6);

        if( SlotContent->StateFlags.FUA ){
            cmdTable->CFIS.Dev_Head |= ATA_NCQ_FUA_BIT;
        } else {
            cmdTable->CFIS.Dev_Head &= ~ATA_NCQ_FUA_BIT;
        }

    } else {
        cmdTable->CFIS.Features = srbExtension->TaskFile.Current.bFeaturesReg;
        cmdTable->CFIS.Features_Exp = srbExtension->TaskFile.Previous.bFeaturesReg;
        cmdTable->CFIS.SectorCount = srbExtension->TaskFile.Current.bSectorCountReg;
        cmdTable->CFIS.SectorCount_Exp = srbExtension->TaskFile.Previous.bSectorCountReg;
        cmdTable->CFIS.Dev_Head = srbExtension->TaskFile.Current.bDriveHeadReg;
    }

  //1.1 Map SRB fields to CFIS fields
    cmdTable->CFIS.SectorNumber = srbExtension->TaskFile.Current.bSectorNumberReg;
    cmdTable->CFIS.SecNum_Exp = srbExtension->TaskFile.Previous.bSectorNumberReg;

    cmdTable->CFIS.CylLow = srbExtension->TaskFile.Current.bCylLowReg;
    cmdTable->CFIS.CylLow_Exp = srbExtension->TaskFile.Previous.bCylLowReg;

    cmdTable->CFIS.CylHigh = srbExtension->TaskFile.Current.bCylHighReg;
    cmdTable->CFIS.CylHigh_Exp = srbExtension->TaskFile.Previous.bCylHighReg;

    cmdTable->CFIS.ICC = 0;
    cmdTable->CFIS.Control = 0; // Device control consists of the 48bit HighOrderByte, SRST and nIEN.  None apply here.

    cmdTable->CFIS.Auxiliary7_0 = 0;
    cmdTable->CFIS.Auxiliary15_8 = 0;
    cmdTable->CFIS.Auxiliary23_16 = 0;
    cmdTable->CFIS.Auxiliary31_24 = 0;
}

VOID
SRBtoATAPI_CFIS(
    PAHCI_CHANNEL_EXTENSION ChannelExtension,
    PSLOT_CONTENT SlotContent
  )
/*++
    Populates CFIS structure with an ATAPI command in the given slot
It assumes:
    The slot is empty and not being used
Called by:
    AhciHwStartIo

It performs:
    (overview)
    1 Popluate the ACMD
    2 Popluate the PACKET command
    (details)
    1.1 Memcopy CDB into ACMD
    2.1 Put the PACKET Command in the CFIS
    2.2 Populate DMA bit properly

Affected Variables/Registers:
    Command Table
--*/
{
                        //These are used for copying a CDB to ACMD on ATAPI commands.
    ULONG dataLength;
    PVOID cdb;

  //1.1 Memcopy CDB into ACMD
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(SlotContent->Srb);
    PAHCI_COMMAND_TABLE cmdTable = (PAHCI_COMMAND_TABLE)srbExtension;

    UNREFERENCED_PARAMETER(ChannelExtension);

    cdb = SrbGetCdb(SlotContent->Srb);
    dataLength = RequestGetDataTransferLength(SlotContent->Srb);

    if (cdb != NULL) {
        StorPortCopyMemory((PVOID)cmdTable->ACMD, cdb, 16);
    } else {
        NT_ASSERT(cdb != NULL);
    }

  //2.1 Put the PACKET Command in the CFIS
    cmdTable->CFIS.FisType = 0x27;
    cmdTable->CFIS.PMPort = 0;      // StorAHCI doesn't support Port Multiplier
    cmdTable->CFIS.Reserved1 = 0;
    cmdTable->CFIS.C = 1;
    cmdTable->CFIS.Command = IDE_COMMAND_ATAPI_PACKET;   //A0 is the PACKET command

  //2.2 Populate DMA bit properly, only use it for data transfer
    if (IsDmaCommand(srbExtension->Flags)) {
        cmdTable->CFIS.Features = 0x1;

        //If word 62 bit 15 is set to one, then the DMADIR bit in the PACKET command is required by the device for
        //PACKET commands using the DMA data transfer protocol and:
        //  a) word 63 bits (2:0);        -- MultiWordDMASupport & 0x7 == 0
        //  b) word 49 bit 15;            -- Capabilities.InterleavedDmaSupported
        //  c) word 49 bit 8; and         -- Capabilities.DmaSupported
        //  d) word 88 bits (6:0),        -- UltraDMASupport & 0x7F == 0
        //shall be cleared to zero.
        if ( (ChannelExtension->DeviceExtension[0].IdentifyPacketData->DMADIR.DMADIRBitRequired != 0) &&
             (ChannelExtension->DeviceExtension[0].IdentifyPacketData->Capabilities.DmaSupported != 0) &&
             (ChannelExtension->DeviceExtension[0].IdentifyPacketData->Capabilities.InterleavedDmaSupported == 0) &&
             ((ChannelExtension->DeviceExtension[0].IdentifyPacketData->MultiWordDMASupport & 0x7) == 0) &&
             ((ChannelExtension->DeviceExtension[0].IdentifyPacketData->UltraDMASupport & 0x7F) == 0) ) {
            //bit 0 is DMA, bit 2 is DMADIR. (0 = transfer to the device; 4 = transfer to the host).
            cmdTable->CFIS.Features |= (srbExtension->Flags & ATA_FLAGS_DATA_IN) ? 4 : 0;
        }
    } else {
        cmdTable->CFIS.Features = 0x0;
    }

  //2.1 Put the PACKET Command in the CFIS
    cmdTable->CFIS.SectorNumber = 0;
    cmdTable->CFIS.CylLow = (UCHAR)dataLength; //put the low byte of the in CylLow
    dataLength >>= 8;
    cmdTable->CFIS.CylHigh = (UCHAR)dataLength; //put the high byte in CylHigh
    cmdTable->CFIS.Dev_Head = 0;

    cmdTable->CFIS.SecNum_Exp = 0;
    cmdTable->CFIS.CylLow_Exp = 0;
    cmdTable->CFIS.CylHigh_Exp = 0;
    cmdTable->CFIS.Features_Exp = 0;

    cmdTable->CFIS.SectorCount = 0;
    cmdTable->CFIS.SectorCount_Exp = 0;
    cmdTable->CFIS.ICC = 0;
    cmdTable->CFIS.Control = 0; // Device control consists of the 48bit HighOrderByte, SRST and nIEN.  None apply here.

    cmdTable->CFIS.Auxiliary7_0 = 0;
    cmdTable->CFIS.Auxiliary15_8 = 0;
    cmdTable->CFIS.Auxiliary23_16 = 0;
    cmdTable->CFIS.Auxiliary31_24 = 0;
}

VOID
CfistoATA_CFIS(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSLOT_CONTENT SlotContent
  )
/*++
    Populates CFIS structure with an ATA command in the given slot from a CFIS data structure.
It assumes:
    The slot is empty and not being used
Called by:
    AhciHwStartIo

It performs:
    1 Copy CFIS structure from SrbExtension
    2 Make sure common data fields value are appropriate

Affected Variables/Registers:
    Command Table
--*/
{
    PAHCI_SRB_EXTENSION     srbExtension = GetSrbExtension(SlotContent->Srb);
    PAHCI_COMMAND_TABLE     cmdTable = (PAHCI_COMMAND_TABLE)srbExtension;

    UNREFERENCED_PARAMETER(ChannelExtension);

    //
    // Copy CFIS data structure.
    //
    StorPortCopyMemory(&cmdTable->CFIS, &srbExtension->Cfis, sizeof(AHCI_H2D_REGISTER_FIS));

    //
    // When write a hiber file, the request can be retried.
    // Retry the command without Hybrid Hinting info.
    //
    if (IsDumpHiberMode(ChannelExtension->AdapterExtension) && 
        IsNCQWriteCommand(srbExtension) &&
        (srbExtension->RetryCount > 0) &&
        (ChannelExtension->StateFlags.HybridInfoEnabledOnHiberFile == 0)) {
        cmdTable->CFIS.Auxiliary23_16 = 0;
    }

    //
    // Set common data fields.
    //
    if( IsNCQCommand(srbExtension) ) {
        cmdTable->CFIS.Count7_0 = (srbExtension->QueueTag << 3);
    }
    cmdTable->CFIS.FisType = 0x27;
    cmdTable->CFIS.PMPort = 0;      // StorAHCI doesn't support Port Multiplier
    cmdTable->CFIS.Reserved1 = 0;
    cmdTable->CFIS.C = 1;

    cmdTable->CFIS.Auxiliary15_8 = 0;
    cmdTable->CFIS.Auxiliary31_24 = 0;

    cmdTable->CFIS.ICC = 0;
    cmdTable->CFIS.Control = 0;     // Device control consists of the 48bit HighOrderByte, SRST and nIEN.  None apply here.
}


ULONG
SRBtoPRDT(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSLOT_CONTENT SlotContent
  )
/*++

It assumes:
    MDLs and ScatterGatherList entries will not violate PRDT rules
Called by:
    AhciHwStartIo
It performs:
    (overview)
    1 Get the DataBuffer's address
    2 Map SGL entries into PRDT entries
    (details)
    1.1 Get the ScatterGatherList
    1.2 Verify that the DataBuffer is properly aligned
    2.1 Map SGL entries into PRDT entries
    2.2 Break up all 128K single entry IO into 2 64K IO entries
    2.3 Verify that the DataLength is even

Affected Variables/Registers:

Return Values:
    The number of entries generated to fill the PRDT
    If the value returned is -1 the PRDT could not be built.
--*/
{
    ULONG                       i;
    PAHCI_SRB_EXTENSION         srbExtension = GetSrbExtension(SlotContent->Srb);
    PAHCI_COMMAND_TABLE         cmdTable = (PAHCI_COMMAND_TABLE)srbExtension;
    PLOCAL_SCATTER_GATHER_LIST  sgl = srbExtension->Sgl;

    if (sgl == NULL) {
       //return as invalid request in case of cannot get scatter gather list.
        NT_ASSERT(FALSE);
        return (ULONG)-1;
    }

    for (i = 0; i < sgl->NumberOfElements; i++) {
      //1.2 Verify that the DataBuffer is properly aligned
        if ( (sgl->List[i].PhysicalAddress.LowPart & 0x1) == 0) {
          //2.1 Map SGL entries into PRDT entries
            if (sgl->List[i].Length != 0x20000) {
                cmdTable->PRDT[i].DBA.AsUlong = sgl->List[i].PhysicalAddress.LowPart;
                if( ChannelExtension->AdapterExtension->CAP.S64A) {//If the controller supports 64 bits, write the high part too
                    cmdTable->PRDT[i].DBAU = sgl->List[i].PhysicalAddress.HighPart;
                }
          //2.2 Break up a 128K single entry IO into 2 64K IO entries (128K is max transfer so there can be only 1 in any SGL)
          //    although one entry can represent at max 4M length IO, some adapters cannot handle a DBC >= 128K.
            } else {
                // Entry 0
                cmdTable->PRDT[0].DBA.AsUlong = sgl->List[0].PhysicalAddress.LowPart;
                if( ChannelExtension->AdapterExtension->CAP.S64A) {//If the controller supports 64 bits, write the high part too
                    cmdTable->PRDT[0].DBAU = sgl->List[0].PhysicalAddress.HighPart;
                }
                cmdTable->PRDT[0].DI.DBC = (0x10000 - 1);
                // Entry 1
                cmdTable->PRDT[1].DBA.AsUlong = (sgl->List[0].PhysicalAddress.LowPart + 0x10000);
                if( ChannelExtension->AdapterExtension->CAP.S64A) {//If the controller supports 64 bits, write the high part too
                    if ( (sgl->List[0].PhysicalAddress.LowPart + 0x10000) < sgl->List[0].PhysicalAddress.LowPart) {
                        cmdTable->PRDT[1].DBAU = (sgl->List[0].PhysicalAddress.HighPart + 1); //add 1 to the highpart if adding 0x10000 caused a rollover
                    } else {
                        cmdTable->PRDT[1].DBAU = sgl->List[0].PhysicalAddress.HighPart;
                    }
                }
                cmdTable->PRDT[1].DI.DBC = (0x10000 - 1);
                return 2;
            }
        } else {
            NT_ASSERT(FALSE); //Shall Not Pass
            return (ULONG)-1;
        }

      //1.3 Verify that the DataLength is even
        // all SATA transfers must be even
        // DBC is a 0 based number (i.e. 0 is 1, 1 is 2, etc.
        // sgl->Elements.Length is not (i.e. 0 is 0, 1 is 1, etc.
        if( (sgl->List[i].Length & 1) == 0 ) {                    //therefore length must be even here
          //2.3 Set Datalength in the PRDT entries
            cmdTable->PRDT[i].DI.DBC = sgl->List[i].Length - 1;         //but it must be  odd here
        } else if (sgl->List[i].Length <= RequestGetDataTransferLength(SlotContent->Srb)) {
            // Storport may send down SCSI commands with odd number of data transfer length, and it builds SGL using that transfer length value.
            // we use the length -1 to get as much data as we can. If the data length is over (length - 1), buffer overrun will be reported when the command is completed.
            RequestSetDataTransferLength(SlotContent->Srb, RequestGetDataTransferLength(SlotContent->Srb) - 1);
            cmdTable->PRDT[i].DI.DBC = sgl->List[i].Length - 2;
        } else {
            NT_ASSERT(FALSE); //Shall Not Pass
            return (ULONG)-1;
        }
    }
    return sgl->NumberOfElements;
}

VOID
SRBtoCmdHeader(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSLOT_CONTENT SlotContent,
    _In_ ULONG Length,
    _In_ BOOLEAN Reset
    )
/*++
It assumes:
    The chosen slot is available and not in use
Called by:
    AhciHwStartIo
It performs:
    Steps defined in AHCI 1.2 section 5.5.1 step #3
Affected Variables/Registers:
    Command Header
--*/
{
    PAHCI_COMMAND_HEADER    cmdHeader = SlotContent->CmdHeader;
    PSTORAGE_REQUEST_BLOCK  srb = SlotContent->Srb;
    PAHCI_SRB_EXTENSION     srbExtension = GetSrbExtension(srb);

    UNREFERENCED_PARAMETER(ChannelExtension);

//  a.  PRDTL containing the number of entries in the PRD table
    cmdHeader->DI.PRDTL = Length;
//  b.  CFL set to the length of the command in the CFIS area
    cmdHeader->DI.CFL = 5;
//  c.  A bit set if it is an ATAPI command
    cmdHeader->DI.A = (srbExtension->AtaFunction & ATA_FUNCTION_ATAPI_COMMAND) ? 1 : 0;
//  d.  W (Write) bit set if data is going to the device
    cmdHeader->DI.W = (srbExtension->Flags & ATA_FLAGS_DATA_OUT) ? 1 : 0;
//  e.  P (Prefetch) bit optionally set (see rules in section 5.5.2)
    //Some controllers have problems if P is set.
    cmdHeader->DI.P = 0;
//  f.  If a Port Multiplier is attached, the PMP field set to the correct Port Multiplier port.
    cmdHeader->DI.PMP = 0;

    //Reset
    cmdHeader->DI.R = Reset;
    cmdHeader->DI.B = 0;
    cmdHeader->DI.C = Reset;

    //initialize the PRD byte count
    cmdHeader->PRDBC = 0;

    cmdHeader->Reserved[0] = 0;
    cmdHeader->Reserved[1] = 0;
    cmdHeader->Reserved[2] = 0;
    cmdHeader->Reserved[3] = 0;
}

BOOLEAN
AhciProcessIo(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ BOOLEAN AtDIRQL
    )
/*
This routine does following:
    1. Check if no device command associated with Srb, or the port failed to start. Bail out in these cases.
    2. Central place for specail command handling.
    3. Get available slot
    4. Call AhciFormIo() to Fill the slot; Program command Table & Header; Put IO in Slice

Note: This routine can be called even the Port is stopped.
      This routine and the calling routine AhciFormIo() does NOT program IO to adapter.
*/
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);
    UCHAR   pathId = 0;
    UCHAR   targetId = 0;
    UCHAR   lun = 0;

    SrbGetPathTargetLun(Srb, &pathId, &targetId, &lun);

    //1.0 complete Srb if no command should be sent to device.
    if (srbExtension->AtaFunction == 0) {
        NT_ASSERT(FALSE); // should investigate if this ASSERT fires
        Srb->SrbStatus = SRB_STATUS_ERROR;
        MarkSrbToBeCompleted(Srb);
        AhciCompleteRequest(ChannelExtension, Srb, AtDIRQL);
        return TRUE;
    }

  //1.1 Check if command processing should happen
    if (ChannelExtension->StartState.ChannelNextStartState == StartFailed) {
        Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
        MarkSrbToBeCompleted(Srb);
        AhciCompleteRequest(ChannelExtension, Srb, AtDIRQL);
        return TRUE;
    }

  //2. central palce for command special handling. this part cannot be put in command translation layer, as there might be internal command also needs special handling.
    //2.1 central place for special handling of Enable/Disable WRITE CACHE, update persistent settings command.
    if ( (srbExtension->AtaFunction == ATA_FUNCTION_ATA_COMMAND) &&
         (srbExtension->TaskFile.Current.bCommandReg == IDE_COMMAND_SET_FEATURE) ) {
        ////
        if (srbExtension->TaskFile.Current.bFeaturesReg == IDE_FEATURE_DISABLE_WRITE_CACHE) {
            //IDE_FEATURE_DISABLE_WRITE_CACHE => Modify Persistent Settings
            UpdateSetFeatureCommands(ChannelExtension, IDE_FEATURE_ENABLE_WRITE_CACHE, IDE_FEATURE_DISABLE_WRITE_CACHE, 0, 0);
        }
        if (srbExtension->TaskFile.Current.bFeaturesReg == IDE_FEATURE_ENABLE_WRITE_CACHE) {
            //IDE_FEATURE_ENABLE_WRITE_CACHE => Modify Persistent Settings
            UpdateSetFeatureCommands(ChannelExtension, IDE_FEATURE_DISABLE_WRITE_CACHE, IDE_FEATURE_ENABLE_WRITE_CACHE, 0, 0);
        }
    }

    //2.2 update the Identify command for enumeration. This needs to be here waiting for PxSIG to be accessible.
    if ( (srbExtension->AtaFunction == ATA_FUNCTION_ATA_IDENTIFY) &&
         (srbExtension->CompletionRoutine == AhciPortIdentifyDevice) &&
         (srbExtension->TaskFile.Current.bCommandReg == IDE_COMMAND_NOT_VALID) ) {
        // for enumeration command, it's safe now to check the PxSIG register as starting port has been executed.
        ULONG   sig = 0;
        sig = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SIG.AsUlong);

        if (sig == 0x101) { //ATA
            srbExtension->TaskFile.Current.bCommandReg = IDE_COMMAND_IDENTIFY;
        } else {            //ATAPI
            srbExtension->TaskFile.Current.bCommandReg = IDE_COMMAND_ATAPI_IDENTIFY;
        }
    }

    //2.3 this is central place to update this register value in case it's wrongly set.
    if (IsAtaCommand(srbExtension->AtaFunction)) {
        SetDeviceReg((&srbExtension->TaskFile.Current), 0);   //make sure set to device0
    }

  //3 Find an available slot/tag (AHCI 1.1 Section 5.5.1)
    GetAvailableSlot(ChannelExtension, Srb);    //srbExtension->QueueTag will be set

    // 3.1 If no tag is available, reject the command to be retried later
    if ( srbExtension->QueueTag > ChannelExtension->AdapterExtension->CAP.NCS ) {
        //wait for 8 IO (random picked number) being completed before re-starting sending IO to miniport for this device.
        // if filled up,
        if (!IsMiniportInternalSrb(ChannelExtension, Srb)) {
            // report busy only for IOs from Storport
            StorPortDeviceBusy(ChannelExtension->AdapterExtension, pathId, targetId, lun, min(8, ChannelExtension->AdapterExtension->CAP.NCS));
        }
        Srb->SrbStatus = SRB_STATUS_BUSY;
        MarkSrbToBeCompleted(Srb);
        AhciCompleteRequest(ChannelExtension, Srb, AtDIRQL);
        RecordExecutionHistory(ChannelExtension, 0x10030020);   //AllocateQueueTagFailed
        return TRUE;
    }

#ifdef DBG
    {
        ULONG   ci;
        ULONG   sact;
        ci = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->CI);
        sact = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SACT);
        // 3.2 if the tag returned can't be used, something is seriously wrong. Reject the command to be retried later
        if( ( ( (1 << srbExtension->QueueTag) & ci) > 0) || ( ( (1 << srbExtension->QueueTag) & sact) > 0) ) {
            NT_ASSERT(FALSE); //can this happen? catch it.

            if (!IsMiniportInternalSrb(ChannelExtension, Srb)) {
                // pause device only for IOs from Storport
                StorPortPauseDevice(ChannelExtension->AdapterExtension, pathId, targetId, lun, 1);   //pause for 1 seconds.
            }
            Srb->SrbStatus = SRB_STATUS_BUSY;
            MarkSrbToBeCompleted(Srb);
            AhciCompleteRequest(ChannelExtension, Srb, AtDIRQL);
            RecordExecutionHistory(ChannelExtension, 0x10040020);//Tag given for slot in use
            return TRUE;
        }
    }
#endif
    //srbExtension->QueueTag and Slot[srbExtension->QueueTag] are now guaranteed ready.

    return AhciFormIo (ChannelExtension, Srb, AtDIRQL);
}

BOOLEAN
AhciFormIo(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ BOOLEAN AtDIRQL
    )
/*
    Caller: only AhciProcessIo()
*/
{
    PAHCI_SRB_EXTENSION   srbExtension;
    PSLOT_CONTENT         slotContent;
    PAHCI_COMMAND_TABLE   cmdTable;
    PAHCI_COMMAND_HEADER  cmdHeader;
    STOR_PHYSICAL_ADDRESS cmdTablePhysicalAddress;
    ULONG                 prdtLength = 0;
    PCDB                  cdb = SrbGetCdb(Srb);

    srbExtension = GetSrbExtension(Srb);

    // command header is allocated in ChannelExtension for all command slots, get the correct one.
    cmdHeader = ChannelExtension->CommandList;
    cmdHeader += srbExtension->QueueTag;

    slotContent = &ChannelExtension->Slot[srbExtension->QueueTag];

  // 1. setup slot content and log history
    slotContent->Srb = Srb;
    slotContent->CmdHeader = cmdHeader;

  //1.1 Update FUA tracking flag if needed
    if( !IsReturnResults(srbExtension->Flags) &&
        (cdb != NULL) &&
        ((cdb->CDB10.OperationCode == SCSIOP_WRITE) || (cdb->CDB10.OperationCode == SCSIOP_WRITE16)) &&
        (cdb->CDB10.ForceUnitAccess == 1) &&
        IsFuaSupported(ChannelExtension) ) {
        // Keep track of FUA to add it back in when the command is put in the FIS
        slotContent->StateFlags.FUA = TRUE;
    }

    if (LogCommand(ChannelExtension->AdapterExtension->LogFlags)) {
        // 1/2 Log command starting part for our debugging records
        PCOMMAND_HISTORY cmdHistory;

        ChannelExtension->CommandHistoryNextAvailableIndex++;
        ChannelExtension->CommandHistoryNextAvailableIndex %= 64;
        slotContent->CommandHistoryIndex = ChannelExtension->CommandHistoryNextAvailableIndex;

        cmdHistory = &ChannelExtension->CommandHistory[slotContent->CommandHistoryIndex];
        cmdHistory->Tag = srbExtension->QueueTag;
        StorPortCopyMemory(&cmdHistory->InitialTaskFile, &srbExtension->TaskFile, 0x10);
        StorPortCopyMemory(&cmdHistory->InitialPx, ChannelExtension->Px, 0x40);
        cmdHistory->Function = SrbGetSrbFunction(slotContent->Srb);
        cmdHistory->SrbStatus = 0;
    }

    //already get a slot, after this point, any request completion effort needs to release the slot.
    // just like what's done in: ReleaseSlottedCommand()

  //2. Program the CFIS in the CommandTable (allocated in srbExtension)
    cmdTable = (PAHCI_COMMAND_TABLE)srbExtension;
    AhciZeroMemory((PCHAR)cmdTable, sizeof(AHCI_COMMAND_TABLE));

    if ( IsAtapiCommand(srbExtension->AtaFunction) ) {
        SRBtoATAPI_CFIS(ChannelExtension, slotContent);
    } else if ( IsAtaCfisPayload(srbExtension->AtaFunction) ) {
        CfistoATA_CFIS(ChannelExtension, slotContent);
    } else if ( IsAtaCommand(srbExtension->AtaFunction) ) {
        SRBtoATA_CFIS(ChannelExtension, slotContent);
    } else {
        NT_ASSERT(FALSE);
        slotContent->Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        AhciCompleteJustSlottedRequest(ChannelExtension, Srb, AtDIRQL);
        RecordExecutionHistory(ChannelExtension, 0x10070020);   //Invalid ATA Function
        return TRUE;
    }


  //3. Build the PRD Table in CommandTable.
    if( IsDataTransferNeeded(slotContent->Srb) ) {
        prdtLength = SRBtoPRDT(ChannelExtension, slotContent);
        if (prdtLength == -1) {
            NT_ASSERT(FALSE);
            slotContent->Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            AhciCompleteJustSlottedRequest(ChannelExtension, Srb, AtDIRQL);
            RecordExecutionHistory(ChannelExtension, 0x10060020);   //Invalid SGL
            return TRUE;
        }
    }

  //4. Program the Command Header (allocated in ChannelExtension for all command slots)
    SRBtoCmdHeader(ChannelExtension, slotContent, prdtLength, FALSE);

  //4.1. Get the Command Table's physical address to verify the alignment and program  cmdHeader->CTBA
    if ((PSTORAGE_REQUEST_BLOCK)&ChannelExtension->Local.Srb == Srb) {
        cmdTablePhysicalAddress = ChannelExtension->Local.SrbExtensionPhysicalAddress;
    } else if ((PSTORAGE_REQUEST_BLOCK)&ChannelExtension->Sense.Srb == Srb) {
        cmdTablePhysicalAddress = ChannelExtension->Sense.SrbExtensionPhysicalAddress;
    } else {
        ULONG length;
        cmdTablePhysicalAddress = StorPortGetPhysicalAddress(ChannelExtension->AdapterExtension,
                                                             NULL,
                                                             srbExtension,
                                                             &length);
    }
    if ((cmdTablePhysicalAddress.LowPart % 128) == 0 ) {
        cmdHeader->CTBA.AsUlong = cmdTablePhysicalAddress.LowPart;
    } else {
        slotContent->Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        AhciCompleteJustSlottedRequest(ChannelExtension, Srb, AtDIRQL);
        RecordExecutionHistory(ChannelExtension, 0x10080020);//Invalid PhyscialAddress alignment
        return TRUE;
    }
    //If the controller supports 64 bits, write the high part too
    if (ChannelExtension->AdapterExtension->CAP.S64A)    {
        cmdHeader->CTBAU = cmdTablePhysicalAddress.HighPart;
    }

  //5. Sort it, put request in one of IO Slices.
    //5.1. Keep track of high priority outside of the Slices
    if ( IsHighPriorityCommand(srbExtension->Flags) ){
        ChannelExtension->SlotManager.HighPriorityAttribute |= ( 1 << srbExtension->QueueTag);
    }
    //5.2. Put the slot content in the correct Slice
    if ( IsNCQCommand(srbExtension) ) {
        ChannelExtension->SlotManager.NCQueueSlice |= ( 1 << srbExtension->QueueTag);
    } else if ( IsReturnResults(srbExtension->Flags) ) {
        ChannelExtension->SlotManager.SingleIoSlice |= ( 1 << srbExtension->QueueTag);
    } else if ( IsNormalCommand(slotContent->Srb) ) {
        ChannelExtension->SlotManager.NormalQueueSlice |= ( 1 << srbExtension->QueueTag);
    } else {
        ChannelExtension->SlotManager.SingleIoSlice |= ( 1 << srbExtension->QueueTag);
    }


    if (LogExecuteFullDetail(ChannelExtension->AdapterExtension->LogFlags)) {
        RecordExecutionHistory(ChannelExtension, 0x10000020);//Exit AhciHwStartIo
    }

    return TRUE;
}

PSCSI_REQUEST_BLOCK
BuildRequestSenseSrb(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  FailingSrb
    )
{
    STOR_PHYSICAL_ADDRESS   dataBufferPhysicalAddress;
    ULONG                   length;
    PCDB                    cdb;
    PSCSI_REQUEST_BLOCK     senseSrb = &ChannelExtension->Sense.Srb;
    PAHCI_SRB_EXTENSION     srbExtension = ChannelExtension->Sense.SrbExtension;
    PVOID                   srbSenseBuffer = NULL;
    UCHAR                   srbSenseBufferLength = 0;

    RequestGetSrbScsiData(FailingSrb, NULL, NULL, &srbSenseBuffer, &srbSenseBufferLength);

    //1. valdiate if senseSrb should be used.
    if ( (srbExtension == NULL) || (srbExtension->AtaFunction != 0) ||
         (srbSenseBuffer == NULL) || (srbSenseBufferLength == 0) ) {
        // Request Sense is for ATAPI commands which are single IOs. At the same time there should be only one command failed and needs this Srb.
        NT_ASSERT(srbExtension && srbExtension->AtaFunction == 0);
        return NULL;
    }

    //2. initialize Srb and SrbExtension structures.
    AhciZeroMemory((PCHAR)senseSrb, sizeof(SCSI_REQUEST_BLOCK));
    AhciZeroMemory((PCHAR)srbExtension, sizeof(AHCI_SRB_EXTENSION));

    //3. setup Srb and CDB. Note that Sense Srb uses SCSI_REQUEST_BLOCK type.
    senseSrb->Length = sizeof(SCSI_REQUEST_BLOCK);
    senseSrb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    senseSrb->PathId = (UCHAR)ChannelExtension->PortNumber;
    senseSrb->SrbFlags = SRB_FLAGS_DATA_IN;
    senseSrb->DataBuffer = srbSenseBuffer;
    senseSrb->DataTransferLength = srbSenseBufferLength;
    senseSrb->SrbExtension = (PVOID)srbExtension;
    senseSrb->OriginalRequest = (PVOID)FailingSrb;
    senseSrb->TimeOutValue = 1;
    senseSrb->CdbLength = 6;

    cdb = (PCDB)&senseSrb->Cdb;

    cdb->CDB6INQUIRY.OperationCode = SCSIOP_REQUEST_SENSE;
    cdb->CDB6INQUIRY.AllocationLength = (UCHAR)senseSrb->DataTransferLength;

    //4. fill in the srbExtension fields
    srbExtension->AtaFunction = ATA_FUNCTION_REQUEST_SENSE;
    srbExtension->Flags = ATA_FLAGS_DATA_IN;

    dataBufferPhysicalAddress = StorPortGetPhysicalAddress(ChannelExtension->AdapterExtension, NULL, srbSenseBuffer, &length);
    if (dataBufferPhysicalAddress.QuadPart == 0) {
        NT_ASSERT(FALSE);  //Shall Not Pass
        return NULL;
    }

    srbExtension->LocalSgl.NumberOfElements = 1;
    srbExtension->LocalSgl.List[0].PhysicalAddress.LowPart = dataBufferPhysicalAddress.LowPart;
    srbExtension->LocalSgl.List[0].PhysicalAddress.HighPart = dataBufferPhysicalAddress.HighPart;
    srbExtension->LocalSgl.List[0].Length = senseSrb->DataTransferLength;
    srbExtension->Sgl = &srbExtension->LocalSgl;

    return senseSrb;
}

VOID
AhciPortFailAllIos(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ UCHAR   SrbStatus,
    _In_ BOOLEAN AtDIRQL
    )
{
    PSTORAGE_REQUEST_BLOCK srb;
    UCHAR i;

    // complete all rquests still in queue
    srb = RemoveQueue(ChannelExtension, &ChannelExtension->SrbQueue, 0xDEADC0DE, 0x1F);
    while (srb != NULL) {
        srb->SrbStatus = SrbStatus;
        MarkSrbToBeCompleted(srb);
        AhciCompleteRequest(ChannelExtension, srb, AtDIRQL);
        srb = RemoveQueue(ChannelExtension, &ChannelExtension->SrbQueue, 0xDEADC0DE, 0x1F);
    }

    // complete all requests in slots
    for (i = 0; i <= ChannelExtension->AdapterExtension->CAP.NCS; i++) {
        if (ChannelExtension->Slot[i].Srb != NULL) {
            ChannelExtension->Slot[i].Srb->SrbStatus = SrbStatus;
            AhciCompleteJustSlottedRequest(ChannelExtension, ChannelExtension->Slot[i].Srb, AtDIRQL);
        }
    }

    // requests in completion queue will be completed by AhciPortSrbCompletionDpcRoutine.

    return;
}

VOID
AhciPortSrbCompletionDpcRoutine(
    _In_ PSTOR_DPC  Dpc,
    _In_ PVOID      AdapterExtension,
    _In_opt_ PVOID  SystemArgument1,
    _In_opt_ PVOID  SystemArgument2
  )
{
    PAHCI_CHANNEL_EXTENSION channelExtension = (PAHCI_CHANNEL_EXTENSION)SystemArgument1;
    STOR_LOCK_HANDLE        lockhandle = {0};
    PSTORAGE_REQUEST_BLOCK  srb = NULL;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument2);

    if (channelExtension == NULL) {
        NT_ASSERT(channelExtension != NULL);
        return;
    }

    do {
        StorPortAcquireSpinLock(AdapterExtension, InterruptLock, NULL, &lockhandle);
        srb = RemoveQueue(channelExtension, &channelExtension->CompletionQueue, 0xDEADC0DE, 0x9F);
        StorPortReleaseSpinLock(AdapterExtension, &lockhandle);

        if (srb != NULL) {
            PAHCI_SRB_EXTENSION     srbExtension = GetSrbExtension(srb);
            PSRB_COMPLETION_ROUTINE completionRoutine = srbExtension->CompletionRoutine;

            BOOLEAN completeSrb = TRUE;

            srbExtension->AtaFunction = 0; // clear this field.
            srbExtension->CompletionRoutine = NULL;

            if (completionRoutine != NULL) {

                completionRoutine(channelExtension, srb);

                // a request with STATUS_BUS_RESET should be completed after running completion routine.
                if ( (srbExtension->AtaFunction != 0) &&
                     (!SrbShouldBeCompleted(srbExtension->Flags)) &&
                     (srb->SrbStatus != SRB_STATUS_BUS_RESET) ) {
                    // new command associated needs to be processed, do not complete the request.
                    StorPortAcquireSpinLock(AdapterExtension, InterruptLock, NULL, &lockhandle);
                    AhciProcessIo(channelExtension, srb, TRUE);
                    ActivateQueue(channelExtension, TRUE);
                    StorPortReleaseSpinLock(AdapterExtension, &lockhandle);
                    // this Srb should not be completed yet
                    completeSrb = FALSE;
                } else if (SrbShouldBeCompleted(srbExtension->Flags)) {
                    // clear the flag
                    CLRMASK(srbExtension->Flags, ATA_FLAGS_COMPLETE_SRB);
                }
            } else {
                // a Srb without completion routine should be completed.
            }

            if (completeSrb) {
                // release active reference for port/device and adapter.
                if ((srbExtension->Flags & ATA_FLAGS_ACTIVE_REFERENCE) != 0) {
                    PortReleaseActiveReference(channelExtension, srb);
                }

                if (!IsMiniportInternalSrb(channelExtension, srb)) {
                    NT_ASSERT(srb->SrbStatus != SRB_STATUS_PENDING);
                    StorPortNotification(RequestComplete, AdapterExtension, srb);
                }
            }
        }

    } while (srb != NULL);

    return;
}

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4214)
#pragma warning(default:4201)
#endif

