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

#include "mongo/db/read_concern.h"
#include "mongo/base/shim.h"
#include "mongo/db/repl/speculative_majority_read_info.h"

namespace mongo {

//注册见setPrepareConflictBehaviorForReadConcernRegistration
void setPrepareConflictBehaviorForReadConcern(OperationContext* opCtx,
                                              const repl::ReadConcernArgs& readConcernArgs,
                                              PrepareConflictBehavior prepareConflictBehavior) {
    static auto w = MONGO_WEAK_FUNCTION_DEFINITION(setPrepareConflictBehaviorForReadConcern);
	//对应setPrepareConflictBehaviorForReadConcernImpl
    return w(opCtx, readConcernArgs, prepareConflictBehavior);
}

//ServiceEntryPointMongod::Hooks中的waitForReadConcern调用
////execCommandDatabase->behaviors.waitForReadConcern调用

//注册见waitForReadConcernRegistration
Status waitForReadConcern(OperationContext* opCtx,
                          const repl::ReadConcernArgs& readConcernArgs,
                          bool allowAfterClusterTime) {
    static auto w = MONGO_WEAK_FUNCTION_DEFINITION(waitForReadConcern);
	//对应waitForReadConcernImpl
    return w(opCtx, readConcernArgs, allowAfterClusterTime);
}

////service_entry_point_mongod.cpp中的ForLinearizableReadConcern中调用
Status waitForLinearizableReadConcern(OperationContext* opCtx, int readConcernTimeout) {
    static auto w = MONGO_WEAK_FUNCTION_DEFINITION(waitForLinearizableReadConcern);
	//对应waitForLinearizableReadConcernImpl
    return w(opCtx, readConcernTimeout);
}

//waitForSpeculativeMajorityReadConcernRegistration中注册
Status waitForSpeculativeMajorityReadConcern(
    OperationContext* opCtx, repl::SpeculativeMajorityReadInfo speculativeReadInfo) {
    static auto w = MONGO_WEAK_FUNCTION_DEFINITION(waitForSpeculativeMajorityReadConcern);
	//对应waitForSpeculativeMajorityReadConcernImpl
    return w(opCtx, speculativeReadInfo);
}

}  // namespace mongo
