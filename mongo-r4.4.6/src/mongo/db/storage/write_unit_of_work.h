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

#include <memory>

namespace mongo {

class OperationContext;

/**
 * The WriteUnitOfWork is an RAII type that begins a storage engine write unit of work on both the
 * Locker and the RecoveryUnit of the OperationContext. Any writes that occur during the lifetime of
 * this object will be committed when commit() is called, and rolled back (aborted) when the object
 * is destructed without a call to commit() or release().
 *
 * A WriteUnitOfWork can be nested with others, but only the top level WriteUnitOfWork will commit
 * the unit of work on the RecoveryUnit. If a low level WriteUnitOfWork aborts, any parents will
 * also abort.
 */
//Dur_recovery_unit.h (mongo\src\mongo\db\storage\mmap_v1):class DurRecoveryUnit : public RecoveryUnit {
//Ephemeral_for_test_recovery_unit.h (mongo\src\mongo\db\storage\ephemeral_for_test):class EphemeralForTestRecoveryUnit : public RecoveryUnit {
//Heap_record_store_btree.h (mongo\src\mongo\db\storage\mmap_v1):class HeapRecordStoreBtreeRecoveryUnit : public RecoveryUnit {
//Recovery_unit_noop.h (mongo\src\mongo\db\storage):class RecoveryUnitNoop : public RecoveryUnit {
//Wiredtiger_recovery_unit.h (mongo\src\mongo\db\storage\wiredtiger):class WiredTigerRecoveryUnit final : public RecoveryUnit {

/**
 * A RecoveryUnit is responsible for ensuring that data is persisted.
 * All on-disk information must be mutated through this interface.
 一个recoveryunit负责确保数据持久化。所有磁盘上的信息都必须通过这个接口进行修改。
 */
/*
RecoveryUnit封装了wiredTiger层的事务。RecoveryUnit::_txnOpen 对应于WT层的beginTransaction。 
RecoveryUnit::_txnClose封装了WT层的commit_transaction和rollback_transaction。
*/
//OperationContext::_recoveryUnit为RecoveryUnit类类型, WiredTigerRecoveryUnit继承该类
//WiredTigerRecoveryUnit继承该类

//注意WriteUnitOfWork和RecoveryUnit的关系，WriteUnitOfWork的相关接口最终都会调用RecoveryUnit接口
//OperationContext._recoveryUnit  OperationContext._writeUnitOfWork分别对应RecoveryUnit和WriteUnitOfWork


/*
insertDocuments {
    WriteUnitOfWork wuow1(opCtx);
    ...
    CollectionImpl::insertDocuments->OpObserverImpl::onInserts
    {
        WriteUnitOfWork wuow2(opCtx);
        ...
        wuow2.commit();
    }
    ...
    wuow1.commit(); 
}

wuow1和wuow2的_opCtx是一样的，所以对应的_opCtx->recoveryUnit()也是同一个

*/

//使用可以参考insertDocuments  makeCollection    事务封装
//OperationContext._writeUnitOfWork为该类型，表示该操作对应的事务相关封装
class WriteUnitOfWork { //使用可以参考mongo::insertDocuments  makeCollection    事务封装
    WriteUnitOfWork(const WriteUnitOfWork&) = delete;
    WriteUnitOfWork& operator=(const WriteUnitOfWork&) = delete;

public:
    /**
     * The RecoveryUnitState is used to ensure valid state transitions.
     */
    enum RecoveryUnitState {
        kNotInUnitOfWork,   // not in a unit of work, no writes allowed
        kActiveUnitOfWork,  // in a unit of work that still may either commit or abort
        kFailedUnitOfWork   // in a unit of work that has failed and must be aborted
    };

    WriteUnitOfWork(OperationContext* opCtx);

    ~WriteUnitOfWork();

    /**
     * Creates a top-level WriteUnitOfWork without changing RecoveryUnit or Locker state. For use
     * when the RecoveryUnit and Locker are in active or failed state.
     */
    static std::unique_ptr<WriteUnitOfWork> createForSnapshotResume(OperationContext* opCtx,
                                                                    RecoveryUnitState ruState);

    /**
     * Releases the OperationContext RecoveryUnit and Locker objects from management without
     * changing state. Allows for use of these objects beyond the WriteUnitOfWork lifespan. Prepared
     * units of work are not allowed be released. Returns the state of the RecoveryUnit.
     */
    RecoveryUnitState release();

    /**
     * Transitions the WriteUnitOfWork to the "prepared" state. The RecoveryUnit state in the
     * OperationContext must be active. The WriteUnitOfWork may not be nested and will invariant in
     * that case. Will throw CommandNotSupported if the storage engine does not support prepared
     * transactions. May throw WriteConflictException.
     *
     * No subsequent operations are allowed except for commit or abort (when the object is
     * destructed).
     */
    void prepare();

    /**
     * Commits the WriteUnitOfWork. If this is the top level unit of work, the RecoveryUnit's unit
     * of work is committed. Commit can only be called once on an active unit of work, and may not
     * be called on a released WriteUnitOfWork.
     */
    void commit();

private:
    WriteUnitOfWork() = default;  // for createForSnapshotResume

    OperationContext* _opCtx;

    bool _toplevel;

    bool _committed = false;
    bool _prepared = false;
    bool _released = false;
};

std::ostream& operator<<(std::ostream& os, WriteUnitOfWork::RecoveryUnitState state);

}  // namespace mongo
