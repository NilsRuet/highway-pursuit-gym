#include "Injection.hpp"
#include <iostream>
#include <shlwapi.h>
#include <psapi.h>

using namespace Shared;

namespace Injection
{
    namespace
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

        // Inject the DLL thread
        bool RemoteThread(HANDLE process, LPCSTR moduleName, LPCSTR procName, LPVOID pBuffer, LPCVOID arg, SIZE_T argSize)
        {
            // Write the DLL path into the allocated memory
            if (!WriteProcessMemory(process, pBuffer, arg, argSize, NULL))
            {
                std::cerr << "Failed to write arg to memory. Error: " << GetLastError() << std::endl;
                return false;
            }

            // Get the address of procedure
            HMODULE pModule = GetModuleHandleA(moduleName);
            if (pModule == NULL)
            {
                std::cerr << "Failed to get module '" << moduleName << "'. Error: " << GetLastError() << std::endl;
                return false;
            }
            LPVOID pProc = reinterpret_cast<LPVOID>(GetProcAddress(pModule, procName));
            if (!pProc)
            {
                std::cerr << "Failed to get address of '" << procName << "'. Error: " << GetLastError() << std::endl;
                return false;
            }

            // Create a remote thread in the target process to call LoadLibraryA with the DLL path
            HANDLE hThread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)pProc, pBuffer, 0, NULL);
            if (!hThread)
            {
                std::cerr << "Failed to create remote thread in target process. Error: " << GetLastError() << std::endl;
                return false;
            }

            // Wait for the loading to finish and close the handles
            WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);

            return true;
        }

        // Injects the DLL and manages the dll arg memory
        bool InjectDll(HANDLE process, const std::string& targetDllPath)
        {
            // Allocate memory in the target process for the DLL path/args
            LPVOID pDllPath = VirtualAllocEx(process, NULL, targetDllPath.size() + 1, MEM_COMMIT, PAGE_READWRITE);
            if (!pDllPath)
            {
                std::cerr << "Failed to allocate memory in target process. Error: " << GetLastError() << std::endl;
                return false;
            }

            bool res = RemoteThread(process, "kernel32.dll", "LoadLibraryA", pDllPath, targetDllPath.c_str(), targetDllPath.size() + 1);
            VirtualFreeEx(process, pDllPath, 0, MEM_RELEASE);
            return res;
        }

        // Injects the DLL and manages the args memory
        // The given DLL has to be already loaded
        bool CallProcedure(HANDLE process, const std::string& dllPath, const std::string& procName, const HighwayPursuitArgs& args)
        {
            // Allocate memory in the target process for the DLL path/args
            LPVOID pArgument = VirtualAllocEx(process, NULL, sizeof(HighwayPursuitArgs), MEM_COMMIT, PAGE_READWRITE);
            if (!pArgument)
            {
                std::cerr << "Failed to allocate memory for args in target process. Error: " << GetLastError() << std::endl;
                return false;
            }

            bool res = RemoteThread(process, dllPath.c_str(), procName.c_str(), pArgument, &args, sizeof(HighwayPursuitArgs));
            VirtualFreeEx(process, pArgument, 0, MEM_RELEASE);
            return res;
        }
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

        // Inject DLL
        if (!InjectDll(pi.hProcess, targetDllPath))
        {
            TerminateProcess(pi.hProcess, 1);
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            return false;
        }

        // Load the library to be able to call CreateRemoteThread
        HMODULE hModule = LoadLibraryA(dllPath.c_str());
        if (hModule == NULL)
        {
            std::cerr << "Failed to load library. Error: " << GetLastError() << std::endl;
            TerminateProcess(pi.hProcess, 1);
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            return false;
        }

        // Call init function (install hooks)
        if (!CallProcedure(pi.hProcess, targetDllPath, "Initialize", args))
        {
            FreeLibrary(hModule);
            TerminateProcess(pi.hProcess, 1);
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            return false;
        }

        // Wake up process
        ResumeThread(pi.hThread);

        // Call server main
        if (!CallProcedure(pi.hProcess, targetDllPath, "Run", args))
        {
            FreeLibrary(hModule);
            TerminateProcess(pi.hProcess, 1);
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            return false;
        }

        // Clean up
        FreeLibrary(hModule);
        TerminateProcess(pi.hProcess, 0);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return true;
    }
}