// Copyright (c) 2015-2015 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "Framework/ThreadEnvironment.hpp"
#include <Poco/Logger.h>
#include <iostream>
#include <cassert>

ThreadEnvironment::ThreadEnvironment(const Pothos::ThreadPoolArgs &args):
    _args(args),
    _waitModeEnabled(_args.yieldMode != "SPIN"),
    _configurationSignature(0)
{
    return;
}

ThreadEnvironment::~ThreadEnvironment(void)
{
    //tear-down all tasks if not done by caller
    while (not _handleToTask.empty())
    {
        this->unregisterTask(_handleToTask.begin()->first);
    }
}

void ThreadEnvironment::registerTask(void *handle, TaskData::Task task, TaskData::Wake wake)
{
    std::lock_guard<std::mutex> lock(_registrationMutex);

    //register the new task and bump the signature to notify threads
    {
        std::lock_guard<std::mutex> lock0(_handleUpdateMutex);
        _handleToTask[handle].reset(new TaskData(task, wake));
    }
    _configurationSignature++;

    //single task mode: spawn a new thread for this task
    if (_args.numThreads == 0)
    {
        _handleToThread[handle] = std::thread(std::bind(&ThreadEnvironment::singleProcessLoop, this, handle));
    }

    //pool mode: start a thread if the pool size is too small
    else
    {
        if (_threadPool.size() < _args.numThreads)
        {
            size_t index = _threadPool.size();
            _threadPool.push_back(std::thread(std::bind(&ThreadEnvironment::poolProcessLoop, this, index)));
        }
        assert(_threadPool.size() <= _args.numThreads);
    }
}

void ThreadEnvironment::unregisterTask(void *handle)
{
    std::lock_guard<std::mutex> lock(_registrationMutex);
    std::shared_ptr<TaskData> data;

    //unregister the new task and bump the signature to notify threads
    {
        std::lock_guard<std::mutex> lock0(_handleUpdateMutex);
        std::swap(data, _handleToTask[handle]);
        _handleToTask.erase(handle);
    }
    _configurationSignature++;

    //wake every known task to accept the new config state
    data->wake();
    for (const auto &pair : _handleToTask) pair.second->wake();

    //single task mode: stop the explicit task for this handle
    if (_args.numThreads == 0)
    {
        _handleToThread[handle].join();
        _handleToThread.erase(handle);
    }

    //pool mode: stop a thread if the pool size is too large
    else
    {
        if (_threadPool.size() > _handleToTask.size())
        {
            _threadPool.back().join();
            _threadPool.resize(_handleToTask.size());
        }
        assert(_threadPool.size() <= _args.numThreads);
    }

    //wait for all threads to relinquish the old configuration
    while (not data.unique()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

/*!
 * Call wake on all tasks that are busy:
 * Busy tasks will fail the test and set and may be in a CV wait state.
 * Do not call wake on the caller task since that would have no effect.
 * \param tasks a map of all tasks from the caller task
 * \param self the caller task itself to avoid self-wake
 */
template <typename TasksType>
void wakeAllBusyTasks(const TasksType &tasks, void *self)
{
    for (const auto &task : tasks)
    {
        if (task.first == self) continue;

        //if we fail to get the lock, a thread may be waiting
        if (task.second->flag.test_and_set(std::memory_order_acquire))
        {
            task.second->wake();
        }
        //got the lock, so there is nothing to wake -> unlock
        else
        {
            task.second->flag.clear(std::memory_order_release);
        }
    }
}

/*!
 * Thread pool wait and wake-up mechanics:
 * The goal is to ensure that when wait mode is enabled,
 * threads are not allowed to wait in a task when another
 * task is immediately capable of performing useful work.
 *
 * Threads are given a chance to wait in a task only after
 * failing to acquire N times, where N is the number of tasks.
 * Once a task is successfully acquired and completed,
 * or once the attempted acquisition with "wait" returns,
 * the failure count reset and the mechanism begins again.
 *
 * After a thread successfully acquires the task context,
 * it must wake up all other potentially waiting threads.
 * This ensures that threads will be available to process
 * new tasks that are capable of performing useful work.
 */

void ThreadEnvironment::poolProcessLoop(size_t index)
{
    this->applyThreadConfig();
    size_t failAcquireCount = 0;
    size_t localSignature = 0;
    std::map<void *, std::shared_ptr<TaskData>> localTasks;
    auto it = localTasks.end();

    while (true)
    {
        //check for a configuration change and update the local state
        if (_configurationSignature != localSignature)
        {
            std::lock_guard<std::mutex> lock(_handleUpdateMutex);
            localTasks = _handleToTask;
            it = localTasks.end();
            localSignature = _configurationSignature;
            failAcquireCount = 0; //reset fail count
        }

        //pool mode, index out of range
        if (index >= localTasks.size()) return;

        //perform a task and increment
        if (it == localTasks.end()) it = localTasks.begin();
        if (not it->second->flag.test_and_set(std::memory_order_acquire))
        {
            const bool waitOnce = _waitModeEnabled and failAcquireCount >= localTasks.size();
            if (it->second->task(waitOnce))
            {
                //the task was successfully executed, wake all other potential blockers
                if (_waitModeEnabled) wakeAllBusyTasks(localTasks, it->first);
                failAcquireCount = 0; //reset fail count
            }
            else failAcquireCount++;
            if (waitOnce) failAcquireCount = 0; //reset fail count
            it->second->flag.clear(std::memory_order_release);
        }
        else failAcquireCount++;
        it++;
    }
}

void ThreadEnvironment::singleProcessLoop(void *handle)
{
    this->applyThreadConfig();
    size_t localSignature = 0;
    std::map<void *, std::shared_ptr<TaskData>> localTasks;
    auto it = localTasks.end();

    while (true)
    {
        //check for a configuration change and update the local state
        if (_configurationSignature != localSignature)
        {
            std::lock_guard<std::mutex> lock(_handleUpdateMutex);
            localTasks = _handleToTask;
            it = localTasks.find(handle);
            localSignature = _configurationSignature;
        }

        //handle mode, handle not in tasks
        if (it == localTasks.end()) return;

        //perform the task
        it->second->task(_waitModeEnabled);
    }
}

void ThreadEnvironment::applyThreadConfig(void)
{
    //set priority -- log message only on first failure
    {
        const auto errorMsg = ThreadEnvironment::setPriority(_args.priority);
        static bool showErrorMsg = true;
        if (not errorMsg.empty() and showErrorMsg)
        {
            showErrorMsg = false;
            poco_error_f1(Poco::Logger::get("Pothos.ThreadPool"), "Failed to set thread priority %s", errorMsg);
        }
    }

    //set CPU affinity -- log message only on first failure
    if (_args.affinityMode == "CPU")
    {
        const auto errorMsg = ThreadEnvironment::setCPUAffinity(_args.affinity);
        static bool showErrorMsg = true;
        if (not errorMsg.empty() and showErrorMsg)
        {
            showErrorMsg = false;
            poco_error_f1(Poco::Logger::get("Pothos.ThreadPool"), "Failed to set CPU affinity %s", errorMsg);
        }
    }

    //set NUMA affinity -- log message only on first failure
    if (_args.affinityMode == "NUMA")
    {
        const auto errorMsg = ThreadEnvironment::setNodeAffinity(_args.affinity);
        static bool showErrorMsg = true;
        if (not errorMsg.empty() and showErrorMsg)
        {
            showErrorMsg = false;
            poco_error_f1(Poco::Logger::get("Pothos.ThreadPool"), "Failed to set NUMA affinity %s", errorMsg);
        }
    }
}
