#pragma once
#include <atomic>
#include <vector>
#include <mutex>
#include <Windows.h>

#include <varargs.h>
class ETool {
public:
	static void ETrace(const char* format, ...) {
		va_list ap;
		va_start(ap, format);
		std::string sBuffer;
		sBuffer.resize(1024 * 10);
		vsprintf((char*)(sBuffer.c_str()), format, ap);
		OutputDebugStringA(sBuffer.c_str());
		va_end(ap);
	}
};

#ifndef TRACE  //���û�ж���TRACE
#define TRACE ETool::ETrace
#endif

class ThreadFuncBase {};  // ������Ϊһ�����࣬������������������
typedef int (ThreadFuncBase::* FUNCTYPE)();   //����һ������ֵ��int�ĳ�Ա����ָ��

class ThreadWorker {
public:
	//���캯��
	ThreadWorker() :thiz(NULL), func(NULL) {};
	ThreadWorker(void* obj, FUNCTYPE f) 
		:thiz((ThreadFuncBase*)obj), func(f) {}
	ThreadWorker(const ThreadWorker& worker) {  // ���ƹ��죬���ڴ���һ���µ� ThreadWorker ���󣬸��� thiz �� func
		thiz = worker.thiz;
		func = worker.func;
	}
	ThreadWorker& operator=(const ThreadWorker& worker) { //��ֵ��������أ����ڽ�һ�� ThreadWorker�����ֵ���Ƹ���һ������
		if (this != &worker) {
			thiz = worker.thiz;
			func = worker.func;
		}
		return *this;
	}
	//���������������operator()�������غ���������������һ������ThreadWorker�Ķ���
	int operator()() {
		if (IsValid()) {
			return (thiz->*func)();  // ���ó�Ա�����������ؽ��
		}
		return -1;
	}
	bool IsValid() const {
		return (thiz != NULL) && (func != NULL);  // ��Ա�Ƿ�Ϊ�ǿ�
	}
private:
	ThreadFuncBase* thiz;
	FUNCTYPE func;
};

class EdoyunThread
{
public:
	EdoyunThread() {
		m_hThread = NULL;
		m_bStatus = false;
	}

	~EdoyunThread() {
		Stop();
	}

	//true ��ʾ�ɹ� false��ʾʧ��
	bool Start() {
		m_bStatus = true;
		m_hThread = (HANDLE)_beginthread(&EdoyunThread::ThreadEntry, 0, this);
		if (!IsValid()) {
			m_bStatus = false;
		}
		return m_bStatus;
	}

	//����true��ʾ��Ч ����false��ʾ�߳��쳣�����Ѿ���ֹ
	bool IsValid() {
		if (m_hThread == NULL || (m_hThread == INVALID_HANDLE_VALUE))return false;
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;  // ��Ч
	}

	bool Stop() {
		if (m_bStatus == false)return true;
		m_bStatus = false;
		bool ret = WaitForSingleObject(m_hThread, INFINITE) == WAIT_OBJECT_0;
		UpdateWorker();
		return ret;
	}

	void UpdateWorker(const ::ThreadWorker& worker = ::ThreadWorker()) {
		if (m_worker.load() != NULL && m_worker.load() != &worker) {
			::ThreadWorker* pWorker = m_worker.load();
			TRACE("delete pWorker = %08X m_worker = %08X\r\n", pWorker, m_worker.load());
			m_worker.store(NULL);
			delete pWorker;
		}
		if (!worker.IsValid()) {
			m_worker.store(NULL);
			return;
		}
		m_worker.store(new ::ThreadWorker(worker));
	}

	//true��ʾ���� false��ʾ�Ѿ������˹���
	bool IsIdle() {
		if (m_worker == NULL)return true;
		return !m_worker.load()->IsValid();
	}
private:
	void ThreadWorker() {
		while (m_bStatus) {
			if (m_worker == NULL) {
				Sleep(1);
				continue;
			}
			::ThreadWorker worker = *m_worker.load();
			if (worker.IsValid()) {
				int ret = worker(); // ����0����
				if (ret != 0) {
					TRACE("thread found warning code %d\r\n", ret);
				}
				if (ret < 0) {
					m_worker.store(NULL);
				}
			}
			else {
				Sleep(1);
			}
		}
	}
	static void ThreadEntry(void* arg) {
		EdoyunThread* thiz = (EdoyunThread*)arg;
		if (thiz) {
			thiz->ThreadWorker();
		}
		_endthread();
	}
private:
	HANDLE m_hThread;
	bool m_bStatus;//false ��ʾ�߳̽�Ҫ�ر�  true ��ʾ�߳���������
	std::atomic<::ThreadWorker*> m_worker;
};

class EdoyunThreadPool
{
public:
	EdoyunThreadPool(size_t size) {
		m_threads.resize(size);
		for (size_t i = 0; i < size; i++)
			m_threads[i] = new EdoyunThread();
	}
	EdoyunThreadPool() {}
	~EdoyunThreadPool() {
		Stop();
		for (size_t i = 0; i < m_threads.size(); i++)
		{
			EdoyunThread* pThread = m_threads[i];
			m_threads[i] = NULL;
			delete pThread;
		}

		m_threads.clear();
	}
	bool Invoke() {
		// �������ɹ�����ɹ�������Ҫ�رյ�
		bool ret = true;
		for (size_t i = 0; i < m_threads.size(); i++) {
			if (m_threads[i]->Start() == false) {
				ret = false;
				break;
			}
		}
		if (ret == false) {
			for (size_t i = 0; i < m_threads.size(); i++) {
				m_threads[i]->Stop();
			}
		}
		return ret;
	}
	void Stop() {
		for (size_t i = 0; i < m_threads.size(); i++) {
			m_threads[i]->Stop();
		}
	}

	//����-1 ��ʾ����ʧ�ܣ������̶߳���æ ���ڵ���0����ʾ��n���̷߳��������������
	int DispatchWorker(const ThreadWorker& worker) {
		int index = -1;
		m_lock.lock();   // ��֤ÿ��ֻ��һ���̷߳���worker
		for (size_t i = 0; i < m_threads.size(); i++) {
			if (m_threads[i] != NULL && m_threads[i]->IsIdle()) {
				m_threads[i]->UpdateWorker(worker);
				index = i;
				break;
			}
		}
		m_lock.unlock();
		return index;
	}

	bool CheckThreadValid(size_t index) {
		if (index < m_threads.size()) {
			return m_threads[index]->IsValid();
		}
		return false;
	}
private:
	std::mutex m_lock;
	std::vector<EdoyunThread*> m_threads;
};