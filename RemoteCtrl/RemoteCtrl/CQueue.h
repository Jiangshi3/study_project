#pragma once
#include "pch.h"
#include <atomic>
#include <list>
#include "CThread.h"

// ����һ��ģ���࣬Ӧ��д��ͷ�ļ���
template<class T>
class CQueue
{// �̰߳�ȫ�Ķ��У�����IOCPʵ�֣�
public:
	enum {
		QNone,
		QPush,
		QPop,
		QSize,
		QClear
	};
	typedef struct IocpParam {
		size_t nOperator; // ����
		T Data; // ����
		HANDLE hEvent;  // pop������Ҫ��
		IocpParam() {
			nOperator = QNone;
		}
		IocpParam(int op, const T& data, HANDLE hEve = NULL) {
			nOperator = op;
			Data = data;
			hEvent = hEve;
		}
	}PPARAM; // Post Parameter ����Ͷ����Ϣ�Ľṹ��
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
			HANDLE hTemp = m_hCompletionPort;  // �����Ա��
			m_hCompletionPort = NULL;
			CloseHandle(hTemp); 
		}
	}
	bool PushBack(const T& data) {	
		PPARAM* pParam = new PPARAM(QPush, data); // new��post��ȥ�Ͳ��ù���
		if (m_lock) {  // ����һ��������Post�����ܵĽ�
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
		PPARAM Param(QPop, data, hEvent);  // �ֲ�����
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
			data = Param.Data;  // �õ�
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
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;  // WAIT_OBJECT_0���ں˶����Ѿ����� signaled ״̬��
		if (ret) {
			return Param.nOperator;  // �õ�
		}
		return -1;
	}
	bool Clear() {
		PPARAM* pParam = new PPARAM(QClear, T());
		if (m_lock) {
			delete pParam;
			return false;
		}
		// ����û��ʹ��overlapped��ʹ����(ULONG_PTR)pParam��ΪCompletionKey������ӳ��
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
			delete pParam; // QPush��QClear����Ҫdelete
			// printf("delete pParam:%08X\r\n", pParam);
			break;
		case QPop:
			if (m_lstData.size() > 0) {
				pParam->Data = m_lstData.front();
				// printf("before pop_front: m_lstData.front:%s\r\n", m_lstData.front().c_str());
				m_lstData.pop_front();
				if (pParam->hEvent != NULL)
					SetEvent(pParam->hEvent);  // QPop��QSize��Ҫ�����¼�
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
		// �����ԣ��ڶ��߳����ܹ���׳��  �����Ϣ�������滹�У��ͼ�������һ��
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
	std::atomic<bool> m_lock;  // ������������
};


template<class T>
class CSendQueue :public CQueue<T>, public CThreadFuncBase {
public:
	typedef int (CThreadFuncBase::* MYCALLBACK)(T& data);
	// �ص�������ʵ���յ����ݺ�Ҫ��������
	CSendQueue(CThreadFuncBase* obj, MYCALLBACK callback) :CQueue<T>(), m_base(obj), m_callback(callback) 
	{
		m_thread.Start();
		m_thread.UpdateWorker(::CThreadWorker(this, (FUNCTYPE) & CSendQueue<T>::threadTick));
	}
	virtual ~CSendQueue(){ // ������		
		m_base = NULL;
		m_callback = NULL;
		m_thread.Stop();
	}  

protected:
	virtual bool PopFront(T& data) {  
		// TODO������������ӣ���ô����PopFront()����ִ���ĸ��أ� ���϶��Ǹ��ݾ���Ķ�����ִ�ж�Ӧ���麯����
		return false;
	}
	bool PopFront() {
		typename CQueue<T>::PPARAM* Param = new typename CQueue<T>::PPARAM(CQueue<T>::QPop, T());  // TODO: new PPARAM��new IocpParamӦ����һ���ģ� ��ʦ�����õ�IocpParam��������ȷ������
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
		return 0;  // ����0���߳�����ͻ����ִ��
	}
	// TODO: DealParamParam();Ӧ�ò��ᱻִ�е��ɣ�Ӧ��ֻ��ִ�и����DealParam();��Ϊֻ���ڸ�����void threadMain();����GetQueuedCompletionStatus();
	// ans: �ᱻִ�е�; ��������������д��DealParam(); ����void threadMain();����Ҫ��д��ֱ�Ӽ̳й����� c++��̬ѽ���ǣ�
	// ����ʹ��typename�Ǹ��߱������������ģ��<T>����һ����֪�Ĳ�����
	virtual void DealParam(typename CQueue<T>::PPARAM* pParam) {  
		switch (pParam->nOperator) {
		case CQueue<T>::QPush:
			CQueue<T>::m_lstData.push_back(pParam->Data);
			delete pParam; // QPush��QClear����Ҫdelete
			// printf("delete pParam:%08X\r\n", pParam);
			break;
		case CQueue<T>::QPop:
			if (CQueue<T>::m_lstData.size() > 0) {
				pParam->Data = CQueue<T>::m_lstData.front();
				// ����Ļص�����ִ�еķ��͹���
				// ���m_lstData.front()��һ���ܴ�ģ�һ�η����꣬�Ͳ���pop�������Իص������Ĳ������յ���һ�����ã�ÿ�η���һ�أ�����귵��0��
				if ((m_base->*m_callback)(pParam->Data) == 0) {  // m_lstData.front()��ȫ���������֮����pop��
					CQueue<T>::m_lstData.pop_front();
				}				
			}
			delete pParam; // TODO ��Ҫɾ������new��????
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
