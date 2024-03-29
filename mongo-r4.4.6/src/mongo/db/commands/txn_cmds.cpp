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

#define MONGO_LOGV2_DEFAULT_COMPONENT ::mongo::logv2::LogComponent::kTransaction

#include "mongo/platform/basic.h"

#include "mongo/bson/bsonobj.h"
#include "mongo/bson/bsonobjbuilder.h"
#include "mongo/db/commands.h"
#include "mongo/db/commands/txn_cmds_gen.h"
#include "mongo/db/curop_failpoint_helpers.h"
#include "mongo/db/op_observer.h"
#include "mongo/db/operation_context.h"
#include "mongo/db/repl/repl_client_info.h"
#include "mongo/db/s/sharding_state.h"
#include "mongo/db/s/transaction_coordinator_service.h"
#include "mongo/db/service_context.h"
#include "mongo/db/transaction_participant.h"
#include "mongo/db/transaction_validation.h"
#include "mongo/logv2/log.h"

namespace mongo {
namespace {

MONGO_FAIL_POINT_DEFINE(participantReturnNetworkErrorForAbortAfterExecutingAbortLogic);
MONGO_FAIL_POINT_DEFINE(participantReturnNetworkErrorForCommitAfterExecutingCommitLogic);
MONGO_FAIL_POINT_DEFINE(hangBeforeCommitingTxn);
MONGO_FAIL_POINT_DEFINE(hangBeforeAbortingTxn);
// TODO SERVER-39704: Remove this fail point once the router can safely retry within a transaction
// on stale version and snapshot errors.
MONGO_FAIL_POINT_DEFINE(dontRemoveTxnCoordinatorOnAbort);


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

class CmdCommitTxn : public BasicCommand {
public:
    CmdCommitTxn() : BasicCommand("commitTransaction") {}

    AllowedOnSecondary secondaryAllowed(ServiceContext*) const override {
        return AllowedOnSecondary::kNever;
    }

    virtual bool adminOnly() const {
        return true;
    }

    bool supportsWriteConcern(const BSONObj& cmd) const override {
        return true;
    }

    std::string help() const override {
        return "Commits a transaction";
    }

    Status checkAuthForOperation(OperationContext* opCtx,
                                 const std::string& dbname,
                                 const BSONObj& cmdObj) const override {
        return Status::OK();
    }

	//CmdCommitTxn::run
    bool run(OperationContext* opCtx,
             const std::string& dbname,
             const BSONObj& cmdObj,
             BSONObjBuilder& result) override {
        IDLParserErrorContext ctx("commitTransaction");
		//解析出请求中的CommitTransaction信息
        auto cmd = CommitTransaction::parse(ctx, cmdObj);
		//获取TransactionParticipant，操作上下文和Participant一一对应
        auto txnParticipant = TransactionParticipant::get(opCtx);
        uassert(ErrorCodes::CommandFailed,
                "commitTransaction must be run within a transaction",
                txnParticipant);

        LOGV2_DEBUG(20507,
                    3,
                    "Received commitTransaction for transaction with txnNumber "
                    "{txnNumber} on session {sessionId}",
                    "Received commitTransaction",
                    "txnNumber"_attr = opCtx->getTxnNumber(),
                    "sessionId"_attr = opCtx->getLogicalSessionId()->toBSON());

        // commitTransaction is retryable.
        //参考https://docs.mongodb.com/manual/core/transactions-in-applications/#std-label-txn-mongo-shell-example
        //  中的客户端CommitWithRetry重试
        if (txnParticipant.transactionIsCommitted()) {
            // We set the client last op to the last optime observed by the system to ensure that
            // we wait for the specified write concern on an optime greater than or equal to the
            // commit oplog entry.
            auto& replClient = repl::ReplClientInfo::forClient(opCtx->getClient());
            replClient.setLastOpToSystemLastOpTime(opCtx);
            if (MONGO_unlikely(
                    participantReturnNetworkErrorForCommitAfterExecutingCommitLogic.shouldFail())) {
                uasserted(ErrorCodes::HostUnreachable,
                          "returning network error because failpoint is on");
            }

            return true;
        }

        uassert(ErrorCodes::NoSuchTransaction,
                "Transaction isn't in progress",
                txnParticipant.transactionIsOpen());

        CurOpFailpointHelpers::waitWhileFailPointEnabled(
            &hangBeforeCommitingTxn, opCtx, "hangBeforeCommitingTxn");

		//CommitTransaction::getCommitTimestamp
        auto optionalCommitTimestamp = cmd.getCommitTimestamp();
        if (optionalCommitTimestamp) {
            // commitPreparedTransaction will throw if the transaction is not prepared.
            //CmdCommitTxn::run
            txnParticipant.commitPreparedTransaction(opCtx, optionalCommitTimestamp.get(), {});
        } else {
            if (ShardingState::get(opCtx)->canAcceptShardedCommands().isOK() ||
                serverGlobalParams.clusterRole == ClusterRole::ConfigServer) {
                TransactionCoordinatorService::get(opCtx)->cancelIfCommitNotYetStarted(
                    opCtx, *opCtx->getLogicalSessionId(), *opCtx->getTxnNumber());
            }

            // commitUnpreparedTransaction will throw if the transaction is prepared.
            txnParticipant.commitUnpreparedTransaction(opCtx);
        }

        if (MONGO_unlikely(
                participantReturnNetworkErrorForCommitAfterExecutingCommitLogic.shouldFail())) {
            uasserted(ErrorCodes::HostUnreachable,
                      "returning network error because failpoint is on");
        }

        return true;
    }

} commitTxn;

static const Status kOnlyTransactionsReadConcernsSupported{
    ErrorCodes::InvalidOptions, "only read concerns valid in transactions are supported"};
static const Status kDefaultReadConcernNotPermitted{ErrorCodes::InvalidOptions,
                                                    "default read concern not permitted"};
class CmdAbortTxn : public BasicCommand {
public:
    CmdAbortTxn() : BasicCommand("abortTransaction") {}

    AllowedOnSecondary secondaryAllowed(ServiceContext*) const override {
        return AllowedOnSecondary::kNever;
    }

    virtual bool adminOnly() const {
        return true;
    }

    bool supportsWriteConcern(const BSONObj& cmd) const override {
        return true;
    }

    ReadConcernSupportResult supportsReadConcern(const BSONObj& cmdObj,
                                                 repl::ReadConcernLevel level) const override {
        // abortTransaction commences running inside a transaction (even though the transaction will
        // be ended by the time it completes).  Therefore it needs to accept any readConcern which
        // is valid within a transaction.  However it is not appropriate to apply the default
        // readConcern, since the readConcern of the transaction (set by the first operation) is
        // what must apply.
        return {{!isReadConcernLevelAllowedInTransaction(level),
                 kOnlyTransactionsReadConcernsSupported},
                {kDefaultReadConcernNotPermitted}};
    }

    std::string help() const override {
        return "Aborts a transaction";
    }

    Status checkAuthForOperation(OperationContext* opCtx,
                                 const std::string& dbname,
                                 const BSONObj& cmdObj) const override {
        return Status::OK();
    }

    bool run(OperationContext* opCtx,
             const std::string& dbname,
             const BSONObj& cmdObj,
             BSONObjBuilder& result) override {
        auto txnParticipant = TransactionParticipant::get(opCtx);
        uassert(ErrorCodes::CommandFailed,
                "abortTransaction must be run within a transaction",
                txnParticipant);

        LOGV2_DEBUG(20508,
                    3,
                    "Received abortTransaction for transaction with txnNumber {txnNumber} "
                    "on session {sessionId}",
                    "Received abortTransaction",
                    "txnNumber"_attr = opCtx->getTxnNumber(),
                    "sessionId"_attr = opCtx->getLogicalSessionId()->toBSON());

        uassert(ErrorCodes::NoSuchTransaction,
                "Transaction isn't in progress",
                txnParticipant.transactionIsOpen());

        CurOpFailpointHelpers::waitWhileFailPointEnabled(
            &hangBeforeAbortingTxn, opCtx, "hangBeforeAbortingTxn");

        if (!MONGO_unlikely(dontRemoveTxnCoordinatorOnAbort.shouldFail()) &&
            (ShardingState::get(opCtx)->canAcceptShardedCommands().isOK() ||
             serverGlobalParams.clusterRole == ClusterRole::ConfigServer)) {
            TransactionCoordinatorService::get(opCtx)->cancelIfCommitNotYetStarted(
                opCtx, *opCtx->getLogicalSessionId(), *opCtx->getTxnNumber());
        }

        txnParticipant.abortTransaction(opCtx);

        if (MONGO_unlikely(
                participantReturnNetworkErrorForAbortAfterExecutingAbortLogic.shouldFail())) {
            uasserted(ErrorCodes::HostUnreachable,
                      "returning network error because failpoint is on");
        }

        return true;
    }

} abortTxn;

}  // namespace
}  // namespace mongo
