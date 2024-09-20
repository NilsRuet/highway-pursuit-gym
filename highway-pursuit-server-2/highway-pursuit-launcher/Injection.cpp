#include "Injection.hpp"
#include <iostream>
#include <shlwapi.h>
#include <psapi.h>

namespace Injection
{
    std::string GetFullPath(const std::string& relativePath)
    {
        char fullPath[MAX_PATH];  // Buffer to hold the full path
        DWORD result = GetFullPathNameA(relativePath.c_str(), MAX_PATH, fullPath, nullptr);

        if (result == 0)
        {
            // If the result is 0, an error occurred
            std::cerr << "Failed to get full path. Error: " << GetLastError() << std::endl;
            return "";
        }

        // Return the full path as a string
        return std::string(fullPath);
    }


    bool CreateAndInject(const std::string& targetExePath, const std::string& dllPath, const HighwayPursuitArgs& args)
    {
        std::string targetDllPath = GetFullPath(dllPath);

        // Create process in a suspended state
        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi = { 0 };
        if (!CreateProcessA(targetExePath.c_str(), NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi))
        {
            std::cerr << "Failed to create process. Error: " << GetLastError() << std::endl;
            return false;
        }

        // Allocate memory in the target process for the DLL path/args
        LPVOID pDllPath = VirtualAllocEx(pi.hProcess, NULL, targetDllPath.size() + 1, MEM_COMMIT, PAGE_READWRITE);
        LPVOID pArgument = VirtualAllocEx(pi.hProcess, NULL, sizeof(HighwayPursuitArgs), MEM_COMMIT, PAGE_READWRITE);
        if (!pDllPath || !pArgument)
        {
            std::cerr << "Failed to allocate memory in target process. Error: " << GetLastError() << std::endl;
            TerminateProcess(pi.hProcess, 1);
            return false;
        }

        // Write the DLL path into the allocated memory
        if (!WriteProcessMemory(pi.hProcess, pDllPath, targetDllPath.c_str(), targetDllPath.size() + 1, NULL))
        {
            std::cerr << "Failed to write DLL path to target process memory. Error: " << GetLastError() << std::endl;
            VirtualFreeEx(pi.hProcess, pDllPath, 0, MEM_RELEASE);
            TerminateProcess(pi.hProcess, 1);
            return false;
        }

        // Write the args into the allocated memory
        if (!WriteProcessMemory(pi.hProcess, pArgument, &args, sizeof(HighwayPursuitArgs), NULL))
        {
            std::cerr << "Failed to write argument to target process memory. Error: " << GetLastError() << std::endl;
            VirtualFreeEx(pi.hProcess, pDllPath, 0, MEM_RELEASE);
            VirtualFreeEx(pi.hProcess, pArgument, 0, MEM_RELEASE);
            TerminateProcess(pi.hProcess, 1);
            return false;
        }

        // Get the address of LoadLibraryA in kernel32.dll
        HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
        if (kernel32 == NULL)
        {
            std::cerr << "Failed to get module 'kernel32.dll'. Error: " << GetLastError() << std::endl;
            VirtualFreeEx(pi.hProcess, pDllPath, 0, MEM_RELEASE);
            VirtualFreeEx(pi.hProcess, pArgument, 0, MEM_RELEASE);
            TerminateProcess(pi.hProcess, 1);
            return false;
        }

        LPVOID pLoadLibrary = (LPVOID)GetProcAddress(kernel32, "LoadLibraryA");
        if (!pLoadLibrary)
        {
            std::cerr << "Failed to get address of LoadLibraryA. Error: " << GetLastError() << std::endl;
            VirtualFreeEx(pi.hProcess, pDllPath, 0, MEM_RELEASE);
            VirtualFreeEx(pi.hProcess, pArgument, 0, MEM_RELEASE);
            TerminateProcess(pi.hProcess, 1);
            return false;
        }

        // Create a remote thread in the target process to call LoadLibraryA with the DLL path
        HANDLE hThread = CreateRemoteThread(pi.hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibrary, pDllPath, 0, NULL);
        if (!hThread)
        {
            std::cerr << "Failed to create remote thread in target process. Error: " << GetLastError() << std::endl;
            VirtualFreeEx(pi.hProcess, pDllPath, 0, MEM_RELEASE);
            VirtualFreeEx(pi.hProcess, pArgument, 0, MEM_RELEASE);
            TerminateProcess(pi.hProcess, 1);
            return false;
        }

        // Wait for the loading to finish and close the handles
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);

        // Get the address of LoadLibraryA in kernel32.dll
        HMODULE serverDLL = LoadLibraryA(targetDllPath.c_str());
        if (serverDLL == NULL)
        {
            std::cerr << "Failed to load module '" << targetDllPath << "'. Error: " << GetLastError() << std::endl;
            VirtualFreeEx(pi.hProcess, pDllPath, 0, MEM_RELEASE);
            VirtualFreeEx(pi.hProcess, pArgument, 0, MEM_RELEASE);
            TerminateProcess(pi.hProcess, 1);
            return false;
        }

        LPVOID pEntryPoint = (LPVOID)GetProcAddress(serverDLL, "EntryPoint");
        if (!pEntryPoint)
        {
            std::cerr << "Failed to get address of EntryPoint. Error: " << GetLastError() << std::endl;
            VirtualFreeEx(pi.hProcess, pDllPath, 0, MEM_RELEASE);
            VirtualFreeEx(pi.hProcess, pArgument, 0, MEM_RELEASE);
            TerminateProcess(pi.hProcess, 1);
            return false;
        }

        // Create a remote thread to call the InitFunction in the DLL and pass the argument memory
        HANDLE hEntryThread = CreateRemoteThread(pi.hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pEntryPoint, pArgument, 0, NULL);
        if (!hEntryThread)
        {
            std::cerr << "Failed to create remote thread for EntryPoint. Error: " << GetLastError() << std::endl;
            VirtualFreeEx(pi.hProcess, pDllPath, 0, MEM_RELEASE);
            VirtualFreeEx(pi.hProcess, pArgument, 0, MEM_RELEASE);
            TerminateProcess(pi.hProcess, 1);
            return false;
        }

        // Release resources
        WaitForSingleObject(hEntryThread, INFINITE);
        CloseHandle(hEntryThread);

        VirtualFreeEx(pi.hProcess, pDllPath, 0, MEM_RELEASE);
        VirtualFreeEx(pi.hProcess, pArgument, 0, MEM_RELEASE);

        // Wake up process
        ResumeThread(pi.hThread);

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        return true;
    }
}