#pragma once
#include "MSocket.h"
#include "CThread.h"

// 由服务器去通知用户（用回调函数）
typedef int(*AcceptFunc)(void* arg, MSOCKET& client);
typedef int(*RecvFunc)(void* arg, const MBuffer& buffer);
typedef int(*SendFunc)(void* arg, MSOCKET& client, int ret);  // :发送成功以后去通知用户；应答；回调函数
typedef int(*RecvFromFunc)(void* arg, const MBuffer& buffer, MSockaddrIn& addr);  // UDP
typedef int(*SendToFunc)(void* arg, const MSockaddrIn& addr, int ret);  // UDP sendto()后的应答;回调函数

class MNetwork
{
};

// 方便我们去使用参数
class MServerParameter {
public:
	MServerParameter(
		const std::string& ip = "0.0.0.0", 
		short port = 9527, 
		MTYPE type = MTypeTCP,
		AcceptFunc acceptf = NULL,
		RecvFunc recvf = NULL,
		SendFunc sendf = NULL,
		RecvFromFunc recvfromf = NULL,
		SendToFunc sendtof = NULL
	);
	// 输入     TODO：应该是写反了，这种是输出
	MServerParameter& operator<<(AcceptFunc func);
	MServerParameter& operator<<(RecvFunc func);
	MServerParameter& operator<<(SendFunc func);
	MServerParameter& operator<<(RecvFromFunc func);
	MServerParameter& operator<<(SendToFunc func);
	MServerParameter& operator<<(const std::string& ip);
	MServerParameter& operator<<(short port);
	MServerParameter& operator<<(MTYPE type);
	// 输出     TODO：应该是写反了，这种是输入
	MServerParameter& operator>>(AcceptFunc& func);
	MServerParameter& operator>>(RecvFunc& func);
	MServerParameter& operator>>(SendFunc& func);
	MServerParameter& operator>>(RecvFromFunc& func);
	MServerParameter& operator>>(SendToFunc& func);
	MServerParameter& operator>>(std::string& ip);
	MServerParameter& operator>>(short& port);
	MServerParameter& operator>>(MTYPE& type);
	// 拷贝构造； 用于同类型的赋值
	MServerParameter(const MServerParameter& param);
	// 拷贝赋值
	MServerParameter& operator=(const MServerParameter& param);

	std::string m_ip;
	short m_port;
	MTYPE m_type;
	AcceptFunc m_accept;
	RecvFunc m_recv;
	SendFunc m_send;   // send()后的应答
	RecvFromFunc m_recvfrom;
	SendToFunc m_sendto;  // UDP sendto()后的应答
};


class MServer :public CThreadFuncBase {
public:
	// 如果本地需要跑多个服务器，那么可以先运行一个无参的MServer(); 然后再运行多个有参的并放入一个列表中；
	MServer(const MServerParameter& param);  // 何时设置关键参数，是需要依据个人开发经验和实际需求去调整
	~MServer();
	int Invoke(void* arg);
	int Send(MSOCKET& client, const MBuffer& buffer); // 【暴露给用户的接口，用户去调用的函数】用户去通知服务器（不使用回调函数）
	int SendTo(MSockaddrIn& addr, const MBuffer& buffer);  // UDP的SendTo()只需要addr就可
	int Stop();
private:
	int threadFunc();
	int threadUDPFunc();
	int threadTCPFunc();
private:
	// 这些参数都是需要用户传进来的
	MServerParameter m_params;
	void* m_args;
	CThread m_thread;
	MSOCKET m_sock;
	std::atomic<bool> m_stop;
};

