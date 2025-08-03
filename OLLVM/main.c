#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>

void list_running_processes() {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;

    // Take a snapshot of all processes in the system
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        printf("Failed to take process snapshot.\n");
        return;
    }

    // Initialize the PROCESSENTRY32 structure
    pe32.dwSize = sizeof(PROCESSENTRY32);

    // Retrieve information about the first process
    if (!Process32First(hProcessSnap, &pe32)) {
        printf("Failed to retrieve information about the first process.\n");
        CloseHandle(hProcessSnap);
        return;
    }

    // Loop through the processes
    printf("PID\tProcess Name\n");
    printf("============================\n");
    do {
        printf("%u\t%s\n", pe32.th32ProcessID, pe32.szExeFile);
    } while (Process32Next(hProcessSnap, &pe32));

    // Clean up the snapshot handle
    CloseHandle(hProcessSnap);
}

int main() {
    list_running_processes();
    return 0;
}