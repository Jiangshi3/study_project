#include "pch.h"
#include "MNetwork.h"

MServer::MServer(const MServerParameter& param) :m_stop(false), m_args(NULL)
{
	m_params = param; // MServerParameter要实现拷贝赋值
	// m_thread.Start();  // 不在这里启动线程
	m_thread.UpdateWorker(CThreadWorker(this, (FUNCTYPE)&MServer::threadFunc));   //  (FUNCTYPE) & MServer::threadFunc
}

MServer::~MServer()
{
	Stop();
}

int MServer::Invoke(void* arg)
{	
	// 现在是在成员函数、Invoke()阶段；
	m_sock.reset(new MSocket(m_params.m_type));
	if (*m_sock == INVALID_SOCKET) {
		printf("%s(%d)%s socket() ERROR(%d)!\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
		return -1;
	}
	// MSockaddrIn client;
	if (m_sock->bind(m_params.m_ip, m_params.m_port) == -1)
	{
		printf("%s(%d)%s bind() ERROR(%d)!\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
		return -2;
	}
	if (m_params.m_type == MTypeTCP) {
		if (m_sock->listen() == -1)
			return -3;
	}	
	if (m_thread.Start() == false) {
		return -4;
	}
	m_args = arg;
	return 0;
}

int MServer::Send(MSOCKET& client, const MBuffer& buffer)
{
	int ret = m_sock->send(buffer);  // TODO:待优化，可能发送虽然成功，但不完整，没有发送全部；
	if (m_params.m_send) m_params.m_send(m_args, client, ret);  // SendFunc(void* arg, MSOCKET& client, int ret);
	return ret;
}

int MServer::SendTo(MSockaddrIn& addr, const MBuffer& buffer)
{
	int ret = m_sock->sendto(buffer, addr);  // TODO:待优化，可能发送虽然成功，但不完整，没有发送全部；
	if (m_params.m_sendto) m_params.m_sendto(m_args,addr, ret); // SendToFunc(void* arg, const MSockaddrIn& addr, int ret);
	return ret;
}

int MServer::Stop()
{
	if (m_stop == false) {
		m_sock->close();
		m_stop = true;
		m_thread.Stop();
	}
	return 0;
}

int MServer::threadFunc()
{
	if (m_params.m_type == MTypeTCP) {
		return threadTCPFunc();
	}
	else {
		return threadUDPFunc();
	}
}

int MServer::threadUDPFunc()
{
	MBuffer buf(1024 * 256);
	MSockaddrIn client;
	int ret = 0;
	while (!m_stop) {
		ret = m_sock->recvfrom(buf, client);
		if (ret > 0) {
			// client.update(); // 已经在recvfrom()里面调用过了
			// recvfrom()成功后执行对应的回调函数RecvFromFunc;    交给用户去做
			if (m_params.m_recvfrom != NULL) {
				m_params.m_recvfrom(m_args, buf, client);
			}			
		}
		else {
			printf("%s(%d)%s recvfrom() ERROR(%d)! ret=%d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
			break;
		}
	}
	if (m_stop == false) m_stop = true;
	m_sock->close();
	printf("%s(%d)%s quit!\r\n", __FILE__, __LINE__, __FUNCTION__);
	return 0;
}

int MServer::threadTCPFunc()
{
	return 0;
}

/*
if (lstclients.size() <= 0) {  // 对于第一个连接进来的，返回一个ack
				lstclients.push_back(client);
				printf("%s(%d)%s IP:%s port:%d\r\n", __FILE__, __LINE__, __FUNCTION__, client.GetIP().c_str(), client.GetPort());
				ret = sock->sendto(buf, client);
				printf("%s(%d)%s ret=%d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
			}
			else {  // 发送回去struct sockaddr_in
				printf("%s(%d)%s IP:%s port:%d\r\n", __FILE__, __LINE__, __FUNCTION__, client.GetIP().c_str(), client.GetPort());
				buf.Update((void*)&lstclients.front(), lstclients.front().size());
				ret = sock->sendto(buf, client);
				printf("%s(%d)%s ret=%d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
			}
*/

MServerParameter::MServerParameter(
	const std::string& ip, short port, MTYPE type, 
	AcceptFunc acceptf, RecvFunc recvf, SendFunc sendf, 
	RecvFromFunc recvfromf, SendToFunc sendtof
)
{
	m_ip = ip;
	m_port = port;
	m_type = type;
	m_accept = acceptf;
	m_recv = recvf;
	m_send = sendf;
	m_recvfrom = recvfromf;
	m_sendto = sendtof;
}

MServerParameter& MServerParameter::operator<<(AcceptFunc func)
{
	m_accept = func;
	return *this;
}

MServerParameter& MServerParameter::operator<<(RecvFunc func)
{
	m_recv = func;
	return *this;
}

MServerParameter& MServerParameter::operator<<(SendFunc func)
{
	m_send = func;
	return *this;
}

MServerParameter& MServerParameter::operator<<(RecvFromFunc func)
{
	m_recvfrom = func;
	return *this;
}

MServerParameter& MServerParameter::operator<<(SendToFunc func)
{
	m_sendto = func;
	return *this;
}

MServerParameter& MServerParameter::operator<<(const std::string& ip)
{
	m_ip = ip;
	return *this;
}

MServerParameter& MServerParameter::operator<<(short port)
{
	m_port = port;
	return *this;
}

MServerParameter& MServerParameter::operator<<(MTYPE type)
{
	m_type = type;
	return *this;
}


MServerParameter& MServerParameter::operator>>(AcceptFunc& func)
{
	func = m_accept;
	return *this;
}

MServerParameter& MServerParameter::operator>>(RecvFunc& func)
{
	m_recv = func;
	return *this;
}

MServerParameter& MServerParameter::operator>>(SendFunc& func)
{
	m_send = func;
	return *this;
}

MServerParameter& MServerParameter::operator>>(RecvFromFunc& func)
{
	m_recvfrom = func;
	return *this;
}

MServerParameter& MServerParameter::operator>>(SendToFunc& func)
{
	m_sendto = func;
	return *this;
}

MServerParameter& MServerParameter::operator>>(std::string& ip)
{
	ip = m_ip;
	return *this;
}

MServerParameter& MServerParameter::operator>>(short& port)
{
	m_port = port;
	return *this;
}

MServerParameter& MServerParameter::operator>>(MTYPE& type)
{
	m_type = type;
	return *this;
}

MServerParameter::MServerParameter(const MServerParameter& param)
{
	m_ip = param.m_ip;
	m_port = param.m_port;
	m_type = param.m_type;
	m_accept = param.m_accept;
	m_recv = param.m_recv;
	m_send = param.m_send;
	m_recvfrom = param.m_recvfrom;
	m_sendto = param.m_sendto;
}

MServerParameter& MServerParameter::operator=(const MServerParameter& param)
{
	if (this != &param) {
		m_ip = param.m_ip;
		m_port = param.m_port;
		m_type = param.m_type;
		m_accept = param.m_accept;
		m_recv = param.m_recv;
		m_send = param.m_send;
		m_recvfrom = param.m_recvfrom;
		m_sendto = param.m_sendto;
	}
	return *this;	
}
