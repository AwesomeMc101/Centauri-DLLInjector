# Tensora-DLLInjector
DearImgui DLL injector with options for suspension, syscall, and internal address locater. 

![image](https://github.com/user-attachments/assets/87484ba5-20d4-4c92-bad8-0d8cfe9ac95e)


# Injection Type
* Basic dllpath injection. DLL is selected with the basic windows open window and the application has a process scanner with search query

# Security
## Injection Suspension
* Suspends all process threads during injection and resumes them after.

## Internal Function Scanner
* Locates the addresses of internal memory functions in the target process. I.e. finds LoadLibraryA in target and loads DLL with that addr.
* Locates Syscall internal addresses if that feature is enabled

## Syscall
* Utilizes system Nt calls like NtAllocateVirtualMemory during injection
* NtAllocateVirtualMemory, NtCreateThreadEx, NtFreeVirtualMemory
