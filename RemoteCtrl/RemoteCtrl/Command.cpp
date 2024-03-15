#include "pch.h"
#include "Command.h"

CCommand::CCommand():threadid(0)
{
	struct  
	{
		int nCmd;
		CMDFUNC func;
	}data[] = {
		{1, &CCommand::MakeDriverInfo},
		{2, &CCommand::MakeDirectoryInfo},
		{3, &CCommand::RunFile},
		{4, &CCommand::DownloadFile},
		{5, &CCommand::MouseEvent},
		{6, &CCommand::SendScreen},
		{7, &CCommand::LockMachine},
		{8, &CCommand::UnlockMachine},
		{9, &CCommand::DeleteLocalFile},
		{1981, &CCommand::TextConnect},
		{-1, NULL}
	};  // 后续如果增加功能，只需要修改这里的data[]
	for (int i = 0; data[i].nCmd != -1; i++) {
		m_mapFunction.insert(std::pair<int, CMDFUNC>(data[i].nCmd, data[i].func));
	}
}

int CCommand::ExcuteCommand(int nCmd)
{
	std::map<int, CMDFUNC>::iterator it = m_mapFunction.find(nCmd);
	if (it == m_mapFunction.end()) {
		return -1;
	}
	// it->second 是一个成员函数指针，不能直接调用。
	// 如果你有一个指向成员函数的指针，你必须使用对象或对象指针以及成员函数指针运算符 ->* 或 .* 来调用该函数
	// return (this->*it->second)();  // 可以
	return (this->*(it->second))();
	// return it->second();  错误
}
