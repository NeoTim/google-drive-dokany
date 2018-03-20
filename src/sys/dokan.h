/*
  Dokan : user-mode file system library for Windows

  Copyright (C) 2017 - 2018 Google, Inc.
  Copyright (C) 2015 - 2016 Adrien J. <liryna.stark@gmail.com> and Maxime C. <maxime@islog.com>
  Copyright (C) 2007 - 2011 Hiroki Asakawa <info@dokan-dev.net>

  http://dokan-dev.github.io

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free
Software Foundation; either version 3 of the License, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program. If not, see <http://www.gnu.org/licenses/>.
*/

/*++


--*/

#ifndef DOKAN_H_
#define DOKAN_H_

#include <ntifs.h>
#include <ntdddisk.h>
#include <ntstrsafe.h>

#include "public.h"

//
// DEFINES
//

#define DOKAN_DEBUG_DEFAULT 0

extern ULONG g_Debug;
extern LOOKASIDE_LIST_EX g_DokanCCBLookasideList;
extern LOOKASIDE_LIST_EX g_DokanFCBLookasideList;
extern LOOKASIDE_LIST_EX g_DokanEResourceLookasideList;

#ifndef DOKAN_GLOBAL_DEVICE_NAME
#define DOKAN_GLOBAL_DEVICE_NAME L"\\Device\\Dokan_" DOKAN_MAJOR_API_VERSION
#endif
#ifndef DOKAN_GLOBAL_SYMBOLIC_LINK_NAME
#define DOKAN_GLOBAL_SYMBOLIC_LINK_NAME                                        \
  L"\\DosDevices\\Global\\Dokan_" DOKAN_MAJOR_API_VERSION
#endif
#ifndef DOKAN_GLOBAL_FS_DISK_DEVICE_NAME
#define DOKAN_GLOBAL_FS_DISK_DEVICE_NAME                                       \
  L"\\Device\\DokanFs" DOKAN_MAJOR_API_VERSION
#endif
#ifndef DOKAN_GLOBAL_FS_CD_DEVICE_NAME
#define DOKAN_GLOBAL_FS_CD_DEVICE_NAME                                         \
  L"\\Device\\DokanCdFs" DOKAN_MAJOR_API_VERSION
#endif

#define DOKAN_DISK_DEVICE_NAME L"\\Device\\Volume"
#define DOKAN_SYMBOLIC_LINK_NAME L"\\DosDevices\\Global\\Volume"
#ifndef DOKAN_NET_DEVICE_NAME
#define DOKAN_NET_DEVICE_NAME                                                  \
  L"\\Device\\DokanRedirector" DOKAN_MAJOR_API_VERSION
#endif
#ifndef DOKAN_NET_SYMBOLIC_LINK_NAME
#define DOKAN_NET_SYMBOLIC_LINK_NAME                                           \
  L"\\DosDevices\\Global\\DokanRedirector" DOKAN_MAJOR_API_VERSION
#endif

#ifndef VOLUME_LABEL
#define VOLUME_LABEL L"DOKAN"
#endif
// {D6CC17C5-1734-4085-BCE7-964F1E9F5DE9}
#ifndef DOKAN_BASE_GUID
#define DOKAN_BASE_GUID                                                        \
  {                                                                            \
    0xd6cc17c5, 0x1734, 0x4085, {                                              \
      0xbc, 0xe7, 0x96, 0x4f, 0x1e, 0x9f, 0x5d, 0xe9                           \
    }                                                                          \
  }
#endif

#define TAG (ULONG)'AKOD'

#define DOKAN_MDL_ALLOCATED 0x1

#ifdef ExAllocatePool
#undef ExAllocatePool
#endif
#if _WIN32_WINNT >= _WIN32_WINNT_WIN8
#define ExAllocatePool(size) ExAllocatePoolWithTag(NonPagedPoolNx, size, TAG)
#else
#define ExAllocatePool(size) ExAllocatePoolWithTag(NonPagedPool, size, TAG)
#endif

#if _WIN32_WINNT >= _WIN32_WINNT_WIN8
#define MmGetSystemAddressForMdlNormalSafe(mdl)                                \
  MmGetSystemAddressForMdlSafe(mdl, NormalPagePriority | MdlMappingNoExecute)
#else
#define MmGetSystemAddressForMdlNormalSafe(mdl)                                \
  MmGetSystemAddressForMdlSafe(mdl, NormalPagePriority)
#endif

#define DRIVER_CONTEXT_EVENT 2
#define DRIVER_CONTEXT_IRP_ENTRY 3

#define DOKAN_IRP_PENDING_TIMEOUT (1000 * 15)               // in millisecond
#define DOKAN_IRP_PENDING_TIMEOUT_RESET_MAX (1000 * 60 * 5) // in millisecond
#define DOKAN_CHECK_INTERVAL (1000 * 5)                     // in millisecond

#define DOKAN_KEEPALIVE_TIMEOUT_DEFAULT (1000 * 15) // in millisecond

#if _WIN32_WINNT > 0x501

#define DDbgPrint(...)                                                         \
  if (g_Debug) {                                                               \
    KdPrintEx(                                                                 \
        (DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "[DokanFS] " __VA_ARGS__));  \
  }
#else
#define DDbgPrint(...)                                                         \
  if (g_Debug) {                                                               \
    DbgPrint("[DokanFS] " __VA_ARGS__);                                        \
  }
#endif

#if _WIN32_WINNT < 0x0501
extern PFN_FSRTLTEARDOWNPERSTREAMCONTEXTS DokanFsRtlTeardownPerStreamContexts;
#endif

extern UNICODE_STRING FcbFileNameNull;
#define DokanPrintFileName(FileObject)                                         \
  DDbgPrint("  FileName: %wZ FCB.FileName: %wZ\n", &FileObject->FileName,      \
            FileObject->FsContext2                                             \
                ? (((PDokanCCB)FileObject->FsContext2)->Fcb                    \
                       ? &((PDokanCCB)FileObject->FsContext2)->Fcb->FileName   \
                       : &FcbFileNameNull)                                     \
                : &FcbFileNameNull)

extern NPAGED_LOOKASIDE_LIST DokanIrpEntryLookasideList;
#define DokanAllocateIrpEntry()                                                \
  ExAllocateFromNPagedLookasideList(&DokanIrpEntryLookasideList)
#define DokanFreeIrpEntry(IrpEntry)                                            \
  ExFreeToNPagedLookasideList(&DokanIrpEntryLookasideList, IrpEntry)

//
//  Undocumented definition of ExtensionFlags
//

#ifndef DOE_UNLOAD_PENDING
#define DOE_UNLOAD_PENDING 0x00000001
#define DOE_DELETE_PENDING 0x00000002
#define DOE_REMOVE_PENDING 0x00000004
#define DOE_REMOVE_PROCESSED 0x00000008
#define DOE_START_PENDING 0x00000010
#endif

//
// FSD_IDENTIFIER_TYPE
//
// Identifiers used to mark the structures
//
// Note: the DCB identifier has been changed to prevent DriveFS 210 and older
// drivers from mounting disks created by newer versions.
//
typedef enum _FSD_IDENTIFIER_TYPE {
  DGL = ':DGL', // Dokan Global
  DCB = ':GCB', // Disk Control Block
  VCB = ':VCB', // Volume Control Block
  FCB = ':FCB', // File Control Block
  CCB = ':CCB', // Context Control Block
  FREED_FCB = ':FFC', // FCB that has been freed
} FSD_IDENTIFIER_TYPE;

//
// FSD_IDENTIFIER
//
// Header put in the beginning of every structure
//
typedef struct _FSD_IDENTIFIER {
  FSD_IDENTIFIER_TYPE Type;
  ULONG Size;
} FSD_IDENTIFIER, *PFSD_IDENTIFIER;

#define GetIdentifierType(Obj) (((PFSD_IDENTIFIER)Obj)->Type)

//
// DATA
//

typedef struct _IRP_LIST {
  LIST_ENTRY ListHead;
  KEVENT NotEmpty;
  KSPIN_LOCK ListLock;
} IRP_LIST, *PIRP_LIST;

typedef struct _DOKAN_CONTROL {
  ULONG Type;            // File System Type
  WCHAR MountPoint[260]; // Mount Point
  WCHAR UNCName[64];
  WCHAR DeviceName[64];        // Disk Device Name
  PDEVICE_OBJECT DeviceObject; // Volume Device Object
} DOKAN_CONTROL, *PDOKAN_CONTROL;

typedef struct _MOUNT_ENTRY {
  LIST_ENTRY ListEntry;
  DOKAN_CONTROL MountControl;
} MOUNT_ENTRY, *PMOUNT_ENTRY;

typedef struct _DOKAN_GLOBAL {
  FSD_IDENTIFIER Identifier;
  ERESOURCE Resource;
  PDEVICE_OBJECT DeviceObject;
  PDEVICE_OBJECT FsDiskDeviceObject;
  PDEVICE_OBJECT FsCdDeviceObject;
  ULONG MountId;
  // the list of waiting IRP for mount service
  IRP_LIST PendingService;
  IRP_LIST NotifyService;

  PKTHREAD DeviceDeleteThread;

  LIST_ENTRY MountPointList;
  LIST_ENTRY DeviceDeleteList;
  KEVENT KillDeleteDeviceEvent;

  GUID DriverVersion;

} DOKAN_GLOBAL, *PDOKAN_GLOBAL;

typedef struct _DOKAN_LOGGER {

  PDRIVER_OBJECT DriverObject;
  UCHAR MajorFunctionCode;

} DOKAN_LOGGER, *PDOKAN_LOGGER;


#define DOKAN_INIT_LOGGER(logger, driverObject, majorFunctionCode)             \
  DOKAN_LOGGER logger;                                                         \
  logger.DriverObject = driverObject;                                          \
  logger.MajorFunctionCode = majorFunctionCode;

// Logs an error to the Windows event log, even in production, with the given
// status, and returns the status passed in.
NTSTATUS DokanLogError(__in PDOKAN_LOGGER Logger,
                       __in NTSTATUS Status,
                       __in LPCTSTR Format,
                       ...);

// Logs an informational message to the Windows event log, even in production.
VOID DokanLogInfo(__in PDOKAN_LOGGER Logger, __in LPCTSTR Format, ...);

// A compact stack trace that can be easily logged.
typedef struct _DokanBackTrace {
  // The full address of a point-of-reference instruction near where the logging
  // occurs. One should be able to find this instruction in the disassembly of
  // the driver by seeing the log message content aside from this value. This
  // value then tells you the absolute address of that instruction at runtime.
  ULONG64 Address;

  // Three return addresses truncated to their lowest 20 bits. The lowest 20
  // bits of this value is the most distant return address, the next 20 bits are
  // the next frame up, etc. To find each of the 3 instructions referenced here,
  // one replaces the lowest 20 bits of Ip.
  ULONG64 ReturnAddresses;
} DokanBackTrace, *PDokanBackTrace;

// Captures a trace where Address is the full address of the call site
// instruction after the DokanCaptureBackTrace call, and ReturnAddresses
// indicates the 3 return addresses below that.
VOID DokanCaptureBackTrace(__out PDokanBackTrace Trace);

// make sure Identifier is the top of struct
typedef struct _DokanDiskControlBlock {

  FSD_IDENTIFIER Identifier;

  ERESOURCE Resource;

  PDOKAN_GLOBAL Global;
  PDRIVER_OBJECT DriverObject;
  PDEVICE_OBJECT DeviceObject;

  PVOID Vcb;

  // the list of waiting Event
  IRP_LIST PendingIrp;
  IRP_LIST PendingEvent;
  IRP_LIST NotifyEvent;

  PUNICODE_STRING DiskDeviceName;
  PUNICODE_STRING SymbolicLinkName;
  PUNICODE_STRING MountPoint;
  PUNICODE_STRING UNCName;
  LPWSTR VolumeLabel;

  DEVICE_TYPE DeviceType;
  DEVICE_TYPE VolumeDeviceType;
  ULONG DeviceCharacteristics;
  HANDLE MupHandle;
  UNICODE_STRING MountedDeviceInterfaceName;
  UNICODE_STRING DiskDeviceInterfaceName;

  // When timeout is occuerd, KillEvent is triggered.
  KEVENT KillEvent;

  KEVENT ReleaseEvent;

  // the thread to deal with timeout
  PKTHREAD TimeoutThread;
  PKTHREAD EventNotificationThread;

  // When UseAltStream is 1, use Alternate stream
  USHORT UseAltStream;
  USHORT UseMountManager;
  USHORT MountGlobally;
  USHORT FileLockInUserMode;

  // to make a unique id for pending IRP
  ULONG SerialNumber;

  ULONG MountId;
  ULONG Flags;

  CACHE_MANAGER_CALLBACKS CacheManagerCallbacks;
  CACHE_MANAGER_CALLBACKS CacheManagerNoOpCallbacks;

  ULONG IrpTimeout;

  IO_REMOVE_LOCK RemoveLock;
  BOOLEAN LockDebugEnabled;

} DokanDCB, *PDokanDCB;

#define IS_DEVICE_READ_ONLY(DeviceObject)                                      \
  (DeviceObject->Characteristics & FILE_READ_ONLY_DEVICE)

// Information that can be attached to an object whose use is governed by an
// ERESOURCE, to help diagnose locking problems.
typedef struct _DokanResourceDebugInfo {
  // A description of the call site in the code where the resource was
  // exclusively acquired. If it is not exclusively acquired currently, this is
  // NULL.
  const char* ExclusiveLockSite;

  // The thread in which the lock was exclusively acquired. If it is not
  // exclusively acquired currently, this is NULL.
  PKTHREAD ExclusiveOwnerThread;

  // The number of active acquisitions held by ExclusiveOwnerThread. A value
  // greater than 1 means it has been recursively acquired.
  ULONG ExclusiveLockCount;
} DokanResourceDebugInfo, *PDokanResourceDebugInfo;

typedef struct _DokanVolumeControlBlock {

  FSD_IDENTIFIER Identifier;

  FSRTL_ADVANCED_FCB_HEADER VolumeFileHeader;
  SECTION_OBJECT_POINTERS SectionObjectPointers;
  FAST_MUTEX AdvancedFCBHeaderMutex;

  ERESOURCE Resource;
  PDEVICE_OBJECT DeviceObject;
  PDokanDCB Dcb;
  LIST_ENTRY NextFCB;

  // NotifySync is used by notify directory change
  PNOTIFY_SYNC NotifySync;
  LIST_ENTRY DirNotifyList;

  LONG FcbAllocated;
  LONG FcbFreed;
  LONG CcbAllocated;
  LONG CcbFreed;
  ULONG Flags;
  BOOLEAN HasEventWait;
  DokanResourceDebugInfo ResourceDebugInfo;
  DOKAN_LOGGER ResourceLogger;

  // A mask that all Fcbs created for this volume match. We update this when we
  // deal each one out. In practice, they all tend to have the same first 40
  // bits on x64.
  ULONG_PTR ValidFcbMask;

  // Whether keep-alive has been activated on this volume.
  BOOLEAN IsKeepaliveActive;

} DokanVCB, *PDokanVCB;

// Flags for volume
#define VCB_MOUNTED 0x00000004
#define VCB_DISMOUNT_PENDING 0x00000008

// Flags for device
#define DCB_DELETE_PENDING 0x00000001

typedef struct _DokanFileControlBlock {
  // Locking: Identifier is read-only, no locks needed.
  FSD_IDENTIFIER Identifier;

  // Locking: FIXME
  FSRTL_ADVANCED_FCB_HEADER AdvancedFCBHeader;
  // Locking: FIXME
  SECTION_OBJECT_POINTERS SectionObjectPointers;

  // Locking: FIXME
  FAST_MUTEX AdvancedFCBHeaderMutex;

  // Locking: Lock for paging io.
  ERESOURCE PagingIoResource;

  // Locking: Vcb pointer is read-only, no locks needed.
  PDokanVCB Vcb;
  // Locking: DokanFCBLock{RO,RW} and usually vcb lock
  LIST_ENTRY NextFCB;
  // Locking: DokanFCBLock{RO,RW}
  LIST_ENTRY NextCCB;

  // Locking: Atomics - not behind an accessor.
  LONG FileCount;

  // Locking: Use atomic flag operations - DokanFCBFlags*
  ULONG Flags;
  // Locking: Functions are ok to call concurrently.
  SHARE_ACCESS ShareAccess;

  // Locking: DokanFCBLock{RO,RW} - e.g. renames change this field.
  // Modifications must lock the VCB followed by the FCB. Reads may
  // lock either one.
  UNICODE_STRING FileName;

  // Locking: FsRtl routines should be enough after initialization.
  FILE_LOCK FileLock;

#if (NTDDI_VERSION < NTDDI_WIN8)
  //
  //  The following field is used by the oplock module
  //  to maintain current oplock information.
  //
  // Locking: DokanFCBLock{RO,RW}
  OPLOCK Oplock;
#endif

  // uint32 ReferenceCount;
  // uint32 OpenHandleCount;
  DokanResourceDebugInfo ResourceDebugInfo;
  DokanResourceDebugInfo PagingIoResourceDebugInfo;

  // A keep-alive FCB is a special FCB whose last cleanup triggers automatic
  // unmounting. This is meant to unmount the file system when the owning
  // process abruptly terminates, replacing dokan's original ping/timeout-based
  // mechanism. The owning process must open the special keepalive file name,
  // then issue a FSCTL_ACTIVATE_KEEPALIVE DeviceIoControl to that file handle
  // (which sets the IsKeepaliveActive flag), and hold the handle open until
  // after normal unmounting. The DeviceIoControl step ensures that if a filter
  // turns the CreateFile into a CreateFile + CloseHandle + CreateFile sequence,
  // the hidden CloseHandle call doesn't trigger unmounting.

  // TRUE if this FCB points to the keep-alive file name. This prevents the FCB
  // from dispatching normally to user mode, but the IsKeepaliveActive flag must
  // also be true in order for auto-unmounting to happen.
  BOOLEAN IsKeepalive;

} DokanFCB, *PDokanFCB;

#define DokanResourceLockRO(resource)                            \
    {                                                            \
      KeEnterCriticalRegion();                                   \
      ExAcquireResourceSharedLite(resource, TRUE);               \
    }

#define DokanResourceLockRW(resource)                            \
    ExEnterCriticalRegionAndAcquireResourceExclusive(resource)

#define DokanResourceUnlock(resource)                            \
    ExReleaseResourceAndLeaveCriticalRegion(resource)

VOID DokanResourceLockWithDebugInfo(__in BOOLEAN Writable,
                                    __in PERESOURCE Resource,
                                    __in PDokanResourceDebugInfo DebugInfo,
                                    __in PDOKAN_LOGGER Logger,
                                    __in const char* Site,
                                    __in const UNICODE_STRING* ResourceName,
                                    __in const void* ResourcePointer);

VOID DokanResourceUnlockWithDebugInfo(__in PERESOURCE Resource,
                                      __in PDokanResourceDebugInfo DebugInfo);

#define DokanStringizeInternal(x) #x
#define DokanStringize(x) DokanStringizeInternal(x)
#define DokanCallSiteID __FUNCTION__ ":" DokanStringize(__LINE__)

#define DokanFCBLockDebugEnabled(fcb) \
    ((fcb)->Vcb != NULL && (fcb)->Vcb->Dcb->LockDebugEnabled)

#define DokanFCBLockRW(fcb)                                      \
    if (DokanFCBLockDebugEnabled(fcb)) {                         \
      DokanResourceLockWithDebugInfo(                            \
          TRUE,                                                  \
          (fcb)->AdvancedFCBHeader.Resource,                     \
          &(fcb)->ResourceDebugInfo,                             \
          &(fcb)->Vcb->ResourceLogger,                           \
          DokanCallSiteID,                                       \
          &(fcb)->FileName,                                      \
          (fcb));                                                \
    } else {                                                     \
      DokanResourceLockRW((fcb)->AdvancedFCBHeader.Resource);    \
    }

#define DokanFCBLockRO(fcb)                                      \
    if (DokanFCBLockDebugEnabled(fcb)) {                         \
      DokanResourceLockWithDebugInfo(                            \
          FALSE,                                                 \
          (fcb)->AdvancedFCBHeader.Resource,                     \
          &(fcb)->ResourceDebugInfo,                             \
          &(fcb)->Vcb->ResourceLogger,                           \
          DokanCallSiteID,                                       \
          &(fcb)->FileName,                                      \
          (fcb));                                                \
    } else {                                                     \
      DokanResourceLockRO((fcb)->AdvancedFCBHeader.Resource);    \
    }

#define DokanFCBUnlock(fcb)                                      \
    if (DokanFCBLockDebugEnabled(fcb)) {                         \
      DokanResourceUnlockWithDebugInfo(                          \
          (fcb)->AdvancedFCBHeader.Resource,                     \
          &(fcb)->ResourceDebugInfo);                            \
    } else {                                                     \
      DokanResourceUnlock((fcb)->AdvancedFCBHeader.Resource);    \
    }

#define DokanPagingIoLockRW(fcb)                                 \
    if (DokanFCBLockDebugEnabled(fcb)) {                         \
      DokanResourceLockWithDebugInfo(                            \
          TRUE,                                                  \
          &(fcb)->PagingIoResource,                              \
          &(fcb)->PagingIoResourceDebugInfo,                     \
          &(fcb)->Vcb->ResourceLogger,                           \
          DokanCallSiteID,                                       \
          &(fcb)->FileName,                                      \
          (fcb));                                                \
    } else {                                                     \
      DokanResourceLockRW(&(fcb)->PagingIoResource);             \
    }

#define DokanPagingIoLockRO(fcb)                                 \
    if (DokanFCBLockDebugEnabled(fcb)) {                         \
      DokanResourceLockWithDebugInfo(                            \
          FALSE,                                                 \
          &(fcb)->PagingIoResource,                              \
          &(fcb)->PagingIoResourceDebugInfo,                     \
          &(fcb)->Vcb->ResourceLogger,                           \
          DokanCallSiteID,                                       \
          &(fcb)->FileName,                                      \
          (fcb));                                                \
    } else {                                                     \
      DokanResourceLockRO(&(fcb)->PagingIoResource);             \
    }

#define DokanPagingIoUnlock(fcb)                                 \
    if (DokanFCBLockDebugEnabled(fcb)) {                         \
      DokanResourceUnlockWithDebugInfo(                          \
          &(fcb)->PagingIoResource,                              \
          &(fcb)->PagingIoResourceDebugInfo);                    \
    } else {                                                     \
      DokanResourceUnlock(&(fcb)->PagingIoResource);             \
    }

#define DokanVCBLockRW(vcb)                                      \
    if ((vcb)->Dcb->LockDebugEnabled) {                          \
      DokanResourceLockWithDebugInfo(                            \
          TRUE,                                                  \
          &(vcb)->Resource,                                      \
          &(vcb)->ResourceDebugInfo,                             \
          &(vcb)->ResourceLogger,                                \
          DokanCallSiteID,                                       \
          (vcb)->Dcb->MountPoint,                                \
          (vcb));                                                \
    } else {                                                     \
      DokanResourceLockRW(&(vcb)->Resource);                     \
    }

#define DokanVCBLockRO(vcb)                                      \
    if ((vcb)->Dcb->LockDebugEnabled) {                          \
      DokanResourceLockWithDebugInfo(                            \
          FALSE,                                                 \
          &(vcb)->Resource,                                      \
          &(vcb)->ResourceDebugInfo,                             \
          &(vcb)->ResourceLogger,                                \
          DokanCallSiteID,                                       \
          (vcb)->Dcb->MountPoint,                                \
          (vcb));                                                \
    } else {                                                     \
      DokanResourceLockRO(&(vcb)->Resource);                     \
    }

#define DokanVCBUnlock(vcb)                                      \
    if ((vcb)->Dcb->LockDebugEnabled) {                          \
      DokanResourceUnlockWithDebugInfo(                          \
          &(vcb)->Resource,                                      \
          &(vcb)->ResourceDebugInfo);                            \
    } else {                                                     \
      DokanResourceUnlock(&(vcb)->Resource);                     \
    }

typedef struct _DokanContextControlBlock {
  // Locking: Read only field. No locking needed.
  FSD_IDENTIFIER Identifier;
  // Locking: Main lock for CCBs.
  ERESOURCE Resource;
  // Locking: Read only field. No locking needed.
  PDokanFCB Fcb;
  // Locking: Modified with the *FCB* lock held.
  LIST_ENTRY NextCCB;

  ULONG64 Context;
  ULONG64 UserContext;

  PWCHAR SearchPattern;
  ULONG SearchPatternLength;

  // Locking: Use atomic flag operations - DokanCCBFlags*
  ULONG Flags;

  // Locking: Read only field. No locking needed.
  ULONG MountId;
} DokanCCB, *PDokanCCB;

//
//  The following macro is used to retrieve the oplock structure within
//  the Fcb. This structure was moved to the advanced Fcb header
//  in Win8.
//
#if (NTDDI_VERSION >= NTDDI_WIN8)
#define DokanGetFcbOplock(F) &(F)->AdvancedFCBHeader.Oplock
#else
#define DokanGetFcbOplock(F) &(F)->Oplock
#endif

// IRP list which has pending status
// this structure is also used to store event notification IRP
typedef struct _IRP_ENTRY {
  LIST_ENTRY ListEntry;
  ULONG SerialNumber;
  PIRP Irp;
  PIO_STACK_LOCATION IrpSp;
  PFILE_OBJECT FileObject;
  BOOLEAN CancelRoutineFreeMemory;
  ULONG Flags;
  LARGE_INTEGER TickCount;
  PIRP_LIST IrpList;
} IRP_ENTRY, *PIRP_ENTRY;

typedef struct _DEVICE_ENTRY {
  LIST_ENTRY ListEntry;
  PDEVICE_OBJECT DiskDeviceObject;
  PDEVICE_OBJECT VolumeDeviceObject;
  ULONG Counter;
} DEVICE_ENTRY, *PDEVICE_ENTRY;

typedef struct _DRIVER_EVENT_CONTEXT {
  LIST_ENTRY ListEntry;
  PKEVENT Completed;
  EVENT_CONTEXT EventContext;
} DRIVER_EVENT_CONTEXT, *PDRIVER_EVENT_CONTEXT;

DRIVER_INITIALIZE DriverEntry;

__drv_dispatchType(IRP_MJ_CREATE) __drv_dispatchType(IRP_MJ_CLOSE)
    __drv_dispatchType(IRP_MJ_READ) __drv_dispatchType(IRP_MJ_WRITE)
        __drv_dispatchType(IRP_MJ_FLUSH_BUFFERS) __drv_dispatchType(
            IRP_MJ_CLEANUP) __drv_dispatchType(IRP_MJ_DEVICE_CONTROL)
            __drv_dispatchType(IRP_MJ_FILE_SYSTEM_CONTROL) __drv_dispatchType(
                IRP_MJ_DIRECTORY_CONTROL)
                __drv_dispatchType(IRP_MJ_QUERY_INFORMATION) __drv_dispatchType(
                    IRP_MJ_SET_INFORMATION)
                    __drv_dispatchType(IRP_MJ_QUERY_VOLUME_INFORMATION)
                        __drv_dispatchType(IRP_MJ_SET_VOLUME_INFORMATION)
                            __drv_dispatchType(
                                IRP_MJ_SHUTDOWN) __drv_dispatchType(IRP_MJ_PNP)
                                __drv_dispatchType(IRP_MJ_LOCK_CONTROL)
                                    __drv_dispatchType(IRP_MJ_QUERY_SECURITY)
                                        __drv_dispatchType(
                                            IRP_MJ_SET_SECURITY) NTSTATUS
    DokanBuildRequest(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
DokanDispatchClose(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
DokanDispatchCreate(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
DokanDispatchRead(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
DokanDispatchWrite(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
DokanDispatchFlush(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
DokanDispatchQueryInformation(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
DokanDispatchSetInformation(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
DokanDispatchQueryVolumeInformation(__in PDEVICE_OBJECT DeviceObject,
                                    __in PIRP Irp);

NTSTATUS
DokanDispatchSetVolumeInformation(__in PDEVICE_OBJECT DeviceObject,
                                  __in PIRP Irp);

NTSTATUS
DokanDispatchDirectoryControl(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
DokanDispatchFileSystemControl(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
DokanDispatchDeviceControl(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
DokanDispatchLock(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
DokanDispatchCleanup(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
DokanDispatchShutdown(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
DokanDispatchQuerySecurity(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
DokanDispatchSetSecurity(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
DokanDispatchPnp(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
QueryDeviceRelations(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

DRIVER_UNLOAD DokanUnload;

DRIVER_CANCEL DokanEventCancelRoutine;

DRIVER_CANCEL DokanIrpCancelRoutine;

VOID DokanOplockComplete(IN PVOID Context, IN PIRP Irp);

VOID DokanPrePostIrp(IN PVOID Context, IN PIRP Irp);

DRIVER_DISPATCH DokanRegisterPendingIrpForEvent;

DRIVER_DISPATCH DokanRegisterPendingIrpForService;

DRIVER_DISPATCH DokanCompleteIrp;

DRIVER_DISPATCH DokanResetPendingIrpTimeout;

DRIVER_DISPATCH DokanGetAccessToken;

NTSTATUS
DokanCheckShareAccess(_In_ PFILE_OBJECT FileObject, _In_ PDokanFCB FcbOrDcb,
                      _In_ ACCESS_MASK DesiredAccess, _In_ ULONG ShareAccess);

NTSTATUS
DokanGetMountPointList(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp,
                       __in PDOKAN_GLOBAL dokanGlobal);

NTSTATUS
DokanDispatchRequest(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
DokanEventRelease(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
DokanGlobalEventRelease(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
DokanExceptionFilter(__in PIRP Irp, __in PEXCEPTION_POINTERS ExceptionPointer);

NTSTATUS
DokanExceptionHandler(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp,
                      __in NTSTATUS ExceptionCode);

DRIVER_DISPATCH DokanEventStart;

DRIVER_DISPATCH DokanEventWrite;

PEVENT_CONTEXT
AllocateEventContextRaw(__in ULONG EventContextLength);

PEVENT_CONTEXT
AllocateEventContext(__in PDokanDCB Dcb, __in PIRP Irp,
                     __in ULONG EventContextLength, __in_opt PDokanCCB Ccb);

VOID DokanFreeEventContext(__in PEVENT_CONTEXT EventContext);

NTSTATUS
DokanRegisterPendingIrp(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp,
                        __in PEVENT_CONTEXT EventContext, __in ULONG Flags);

VOID DokanEventNotification(__in PIRP_LIST NotifyEvent,
                            __in PEVENT_CONTEXT EventContext);

VOID DokanCompleteDirectoryControl(__in PIRP_ENTRY IrpEntry,
                                   __in PEVENT_INFORMATION EventInfo);

VOID DokanCompleteRead(__in PIRP_ENTRY IrpEntry,
                       __in PEVENT_INFORMATION EventInfo);

VOID DokanCompleteWrite(__in PIRP_ENTRY IrpEntry,
                        __in PEVENT_INFORMATION EventInfo);

VOID DokanCompleteQueryInformation(__in PIRP_ENTRY IrpEntry,
                                   __in PEVENT_INFORMATION EventInfo);

VOID DokanCompleteSetInformation(__in PIRP_ENTRY IrpEntry,
                                 __in PEVENT_INFORMATION EventInfo);

VOID DokanCompleteCreate(__in PIRP_ENTRY IrpEntry,
                         __in PEVENT_INFORMATION EventInfo);

VOID DokanCompleteCleanup(__in PIRP_ENTRY IrpEntry,
                          __in PEVENT_INFORMATION EventInfo);

VOID DokanCompleteLock(__in PIRP_ENTRY IrpEntry,
                       __in PEVENT_INFORMATION EventInfo);

VOID DokanCompleteQueryVolumeInformation(__in PIRP_ENTRY IrpEntry,
                                         __in PEVENT_INFORMATION EventInfo,
                                         __in PDEVICE_OBJECT DeviceObject);

VOID DokanCompleteFlush(__in PIRP_ENTRY IrpEntry,
                        __in PEVENT_INFORMATION EventInfo);

VOID DokanCompleteQuerySecurity(__in PIRP_ENTRY IrpEntry,
                                __in PEVENT_INFORMATION EventInfo);

VOID DokanCompleteSetSecurity(__in PIRP_ENTRY IrpEntry,
                              __in PEVENT_INFORMATION EventInfo);

VOID DokanNoOpRelease(__in PVOID Fcb);

BOOLEAN
DokanNoOpAcquire(__in PVOID Fcb, __in BOOLEAN Wait);

NTSTATUS
DokanCreateGlobalDiskDevice(__in PDRIVER_OBJECT DriverObject,
                            __out PDOKAN_GLOBAL *DokanGlobal);

NTSTATUS
DokanCreateDiskDevice(__in PDRIVER_OBJECT DriverObject, __in ULONG MountId,
                      __in PWCHAR MountPoint, __in PWCHAR UNCName,
                      __in PSECURITY_DESCRIPTOR VolumeSecurityDescriptor,
                      __in PWCHAR BaseGuid, __in PDOKAN_GLOBAL DokanGlobal,
                      __in DEVICE_TYPE DeviceType,
                      __in ULONG DeviceCharacteristics,
                      __in BOOLEAN MountGlobally, __in BOOLEAN UseMountManager,
                      __out PDokanDCB *Dcb);

VOID DokanInitVpb(__in PVPB Vpb, __in PDEVICE_OBJECT VolumeDevice);
VOID DokanDeleteDeviceObject(__in PDokanDCB Dcb);
NTSTATUS IsMountPointDriveLetter(__in PUNICODE_STRING mountPoint);
VOID DokanDeleteMountPoint(__in PDokanDCB Dcb);
VOID DokanPrintNTStatus(NTSTATUS Status);

NTSTATUS DokanOplockRequest(__in PIRP *pIrp);
NTSTATUS DokanCommonLockControl(__in PIRP Irp);

NTSTATUS DokanRegisterUncProviderSystem(PDokanDCB dcb);
VOID DokanCompleteIrpRequest(__in PIRP Irp, __in NTSTATUS Status,
                             __in ULONG_PTR Info);

VOID DokanNotifyReportChange0(__in PDokanFCB Fcb, __in PUNICODE_STRING FileName,
                              __in ULONG FilterMatch, __in ULONG Action);

VOID DokanNotifyReportChange(__in PDokanFCB Fcb, __in ULONG FilterMatch,
                             __in ULONG Action);

PDokanFCB DokanAllocateFCB(__in PDokanVCB Vcb, __in PWCHAR FileName,
                           __in ULONG FileNameLength);

NTSTATUS
DokanFreeFCB(__in PDokanVCB Vcb, __in PDokanFCB Fcb);

PDokanCCB DokanAllocateCCB(__in PDokanDCB Dcb, __in PDokanFCB Fcb);

NTSTATUS
DokanFreeCCB(__in PDokanCCB Ccb);

NTSTATUS
DokanStartCheckThread(__in PDokanDCB Dcb);

VOID DokanStopCheckThread(__in PDokanDCB Dcb);

BOOLEAN
DokanCheckCCB(__in PDokanDCB Dcb, __in_opt PDokanCCB Ccb);

VOID DokanInitIrpList(__in PIRP_LIST IrpList);

NTSTATUS
DokanStartEventNotificationThread(__in PDokanDCB Dcb);

VOID DokanStopEventNotificationThread(__in PDokanDCB Dcb);

VOID DokanUpdateTimeout(__out PLARGE_INTEGER KickCount, __in ULONG Timeout);

VOID DokanUnmount(__in PDokanDCB Dcb);

BOOLEAN IsUnmountPending(__in PDEVICE_OBJECT DeviceObject);

BOOLEAN IsMounted(__in PDEVICE_OBJECT DeviceObject);

BOOLEAN IsDeletePending(__in PDEVICE_OBJECT DeviceObject);

BOOLEAN IsUnmountPendingVcb(__in PDokanVCB vcb);

PMOUNT_ENTRY
FindMountEntry(__in PDOKAN_GLOBAL dokanGlobal, __in PDOKAN_CONTROL DokanControl,
               __in BOOLEAN lockGlobal);

VOID PrintIdType(__in VOID *Id);

NTSTATUS
DokanAllocateMdl(__in PIRP Irp, __in ULONG Length);

VOID DokanFreeMdl(__in PIRP Irp);

PUNICODE_STRING
DokanAllocateUnicodeString(__in PCWSTR String);

ULONG
PointerAlignSize(ULONG sizeInBytes);

VOID DokanCreateMountPoint(__in PDokanDCB Dcb);
NTSTATUS DokanSendVolumeArrivalNotification(PUNICODE_STRING DeviceName);

VOID FlushFcb(__in PDokanFCB fcb, __in_opt PFILE_OBJECT fileObject);
BOOLEAN StartsWith(__in PUNICODE_STRING str, __in PUNICODE_STRING prefix);

static UNICODE_STRING sddl = RTL_CONSTANT_STRING(
    L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)(A;;GRGWGX;;;WD)(A;;GRGX;;;RC)");

#define SetLongFlag(_F, _SF) DokanSetFlag(&(_F), (ULONG)(_SF))
#define ClearLongFlag(_F, _SF) DokanClearFlag(&(_F), (ULONG)(_SF))

__inline VOID DokanSetFlag(PULONG Flags, ULONG FlagBit) {
  ULONG _ret = InterlockedOr((PLONG)Flags, FlagBit);
  UNREFERENCED_PARAMETER(_ret);
  ASSERT(*Flags == (_ret | FlagBit));
}

__inline VOID DokanClearFlag(PULONG Flags, ULONG FlagBit) {
  ULONG _ret = InterlockedAnd((PLONG)Flags, ~FlagBit);
  UNREFERENCED_PARAMETER(_ret);
  ASSERT(*Flags == (_ret & (~FlagBit)));
}

#define IsFlagOn(a, b) ((BOOLEAN)(FlagOn(a, b) == b))

#define DokanFCBFlagsGet(fcb) ((fcb)->Flags)
#define DokanFCBFlagsIsSet(fcb, bit) (((fcb)->Flags) & (bit))
#define DokanFCBFlagsSetBit(fcb, bit) SetLongFlag((fcb)->Flags, (bit))
#define DokanFCBFlagsClearBit(fcb, bit) ClearLongFlag((fcb)->Flags, (bit))

#define DokanCCBFlagsGet DokanFCBFlagsGet
#define DokanCCBFlagsIsSet DokanFCBFlagsIsSet
#define DokanCCBFlagsSetBit DokanFCBFlagsSetBit
#define DokanCCBFlagsClearBit DokanFCBFlagsClearBit

ULONG DokanSearchWcharinUnicodeStringWithUlong(__in PUNICODE_STRING inputPUnicodeString, __in WCHAR targetWchar,
	__in ULONG offsetPosition, __in int isIgnoreTargetWchar);

#endif // DOKAN_H_
