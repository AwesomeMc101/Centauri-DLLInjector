#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include <winternl.h>
#include <vector>
extern "C" {
    NTSTATUS NTAPI NtAllocateVirtualMemory(
        HANDLE    ProcessHandle,
        PVOID* BaseAddress,
        ULONG_PTR ZeroBits,
        PSIZE_T   RegionSize,
        ULONG     AllocationType,
        ULONG     Protect
    );

	NTSTATUS NTAPI NtCreateThreadEx(
		PHANDLE hThread,
		ACCESS_MASK DesiredAccess,
		PVOID ObjectAttributes,
		HANDLE ProcessHandle,
		PVOID lpStartAddress,
		PVOID lpParameter,
		ULONG Flags,
		SIZE_T StackZeroBits,
		SIZE_T SizeOfStackCommit,
		SIZE_T SizeOfStackReserve,
		PVOID lpBytesBuffer
	);

	__kernel_entry NTSYSCALLAPI NTSTATUS NtFreeVirtualMemory(
		HANDLE  ProcessHandle,
		PVOID* BaseAddress,
		PSIZE_T RegionSize,
		ULONG   FreeType
	);
}


//stuff to return to tell user what they probs did wrong
typedef enum {
	SUCCESS = 0x1,
	FAIL_NOPROC = 0x2,
	FAIL_NOMEM = 0x3,
	FAIL_INJECT = 0x4,
	FAIL_THREAD = 0x5,
	FAIL_LOADLIBADDR = 0x6,
	FAIL_PREFADDRESS = 0x7
} InjectResults;

InjectResults baseInjector(char* p, DWORD pid, bool suspend, bool findInternalAddr, bool);

[[deprecated]]
InjectResults noExecInjector(char* p, DWORD pid, bool suspend, bool hijack, bool findInternalAddr);
