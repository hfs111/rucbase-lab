#include "lock_manager.h"

/**
 * 申请行级读锁
 * @param txn 要申请锁的事务对象指针
 * @param rid 加锁的目标记录ID
 * @param tab_fd 记录所在的表的fd
 * @return 返回加锁是否成功
 */
bool LockManager::LockSharedOnRecord(Transaction *txn, const Rid &rid, int tab_fd) {
    // Todo:
    // 1. 通过mutex申请访问全局锁表
    // 2. 检查事务的状态
    // 3. 查找当前事务是否已经申请了目标数据项上的锁，如果存在则根据锁类型进行操作，否则执行下一步操作
    // 4. 将要申请的锁放入到全局锁表中，并通过组模式来判断是否可以成功授予锁
    // 5. 如果成功，更新目标数据项在全局锁表中的信息，否则阻塞当前操作
    // 提示：步骤5中的阻塞操作可以通过条件变量来完成，所有加锁操作都遵循上述步骤，在下面的加锁操作中不再进行注释提示
    std::unique_lock<std::mutex> lock(latch_);
    //检查事务状态，处于SHRINKING状态不支持加锁
    if (txn->GetIsolationLevel() == IsolationLevel::READ_UNCOMMITTED) {
        txn->SetState(TransactionState::ABORTED);
        throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_ON_SHIRINKING);
    }

    //查找是否申请过目标数据项的锁
    auto lock_data_id = LockDataId(tab_fd, rid, LockDataType::RECORD);
    //if(lock_table_.count(lock_data_id))
    //    return true; //申请过就直接返回true ？？ 这里不太理解需要做什么
    
    txn->GetLockSet()->insert(lock_data_id);
    txn->SetState(TransactionState::GROWING); //将事务状态设置为扩张期
    //插入到全局锁表，granted_默认false
    LockRequest *request = new LockRequest(txn->GetTransactionId(), LockMode::SHARED);
    lock_table_[lock_data_id].request_queue_.push_back(*request);
    lock_table_[lock_data_id].shared_lock_num_++; //共享锁加一
    //根据组模式判断是否能成功加锁
    //行级锁不会出现IS，IX，因此行级读锁只在X时阻塞!
    while(lock_table_[lock_data_id].group_lock_mode_ != GroupLockMode::NON_LOCK &&
          lock_table_[lock_data_id].group_lock_mode_ != GroupLockMode::S){
        lock_table_[lock_data_id].cv_.wait(lock);
    }

    //等待阻塞信号恢复后，更新全局锁信息
    request->granted_ = true;
    lock_table_[lock_data_id].group_lock_mode_ = GroupLockMode::S;                                                                       
    lock_table_[lock_data_id].cv_.notify_all(); //去除阻塞信号
    return true;
}

/**
 * 申请行级写锁
 * @param txn 要申请锁的事务对象指针
 * @param rid 加锁的目标记录ID
 * @param tab_fd 记录所在的表的fd
 * @return 返回加锁是否成功
 */
bool LockManager::LockExclusiveOnRecord(Transaction *txn, const Rid &rid, int tab_fd) {
    std::unique_lock<std::mutex> lock(latch_);
    //检查事务状态，处于SHRINKING状态不支持加锁
    if (txn->GetIsolationLevel() == IsolationLevel::READ_UNCOMMITTED) {
        txn->SetState(TransactionState::ABORTED);
        throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_ON_SHIRINKING);
    }

    //查找是否申请过目标数据项的锁
    auto lock_data_id = LockDataId(tab_fd, rid, LockDataType::RECORD);
    //if(lock_table_.count(lock_data_id))
    //    return true; //申请过就直接返回true ？？ 这里不太理解需要做什么
    
    txn->GetLockSet()->insert(lock_data_id);
    txn->SetState(TransactionState::GROWING); //将事务状态设置为扩张期
    //插入到全局锁表，granted_默认false
    LockRequest *request = new LockRequest(txn->GetTransactionId(), LockMode::SHARED);
    lock_table_[lock_data_id].request_queue_.push_back(*request);
    //lock_table_[lock_data_id].shared_lock_num_++; //共享锁加一
    //根据组模式判断是否能成功加锁
    //行级写锁除了NON_LOCK都需要阻塞
    while(lock_table_[lock_data_id].group_lock_mode_ != GroupLockMode::NON_LOCK){
        lock_table_[lock_data_id].cv_.wait(lock);
    }

    //等待阻塞信号恢复后，更新全局锁信息
    request->granted_ = true;
    lock_table_[lock_data_id].group_lock_mode_ = GroupLockMode::X;                                                                        
    lock_table_[lock_data_id].cv_.notify_all(); //去除阻塞信号
    return true;
}

/**
 * 申请表级读锁
 * @param txn 要申请锁的事务对象指针
 * @param tab_fd 目标表的fd
 * @return 返回加锁是否成功
 */
bool LockManager::LockSharedOnTable(Transaction *txn, int tab_fd) {
    std::unique_lock<std::mutex> lock(latch_);
    //检查事务状态，处于SHRINKING状态不支持加锁
    if (txn->GetIsolationLevel() == IsolationLevel::READ_UNCOMMITTED) {
        txn->SetState(TransactionState::ABORTED);
        throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_ON_SHIRINKING);
    }
    txn->SetState(TransactionState::GROWING); //将事务状态设置为扩张期
    //查找是否申请过目标数据项的锁
    auto lock_data_id = LockDataId(tab_fd, LockDataType::TABLE);
    //if(lock_table_.count(lock_data_id))
    //    return true; //申请过就直接返回true ？？ 这里不太理解需要做什么
    
    txn->GetLockSet()->insert(lock_data_id);
    //插入到全局锁表，granted_默认false
    LockRequest *request = new LockRequest(txn->GetTransactionId(), LockMode::SHARED);
    lock_table_[lock_data_id].request_queue_.push_back(*request);
    lock_table_[lock_data_id].shared_lock_num_++; //共享锁加一
    //根据组模式判断是否能成功加锁
    //表级读锁在IX，SIX，X时阻塞
    while(lock_table_[lock_data_id].group_lock_mode_ != GroupLockMode::NON_LOCK &&
          lock_table_[lock_data_id].group_lock_mode_ != GroupLockMode::S &&
          lock_table_[lock_data_id].group_lock_mode_ != GroupLockMode::IS){
        lock_table_[lock_data_id].cv_.wait(lock);
    }

    //等待阻塞信号恢复后，更新全局锁信息
    request->granted_ = true;
    //NONE, IS, S -> S
    if(lock_table_[lock_data_id].group_lock_mode_ == GroupLockMode::NON_LOCK ||
       lock_table_[lock_data_id].group_lock_mode_ == GroupLockMode::IS)
        lock_table_[lock_data_id].group_lock_mode_ = GroupLockMode::S;
    //IX -> SIX
    else if(lock_table_[lock_data_id].group_lock_mode_ == GroupLockMode::IX)
        lock_table_[lock_data_id].group_lock_mode_ = GroupLockMode::SIX;                                                                     
    
    lock_table_[lock_data_id].cv_.notify_all(); //去除阻塞信号
    return true;
}

/**
 * 申请表级写锁
 * @param txn 要申请锁的事务对象指针
 * @param tab_fd 目标表的fd
 * @return 返回加锁是否成功
 */
bool LockManager::LockExclusiveOnTable(Transaction *txn, int tab_fd) {
    std::unique_lock<std::mutex> lock(latch_);
    //检查事务状态，处于SHRINKING状态不支持加锁
    if (txn->GetIsolationLevel() == IsolationLevel::READ_UNCOMMITTED) {
        txn->SetState(TransactionState::ABORTED);
        throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_ON_SHIRINKING);
    }
    txn->SetState(TransactionState::GROWING); //将事务状态设置为扩张期

    //查找是否申请过目标数据项的锁
    auto lock_data_id = LockDataId(tab_fd, LockDataType::TABLE);
    //if(lock_table_.count(lock_data_id))
    //    return true; //申请过就直接返回true ？？ 这里不太理解需要做什么
    
    txn->GetLockSet()->insert(lock_data_id);
    //插入到全局锁表，granted_默认false
    LockRequest *request = new LockRequest(txn->GetTransactionId(), LockMode::SHARED);
    lock_table_[lock_data_id].request_queue_.push_back(*request);
    //lock_table_[lock_data_id].shared_lock_num_++; //共享锁加一
    //根据组模式判断是否能成功加锁
    //表级写锁同行级写锁
    while(lock_table_[lock_data_id].group_lock_mode_ != GroupLockMode::NON_LOCK){
        lock_table_[lock_data_id].cv_.wait(lock);
    }

    //等待阻塞信号恢复后，更新全局锁信息
    request->granted_ = true;
    lock_table_[lock_data_id].group_lock_mode_ = GroupLockMode::X;                                                                        
    lock_table_[lock_data_id].cv_.notify_all(); //去除阻塞信号
    return true;
}

/**
 * 申请表级意向读锁
 * @param txn 要申请锁的事务对象指针
 * @param tab_fd 目标表的fd
 * @return 返回加锁是否成功
 */
bool LockManager::LockISOnTable(Transaction *txn, int tab_fd) {
    std::unique_lock<std::mutex> lock(latch_);
    //检查事务状态，处于SHRINKING状态不支持加锁
    if (txn->GetIsolationLevel() == IsolationLevel::READ_UNCOMMITTED) {
        txn->SetState(TransactionState::ABORTED);
        throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_ON_SHIRINKING);
    }

    //查找是否申请过目标数据项的锁
    auto lock_data_id = LockDataId(tab_fd, LockDataType::TABLE);
    //if(lock_table_.count(lock_data_id))
    //    return true; //申请过就直接返回true ？？ 这里不太理解需要做什么
    
    txn->GetLockSet()->insert(lock_data_id);
    txn->SetState(TransactionState::GROWING); //将事务状态设置为扩张期
    //插入到全局锁表，granted_默认false
    LockRequest *request = new LockRequest(txn->GetTransactionId(), LockMode::SHARED);
    lock_table_[lock_data_id].request_queue_.push_back(*request);
    //lock_table_[lock_data_id].shared_lock_num_++; //共享锁加一
    //根据组模式判断是否能成功加锁
    //意向读锁除了X锁存在时都可添加
    while(lock_table_[lock_data_id].group_lock_mode_ == GroupLockMode::X){
        lock_table_[lock_data_id].cv_.wait(lock);
    }

    //等待阻塞信号恢复后，更新全局锁信息
    request->granted_ = true;
    //NONE -> IS , IS -> IS; if S, not change
    if(lock_table_[lock_data_id].group_lock_mode_ == GroupLockMode::NON_LOCK)
        lock_table_[lock_data_id].group_lock_mode_ = GroupLockMode::IS;  

    lock_table_[lock_data_id].cv_.notify_all(); //去除阻塞信号
    return true;
}

/**
 * 申请表级意向写锁
 * @param txn 要申请锁的事务对象指针
 * @param tab_fd 目标表的fd
 * @return 返回加锁是否成功
 */
bool LockManager::LockIXOnTable(Transaction *txn, int tab_fd) {
    std::unique_lock<std::mutex> lock(latch_);
    //检查事务状态，处于SHRINKING状态不支持加锁
    if (txn->GetIsolationLevel() == IsolationLevel::READ_UNCOMMITTED) {
        txn->SetState(TransactionState::ABORTED);
        throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_ON_SHIRINKING);
    }

    //查找是否申请过目标数据项的锁
    auto lock_data_id = LockDataId(tab_fd, LockDataType::TABLE);

    //if(lock_table_.count(lock_data_id))
    //    return true; //申请过就直接返回true ？？ 这里不太理解需要做什么
    
    txn->GetLockSet()->insert(lock_data_id);
    txn->SetState(TransactionState::GROWING); //将事务状态设置为扩张期
    //插入到全局锁表，granted_默认false
    LockRequest *request = new LockRequest(txn->GetTransactionId(), LockMode::SHARED);
    lock_table_[lock_data_id].request_queue_.push_back(*request);
    //lock_table_[lock_data_id].shared_lock_num_++; //共享锁加一
    //根据组模式判断是否能成功加锁
    //意向写锁在S，SIX，X时均阻塞
    while (lock_table_[lock_data_id].group_lock_mode_ == GroupLockMode::S ||
           lock_table_[lock_data_id].group_lock_mode_ == GroupLockMode::SIX ||
           lock_table_[lock_data_id].group_lock_mode_ == GroupLockMode::X) {
        lock_table_[lock_data_id].cv_.wait(lock);
    }

    //等待阻塞信号恢复后，更新全局锁信息
    request->granted_ = true;
    // NONE, IS, IX -> IX
    if(lock_table_[lock_data_id].group_lock_mode_ == GroupLockMode::NON_LOCK ||
       lock_table_[lock_data_id].group_lock_mode_ == GroupLockMode::IS)
        lock_table_[lock_data_id].group_lock_mode_ = GroupLockMode::IX;
    // S -> SIX
    else if(lock_table_[lock_data_id].group_lock_mode_ == GroupLockMode::S)
        lock_table_[lock_data_id].group_lock_mode_ = GroupLockMode::SIX; 
                                                                            
    lock_table_[lock_data_id].cv_.notify_all(); //去除阻塞信号
    return true;
}

/**
 * 释放锁
 * @param txn 要释放锁的事务对象指针
 * @param lock_data_id 要释放的锁ID
 * @return 返回解锁是否成功
 */
bool LockManager::Unlock(Transaction *txn, LockDataId lock_data_id) {
    std::unique_lock<std::mutex> lock(latch_);

    txn->SetState(TransactionState::SHRINKING); //将事务状态设置为收缩期
    if (txn->GetLockSet()->find(lock_data_id) == txn->GetLockSet()->end())
        return false; //未找到该锁
    auto it = lock_table_[lock_data_id].request_queue_.begin();
    while (it != lock_table_[lock_data_id].request_queue_.end()) {
        if (it->txn_id_ == txn->GetTransactionId()){
            it = lock_table_[lock_data_id].request_queue_.erase(it); 
        }
        else 
            it++;
    }
    //修改后的组模式
    GroupLockMode mode = GroupLockMode::NON_LOCK;
    //遍历queue
    for (it = lock_table_[lock_data_id].request_queue_.begin();it != lock_table_[lock_data_id].request_queue_.end(); ++it) {
            if (it->granted_ == true) {
                if (it->lock_mode_ == LockMode::EXLUCSIVE) {
                    mode = GroupLockMode::X;
                    break;

                } else if (it->lock_mode_ == LockMode::S_IX) {
                    mode = GroupLockMode::SIX;
                
                } else if (it->lock_mode_ == LockMode::SHARED && mode != GroupLockMode::SIX) {
                    if (mode == GroupLockMode::IX)
                        mode = GroupLockMode::SIX;
                    else
                        mode = GroupLockMode::S;

                } else if (it->lock_mode_ == LockMode::INTENTION_EXCLUSIVE && mode != GroupLockMode::SIX) {
                    if (mode == GroupLockMode::S)
                        mode = GroupLockMode::SIX;
                    else
                        mode = GroupLockMode::IX;

                } else if (it->lock_mode_ == LockMode::INTENTION_SHARED &&
                           (mode == GroupLockMode::NON_LOCK || mode == GroupLockMode::IS)) {
                    mode = GroupLockMode::IS;
                }
            }
        }
    
    lock_table_[lock_data_id].group_lock_mode_ = mode;
    lock_table_[lock_data_id].cv_.notify_all();
    return true;
}