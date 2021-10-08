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

#pragma once

#include <boost/optional.hpp>

namespace mongo {
namespace repl {


//官网 https://docs.mongodb.com/v4.4/reference/read-concern/
//参考 https://mongoing.com/archives/77853

//注意ReadConcernLevel和ReadSource中的RecoveryUnit::kMajorityCommitted等关联，转换参考getNewReadSource

//最终存储在ReadConcernArgs._level中
enum class ReadConcernLevel {
    //local/available：local 和 available 的语义基本一致，都是读操作直接读取本地最新的数据。但是，available 使用
    // MongoDB 分片集群场景下，含特殊语义（为了保证性能，可以返回孤儿文档），
    //生效见canReadAtLastApplied
    kLocalReadConcern,
    //生效见waitForReadConcernImpl  ReplicationCoordinatorImpl::_waitUntilClusterTimeForRead  computeOperationTime
    kMajorityReadConcern,
    //生效参考service_entry_point_mongod.cpp中的waitForLinearizableReadConcern中调用
    //直接读主节点
    kLinearizableReadConcern,
    //available 使用在分片集群场景下，和kLocalReadConcern功能类似，生效见canReadAtLastApplied
    kAvailableReadConcern,
    kSnapshotReadConcern
};

namespace readConcernLevels {

//官网 https://docs.mongodb.com/v4.4/reference/read-concern/
//参考 https://mongoing.com/archives/77853
constexpr std::initializer_list<ReadConcernLevel> all = {ReadConcernLevel::kLocalReadConcern,
                                                         ReadConcernLevel::kMajorityReadConcern,
                                                         ReadConcernLevel::kLinearizableReadConcern,
                                                         ReadConcernLevel::kAvailableReadConcern,
                                                         ReadConcernLevel::kSnapshotReadConcern};


//官网 https://docs.mongodb.com/v4.2/reference/read-concern/
//参考 https://mongoing.com/archives/77853
constexpr StringData kLocalName = "local"_sd;
constexpr StringData kMajorityName = "majority"_sd;
constexpr StringData kLinearizableName = "linearizable"_sd;
constexpr StringData kAvailableName = "available"_sd;
constexpr StringData kSnapshotName = "snapshot"_sd;

boost::optional<ReadConcernLevel> fromString(StringData levelString);
StringData toString(ReadConcernLevel level);

}  // namespace readConcernLevels

}  // namespace repl
}  // namespace mongo
