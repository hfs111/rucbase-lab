//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// config.h
//
// Identification: src/include/common/config.h
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <atomic>
#include <chrono>  // NOLINT
#include <cstdint>

/** Cycle detection is performed every CYCLE_DETECTION_INTERVAL milliseconds. */
extern std::chrono::milliseconds cycle_detection_interval;

/** True if logging should be enabled, false otherwise. */
extern std::atomic<bool> enable_logging;

/** If ENABLE_LOGGING is true, the log should be flushed to disk every LOG_TIMEOUT. */
extern std::chrono::duration<int64_t> log_timeout;

static constexpr int INVALID_FRAME_ID = -1;                                   // invalid frame id
static constexpr int INVALID_PAGE_ID = -1;                                    // invalid page id
static constexpr int INVALID_TXN_ID = -1;                                     // invalid transaction id
static constexpr int INVALID_TIMESTAMP = -1;                                  // invalid transaction timestamp
static constexpr int INVALID_LSN = -1;                                        // invalid log sequence number
static constexpr int HEADER_PAGE_ID = 0;                                      // the header page id
static constexpr int PAGE_SIZE = 4096;                                        // size of a data page in byte
static constexpr int BUFFER_POOL_SIZE = 65536;                                // size of buffer pool
static constexpr int LOG_BUFFER_SIZE = ((BUFFER_POOL_SIZE + 1) * PAGE_SIZE);  // size of a log buffer in byte
static constexpr int BUCKET_SIZE = 50;                                        // size of extendible hash bucket

using frame_id_t = int32_t;  // frame id type, 帧页ID, 页在BufferPool中的存储单元称为帧,一帧对应一页
using page_id_t = int32_t;   // page id type , 页ID
using txn_id_t = int32_t;    // transaction id type
using lsn_t = int32_t;       // log sequence number type
using slot_offset_t = size_t;  // slot offset type
using oid_t = uint16_t;
using timestamp_t = int32_t;  // timestamp type, used for transaction concurrency

// log file
static const std::string LOG_FILE_NAME = "db.log";

// replacer
static const std::string REPLACER_TYPE = "LRU";
