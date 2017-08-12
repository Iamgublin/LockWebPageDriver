// exe.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include<Windows.h>

int main()
{
	CreateEvent(NULL, FALSE, FALSE, L"ZLZ");
	Sleep(INFINITE);
    return 0;
}

