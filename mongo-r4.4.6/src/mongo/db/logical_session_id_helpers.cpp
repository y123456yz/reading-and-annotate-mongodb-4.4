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

#include "mongo/db/logical_session_id_helpers.h"

#include "mongo/db/auth/authorization_session.h"
#include "mongo/db/auth/user.h"
#include "mongo/db/auth/user_name.h"
#include "mongo/db/commands/feature_compatibility_version_documentation.h"
#include "mongo/db/logical_session_cache.h"
#include "mongo/db/operation_context.h"

namespace mongo {

/**
 * This is a safe hash that will not collide with a username because all full usernames include an
 * '@' character.
 */
const auto kNoAuthDigest = SHA256Block::computeHash(reinterpret_cast<const uint8_t*>(""), 0);

//makeLogicalSessionId
SHA256Block getLogicalSessionUserDigestForLoggedInUser(const OperationContext* opCtx) {
    auto client = opCtx->getClient();
    ServiceContext* serviceContext = client->getServiceContext();

    if (AuthorizationManager::get(serviceContext)->isAuthEnabled()) {
        UserName userName;

        const auto user = AuthorizationSession::get(client)->getSingleUser();
        invariant(user);

        uassert(ErrorCodes::BadValue,
                "Username too long to use with logical sessions",
                user->getName().getFullName().length() < kMaximumUserNameLengthForLogicalSessions);

        return user->getDigest();
    } else {
        return kNoAuthDigest;
    }
}

SHA256Block getLogicalSessionUserDigestFor(StringData user, StringData db) {
    if (user.empty() && db.empty()) {
        return kNoAuthDigest;
    }
    const UserName un(user, db);
    const auto& fn = un.getFullName();
    return SHA256Block::computeHash({ConstDataRange(fn.c_str(), fn.size())});
}

//initializeOperationSessionInfo调用，这里填充增加system.sessions中的UUID
LogicalSessionId makeLogicalSessionId(const LogicalSessionFromClient& fromClient,
                                      OperationContext* opCtx,
                                      std::initializer_list<Privilege> allowSpoof) {
    LogicalSessionId lsid;

    lsid.setId(fromClient.getId());

    if (fromClient.getUid()) {
        auto authSession = AuthorizationSession::get(opCtx->getClient());

        uassert(ErrorCodes::Unauthorized,
                "Unauthorized to set user digest in LogicalSessionId",
                std::any_of(allowSpoof.begin(),
                            allowSpoof.end(),
                            [&](const auto& priv) {
                                return authSession->isAuthorizedForPrivilege(priv);
                            }) ||
                    authSession->isAuthorizedForPrivilege(Privilege(
                        ResourcePattern::forClusterResource(), ActionType::impersonate)) ||
                    getLogicalSessionUserDigestForLoggedInUser(opCtx) == fromClient.getUid());

        lsid.setUid(*fromClient.getUid());
    } else {
        lsid.setUid(getLogicalSessionUserDigestForLoggedInUser(opCtx));
    }

    return lsid;
}

LogicalSessionId makeLogicalSessionId(OperationContext* opCtx) {
    LogicalSessionId id{};

    id.setId(UUID::gen());
    id.setUid(getLogicalSessionUserDigestForLoggedInUser(opCtx));

    return id;
}

LogicalSessionId makeSystemLogicalSessionId() {
    LogicalSessionId id{};

    id.setId(UUID::gen());
    id.setUid(internalSecurity.user->getDigest());

    return id;
}

/**
test_auth_repl_4.4.6:PRIMARY> use config
switched to db config
test_auth_repl_4.4.6:PRIMARY> db.system.sessions.aggregate( [  { $listSessions: { allUsers: true } } ] )
{ "_id" : { "id" : UUID("986b0db8-7a37-4fb4-af96-db618268c0bb"), "uid" : BinData(0,"Y5mrDaxi8gv8RmdTsQ+1j7fmkr7JUsabhNmXAheU0fg=") }, "lastUse" : ISODate("2021-07-25T09:38:47.042Z"), "user" : { "name" : "root@admin" } }

*/
//StartSessionCommand::run调用，获取sessio信息
LogicalSessionRecord makeLogicalSessionRecord(OperationContext* opCtx, Date_t lastUse) {
    LogicalSessionId id{};
    LogicalSessionRecord lsr{};

    auto client = opCtx->getClient();
    ServiceContext* serviceContext = client->getServiceContext();
    if (AuthorizationManager::get(serviceContext)->isAuthEnabled()) {
        auto user = AuthorizationSession::get(client)->getSingleUser();
        invariant(user);
		//获取挑战id
        id.setUid(user->getDigest());
		//该session对应的用户信息
        lsr.setUser(StringData(user->getName().toString()));
    } else {//不需要认证挑战id=0
        id.setUid(kNoAuthDigest);
    }
	//生成一个uuid
    id.setId(UUID::gen());

    lsr.setId(id);
    lsr.setLastUse(lastUse);

    return lsr;
}

LogicalSessionRecord makeLogicalSessionRecord(const LogicalSessionId& lsid, Date_t lastUse) {
    LogicalSessionRecord lsr{};

    lsr.setId(lsid);
    lsr.setLastUse(lastUse);

    return lsr;
}

LogicalSessionRecord makeLogicalSessionRecord(OperationContext* opCtx,
                                              const LogicalSessionId& lsid,
                                              Date_t lastUse) {
    auto lsr = makeLogicalSessionRecord(lsid, lastUse);

    auto client = opCtx->getClient();
    ServiceContext* serviceContext = client->getServiceContext();
    if (AuthorizationManager::get(serviceContext)->isAuthEnabled()) {
        auto user = AuthorizationSession::get(client)->getSingleUser();
        invariant(user);

        if (user->getDigest() == lsid.getUid()) {
            lsr.setUser(StringData(user->getName().toString()));
        }
    }

    return lsr;
}


LogicalSessionToClient makeLogicalSessionToClient(const LogicalSessionId& lsid) {
    LogicalSessionIdToClient lsitc;
    lsitc.setId(lsid.getId());

    LogicalSessionToClient id;

    id.setId(lsitc);
    id.setTimeoutMinutes(localLogicalSessionTimeoutMinutes);

    return id;
};

LogicalSessionIdSet makeLogicalSessionIds(const std::vector<LogicalSessionFromClient>& sessions,
                                          OperationContext* opCtx,
                                          std::initializer_list<Privilege> allowSpoof) {
    LogicalSessionIdSet lsids;
    lsids.reserve(sessions.size());
    for (auto&& session : sessions) {
        lsids.emplace(makeLogicalSessionId(session, opCtx, allowSpoof));
    }

    return lsids;
}

namespace logical_session_id_helpers {

void serializeLsidAndTxnNumber(OperationContext* opCtx, BSONObjBuilder* builder) {
    OperationSessionInfo sessionInfo;
    if (opCtx->getLogicalSessionId()) {
        sessionInfo.setSessionId(*opCtx->getLogicalSessionId());
    }
    sessionInfo.setTxnNumber(opCtx->getTxnNumber());
    sessionInfo.serialize(builder);
}

}  // namespace logical_session_id_helpers
}  // namespace mongo
