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

#ifndef TRACE  //如果没有定义TRACE
#define TRACE ETool::ETrace
#endif

class ThreadFuncBase {};  // 可以作为一个基类，用作派生其他类的起点
typedef int (ThreadFuncBase::* FUNCTYPE)();   //定义一个返回值是int的成员函数指针

class ThreadWorker {
public:
	//构造函数
	ThreadWorker() :thiz(NULL), func(NULL) {};
	ThreadWorker(void* obj, FUNCTYPE f) 
		:thiz((ThreadFuncBase*)obj), func(f) {}
	ThreadWorker(const ThreadWorker& worker) {  // 复制构造，用于创建一个新的 ThreadWorker 对象，复制 thiz 和 func
		thiz = worker.thiz;
		func = worker.func;
	}
	ThreadWorker& operator=(const ThreadWorker& worker) { //赋值运算符重载，用于将一个 ThreadWorker对象的值复制给另一个对象。
		if (this != &worker) {
			thiz = worker.thiz;
			func = worker.func;
		}
		return *this;
	}
	//函数调用运算符（operator()）的重载函数，允许你像函数一样调用ThreadWorker的对象
	int operator()() {
		if (IsValid()) {
			return (thiz->*func)();  // 调用成员函数，并返回结果
		}
		return -1;
	}
	bool IsValid() const {
		return (thiz != NULL) && (func != NULL);  // 成员是否为非空
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

	//true 表示成功 false表示失败
	bool Start() {
		m_bStatus = true;
		m_hThread = (HANDLE)_beginthread(&EdoyunThread::ThreadEntry, 0, this);
		if (!IsValid()) {
			m_bStatus = false;
		}
		return m_bStatus;
	}

	//返回true表示有效 返回false表示线程异常或者已经终止
	bool IsValid() {
		if (m_hThread == NULL || (m_hThread == INVALID_HANDLE_VALUE))return false;
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;  // 有效
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

	//true表示空闲 false表示已经分配了工作
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
				int ret = worker(); // 等于0正常
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
	bool m_bStatus;//false 表示线程将要关闭  true 表示线程正在运行
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
		// 都启动成功才算成功，否则要关闭掉
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

	//返回-1 表示分配失败，所有线程都在忙 大于等于0，表示第n个线程分配来做这个事情
	int DispatchWorker(const ThreadWorker& worker) {
		int index = -1;
		m_lock.lock();   // 保证每次只给一个线程分配worker
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