#pragma once

#include "ix_defs.h"
#include "ix_index_handle.h"

/**
 * @brief 用于直接遍历叶子结点，而不用FindLeafPage()来得到叶子结点
 */
class IxScan : public RecScan {
    const IxIndexHandle *ih_;
    Iid iid_;  // 初始为lower（用于遍历的指针）
    Iid end_;  // 初始为upper
    BufferPoolManager *bpm_;

   public:
    IxScan(const IxIndexHandle *ih, const Iid &lower, const Iid &upper, BufferPoolManager *bpm)
        : ih_(ih), iid_(lower), end_(upper), bpm_(bpm) {}

    void next() override;

    bool is_end() const override { return iid_ == end_; }

    Rid rid() const override;

    const Iid &iid() const { return iid_; }
};
