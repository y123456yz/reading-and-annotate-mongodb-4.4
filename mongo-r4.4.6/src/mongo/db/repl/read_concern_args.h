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
#include <string>

#include "mongo/base/status.h"
#include "mongo/db/json.h"
#include "mongo/db/logical_time.h"
#include "mongo/db/read_write_concern_provenance.h"
#include "mongo/db/repl/optime.h"
#include "mongo/db/repl/read_concern_level.h"
#include "mongo/util/time_support.h"

namespace mongo {

class BSONObj;
class OperationContext;

namespace repl {
//通过repl::ReadConcernArgs::get(opCtx);获取本请求对应的ReadConcernArgs信息 
class ReadConcernArgs {
public:
    //参数解析参考ReadConcernArgs::parse
    

    //对应取值见readConcernLevels::fromString
    static constexpr StringData kReadConcernFieldName = "readConcern"_sd;
    static constexpr StringData kAfterOpTimeFieldName = "afterOpTime"_sd;
    static constexpr StringData kAfterClusterTimeFieldName = "afterClusterTime"_sd;
    static constexpr StringData kAtClusterTimeFieldName = "atClusterTime"_sd;
    static constexpr StringData kLevelFieldName = "level"_sd;


    // The "kImplicitDefault" read concern, used by internal operations, is deliberately empty (no
    // 'level' specified). This allows internal operations to specify a read concern, while still
    // allowing it to be either local or available on sharded secondaries.
    //如果没有指定readConcern参数信息，则默认为ReadConcernArgs::kImplicitDefault 
    static const BSONObj kImplicitDefault;

    /**
     * Represents the internal mechanism an operation uses to satisfy 'majority' read concern.
     */
    enum class MajorityReadMechanism {
        // The storage engine will read from a historical, majority committed snapshot of data. This
        // is the default mechanism.
        kMajoritySnapshot,

        // A mechanism to be used when the storage engine does not support reading from a historical
        // snapshot. A query will read from a local (potentially uncommitted) snapshot, and, after
        // reading data, will block until it knows that data has become majority committed.
        kSpeculative
    };

    static ReadConcernArgs& get(OperationContext* opCtx);
    static const ReadConcernArgs& get(const OperationContext* opCtx);

    ReadConcernArgs();

    ReadConcernArgs(boost::optional<ReadConcernLevel> level);

    ReadConcernArgs(boost::optional<OpTime> opTime, boost::optional<ReadConcernLevel> level);

    ReadConcernArgs(boost::optional<LogicalTime> clusterTime,
                    boost::optional<ReadConcernLevel> level);
    /**
     * Format:
     * {
     *    find: "coll"
     *    filter: <Query Object>,
     *    readConcern: { // optional
     *      level: "[majority|local|linearizable|available|snapshot]",
     *      afterOpTime: { ts: <timestamp>, term: <NumberLong> },
     *      afterClusterTime: <timestamp>,
     *      atClusterTime: <timestamp>
     *    }
     * }
     */
    Status initialize(const BSONObj& cmdObj) {
        return initialize(cmdObj[kReadConcernFieldName]);
    }

    /**
     * Initializes the object from the readConcern element in a command object.
     * Use this if you are already iterating over the fields in the command object.
     * This method correctly handles missing BSONElements.
     */
    Status initialize(const BSONElement& readConcernElem);

    /**
     * Initializes the object by parsing the actual readConcern sub-object.
     */
    Status parse(const BSONObj& readConcernObj);

    static ReadConcernArgs fromBSONThrows(const BSONObj& readConcernObj);

    /**
     * Sets the mechanism we should use to satisfy 'majority' reads.
     *
     * Invalid to call unless the read concern level is 'kMajorityReadConcern'.
     */
    void setMajorityReadMechanism(MajorityReadMechanism m);

    /**
     * Returns the mechanism to use for satisfying 'majority' read concern.
     *
     * Invalid to call unless the read concern level is 'kMajorityReadConcern'.
     */
    MajorityReadMechanism getMajorityReadMechanism() const;

    /**
     * Returns whether the read concern is speculative 'majority'.
     */
    bool isSpeculativeMajority() const;

    /**
     * Appends level, afterOpTime, and any other sub-fields in a 'readConcern' sub-object.
     */
    void appendInfo(BSONObjBuilder* builder) const;

    /**
     * Returns true if any of clusterTime, opTime or level arguments are set. Does not differentiate
     * between an unspecified read concern and an empty one (i.e. an empty BSON object).
     */
    bool isEmpty() const;

    /**
     * Returns true if this ReadConcernArgs represents a read concern that was actually specified.
     * If the RC was specified as an empty BSON object this will still be true (unlike isEmpty()).
     * False represents an absent or missing read concern, ie. one which wasn't present at all.
     */
    bool isSpecified() const;

    /**
     *  Returns default kLocalReadConcern if _level is not set.
     */
    ReadConcernLevel getLevel() const;
    /**
     * Checks whether _level is explicitly set.
     */
    bool hasLevel() const;

    /**
     * Returns the opTime. Deprecated: will be replaced with getArgsAfterClusterTime.
     */
    boost::optional<OpTime> getArgsOpTime() const;

    boost::optional<LogicalTime> getArgsAfterClusterTime() const;

    boost::optional<LogicalTime> getArgsAtClusterTime() const;
    BSONObj toBSON() const;
    BSONObj toBSONInner() const;
    std::string toString() const;

    ReadWriteConcernProvenance& getProvenance() {
        return _provenance;
    }
    const ReadWriteConcernProvenance& getProvenance() const {
        return _provenance;
    }

private:
    /**
     * Appends level, afterOpTime, and the other "inner" fields of the read concern args.
     */
    void _appendInfoInner(BSONObjBuilder* builder) const;

    /**
     *  Read data after the OpTime of an operation on this replica set. Deprecated.
     *  The only user is for read-after-optime calls using the config server optime.
     */
    boost::optional<OpTime> _opTime;
    /**
     *  Read data after cluster-wide cluster time.
     */
    //afterClusterTime参考: https://mongoing.com/archives/77853  https://mongoing.com/archives/25302
    boost::optional<LogicalTime> _afterClusterTime;
    /**
     * Read data at a particular cluster time.
     */
    boost::optional<LogicalTime> _atClusterTime; 
    //ReadConcernArgs::parse中解析，ReadConcernArgs::getLevel()中获取该level值
    boost::optional<ReadConcernLevel> _level;

    /**
     * The mechanism to use for satisfying 'majority' reads. Only meaningful if the read concern
     * level is 'majority'.
     */
    MajorityReadMechanism _majorityReadMechanism{MajorityReadMechanism::kMajoritySnapshot};

    /**
     * True indicates that a read concern has been specified (even if it might be empty), as
     * opposed to being absent or missing.
     */
    bool _specified;

    ReadWriteConcernProvenance _provenance;
};

}  // namespace repl
}  // namespace mongo
