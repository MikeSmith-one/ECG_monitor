#pragma once

#include <QQueue>
#include <QMutex>
#include <QMutexLocker>

template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;
    ~ThreadSafeQueue() = default;

    //存入数据（串口子线程调用）
    void enqueue(const T& item) {
        QMutexLocker locker(&m_mutex);
        m_queue.enqueue(item);
    }

    //取出数据（UI主线程/数据库线程调用）
    bool dequeue(T& item) {
        QMutexLocker locker(&m_mutex);
        if (m_queue.isEmpty()) {
            return false;
        }
        item = m_queue.dequeue();
        return true;
    }

    bool isEmpty() const {
        QMutexLocker locker(&m_mutex);
        return m_queue.isEmpty();
    }

    int size() const {
        QMutexLocker locker(&m_mutex);
        return m_queue.size();
    }

    void clear() {
        QMutexLocker locker(&m_mutex);
        m_queue.clear();
    }

private:
    QQueue<T> m_queue;
    mutable QMutex m_mutex;
};