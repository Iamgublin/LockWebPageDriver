// exe.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include<Windows.h>
#define DEVICE_NAME L"\\Device\\Contact Device"
#define SYM_NAME L"\\??\\LockPageDriver"
#define IOCTL_SENDEVENTTOKERNEL (ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN,0x911,METHOD_BUFFERED,FILE_WRITE_DATA|FILE_READ_DATA) 
#define IOCTL_WAITFOR360SE (ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN,0x912,METHOD_BUFFERED,FILE_WRITE_DATA|FILE_READ_DATA)
#define IOCTL_GET360SEPATH (ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN,0x913,METHOD_BUFFERED,FILE_WRITE_DATA|FILE_READ_DATA) 
#define IE L"C:\\Program Files\\Internet Explorer\\iexplore.exe"  
#define CMD L"open http://www.baidu.com/"  
int main()
{
	STARTUPINFO sa = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	DWORD Byteret = 0;
	WCHAR T360SePath[255] = { 0 };

	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	printf("0x%x\n", GetLastError());

	HANDLE hDev = CreateFile(SYM_NAME, GENERIC_ALL, 0, NULL, OPEN_ALWAYS, NULL, NULL);

	DeviceIoControl(hDev, IOCTL_SENDEVENTTOKERNEL, &hEvent, sizeof(hEvent), NULL, 0, &Byteret, NULL);
	DeviceIoControl(hDev, IOCTL_WAITFOR360SE, NULL, 0, NULL, 0, &Byteret, NULL);

	WaitForSingleObject(hEvent, INFINITE);

	DeviceIoControl(hDev, IOCTL_GET360SEPATH, NULL, 0, T360SePath, sizeof(T360SePath), &Byteret, NULL);

	wprintf(TEXT("get 360path:%ws\n"), T360SePath);

	Sleep(3000);

	CreateProcess(T360SePath,
		TEXT("open http://www.baidu.com/"),
		NULL,
		NULL,
		FALSE,
		NULL,
		NULL,
		NULL,
		&sa,
		&pi);
	printf("0x%x\n", GetLastError());

	WCHAR *A = TEXT("C:\\Users\\zlzPC\\AppData\\Roaming\\360se6\\Application\\360se.exe");
	CreateProcess(A,
		TEXT("open http://www.baidu.com/"),
		NULL,
		NULL,
		FALSE,
		NULL,
		NULL,
		NULL,
		&sa,
		&pi);
	Sleep(INFINITE);
    return 0;
}

