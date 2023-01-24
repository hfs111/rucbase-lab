#include "transaction_manager.h"
#include "record/rm_file_handle.h"

std::unordered_map<txn_id_t, Transaction *> TransactionManager::txn_map = {};

/**
 * 事务的开始方法
 * @param txn 事务指针
 * @param log_manager 日志管理器，用于日志lab
 * @return 当前事务指针
 * @tips: 事务的指针可能为空指针
 */
Transaction * TransactionManager::Begin(Transaction *txn, LogManager *log_manager) {
    // Todo:
    // 1. 判断传入事务参数是否为空指针
    // 2. 如果为空指针，创建新事务
    // 3. 把开始事务加入到全局事务表中
    // 4. 返回当前事务指针
    // txn_id_t txn_id = 0;
    // if(txn == nullptr){
    //     txn_id = ++next_txn_id_;         //空指针，需要分配id
    //     txn = new Transaction(txn_id);
    // }
    // else
    //     txn_id = txn->GetTransactionId(); //如果不是空指针，直接获取id
    // if(txn_map.find(txn_id) == txn_map.end())   //如果不存在则加入
    //     txn_map.insert(std::make_pair(txn_id,txn));
    // return txn;
    if (txn == nullptr)
    {
        txn = new Transaction(next_txn_id_++, IsolationLevel::SERIALIZABLE);
        txn->SetState(TransactionState::DEFAULT);
    }
    // 加入全局事务表
    txn_map[txn->GetTransactionId()] = txn;
    return txn;
}

/**
 * 事务的提交方法
 * @param txn 事务指针
 * @param log_manager 日志管理器，用于日志lab
 * @param sm_manager 系统管理器，用于commit，后续会删掉
 */
void TransactionManager::Commit(Transaction * txn, LogManager *log_manager) {
    // Todo:
    // 1. 如果存在未提交的写操作，提交所有的写操作
    // 2. 释放所有锁
    // 3. 释放事务相关资源，eg.锁集
    // 4. 更新事务状态
    if(txn == nullptr) return;

    auto write_set = txn->GetWriteSet();
    if(!write_set->empty()){ //提交所有写操作
        //auto &item = write_set->back();
        write_set->pop_back();
        //不需要提交写操作
        //auto table = item->GetTableName();
        //...
    }
    write_set->clear();

    auto lockset = txn->GetLockSet();
    for(auto it=lockset->begin(); it!=lockset->end();++it){ //释放所有锁
        lock_manager_->Unlock(txn, *it);
    }
    lockset->clear();

    txn->SetState(TransactionState::COMMITTED); //更新事务状态
}

/**
 * 事务的终止方法
 * @param txn 事务指针
 * @param log_manager 日志管理器，用于日志lab
 * @param sm_manager 系统管理器，用于rollback，后续会删掉
 */
void TransactionManager::Abort(Transaction * txn, LogManager *log_manager) {
    // Todo:
    // 1. 回滚所有写操作
    // 2. 释放所有锁
    // 3. 清空事务相关资源，eg.锁集
    // 4. 更新事务状态
    if(txn == nullptr) return;
    //回滚所有写操作
    auto write_set = txn->GetWriteSet();
    while(!write_set->empty()){ 
        auto &item = write_set->back();

        auto context = new Context(lock_manager_, log_manager, txn);
        switch(item->GetWriteType()){
            case WType::INSERT_TUPLE:
                sm_manager_->rollback_insert(item->GetTableName(),item->GetRid(), context);
                break;
            case WType::UPDATE_TUPLE:
                sm_manager_->rollback_update(item->GetTableName(), item->GetRid(), item->GetRecord(), context);
                break;
            case WType::DELETE_TUPLE:
                sm_manager_->rollback_delete(item->GetTableName(),item->GetRecord(), context);
                break;
            default:
                break;
        }
        write_set->pop_back();
    }
    write_set->clear();
    
    auto lockset = txn->GetLockSet();
    for(auto it=lockset->begin(); it!=lockset->end();++it){ //释放所有锁
        lock_manager_->Unlock(txn, *it);
    }
    lockset->clear();

    txn->SetState(TransactionState::ABORTED); //更新事务状态
}

/** 以下函数用于日志实验中的checkpoint */
void TransactionManager::BlockAllTransactions() {}

void TransactionManager::ResumeAllTransactions() {}