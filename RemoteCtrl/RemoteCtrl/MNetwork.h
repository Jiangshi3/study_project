#pragma once
#include "MSocket.h"
#include "CThread.h"

// �ɷ�����ȥ֪ͨ�û����ûص�������
typedef int(*AcceptFunc)(void* arg, MSOCKET& client);
typedef int(*RecvFunc)(void* arg, const MBuffer& buffer);
typedef int(*SendFunc)(void* arg, MSOCKET& client, int ret);  // :���ͳɹ��Ժ�ȥ֪ͨ�û���Ӧ�𣻻ص�����
typedef int(*RecvFromFunc)(void* arg, const MBuffer& buffer, MSockaddrIn& addr);  // UDP
typedef int(*SendToFunc)(void* arg, const MSockaddrIn& addr, int ret);  // UDP sendto()���Ӧ��;�ص�����

class MNetwork
{
};

// ��������ȥʹ�ò���
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
	// ����     TODO��Ӧ����д���ˣ����������
	MServerParameter& operator<<(AcceptFunc func);
	MServerParameter& operator<<(RecvFunc func);
	MServerParameter& operator<<(SendFunc func);
	MServerParameter& operator<<(RecvFromFunc func);
	MServerParameter& operator<<(SendToFunc func);
	MServerParameter& operator<<(const std::string& ip);
	MServerParameter& operator<<(short port);
	MServerParameter& operator<<(MTYPE type);
	// ���     TODO��Ӧ����д���ˣ�����������
	MServerParameter& operator>>(AcceptFunc& func);
	MServerParameter& operator>>(RecvFunc& func);
	MServerParameter& operator>>(SendFunc& func);
	MServerParameter& operator>>(RecvFromFunc& func);
	MServerParameter& operator>>(SendToFunc& func);
	MServerParameter& operator>>(std::string& ip);
	MServerParameter& operator>>(short& port);
	MServerParameter& operator>>(MTYPE& type);
	// �������죻 ����ͬ���͵ĸ�ֵ
	MServerParameter(const MServerParameter& param);
	// ������ֵ
	MServerParameter& operator=(const MServerParameter& param);

	std::string m_ip;
	short m_port;
	MTYPE m_type;
	AcceptFunc m_accept;
	RecvFunc m_recv;
	SendFunc m_send;   // send()���Ӧ��
	RecvFromFunc m_recvfrom;
	SendToFunc m_sendto;  // UDP sendto()���Ӧ��
};


class MServer :public CThreadFuncBase {
public:
	// ���������Ҫ�ܶ������������ô����������һ���޲ε�MServer(); Ȼ�������ж���вεĲ�����һ���б��У�
	MServer(const MServerParameter& param);  // ��ʱ���ùؼ�����������Ҫ���ݸ��˿��������ʵ������ȥ����
	~MServer();
	int Invoke(void* arg);
	int Send(MSOCKET& client, const MBuffer& buffer); // ����¶���û��Ľӿڣ��û�ȥ���õĺ������û�ȥ֪ͨ����������ʹ�ûص�������
	int SendTo(MSockaddrIn& addr, const MBuffer& buffer);  // UDP��SendTo()ֻ��Ҫaddr�Ϳ�
	int Stop();
private:
	int threadFunc();
	int threadUDPFunc();
	int threadTCPFunc();
private:
	// ��Щ����������Ҫ�û���������
	MServerParameter m_params;
	void* m_args;
	CThread m_thread;
	MSOCKET m_sock;
	std::atomic<bool> m_stop;
};

