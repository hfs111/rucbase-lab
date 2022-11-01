#include "replacer/clock_replacer.h"

#include <algorithm>

ClockReplacer::ClockReplacer(size_t num_pages)
    : circular_{num_pages, ClockReplacer::Status::EMPTY_OR_PINNED}, hand_{0}, capacity_{num_pages} {
    // 成员初始化列表语法
    circular_.reserve(num_pages);
	//for(int i=0;i<(int)num_pages;i++)
		//circular_.push_back(Status::EMPTY_OR_PINNED);
}

ClockReplacer::~ClockReplacer() = default;

bool ClockReplacer::Victim(frame_id_t *frame_id) {
    const std::lock_guard<mutex_t> guard(mutex_);
    // Todo: try to find a victim frame in buffer pool with clock scheme
    // and make the *frame_id = victim_frame_id
    // not found, frame_id=nullptr and return false
	for(frame_id_t i = hand_;i<(int)capacity_;i++){
		if(circular_[i] == Status::UNTOUCHED){
			*frame_id = i;
			circular_[i] = Status::EMPTY_OR_PINNED;
			hand_ = i+1;
			return true;
		}
		else if(circular_[i] == Status::ACCESSED)
			circular_[i] = Status::UNTOUCHED;
	}
	for(frame_id_t i = 0;i<(int)capacity_;i++){
		if(circular_[i] == Status::UNTOUCHED){
			*frame_id = i;
			circular_[i] = Status::EMPTY_OR_PINNED;
			hand_ = i+1;
			return true;
		}
		else if(circular_[i] == Status::ACCESSED)
			circular_[i] = Status::UNTOUCHED;
	}
	for(frame_id_t i = 0;i<hand_;i++){
		if(circular_[i] == Status::UNTOUCHED){
			*frame_id = i;
			circular_[i] = Status::EMPTY_OR_PINNED;
			hand_ = i+1;
			return true;
		}
		else if(circular_[i] == Status::ACCESSED)
			circular_[i] = Status::UNTOUCHED;
	}
	frame_id = nullptr;
    return false;
}

void ClockReplacer::Pin(frame_id_t frame_id) {
    const std::lock_guard<mutex_t> guard(mutex_);
    // Todo: you can implement it!
	if(frame_id < 0 || frame_id >= (int)capacity_ || circular_[frame_id] == Status::EMPTY_OR_PINNED)
		return;
	else{
		circular_[frame_id] = Status::EMPTY_OR_PINNED;
	}
}

void ClockReplacer::Unpin(frame_id_t frame_id) {
    const std::lock_guard<mutex_t> guard(mutex_);
    // Todo: you can implement it!
	if(frame_id < 0 || frame_id >= (int)capacity_ || circular_[frame_id] != Status::EMPTY_OR_PINNED)
		return;
	else{
		circular_[frame_id] = Status::ACCESSED;
	}
}

size_t ClockReplacer::Size() {
    const std::lock_guard<mutex_t> guard(mutex_);

    // Todo:
    // 返回在[arg0, arg1)范围内满足特定条件(arg2)的元素的数目
    // return all items that in the range[circular_.begin, circular_.end )
    // and be met the condition: status!=EMPTY_OR_PINNED
    // That is the number of frames in the buffer pool that storage page (NOT EMPTY_OR_PINNED)
	size_t size = 0;
	auto it = circular_.begin();
	while(it != circular_.end()){
		if(*it != Status::EMPTY_OR_PINNED)	
			size++;
		++it;
	}
    return size;
}
