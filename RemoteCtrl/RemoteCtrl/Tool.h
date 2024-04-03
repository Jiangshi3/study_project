#pragma once
class CTool
{
public:
	static void Dump(BYTE* pData, size_t nSize) {
		std::string strOut;
		for (size_t i = 0; i < nSize; i++) {
			char buf[8] = "";
			if (i > 0 && (i % 16 == 0)) strOut += "\n";
			snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
			strOut += buf;
		}
		strOut += '\n';
		OutputDebugStringA(strOut.c_str());
	}

	// 检查当前进程是否以管理员权限运行
	static bool IsAdmin() {
		HANDLE hToken = NULL;  // 用于存储当前进程的访问令牌(Token)
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {  // 获取当前进程的访问令牌;
			MessageBox(NULL, _T("OpenProcessToken失败！"), _T("错误"), 0);
			ShowError();
			return false;
		}
		TOKEN_ELEVATION eve;
		DWORD len = 0;
		if (GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len) == FALSE) {
			ShowError();
			MessageBox(NULL, _T("GetTokenInformation失败！"), _T("错误"), 0);
			return false;
		}
		CloseHandle(hToken);
		if (len == sizeof(eve)) {
			return eve.TokenIsElevated;  // 返回令牌的提升状态
		}
		printf("length of tokeninformation is %d\r\n", len);
		MessageBox(NULL, _T("获取令牌提升状态失败！"), _T("错误"), 0);
		return false;
	}
	
	// 获取最近一次系统错误消息，并将其输出到调试器
	static void ShowError() {
		LPTSTR lpMessageBuf = NULL;
		// strerror(errno);  // 标准c语言库的
		// FormatMessage()格式化系统错误消息
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL, GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMessageBuf, 0, NULL);
		OutputDebugString(lpMessageBuf);  // 将格式化后的错误消息输出到调试器，通常用于调试目的。
		LocalFree(lpMessageBuf);
	}

	static bool RunAsAdmin() {
		// TODO:获取管理员权限，使用该权限创建进程
		//HANDLE hToken = NULL;
		//// LOGON32_LOGON_INTERACTIVE
		//bool ret = LogonUser(L"Administrator", NULL, NULL, LOGON32_LOGON_BATCH, LOGON32_PROVIDER_DEFAULT, &hToken);  // 以指定用户的身份登录系统
		//if (!ret) {
		//    ShowError();
		//    MessageBox(NULL, _T("LogonUser登录错误！"), _T("程序错误"), 0);
		//    exit(0);
		//}
		//OutputDebugString(L"logon administrator success!\r\n");
		/* 本地策略组中要：1、启用管理员账户状态； 2、禁用使用空密码的本地账户*/
		STARTUPINFO si = { 0 };
		PROCESS_INFORMATION pi = { 0 };
		TCHAR sPath[MAX_PATH] = _T("");
		//GetCurrentDirectory(MAX_PATH, sPath);  // 获取当前进程运行的路径
		//CString strCmd = sPath;
		//strCmd += _T("\\RemoteCtrl.exe");
		GetModuleFileName(NULL, sPath, MAX_PATH);
		// 创建进程
		// ret = CreateProcessWithTokenW(hToken, LOGON_WITH_PROFILE, NULL, (LPWSTR)(LPCWSTR)strCmd, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi); 
		BOOL ret = CreateProcessWithLogonW(_T("Administrator"), NULL, NULL, LOGON_WITH_PROFILE, NULL, sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
		// CloseHandle(hToken);
		if (!ret) {
			CTool::ShowError();  // TODO: 去除调试信息
			MessageBox(NULL, sPath, _T("程序错误,创建进程失败"), 0);  // TODO: 后续要去除用于调试的信息
			return false;
		}
		MessageBox(NULL, sPath, _T("创建进程成功"), 0);  // TODO：去除调试信息
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return true;
	}

	// 通过修改开机启动文件夹来实现开机启动
	static BOOL WriteStartupDir(const CString& strPath) {
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);  // 获取当前执行程序的路径
		return CopyFile(sPath, strPath, FALSE);
	}
	// 开机启动的时候，程序的权限是跟随启动用户的；如果两者权限不一致，则会导致程序启动失败
	// 开机启动对环境变量有影响，如果依赖dll(动态库)，则可能启动失败； 
	//   【解决：1、复制这些dll到system32下面或者sysWOW64下面； 2、或者使用静态库而不是动态库】
	//    system32下面多是64为程序； sysWOW64下面多是32为程序；
	// 通过修改注册表来实现开机启动
	static bool WriteRegisterTable(const CString& strPath) {
		CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);  // 获取当前执行程序的路径
		bool ret = CopyFile(sPath, strPath, FALSE);
		if (ret == false) {
			MessageBox(NULL, _T("复制文件失败\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		HKEY hKey = NULL;
		ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);
		if (ret != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			MessageBox(NULL, _T("设置自动开机启动失败！是否权限不足？\r\n程序启动失败\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		ret = RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));  // LPCTSTR： const char*
		if (ret != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			MessageBox(NULL, _T("设置自动开机启动失败！是否权限不足？\r\n程序启动失败\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		RegCloseKey(hKey);
		return true;
	}

	// 用于带MFC项目初始化(通用)
	static bool Init() {
		HMODULE hModule = ::GetModuleHandle(nullptr);
		if (hModule == nullptr) {
			wprintf(L"错误: GetModuleHandle 失败\n");
			return false;
		}
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
		{
			wprintf(L"错误: MFC 初始化失败\n");
			return false;
		}
		return true;
	}
};

