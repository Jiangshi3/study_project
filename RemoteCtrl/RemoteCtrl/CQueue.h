#pragma once
#include "pch.h"
#include <atomic>
#include <list>
#include "CThread.h"

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
	virtual ~CQueue() {
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
	virtual bool PopFront(T& data) {
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
		// 这里没有使用overlapped，使用了(ULONG_PTR)pParam作为CompletionKey来进行映射
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false)
			delete pParam;
		return ret;
	}
protected:
	virtual void DealParam(PPARAM* pParam) {
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
protected:
	std::list<T> m_lstData;
	HANDLE m_hCompletionPort;
	HANDLE m_hThread;
	std::atomic<bool> m_lock;  // 队列正在析构
};


template<class T>
class CSendQueue :public CQueue<T>, public CThreadFuncBase {
public:
	typedef int (CThreadFuncBase::* MYCALLBACK)(T& data);
	// 回调函数：实现收到数据后要做的事情
	CSendQueue(CThreadFuncBase* obj, MYCALLBACK callback) :CQueue<T>(), m_base(obj), m_callback(callback) 
	{
		m_thread.Start();
		m_thread.UpdateWorker(::CThreadWorker(this, (FUNCTYPE) & CSendQueue<T>::threadTick));
	}
	virtual ~CSendQueue(){ // 虚析构		
		m_base = NULL;
		m_callback = NULL;
		m_thread.Stop();
	}  

protected:
	virtual bool PopFront(T& data) {  
		// TODO：如果是这样子，那么调用PopFront()将会执行哪个呢？ 【肯定是根据具体的对象来执行对应的虚函数】
		return false;
	}
	bool PopFront() {
		typename CQueue<T>::PPARAM* Param = new typename CQueue<T>::PPARAM(CQueue<T>::QPop, T());  // TODO: new PPARAM和new IocpParam应该是一样的； 老师两处用的IocpParam；【？不确定？】
		if (CQueue<T>::m_lock) {
			delete Param;
			return false;
		}
		bool ret = PostQueuedCompletionStatus(CQueue<T>::m_hCompletionPort, sizeof(*Param), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			delete Param;
		}
		return ret;
	}

protected:
	int threadTick() {
		if (WaitForSingleObject(CQueue<T>::m_hThread, 0) != WAIT_TIMEOUT)
			return 0;
		if (CQueue<T>::m_lstData.size() > 0) {
			PopFront();
		}
		// Sleep(1);
		return 0;  // 返回0，线程里面就会继续执行
	}
	// TODO: DealParamParam();应该不会被执行到吧，应该只会执行父类的DealParam();因为只有在父类中void threadMain();调用GetQueuedCompletionStatus();
	// ans: 会被执行到; 这里是派生类重写了DealParam(); 对于void threadMain();不需要重写，直接继承过来； c++多态呀这是；
	// 这里使用typename是告诉编译器，这里的模板<T>当成一个已知的参数；
	virtual void DealParam(typename CQueue<T>::PPARAM* pParam) {  
		switch (pParam->nOperator) {
		case CQueue<T>::QPush:
			CQueue<T>::m_lstData.push_back(pParam->Data);
			delete pParam; // QPush和QClear才需要delete
			// printf("delete pParam:%08X\r\n", pParam);
			break;
		case CQueue<T>::QPop:
			if (CQueue<T>::m_lstData.size() > 0) {
				pParam->Data = CQueue<T>::m_lstData.front();
				// 这里的回调函数执行的发送功能
				// 如果m_lstData.front()是一个很大的，一次发不完，就不能pop掉，所以回调函数的参数接收的是一个引用，每次发送一截，最后发完返回0；
				if ((m_base->*m_callback)(pParam->Data) == 0) {  // m_lstData.front()被全部发送完成之后再pop掉
					CQueue<T>::m_lstData.pop_front();
				}				
			}
			delete pParam; // TODO 需要删除吗？有new吗????
			break;
		case CQueue<T>::QSize:
			pParam->nOperator = CQueue<T>::m_lstData.size();
			if (pParam->hEvent != NULL)
				SetEvent(pParam->hEvent);
			break;
		case CQueue<T>::QClear:
			CQueue<T>::m_lstData.clear();
			delete pParam;
			break;
		default:
			OutputDebugStringA("unknown operator!\r\n");
			break;
		}
	}
private:
	CThreadFuncBase* m_base;
	MYCALLBACK m_callback;
	CThread m_thread;
};


typedef CSendQueue<std::vector<char>>::MYCALLBACK SENDCALLBACK;
