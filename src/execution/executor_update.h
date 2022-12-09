#pragma once
#include "execution_defs.h"
#include "execution_manager.h"
#include "executor_abstract.h"
#include "index/ix.h"
#include "system/sm.h"

class UpdateExecutor : public AbstractExecutor {
   private:
    TabMeta tab_;
    std::vector<Condition> conds_;
    RmFileHandle *fh_;
    std::vector<Rid> rids_;
    std::string tab_name_;
    std::vector<SetClause> set_clauses_;
    SmManager *sm_manager_;

   public:
    UpdateExecutor(SmManager *sm_manager, const std::string &tab_name, std::vector<SetClause> set_clauses,
                   std::vector<Condition> conds, std::vector<Rid> rids, Context *context) {
        sm_manager_ = sm_manager;
        tab_name_ = tab_name;
        set_clauses_ = set_clauses;
        tab_ = sm_manager_->db_.get_table(tab_name);
        fh_ = sm_manager_->fhs_.at(tab_name).get();
        conds_ = conds;
        rids_ = rids;
        context_ = context;
    }
    std::unique_ptr<RmRecord> Next() override {
        // Get all necessary index files
        std::vector<IxIndexHandle *> ihs(tab_.cols.size(), nullptr);
        for (auto &set_clause : set_clauses_) {
            auto lhs_col = tab_.get_col(set_clause.lhs.col_name);
            if (lhs_col->index) {
                size_t lhs_col_idx = lhs_col - tab_.cols.begin();
                // lab3 task3 Todo
                // 获取需要的索引句柄,填充vector ihs
                // lab3 task3 Todo end
				ihs[lhs_col_idx] = sm_manager_->ihs_.at(sm_manager_->get_ix_manager()->get_index_name(tab_name_, lhs_col_idx)).get();
            }
        }
        // Update each rid from record file and index file
        for (auto &rid : rids_) {
            auto rec = fh_->get_record(rid, context_);
            // lab3 task3 Todo
            // Remove old entry from index
            // lab3 task3 Todo end
			for(auto &set_clause : set_clauses_) {
				auto lhs_col = tab_.get_col(set_clause.lhs.col_name);
				//update each rid
				memcpy(rec->data + lhs_col->offset, set_clause.rhs.raw->data, lhs_col->len);
				//delete each old entry
				if(lhs_col->index){
					size_t lhs_col_idx = lhs_col - tab_.cols.begin();
					ihs[lhs_col_idx]->delete_entry(rec->data + lhs_col->offset, context_->txn_);
				}
			}

            // record a update operation into the transaction
            RmRecord update_record{rec->size};
            memcpy(update_record.data, rec->data, rec->size);

            // lab3 task3 Todo
            // Update record in record file
            // lab3 task3 Todo end
			fh_->update_record(rid, rec->data, context_);

            // lab3 task3 Todo
            // Insert new entry into index
            // lab3 task3 Todo end
			for(auto &set_clause : set_clauses_) {
                auto lhs_col = tab_.get_col(set_clause.lhs.col_name);
                if(lhs_col->index) {
                    size_t lhs_col_idx = lhs_col - tab_.cols.begin();
                    auto ih = sm_manager_->ihs_.at(sm_manager_->get_ix_manager()->get_index_name(tab_name_, lhs_col_idx)).get();
                    ih->insert_entry(rec->data + lhs_col->offset, rid, context_->txn_);}
            }
        }
        return nullptr;
    }
    Rid &rid() override { return _abstract_rid; }
};
