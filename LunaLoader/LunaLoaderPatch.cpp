#include <windows.h>
#include <string>
#include <cstring>
#include <stdint.h>
#include "LunaLoaderPatch.h"


//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
std::wstring GetLastErrorAsString()
{
    //Get the error message, if any.
    DWORD errorMessageID = ::GetLastError();
    if(errorMessageID == 0)
        return std::wstring(); //No error message has been recorded

    LPWSTR messageBuffer = nullptr;
    size_t size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, NULL);

    std::wstring message(messageBuffer, size);

    //Free the buffer.
    LocalFree(messageBuffer);

    return message;
}

static void setJmpAddr(uint8_t* patch, DWORD patchAddr, DWORD patchOffset, DWORD target)
{
    DWORD* dwordAddr = (DWORD*)&patch[patchOffset+1];
    *dwordAddr = (DWORD)target - (DWORD)(patchAddr + patchOffset + 5);
}

static int patchAStr(HANDLE f, unsigned int at, char* str, unsigned int maxlen)
{
    char data[1024];
    memset(data, 0, maxlen);
    unsigned int i;
    unsigned int len = strlen(str);
    for(i=0; (i<len) && (i<maxlen-1); i++)
    {
        data[i] = str[i];
    }
//    fseek(f, at, SEEK_SET);
//    fwrite(data, 1, maxlen, f);
    SIZE_T bytes = 0;
    BOOL res = WriteProcessMemory(f, (void*)at, (void*)data, maxlen, &bytes);
    if(res == 0)
    {
        std::wstring msg = GetLastErrorAsString();
        MessageBoxW(NULL, msg.c_str(), L"Error!", MB_ICONERROR);
    }
    return bytes;
}

static int patchUStr(HANDLE f, unsigned int at, char* str, unsigned int maxlen)
{
    char data[1024];
    memset(data, 0, maxlen);
    unsigned int i, j;
    unsigned int len = strlen(str);
    for(i=0, j=0; (i<len) && (j<maxlen); i++, j+=2)
    {
        data[j] = str[i];
        data[j+1] = 0;
    }
//    fseek(f, at, SEEK_SET);
//    fwrite(data, 1, maxlen, f);
    SIZE_T bytes = 0;
    BOOL res = WriteProcessMemory(f, (void*)at, (void*)data, maxlen, &bytes);
    if(res == 0)
    {
        std::wstring msg = GetLastErrorAsString();
        MessageBoxW(NULL, msg.c_str(), L"Error!", MB_ICONERROR);
    }
    return bytes;
}

LunaLoaderResult LunaLoaderRun(const wchar_t *pathToSMBX, const wchar_t *cmdLineArgs, const wchar_t *workingDir)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    
    // Prepare command line
    size_t pos = 0;
    std::wstring quotedPathToSMBX(pathToSMBX);
    while ((pos = quotedPathToSMBX.find(L"\"", pos)) != std::string::npos)
    {
        quotedPathToSMBX.replace(pos, 1, L"\\\"");
        pos += 2;
    }

    std::wstring strCmdLine = (
        std::wstring(L"\"") + quotedPathToSMBX + std::wstring(L"\" ") +
        std::wstring(cmdLineArgs)
        );

    uint32_t cmdLineMemoryLen = sizeof(wchar_t) * (strCmdLine.length() + 1); // Include null terminator
    wchar_t* cmdLine = (wchar_t*)malloc(cmdLineMemoryLen);
    std::memcpy(cmdLine, strCmdLine.c_str(), cmdLineMemoryLen);

    // Create process
    if(!CreateProcessW(pathToSMBX, // Launch smbx.exe
        cmdLine,          // Command line
        NULL,             // Process handle not inheritable
        NULL,             // Thread handle not inheritable
        FALSE,            // Set handle inheritance to FALSE
        CREATE_SUSPENDED, // Create in suspended state
        NULL,             // Use parent's environment block
        workingDir,       // Use parent's starting directory 
        &si,              // Pointer to STARTUPINFO structure
        &pi)              // Pointer to PROCESS_INFORMATION structure
        )
    {
        free(cmdLine); cmdLine = NULL;
        return LUNALOADER_CREATEPROCESS_FAIL;
    }
    free(cmdLine);
    cmdLine = NULL;

#if 1// Remote thread way
    std::wstring dllname = L"nothing";
    int          dllname_size = 1024;

    HMODULE msvbvm = LoadLibraryW(L"msvbvm60.dll");
    if(!msvbvm)
    {
        std::wstring msg = GetLastErrorAsString();
        MessageBoxW(NULL, msg.c_str(), L"msvbvm60 Error!", MB_ICONERROR);
        return LUNALOADER_PATCH_FAIL;
    }

    /*
    HMODULE lunadlldll = LoadLibraryW(L"lunadll.dll");
    if(!lunadlldll)
    {
        std::wstring msg = GetLastErrorAsString();
        MessageBoxW(NULL, msg.c_str(), L"lunadll Error!", MB_ICONERROR);
        return LUNALOADER_PATCH_FAIL;
    }*/

    PWSTR pszLibFileRemote = (PWSTR)VirtualAllocEx(pi.hProcess, NULL, dllname_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    LPTHREAD_START_ROUTINE pfnThreadRtn =
            (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleW(L"Kernel32.dll"), "LoadLibraryW");
            //(LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleW(L"user32.dll"), "MessageBoxA");

    HANDLE hThread = NULL;

    dllname = L"msvbvm60.dll";
    if(WriteProcessMemory(pi.hProcess,
                          (void*)pszLibFileRemote,
                          (void*)dllname.c_str(),
                          (dllname.size()+1) * sizeof(wchar_t),
                          NULL) == 0)
    {
        return LUNALOADER_PATCH_FAIL;
    }
    hThread = CreateRemoteThread(pi.hProcess, NULL, 0, pfnThreadRtn, pszLibFileRemote, 0, NULL);
    if(!hThread)
    {
        if(hThread != NULL)
            CloseHandle(hThread);
        return LUNALOADER_PATCH_FAIL;
    }

    // Wait for the remote thread to terminate
    WaitForSingleObject(hThread, INFINITE);
    if(hThread != NULL)
        CloseHandle(hThread);


    dllname = L"LunaDll.dll";
    if(WriteProcessMemory(pi.hProcess,
                          (void*)pszLibFileRemote,
                          (void*)dllname.c_str(),
                          (dllname.size()+1) * sizeof(wchar_t),
                          NULL) == 0)
    {
        if(hThread != NULL)
            CloseHandle(hThread);
        return LUNALOADER_PATCH_FAIL;
    }
    hThread = CreateRemoteThread(pi.hProcess, NULL, 0, pfnThreadRtn, pszLibFileRemote, 0, NULL);
    if(!hThread)
    {
        if(hThread != NULL)
            CloseHandle(hThread);
        return LUNALOADER_PATCH_FAIL;
    }
    // Wait for the remote thread to terminate
    WaitForSingleObject(hThread, INFINITE);
    if(hThread != NULL)
        CloseHandle(hThread);

//    patchUStr(pi.hProcess, 0x27614, (char*)"LunaLUA-SMBX Version 1.3.0.2 http://wohlsoft.ru", 124);
//    patchAStr(pi.hProcess, 0x67F6A, (char*)"LunaLUA-SMBX Version 1.3.0.2 http://wohlsoft.ru", 63);
//    patchAStr(pi.hProcess, 0xA1FE3, (char*)"LunaLUA-SMBX Version 1.3.0.2 http://wohlsoft.ru", 78);
//    patchAStr(pi.hProcess, 0xC9FC0, (char*)"LunaLUA-SMBX Version 1.3.0.2 http://wohlsoft.ru", 65);

#else
    // Patch 1 (jump to Patch 2)
    uintptr_t LoaderPatchAddr1 = 0x40BDD8;

    unsigned char LoaderPatch1[] =
    {
        0xE9, 0x00, 0x00, 0x00, 0x00  // 0x40BDD8 JMP <Patch2>
    };

    // Patch 2 (loads LunaDll.dll)
    unsigned char LoaderPatch2[] =
    {
        0x68, 0x64, 0x6C, 0x6C, 0x00, // 00 PUSH "dll\0"
        0x68, 0x44, 0x6C, 0x6C, 0x2E, // 05 PUSH "Dll."
        0x68, 0x4C, 0x75, 0x6E, 0x61, // 0A PUSH "Luna"
        0x54,                         // 0F PUSH ESP
        0xE8, 0x00, 0x00, 0x00, 0x00, // 10 CALL LoadLibraryA
        0x83, 0xC4, 0x0C,             // 15 ADD ESP, 0C
        0x68, 0x6C, 0xC1, 0x40, 0x00, // 18 PUSH 40C16C (this inst used to be at 0x40BDD8)
        0xE9, 0x00, 0x00, 0x00, 0x00, // 1D JMP 40BDDD
        0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90
    };

    //patchUStr(pi.hProcess, 0x27614, (char*)"LunaLUA-SMBX Version 1.3.0.2 http://wohlsoft.ru", 124);
    //patchAStr(pi.hProcess, 0x67F6A, (char*)"LunaLUA-SMBX Version 1.3.0.2 http://wohlsoft.ru", 63);
    //patchAStr(pi.hProcess, 0xA1FE3, (char*)"LunaLUA-SMBX Version 1.3.0.2 http://wohlsoft.ru", 78);
    //patchAStr(pi.hProcess, 0xC9FC0, (char*)"LunaLUA-SMBX Version 1.3.0.2 http://wohlsoft.ru", 65);

    // Allocate space for Patch 2
    DWORD LoaderPatchAddr2 =
            (DWORD)VirtualAllocEx(
            pi.hProcess,           // Target process
            NULL,                  // Don't request any particular address
            sizeof(LoaderPatch2),  // Length of Patch 2
            MEM_COMMIT,            // Type of memory allocation
            PAGE_EXECUTE_READWRITE // Memory protection type
            );
    if(LoaderPatchAddr2 == (DWORD)NULL)
    {
        return LUNALOADER_PATCH_FAIL;
    }

    // Set Patch1 Addresses
    setJmpAddr(LoaderPatch1, LoaderPatchAddr1, 0x00, LoaderPatchAddr2);

    DWORD pfnThreadRtn = (DWORD)GetProcAddress(GetModuleHandleW(L"Kernel32.dll"), "LoadLibraryA");

    // Set Patch2 Addresses
    setJmpAddr(LoaderPatch2, LoaderPatchAddr2, 0x10, (DWORD)pfnThreadRtn/*&LoadLibraryA*/);
    setJmpAddr(LoaderPatch2, LoaderPatchAddr2, 0x1D, LoaderPatchAddr1 + 5);

    // Patch the entry point...
    if(WriteProcessMemory(pi.hProcess, (void*)LoaderPatchAddr1, LoaderPatch1, sizeof(LoaderPatch1), NULL) == 0 ||
       WriteProcessMemory(pi.hProcess, (void*)LoaderPatchAddr2, LoaderPatch2, sizeof(LoaderPatch2), NULL) == 0)
    {
        std::wstring msg = GetLastErrorAsString();
        MessageBoxW(NULL, msg.c_str(), L"Error!", MB_ICONERROR);
        return LUNALOADER_PATCH_FAIL;
    }
    // Change Patch2 memory protection type
    DWORD TmpDword = 0;
    if( VirtualProtectEx(
        pi.hProcess,
        (void*)LoaderPatchAddr2,
        sizeof(LoaderPatch2),
        PAGE_EXECUTE,
        &TmpDword
    ) == 0)
    {
        return LUNALOADER_PATCH_FAIL;
    }
#endif

    // Resume the main program thread
    ResumeThread(pi.hThread);

    // Close handles
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return LUNALOADER_OK;
}
