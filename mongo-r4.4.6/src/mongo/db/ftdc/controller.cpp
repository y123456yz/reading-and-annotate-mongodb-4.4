/**
 *    Copyright (C) 2018-present MongoDB, Inc.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the Server Side Public License, version 1,
 *    as published by MongoDB, Inc.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    Server Side Public License for more details.
 *
 *    You should have received a copy of the Server Side Public License
 *    along with this program. If not, see
 *    <http://www.mongodb.com/licensing/server-side-public-license>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the Server Side Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#define MONGO_LOGV2_DEFAULT_COMPONENT ::mongo::logv2::LogComponent::kFTDC

#include "mongo/platform/basic.h"

#include "mongo/db/ftdc/controller.h"

#include <memory>

#include "mongo/db/client.h"
#include "mongo/db/ftdc/collector.h"
#include "mongo/db/ftdc/util.h"
#include "mongo/db/jsobj.h"
#include "mongo/logv2/log.h"
#include "mongo/platform/mutex.h"
#include "mongo/stdx/condition_variable.h"
#include "mongo/stdx/thread.h"
#include "mongo/util/concurrency/idle_thread_block.h"
#include "mongo/util/exit.h"
#include "mongo/util/time_support.h"

namespace mongo {
/*
# Copyright (C) 2018-present MongoDB, Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the Server Side Public License, version 1,
# as published by MongoDB, Inc.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# Server Side Public License for more details.
#
# You should have received a copy of the Server Side Public License
# along with this program. If not, see
# <http://www.mongodb.com/licensing/server-side-public-license>.
#
# As a special exception, the copyright holders give permission to link the
# code of portions of this program with the OpenSSL library under certain
# conditions as described in each individual source file and distribute
# linked combinations including the program with the OpenSSL library. You
# must comply with the Server Side Public License in all respects for
# all of the code used other than as permitted herein. If you modify file(s)
# with this exception, you may extend this exception to your version of the
# file(s), but you are not obligated to do so. If you do not wish to do so,
# delete this exception statement from your version. If you delete this
# exception statement from all source files in the program, then also delete
# it in the license file.
#

global:
  cpp_namespace: "mongo"
  cpp_includes:
    - "mongo/db/ftdc/ftdc_server.h"

imports:
  - "mongo/idl/basic_types.idl"

server_parameters:
  //是否使能diagnosticDataCollectionEnabled功能
  diagnosticDataCollectionEnabled:
    description: "Determines whether to enable the collecting and logging of data for diagnostic purposes"
    set_at: [startup, runtime]
    cpp_varname: "ftdcStartupParams.enabled"
    on_update: "onUpdateFTDCEnabled"

  //默认1000
  diagnosticDataCollectionPeriodMillis:
    description: "Specifies the interval, in milliseconds, at which to collect diagnostic data."
    set_at: [startup, runtime]
    cpp_varname: "ftdcStartupParams.periodMillis"
    on_update: "onUpdateFTDCPeriod"
    validator:
        gte: 100

  //diagnose目录最大磁盘消耗200M
  diagnosticDataCollectionDirectorySizeMB:
    description: "Specifies the maximum size, in megabytes, of the diagnostic.data directory"
    set_at: [startup, runtime]
    cpp_varname: "ftdcStartupParams.maxDirectorySizeMB"
    on_update: "onUpdateFTDCDirectorySize"
    validator:
        gte: 10

  //单个文件最大10M
  diagnosticDataCollectionFileSizeMB:
    description: Specifies the maximum size, in megabytes, of each diagnostic file"
    set_at: [startup, runtime]
    cpp_varname: "ftdcStartupParams.maxFileSizeMB"
    on_update: "onUpdateFTDCFileSize"
    validator:
        gte: 1

  //默认2
  diagnosticDataCollectionSamplesPerChunk:
    description: "Internal, Specifies the number of samples per diagnostic archive chunk"
    set_at: [startup, runtime]
    cpp_varname: "ftdcStartupParams.maxSamplesPerArchiveMetricChunk"
    on_update: "onUpdateFTDCSamplesPerChunk"
    validator:
        gte: 2

  //默认10
  diagnosticDataCollectionSamplesPerInterimUpdate:
    description: "Internal, Specifies the number of samples per diagnostic interim update"
    set_at: [startup, runtime]
    cpp_varname: "ftdcStartupParams.maxSamplesPerInterimMetricChunk"
    on_update: "onUpdateFTDCPerInterimUpdate"
    validator:
        gte: 2

  diagnosticDataCollectionDirectoryPath:
    description: "Specify the directory for the diagnostic data directory."
    set_at: [startup, runtime]
    cpp_class: DiagnosticDataCollectionDirectoryPathServerParameter

  //默认false
  diagnosticDataCollectionEnableLatencyHistograms:
    description: "Enable the capture of opLatencies: { histograms: true } } in FTDC."
    set_at: [startup, runtime]
    cpp_vartype: 'AtomicWord<bool>'
    cpp_varname: gDiagnosticDataCollectionEnableLatencyHistograms

  //默认false  是否启用诊断tcmalloc
  diagnosticDataCollectionVerboseTCMalloc:
     description: "Enable the capture of verbose tcmalloc in FTDC."
     set_at: [startup, runtime]
     cpp_vartype: 'AtomicWord<bool>'
     cpp_varname: gDiagnosticDataCollectionVerboseTCMalloc

以上参数可配置，例如
db.adminCommand({setParameter: 1, diagnosticDataCollectionSamplesPerChunk: 300})
db.runCommand( { getParameter: 1, diagnosticDataCollectionSamplesPerChunk:1} )



diagnosticDataCollectionEnabled
diagnosticDataCollectionDirectoryPath
diagnosticDataCollectionDirectorySizeMB
diagnosticDataCollectionFileSizeMB
diagnosticDataCollectionPeriodMillis
//https://docs.mongodb.com/manual/reference/parameters/
*/

Status FTDCController::setEnabled(bool enabled) {
    stdx::lock_guard<Latch> lock(_mutex);

    if (_path.empty()) {
        return Status(ErrorCodes::FTDCPathNotSet,
                      str::stream() << "FTDC cannot be enabled without setting the set parameter "
                                       "'diagnosticDataCollectionDirectoryPath' first.");
    }

    _configTemp.enabled = enabled;
    _condvar.notify_one();

    return Status::OK();
}

void FTDCController::setPeriod(Milliseconds millis) {
    stdx::lock_guard<Latch> lock(_mutex);
    _configTemp.period = millis;
    _condvar.notify_one();
}

void FTDCController::setMaxDirectorySizeBytes(std::uint64_t size) {
    stdx::lock_guard<Latch> lock(_mutex);
    _configTemp.maxDirectorySizeBytes = size;
    _condvar.notify_one();
}

void FTDCController::setMaxFileSizeBytes(std::uint64_t size) {
    stdx::lock_guard<Latch> lock(_mutex);
    _configTemp.maxFileSizeBytes = size;
    _condvar.notify_one();
}

void FTDCController::setMaxSamplesPerArchiveMetricChunk(size_t size) {
    stdx::lock_guard<Latch> lock(_mutex);
    _configTemp.maxSamplesPerArchiveMetricChunk = size;
    _condvar.notify_one();
}

//diagnosticDataCollectionSamplesPerInterimUpdate配置，metrics.interim文件跟新的时间
void FTDCController::setMaxSamplesPerInterimMetricChunk(size_t size) {
    stdx::lock_guard<Latch> lock(_mutex);
    _configTemp.maxSamplesPerInterimMetricChunk = size;
    _condvar.notify_one();
}

Status FTDCController::setDirectory(const boost::filesystem::path& path) {
    stdx::lock_guard<Latch> lock(_mutex);

    if (!_path.empty()) {
        return Status(ErrorCodes::FTDCPathAlreadySet,
                      str::stream() << "FTDC path has already been set to '" << _path.string()
                                    << "'. It cannot be changed.");
    }

    _path = path;

    // Do not notify for the change since it has to be enabled via setEnabled.

    return Status::OK();
}

//registerMongoDCollectors  registerMongoSCollectors
void FTDCController::addPeriodicCollector(std::unique_ptr<FTDCCollectorInterface> collector) {
    {
        stdx::lock_guard<Latch> lock(_mutex);
        invariant(_state == State::kNotStarted);

        _periodicCollectors.add(std::move(collector));
    }
}

//startFTDC
void FTDCController::addOnRotateCollector(std::unique_ptr<FTDCCollectorInterface> collector) {
    {
        stdx::lock_guard<Latch> lock(_mutex);
        invariant(_state == State::kNotStarted);

        _rotateCollectors.add(std::move(collector));
    }
}

//// db.runCommand({getDiagnosticData:1})
BSONObj FTDCController::getMostRecentPeriodicDocument() {
    {
        stdx::lock_guard<Latch> lock(_mutex);
        return _mostRecentPeriodicDocument.getOwned();
    }
}

//startFTDC
void FTDCController::start() {
    LOGV2(20625,
          "Initializing full-time diagnostic data capture",
          "dataDirectory"_attr = _path.generic_string());

    // Start the thread
    _thread = stdx::thread([this] { doLoop(); });

    {
        stdx::lock_guard<Latch> lock(_mutex);

        invariant(_state == State::kNotStarted);
        _state = State::kStarted;
    }
}

void FTDCController::stop() {
    LOGV2(20626, "Shutting down full-time diagnostic data capture");

    {
        stdx::lock_guard<Latch> lock(_mutex);

        bool started = (_state == State::kStarted);

        invariant(_state == State::kNotStarted || _state == State::kStarted);

        if (!started) {
            _state = State::kDone;
            return;
        }

        _configTemp.enabled = false;
        _state = State::kStopRequested;

        // Wake up the thread if sleeping so that it will check if we are done
        _condvar.notify_one();
    }

    _thread.join();

    _state = State::kDone;

    if (_mgr) {
        auto s = _mgr->close();
        if (!s.isOK()) {
            LOGV2(20627,
                  "Failed to close full-time diagnostic data capture file manager",
                  "error"_attr = s);
        }
    }
}

//FTDCController::start()
void FTDCController::doLoop() noexcept {
    // Note: All exceptions thrown in this loop are considered process fatal. The default terminate
    // is used to provide a good stack trace of the issue.
    Client::initThread(kFTDCThreadName);
    Client* client = &cc();

    // Update config
    {
        stdx::lock_guard<Latch> lock(_mutex);
        _config = _configTemp;
    }

    while (true) {
        // Compute the next interval to run regardless of how we were woken up
        // Skipping an interval due to a race condition with a config signal is harmless.
        auto now = getGlobalServiceContext()->getPreciseClockSource()->now();

        // Get next time to run at
        //diagnosticDataCollectionPeriodMillis配置，默认一分钟，ftdc线程主循环体采样定时周期
        auto next_time = FTDCUtil::roundTime(now, _config.period);

        // Wait for the next run or signal to shutdown
        {
            stdx::unique_lock<Latch> lock(_mutex);
            MONGO_IDLE_THREAD_BLOCK;

            // We ignore spurious wakeups by just doing an iteration of the loop
            //类似定时器，如果由参数配置则触发返回，或者定时时间到返回
            auto status = _condvar.wait_until(lock, next_time.toSystemTimePoint());

            // Are we done running?
            if (_state == State::kStopRequested) {
                break;
            }

            // Update the current configuration settings always
            // In unit tests, we may never get a signal when the timeout is 1ms on Windows since
            // MSVC 2013 converts wait_until(now() + 1ms) into ~ wait_for(0) which means it will
            // not wait for the condition variable to be signaled because it uses
            // GetFileSystemTime for now which has ~10 ms granularity.
            _config = _configTemp;

            // if we hit a timeout on the condvar, we need to do another collection
            // if we were signalled, then we have a config update only or were asked to stop
            //如果是修改了配置参数引起的事件，则暂时忽略，继续通过_condvar.wait_until等待超时事件到
            if (status == stdx::cv_status::no_timeout) {
                continue;
            }
        }

        // TODO: consider only running this thread if we are enabled
        // for now, we just keep an idle thread as it is simpler
        //ftdc通过diagnosticDataCollectionEnabled使能
        if (_config.enabled) {
            // Delay initialization of FTDCFileManager until we are sure the user has enabled
            // FTDC
            if (!_mgr) {
                auto swMgr = FTDCFileManager::create(&_config, _path, &_rotateCollectors, client);

                _mgr = uassertStatusOK(std::move(swMgr));
            }
			LOGV2(210627,
                  "FTDCController::doLoop() yang test ...");
			//FTDCCollectorCollection::collect        std::tuple<BSONObj, Date_t>的类型为collectSample
            auto collectSample = _periodicCollectors.collect(client);

            Status s = _mgr->writeSampleAndRotateIfNeeded(
                client, std::get<0>(collectSample), std::get<1>(collectSample));

            uassertStatusOK(s);

            // Store a reference to the most recent document from the periodic collectors
            {
                stdx::lock_guard<Latch> lock(_mutex);
				//也就是最近一次获取的全量诊断信息
                _mostRecentPeriodicDocument = std::get<0>(collectSample);
            }
        }
    }
}

}  // namespace mongo
