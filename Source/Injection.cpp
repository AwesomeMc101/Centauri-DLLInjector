#define _CRT_SECURE_NO_WARNINGS
#include "Injection.hpp"


[[deprecated]]
//function was gonna be used for thread hijacker
HANDLE findThread(int pid) {
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	THREADENTRY32 pE;
	pE.dwSize = sizeof(pE);
	bool pastFirst = 0;
	if (Thread32First(hSnap, &pE)) {
		do {
			if (pE.th32OwnerProcessID == pid) {
				if (pastFirst) {
					HANDLE hT = OpenThread(THREAD_ALL_ACCESS, 0, pE.th32ThreadID);
					if (hT != INVALID_HANDLE_VALUE) {
						CloseHandle(hSnap);
						return hT;
					}
				}
				pastFirst = 1;
			}
		} while (Thread32Next(hSnap, &pE));

		if (pastFirst) { //Only found 1 thread. Hijack it ig? lol.
			Thread32First(hSnap, &pE);
			CloseHandle(hSnap);
			return OpenThread(THREAD_ALL_ACCESS, 0, pE.th32ThreadID);
		}
	}
	CloseHandle(hSnap);
	return INVALID_HANDLE_VALUE;
}

//function to suspend all threads in process. returns handles of every suspended thread
std::vector<HANDLE> suspendProcess(HANDLE hProc) {
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	std::vector<HANDLE> suspendedThreads;

	if (hSnap == INVALID_HANDLE_VALUE) {
		return suspendedThreads;
	}

	int t_pid = GetProcessId(hProc);
	if (t_pid == 0) {
		return suspendedThreads;
	}

	THREADENTRY32 tE = { sizeof(THREADENTRY32) };
	if (Thread32First(hSnap, &tE)) {
		do {
			if (tE.th32OwnerProcessID == t_pid) {
				//thread owned by proc
				HANDLE thr = OpenThread(THREAD_SUSPEND_RESUME, 0, tE.th32ThreadID);
				if (thr == INVALID_HANDLE_VALUE) {
					continue; //i originally had this returning but ig you can just skip thread
				}

				if (SuspendThread(thr)) { //Succeeded
					suspendedThreads.emplace_back(thr); //only log if suspended
				}
				else { //Failed
					CloseHandle(thr); //close cuz resumer wont close unlogged
				}
			}
		} while (Thread32Next(hSnap, &tE));
	}
	return suspendedThreads;
}
//function to resume all threads. idk why i made this bool im too lazy to write the false return on resume
BOOL resumeThreads(std::vector<HANDLE>& threads) {
	for (HANDLE& thr : threads) {
		ResumeThread(thr);
		CloseHandle(thr);
	}
	threads.clear();
	return 1;
}

//thank you stackoverflow for 
const wchar_t* GetWC(const char* c)
{
	const size_t cSize = strlen(c) + 1;
	wchar_t* wc = new wchar_t[cSize];
	mbstowcs(wc, c, cSize);

	return wc;
}

//function to find internal address in process
LPVOID getInternalAddr(HANDLE hTar, const char* mod, const char* proc) {
	HMODULE hMod = GetModuleHandleA(mod);
	if (hMod == nullptr) {
		MessageBoxA(0, "fail 1", "f", MB_OK); //if this fails u need to like reinstall windows or u mistyped sm
		return nullptr;
	}
	LPVOID offSetAddr = GetProcAddress(hMod, proc);
	if (offSetAddr == nullptr) {
		MessageBoxA(0, "fail 2", "f", MB_OK); //lowkey weird if this breaks probably mistype
		return nullptr;
	}

	//find base offset for module (kernel32.dll + WHAT -> loadlibA) cuz then we find kernel32.dll loc in target
	uintptr_t procOffset = (reinterpret_cast<uintptr_t>(offSetAddr) - reinterpret_cast<uintptr_t>(hMod));


	MODULEENTRY32 mE = { 0 };
	mE.dwSize = sizeof(mE);

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetProcessId(hTar));
	if (hSnap == INVALID_HANDLE_VALUE) { 
		MessageBoxA(0, "fail 3", "f", MB_OK); //open a valid process next time
		return nullptr;
	}

	const wchar_t* lMod = GetWC(mod); //:)

	void* rBase = NULL;
	if (Module32First(hSnap, &mE)) {
		do {
			std::cout << "COMP: " << mE.szModule << " && " << mod << "\n";
			if (_wcsicmp(mE.szModule, lMod) == 0) { //is found module the same as our target module
				rBase = mE.modBaseAddr; //yes, log it, break out of loop
				break;
			}
		} while (Module32Next(hSnap, &mE));
	}
	CloseHandle(hSnap); //close handle obv buddy
	if (rBase == NULL) { MessageBoxA(0, "fail 4", "f", MB_OK); return nullptr; } //couldnt find module in process. probs not mistyped cuz wouldve failed 1. idk find a better process to inject into fr
	
	return reinterpret_cast<void*>((reinterpret_cast<uintptr_t>(rBase) + procOffset)); //add initial offset to process' module base
}


InjectResults baseInjector(char* p, DWORD pid, bool suspend, bool findInternalAddr, bool doSyscall) {

//a lot of this stuff doesnt need notes icl lock in
	if (PathFileExistsA(p) == 0) {
		MessageBoxA(0, "Invalid DLL path", "np", MB_OK);
		return FAIL_NOPROC;
	}
	if (GetFileAttributesA(p) == INVALID_FILE_ATTRIBUTES) {
		MessageBoxA(0, "Invalid DLL file", "n2", MB_OK);
		return FAIL_NOPROC;
	}


	std::cout << "injecting: " << p << ".\n";
	std::cout << "Given proc id: " << pid << "\n";


	HANDLE h = OpenProcess(PROCESS_ALL_ACCESS, 0, pid);
	if (h == INVALID_HANDLE_VALUE) {
		return FAIL_NOPROC;
	}


	std::vector<HANDLE> suspendedThreads;
	if (suspend) {
		suspendedThreads = suspendProcess(h);
	}


	void* L = nullptr;
	HMODULE ntDLL = 0;
	if (doSyscall) { //syscalal
		ntDLL = GetModuleHandleA("ntdll.dll"); //for non internal addr
		auto NtAllocateVirtualMemory = (NTSTATUS(NTAPI*)(HANDLE, PVOID*, ULONG_PTR, PSIZE_T, ULONG, ULONG))GetProcAddress(ntDLL, "NtAllocateVirtualMemory");
		
		if (findInternalAddr) {
			auto save = NtAllocateVirtualMemory;
			NtAllocateVirtualMemory = (NTSTATUS(NTAPI*)(HANDLE, PVOID*, ULONG_PTR, PSIZE_T, ULONG, ULONG))getInternalAddr(h, "ntdll.dll", "NtAllocateVirtualMemory");
			if (NtAllocateVirtualMemory == 0x000) {
				NtAllocateVirtualMemory = save; //fallback if internal addr fails. 
			}
		}

		SIZE_T size = strlen(p) + 1;
		NTSTATUS status = NtAllocateVirtualMemory(h, &L, 0, &size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE); //allocate with Nt function
	}
	if (L == nullptr) { //So this will call if they disabled syscall OR syscall fails. 
		std::cout << "alloc fallback!\n"; //technically not always true could just be syscall off
		L = VirtualAllocEx(h, 0, strlen(p) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE); //reg alloc
		//ok yes was lazy didnt find internal valloc stfuuuu
		if (L == nullptr) {
			return FAIL_NOMEM;
		}
	}


	BOOL didWrite = WriteProcessMemory(h, L, p, strlen(p) + 1, 0x00);
	if (!didWrite) {
		CloseHandle(h);
		return FAIL_INJECT;
	}


	LPVOID loadLibraryAddress = nullptr;
	if (findInternalAddr) {
		loadLibraryAddress = getInternalAddr(h, "kernel32.dll", "LoadLibraryA"); //find internal loadlib addr
	}
	if(loadLibraryAddress == nullptr) { //allow non-internal backup if lla fails (kernel32.dll not loaded)?
		loadLibraryAddress = (LPVOID)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryA"); //im not even writing a second nullptr check
		//if this fails, new windows isntall!
	}


	HANDLE hT;
	if (doSyscall) {
		auto NtCreateThreadEx = (NTSTATUS(NTAPI*)(PHANDLE hThread,
			ACCESS_MASK DesiredAccess,
			PVOID ObjectAttributes,
			HANDLE ProcessHandle,
			PVOID lpStartAddress,
			PVOID lpParameter,
			ULONG Flags,
			SIZE_T StackZeroBits,
			SIZE_T SizeOfStackCommit,
			SIZE_T SizeOfStackReserve,
			PVOID lpBytesBuffer)) (findInternalAddr ? getInternalAddr(h, "ntdll.dll", "NtCreateThreadEx") : GetProcAddress(ntDLL, "NtCreateThreadEx"));
		NtCreateThreadEx(&hT, THREAD_ALL_ACCESS, 0, h, loadLibraryAddress, L, 0, 0, 0, 0, 0);
	}
	else {
		hT = CreateRemoteThread(h, 0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibraryAddress), L, 0, 0);
	}
	//HANDLE VALIDITY CHECK
	if (hT == INVALID_HANDLE_VALUE) {
		return FAIL_THREAD;
	}


	if (suspend) { //resume threads
		resumeThreads(suspendedThreads);
	}


	if (doSyscall) {
		auto NtFreeVirtualMemory = (NTSTATUS(NTAPI*)(HANDLE  ProcessHandle,
			PVOID * BaseAddress,
			PSIZE_T RegionSize,
			ULONG   FreeType)) (findInternalAddr ? getInternalAddr(h, "ntdll.dll", "NtFreeVirtualMemory") : GetProcAddress(ntDLL, "NtFreeVirtualMemory"));

		SIZE_T szzz = static_cast<SIZE_T>(strlen(p) + 1);
		NtFreeVirtualMemory(h, &L, &szzz, MEM_RELEASE);
	}
	if (L != 0x0) {
		VirtualFree(L, strlen(p) + 1, MEM_RELEASE); //we know this one
	}


	CloseHandle(h);
	InjectResults F = (hT == 0) ? FAIL_THREAD : SUCCESS;
	CloseHandle(hT);


	return F;
}
