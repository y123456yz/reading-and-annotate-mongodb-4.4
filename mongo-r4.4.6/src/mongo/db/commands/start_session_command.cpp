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

#include "mongo/platform/basic.h"

#include "mongo/base/init.h"
#include "mongo/db/auth/action_set.h"
#include "mongo/db/auth/action_type.h"
#include "mongo/db/auth/authorization_manager.h"
#include "mongo/db/auth/authorization_session.h"
#include "mongo/db/auth/privilege.h"
#include "mongo/db/client.h"
#include "mongo/db/commands.h"
#include "mongo/db/jsobj.h"
#include "mongo/db/logical_session_cache.h"
#include "mongo/db/logical_session_id.h"
#include "mongo/db/logical_session_id_helpers.h"
#include "mongo/db/operation_context.h"
#include "mongo/db/stats/top.h"

namespace mongo {
namespace {

/*
// Create collections:
db.getSiblingDB("mydb1").foo.insert( {abc: 0}, { writeConcern: { w: "majority", wtimeout: 2000 } } );
db.getSiblingDB("mydb2").bar.insert( {xyz: 0}, { writeConcern: { w: "majority", wtimeout: 2000 } } );
// Start a session.
session = db.getMongo().startSession( { readPreference: { mode: "primary" } } );
coll1 = session.getDatabase("mydb1").foo;
coll2 = session.getDatabase("mydb2").bar;
// Start a transaction
session.startTransaction( { readConcern: { level: "local" }, writeConcern: { w: "majority" } } );
// Operations inside the transaction
try {
   coll1.insertOne( { abc: 1 } );
   coll2.insertOne( { xyz: 999 } );
} catch (error) {
   // Abort transaction on error
   session.abortTransaction();
   throw error;
}
// Commit the transaction using write concern set at transaction start
session.commitTransaction();
session.endSession();

https://docs.mongodb.com/manual/core/transactions-in-applications/#std-label-txn-mongo-shell-example

//Session lsid 可以通过调用 startSession 命令让 server 端分配，也可以客户端自己分配，这样可以节省一次网络开销；
//参考https://mongoing.com/%3Fp%3D6084

*/
class StartSessionCommand final : public BasicCommand {
    StartSessionCommand(const StartSessionCommand&) = delete;
    StartSessionCommand& operator=(const StartSessionCommand&) = delete;

public:
    StartSessionCommand() : BasicCommand("startSession") {}

    AllowedOnSecondary secondaryAllowed(ServiceContext*) const override {
        return AllowedOnSecondary::kAlways;
    }

    bool adminOnly() const override {
        return false;
    }

    bool supportsWriteConcern(const BSONObj& cmd) const override {
        return false;
    }

    std::string help() const override {
        return "start a logical session";
    }

    Status checkAuthForOperation(OperationContext* opCtx,
                                 const std::string& dbname,
                                 const BSONObj& cmdObj) const override {
        return Status::OK();
    }

    bool run(OperationContext* opCtx,
             const std::string& db,
             const BSONObj& cmdObj,
             BSONObjBuilder& result) override {
        const auto service = opCtx->getServiceContext();
		//获取对应的LogicalSessionCacheImpl
        const auto lsCache = LogicalSessionCache::get(service);

		//生成一个LogicalSessionRecord
        auto newSessionRecord =
            makeLogicalSessionRecord(opCtx, service->getFastClockSource()->now());

		//LogicalSessionCacheImpl::startSession 新session添加到活跃session列表中
        uassertStatusOK(lsCache->startSession(opCtx, newSessionRecord));

		//返回客户端对应的LogicalSessionToClient信息
        makeLogicalSessionToClient(newSessionRecord.getId()).serialize(&result);

        return true;
    }

} startSessionCommand;

}  // namespace
}  // namespace mongo
