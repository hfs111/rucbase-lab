#pragma once
#include "execution_defs.h"
#include "execution_manager.h"
#include "executor_abstract.h"
#include "index/ix.h"
#include "system/sm.h"

class InsertExecutor : public AbstractExecutor {
   private:
    TabMeta tab_;
    std::vector<Value> values_;
    RmFileHandle *fh_;
    std::string tab_name_;
    Rid rid_;
    SmManager *sm_manager_;

   public:
    InsertExecutor(SmManager *sm_manager, const std::string &tab_name, std::vector<Value> values, Context *context) {
        sm_manager_ = sm_manager;
        tab_ = sm_manager_->db_.get_table(tab_name);
        values_ = values;
        tab_name_ = tab_name;
        if (values.size() != tab_.cols.size()) {
            throw InvalidValueCountError();
        }
        // Get record file handle
        fh_ = sm_manager_->fhs_.at(tab_name).get();
        context_ = context;
    };

    std::unique_ptr<RmRecord> Next() override {
        // lab3 task3 Todo
        // Make record buffer
        // Insert into record file
        // Insert into index
        // lab3 task3 Todo end

		//Make record buffer
        RmRecord rec(fh_->get_file_hdr().record_size);
		for (size_t i = 0; i < values_.size(); i++) {
            auto &col = tab_.cols[i];
            auto &val = values_[i];
            if (col.type != val.type) {
                throw IncompatibleTypeError(coltype2str(col.type), coltype2str(val.type));
            }
            val.init_raw(col.len);
            memcpy(rec.data + col.offset, val.raw->data, col.len);
		}
		//Insert into record file
		rid_ = fh_->insert_record(rec.data, context_);
		WriteRecord *write_record = new WriteRecord(WType::INSERT_TUPLE, tab_name_, rid_);
        context_->txn_->AppendWriteRecord(write_record);
        //Insert into index'
		for(size_t i = 0; i< tab_.cols.size();i++){
			auto &col = tab_.cols[i];
			if (col.index) {
	            auto ih = sm_manager_->ihs_.at(sm_manager_->get_ix_manager()->get_index_name(tab_name_, i)).get();
	            ih->insert_entry(rec.data + col.offset, rid_, context_->txn_);
            }
		}
		return nullptr;
    }
    Rid &rid() override { return rid_; }
};
