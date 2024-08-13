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

	// ��鵱ǰ�����Ƿ��Թ���ԱȨ������
	static bool IsAdmin() {
		HANDLE hToken = NULL;  // ���ڴ洢��ǰ���̵ķ�������(Token)
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {  // ��ȡ��ǰ���̵ķ�������;
			MessageBox(NULL, _T("OpenProcessTokenʧ�ܣ�"), _T("����"), 0);
			ShowError();
			return false;
		}
		TOKEN_ELEVATION eve;
		DWORD len = 0;
		if (GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len) == FALSE) {
			ShowError();
			MessageBox(NULL, _T("GetTokenInformationʧ�ܣ�"), _T("����"), 0);
			return false;
		}
		CloseHandle(hToken);
		if (len == sizeof(eve)) {
			return eve.TokenIsElevated;  // �������Ƶ�����״̬
		}
		printf("length of tokeninformation is %d\r\n", len);
		MessageBox(NULL, _T("��ȡ��������״̬ʧ�ܣ�"), _T("����"), 0);
		return false;
	}
	
	// ��ȡ���һ��ϵͳ������Ϣ�������������������
	static void ShowError() {
		LPTSTR lpMessageBuf = NULL;
		// strerror(errno);  // ��׼c���Կ��
		// FormatMessage()��ʽ��ϵͳ������Ϣ
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL, GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMessageBuf, 0, NULL);
		OutputDebugString(lpMessageBuf);  // ����ʽ����Ĵ�����Ϣ�������������ͨ�����ڵ���Ŀ�ġ�
		LocalFree(lpMessageBuf);
	}

	static bool RunAsAdmin() {
		// TODO:��ȡ����ԱȨ�ޣ�ʹ�ø�Ȩ�޴�������
		//HANDLE hToken = NULL;
		//// LOGON32_LOGON_INTERACTIVE
		//bool ret = LogonUser(L"Administrator", NULL, NULL, LOGON32_LOGON_BATCH, LOGON32_PROVIDER_DEFAULT, &hToken);  // ��ָ���û�����ݵ�¼ϵͳ
		//if (!ret) {
		//    ShowError();
		//    MessageBox(NULL, _T("LogonUser��¼����"), _T("�������"), 0);
		//    exit(0);
		//}
		//OutputDebugString(L"logon administrator success!\r\n");
		/* ���ز�������Ҫ��1�����ù���Ա�˻�״̬�� 2������ʹ�ÿ�����ı����˻�*/
		STARTUPINFO si = { 0 };
		PROCESS_INFORMATION pi = { 0 };
		TCHAR sPath[MAX_PATH] = _T("");
		//GetCurrentDirectory(MAX_PATH, sPath);  // ��ȡ��ǰ�������е�·��
		//CString strCmd = sPath;
		//strCmd += _T("\\RemoteCtrl.exe");
		GetModuleFileName(NULL, sPath, MAX_PATH);
		// ��������
		// ret = CreateProcessWithTokenW(hToken, LOGON_WITH_PROFILE, NULL, (LPWSTR)(LPCWSTR)strCmd, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi); 
		BOOL ret = CreateProcessWithLogonW(_T("Administrator"), NULL, NULL, LOGON_WITH_PROFILE, NULL, sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
		// CloseHandle(hToken);
		if (!ret) {
			CTool::ShowError();  // TODO: ȥ��������Ϣ
			MessageBox(NULL, sPath, _T("�������,��������ʧ��"), 0);  // TODO: ����Ҫȥ�����ڵ��Ե���Ϣ
			return false;
		}
		MessageBox(NULL, sPath, _T("�������̳ɹ�"), 0);  // TODO��ȥ��������Ϣ
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return true;
	}

	// ͨ���޸Ŀ��������ļ�����ʵ�ֿ�������
	static BOOL WriteStartupDir(const CString& strPath) {
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);  // ��ȡ��ǰִ�г����·��
		return CopyFile(sPath, strPath, FALSE);
	}
	// ����������ʱ�򣬳����Ȩ���Ǹ��������û��ģ��������Ȩ�޲�һ�£���ᵼ�³�������ʧ��
	// ���������Ի���������Ӱ�죬�������dll(��̬��)�����������ʧ�ܣ� 
	//   �������1��������Щdll��system32�������sysWOW64���棻 2������ʹ�þ�̬������Ƕ�̬�⡿
	//    system32�������64Ϊ���� sysWOW64�������32Ϊ����
	// ͨ���޸�ע�����ʵ�ֿ�������
	static bool WriteRegisterTable(const CString& strPath) {
		CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);  // ��ȡ��ǰִ�г����·��
		bool ret = CopyFile(sPath, strPath, FALSE);
		if (ret == false) {
			MessageBox(NULL, _T("�����ļ�ʧ��\r\n"), _T("����"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		HKEY hKey = NULL;
		ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);
		if (ret != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			MessageBox(NULL, _T("�����Զ���������ʧ�ܣ��Ƿ�Ȩ�޲��㣿\r\n��������ʧ��\r\n"), _T("����"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		ret = RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));  // LPCTSTR�� const char*
		if (ret != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			MessageBox(NULL, _T("�����Զ���������ʧ�ܣ��Ƿ�Ȩ�޲��㣿\r\n��������ʧ��\r\n"), _T("����"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		RegCloseKey(hKey);
		return true;
	}

	// ���ڴ�MFC��Ŀ��ʼ��(ͨ��)
	static bool Init() {
		HMODULE hModule = ::GetModuleHandle(nullptr);
		if (hModule == nullptr) {
			wprintf(L"����: GetModuleHandle ʧ��\n");
			return false;
		}
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
		{
			wprintf(L"����: MFC ��ʼ��ʧ��\n");
			return false;
		}
		return true;
	}
};

