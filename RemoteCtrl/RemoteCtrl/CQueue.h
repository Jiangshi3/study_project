#pragma once
#include "pch.h"
#include <atomic>
#include <list>

// 对于一个模板类，应该写在头文件中
template<class T>
class CQueue
{// 线程安全的队列（利用IOCP实现）
public:
	enum {
		QNone,
		QPush,
		QPop,
		QSize,
		QClear
	};
	typedef struct IocpParam {
		size_t nOperator; // 操作
		T Data; // 数据
		HANDLE hEvent;  // pop操作需要的
		IocpParam() {
			nOperator = QNone;
		}
		IocpParam(int op, const T& data, HANDLE hEve = NULL) {
			nOperator = op;
			Data = data;
			hEvent = hEve;
		}
	}PPARAM; // Post Parameter 用于投递信息的结构体
public:
	CQueue() {
		m_lock = false;
		m_hCompletionPort= CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
		m_hThread = INVALID_HANDLE_VALUE;
		if (m_hCompletionPort != NULL) {
			m_hThread = (HANDLE)_beginthread(&CQueue<T>::threadEntry, 0, this);
		}
	}
	~CQueue() {
		if (m_lock) return;
		m_lock = true;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL);
		WaitForSingleObject(m_hThread, INFINITE);
		if(m_hCompletionPort!=NULL){
			HANDLE hTemp = m_hCompletionPort;  // 防御性编程
			m_hCompletionPort = NULL;
			CloseHandle(hTemp); 
		}
	}
	bool PushBack(const T& data) {	
		PPARAM* pParam = new PPARAM(QPush, data); // new，post出去就不用管了
		if (m_lock) {  // 让这一步与下面Post尽可能的近
			delete pParam;
			return false;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false) 
			delete pParam;
		// printf("push back done! ret:%d, pParam:%08X\r\n", ret, pParam);
		// printf("push back done! ret:%d, pParam:%08p\r\n", ret, (void*)pParam);
		return ret;
	}
	bool PopFront(T& data) {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		PPARAM Param(QPop, data, hEvent);  // 局部变量
		if (m_lock) {
			CloseHandle(hEvent);
			return false;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return false;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret) {
			data = Param.Data;  // 拿到
		}
		return ret;
	}
	size_t Size() {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		PPARAM Param(QSize, T(), hEvent);
		if (m_lock) {
			CloseHandle(hEvent);
			return -1;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return -1;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;  // WAIT_OBJECT_0：内核对象已经进入 signaled 状态。
		if (ret) {
			return Param.nOperator;  // 拿到
		}
		return -1;
	}
	bool Clear() {
		PPARAM* pParam = new PPARAM(QClear, T());
		if (m_lock) {
			delete pParam;
			return false;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false)
			delete pParam;
		return ret;
	}
private:
	void DealParam(PPARAM* pParam) {
		switch (pParam->nOperator) {
		case QPush:
			m_lstData.push_back(pParam->Data);
			delete pParam; // QPush和QClear才需要delete
			// printf("delete pParam:%08X\r\n", pParam);
			break;
		case QPop:
			if (m_lstData.size() > 0) {
				pParam->Data = m_lstData.front();
				// printf("before pop_front: m_lstData.front:%s\r\n", m_lstData.front().c_str());
				m_lstData.pop_front();
				if (pParam->hEvent != NULL)
					SetEvent(pParam->hEvent);  // QPop和QSize需要设置事件
			}
			break;
		case QSize:
			pParam->nOperator = m_lstData.size();
			if (pParam->hEvent != NULL)
				SetEvent(pParam->hEvent);
			break;
		case QClear:
			m_lstData.clear();
			delete pParam;
			break;
		default:
			OutputDebugStringA("unknown operator!\r\n");
			break;
		}
	}
	static void threadEntry(void* arg) {
		CQueue<T>* thiz = (CQueue<T>*) arg;
		thiz->threadMain();
		_endthread();
	}

	void threadMain() {
		PPARAM* pParam = NULL;
		DWORD dwTransferred = 0;
		ULONG_PTR CompletionKey = 0;
		OVERLAPPED* pOverlapped = NULL;
		while (GetQueuedCompletionStatus(m_hCompletionPort, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE)) 
		{
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {
				printf("thread is prepare to exit!\r\n");
				break;
			}
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);			
		}
		// 防御性；在多线程中能够健壮；  如果消息队列里面还有，就继续处理一下
		while (GetQueuedCompletionStatus(m_hCompletionPort, &dwTransferred, &CompletionKey, &pOverlapped, 0)) 
		{
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {
				printf("thread is prepare to exit!\r\n");
				continue;
			}
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}
		HANDLE hTemp = m_hCompletionPort;
		m_hCompletionPort = NULL;
		CloseHandle(hTemp);
	}
private:
	std::list<T> m_lstData;
	HANDLE m_hCompletionPort;
	HANDLE m_hThread;
	std::atomic<bool> m_lock;  // 队列正在析构
};

