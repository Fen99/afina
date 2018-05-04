#ifndef AFINA_FLAT_COMBINER_HPP
#define AFINA_FLAT_COMBINER_HPP

#include <functional>
#include <exception>
#include <atomic>
#include <thread>
#include <array>

#include "ThreadLocalPointer.hpp"

namespace Afina {
namespace Core {

/**
 * Create new flat combine synchronizaion primitive
 *
 * @template_param T - type of data for operation, should be copy-assignable and should be able to compared with const operator<
 * @template_param QMS
 * Maximum array size that could be passed to a single Combine function call
 */
template<typename T, int QMS = 64>
class FlatCombiner {
	public:
		class OperationWrapper
		{
			private:
				enum class State {
					READY_FOR_EXECUTE,
					EXECUTION,
					COMPLETE
				};

			private:
				std::atomic<State> _state;
				std::exception_ptr _exc;
				T _data_for_operation;

			public:
				OperationWrapper() : _state(COMPLETE), _result(false), _exc(nullptr) {}
		
				bool IsExecutable() const {
					return _state.load(std::memory_order_relaxed) == State::READY_FOR_EXECUTE;
				}

				//No memory guarantee! If true, needs memory_order_acquire fence
				bool IsComplete() const {
					return _state.load(std::memory_order_relaxed) == State::COMPLETE;
				}

				void SetOperation(const T& data) {
					State expected = State::COMPLETE;
					_data_for_operation = data; //Writed from one thread
					ASSERT(_state.compare_exchange_strong(expected, State::READY_FOR_EXECUTE, std::memory_order_release));
				}

				void OnExecutionStart() {
					bool expected = State::READY_FOR_EXECUTE;
					if (!_state.compare_exchange_strong(expected, State::EXECUTION, std::memory_order_acquire)) {
						ASSERT(expected == State::EXECUTION); //for case if this method will be called from combiner
					}
				}

				void OnExecutionComplete(std::exception_ptr exc) {
					_exc = exc;
					bool expected = State::EXECUTION;
					ASSERT(_state.compare_exchange_strong(expected, State::COMPLETE, std::memory_order_release)); //saves changes and _exc
				}

				T& GetData() {
					return _data_for_operation;
				}

				const std::exception_ptr GetException() const {
					return _exc;
				}
		};

		using OperationWrapperPtr = OperationWrapper*;

	private:
		//For sorting in flat combine. Moves all nullptrs to the end
		static bool _CompareOperationWrapperPointers(const OperationWrapperPtr& w1, const OperationWrapperPtr& w2) {
			if (w1 == nullptr) { return false; } //nullptr -> to the end
			if (w2 == nullptr) { return true; }

			return (w1->GetData() < w2->GetData());
		}

	private:
		struct OpNode {
			private: 
				static const size_t _ALIVE_MASK = 1;
				static const size_t _NOT_ALIVE_MASK = ~_ALIVE_MASK;

				// Pointer to the next slot. One bit of pointer is stolen to
				// mark if owner thread is still alive, based on this information
				// combiner/thread_local destructor able to take decision about
				// deleting node.
				// So if stolen bit is set then the only reference left to this slot
				// if the queue. If pointer is zero and bit is set then the only ref
				// left is thread_local storage. If next is zero there are no
				// link left and slot could be deleted
				std::atomic<size_t> _next_and_alive;

				static bool _CheckValueForAvaliability(size_t val) const {
					return ((bool) (val & _ALIVE_MASK));
				}

			public:
				std::atomic<uint64_t> last_active;
				OperationWrapper operation;

				OpNode() : _next_and_alive(_ALIVE_MASK), last_active(0) {}

				/**
				* Remove alive bit from the next_and_alive pointer and return
				* only correct pointer to the next slot
				*/
				OpNode* Next() {
					return (OpNode*) (_next_and_alive.load(std::memory_order_relaxed) & _NOT_ALIVE_MASK);
				}

				//Returns cas result
				//allow_invalidated_change = true => allows changes of current slot if it is invalid (for purge function - for case if this node is a parent)
				bool SetNext(OpNode* node, bool allow_invalidated_change) {
					ASSERT(node != nullptr); //another function for purging elements

					size_t curr_val = _next_and_alive.load(std::memory_order_relaxed);
					size_t current_mask = curr_val & _ALIVE_MASK;

					if (!allow_invalidated_change) { ASSERT(current_mask == _ALIVE_MASK); }
					ASSERT(current_mask == _ALIVE_MASK); //We cannot purge invalidated element

					size_t new_val = ((size_t) node) | current_mask;
					return _next_and_alive.compare_exchange_strong(curr_val, new_val, std::memory_order_relaxed);
				}

				//returns true if purging is successful, false if node was invalidated during this process or if it was invalidated yet
				bool TryPurge() {
					size_t curr_val = _next_and_alive.load(std::memory_order_relaxed);
					if (!_CheckValueForAvaliability(curr_val)) { return false; }

					if (!_next_and_alive.compare_exchange_strong(curr_val, _ALIVE_MASK, std::memory_order_relaxed)) {
						ASSERT(!_CheckValueForAvaliability(curr_val)); //Only one correct case: node was invalidated
						return false;
					}
					return true;
				}

				//Returns true if node is in queue, false otherwise
				bool Invalidate() {
					size_t curr_val = _next_and_alive.load(std::memory_order_relaxed);

					//Correct cases: 1) Node was purged from queue => curr_val == alive_mask (next* = nullptr); 2) Next node was changed
					//Incorrect case: node was invalidated by other thread
					do {
						ASSERT(curr_val != (curr_val & _NOT_ALIVE_MASK)); //OpNode cannot be invalidated twice
					} while (!_next_and_alive.compare_exchange_weak(curr_val, curr_val & _NOT_ALIVE_MASK, std::memory_order_relaxed));

					if (curr_val != _ALIVE_MASK) { return true; }
					else { return false; } //node was already purged	
				}

				bool IsAlive() const {
					_CheckValueForAvaliability(_next_and_alive.load(std::memory_order_relaxed));
				}
		};

	public:
		// Maximum number of pernding operations could be passed to a single Combine call
		static const std::size_t max_call_size = QMS;
		using FlatCombinerShotArrayType = std::array<OperationWrapperPtr, QMS>;
    
		/**
		 * @param Combine function that aplly pending operations onto some data structure. It accepts array
		 * of pending ops and allowed to modify it in any way except delete pointers
		 */
		FlatCombiner(std::function<void(FlatCombinerShotArrayType&)>& combiner, uint64_t saving_time = 100000, bool need_sort_shot = true) :
			_slot(nullptr, _OrphanSlot), _technical_element(new OpNode), _lock(1), _saving_time(saving_time), _combiner(combiner), _is_alive(true)
		{}

		~FlatCombiner() { 
			DestroyCombiner();
		}

		void DestroyCombiner() {
			bool expected = true;
			if (!_is_alive.compare_exchange_strong(expected, false, std::memory_order_relaxed)) { return; }

			while (_TryLock() == 0) { //We should lock the executor
				std::this_thread::yield();
			}
		
			OpNode* curr_element = _queue.load(std::memory_order_relaxed);
			while (!_queue.compare_exchange_weak(curr_element, nullptr, std::memory_order_relaxed)); //try to set nullptr to the head of queue => prevent insert
			while (curr_element != _technical_element) {
				if (curr_slot->operation.IsExecutable()) {
					curr_element->operation.OnExecutionStart(); //sets fence
					curr_element->operation.OnExecutionComplete(new std::exception("FlatCombine object is destroying!"));
				}

				OpNode* next_element = curr_element->Next();
				_DequeueSlot(nullptr, curr_element);
				curr_element = next_element;
			}

			delete _technical_element;
		}

		OperationWrapperPtr GetThreadSlotOperation() {
			return &(_GetThreadSlot()->operation);
		}

		/**
		 * Put pending operation in the queue and try to execute it. Method gets blocked until
		 * slot gets complete, in other words until slot.complete() returns false
		 */
		void ApplyThreadSlot() {
			// TODO: assert slot params
			// TODO: enqueue slot if needs
			// TODO: try to become executor (acquire lock)
			// TODO: scan queue, dequeue stale nodes, prepare array to be passed to combine call
			// TODO: call Combine function
			// TODO: unlock
			// TODO: if lock fails, do thread_yeild and goto 3 TODO
			OpNode* curr_slot = _slot.get();
			ASSERT(curr_slot->operation.IsExecutable()); //SetOperation should be called before apply. 

			while (true) {
				ASSERT(curr_slot->IsAlive());

				uint64_t epoch = _TryLock();
				if (epoch != 0) { //we are an executor
					if (curr_slot->Next() == nullptr) { _InsertSlot(curr_slot); } //We have full control, so this check will be reliable
					_ExecutorFunction(epoch);
					_Unlock();
				}
				else {
					if (curr_slot->operation.IsComplete()) {
						std::atomic_thread_fence(std::memory_order_acquire); //for read changes
						return;
					}
					else {
						//Check if we are in queue. If we are not an executor, purging should finish in some time
						//and we can see that we are not in queue.
						if (curr_slot->Next() == nullptr) {
							_InsertSlot(curr_slot);
						}
						else {
							std::this_thread::yield();
						}
					}
				}
			}

			ASSERT(curr_slot->operation.IsComplete()); //Executor should complete our operation
		}

		/**
		 * Detach calling thread from this flat combiner, in other word
		 * destroy thread slot in the queue
		 */
		void DetachThread() {
			OpNode* result = _slot.get();
			if (result != nullptr) {
				_slot.set(nullptr);
				_OrphanSlot(result);
			}
		}

	private:
		/**
		 * Return pending operation slot to the calling thread, object stays valid as long
		 * as current thread is alive or got called DetachThread method
		 */
		OpNode* _GetThreadSlot() {
			OpNode* result = _slot.get();
			if (result == nullptr) {
				result = new OpNode; //Usage bit has been already set in constructor
				_slot.set(result);
			}

			return result;
		}

		void _ExecutorFunction(uint64_t epoch) {
			OpNode* curr_element = _queue.load(std::memory_order_relaxed);
			OpNode* parent = nullptr;

			FlatCombinerShotArrayType combine_shot;
			combine_shot.fill(nullptr);
			size_t position = 0;
			while (curr_element != _technical_element) {
				if (!curr_element->IsAlive() || (epoch - curr_element->last_active.load(std::memory_order_relaxed) > _saving_time && 
												 !curr_slot->operation.IsExecutable())) {
					OpNode* next_element = curr_element->Next();
					_DequeueSlot(parent, curr_element);
					curr_element = next_element;
					continue;
				}

				if (curr_slot->operation.IsExecutable()) {
					combine_shot[position] = curr_element->operation;
					++position;
					curr_element->last_active.store(epoch, std::memory_order_relaxed);
					curr_element->operation.OnExecutionStart(); //sets fence
				}
				if (position == QMS) {
					if (_need_sort_shot) { std::sort(combine_shot.begin(), combine_shot.end(), _CompareOperationWrapperPointers); }
					_combiner(combine_shot);
					position = 0;
					combine_shot.fill(nullptr);
				}

				parent = curr_element;
				curr_element = curr_element->Next();
			}

			if (position != 0) {
				std::sort(combine_shot.begin(), combine_shot.end(), _CompareOperationWrapperPointers);
				_combiner(combine_shot);
			}
		}

		/**
		 * Try to acquire "lock", in case of success returns current generation. If
		 * fails the return 0
		 * Uses weak fence!
		 *
		 * @param suc memory barrier to set in case of success lock
		 * @param fail memory barrier to set in case of failure
		 */
		uint64_t _TryLock() {
			uint64_t curr_val = _lock.load(std::memory_order_relaxed);
			if (curr_val & _LCK_BIT_MASK) {  //already locked
				return 0; 
			}

			if (!_lock.compare_exchange_weak(curr_val, _LCK_BIT_MASK | (curr_val), std::memory_order_relaxed)) { return 0; }
			else { return curr_val; }
		}

		/**
		 * Try to release "lock". Increase generation number. Assert for fail.
		 *
		 * @param suc memory barier to set in case of success lock
		 */
		void _Unlock() {
			uint64_t curr_val = _lock.load(std::memory_order_relaxed);
			ASSERT(curr_val & _LCK_BIT_MASK); //check if locked
			ASSERT(_lock.compare_exchange_strong(curr_val, _GEN_VAL_MASK & (curr_val + 1), std::memory_order_relaxed));
		}

		bool _IsLocked() const {
			return (bool) (_lock.load(std::memory_order_relaxed) & _LCK_BIT_MASK);
		}

		//inserts slot into the queue
		void _InsertSlot(OpNode* slot) {
			ASSERT(_queue.load(std::memory_order_relaxed) != nullptr); //flat combine was destructed
			ASSERT(slot->Next() == nullptr); //We cannot insert slot that is already in queue

			uint64_t last_active = curr_slot->last_active.load(std::memory_order_relaxed);
			ASSERT(curr_slot->last_active.compare_exchange_strong(last_active, 0, std::memory_order_relaxed)); //mark that slot is new

			OpNode* current_head = _queue.load(std::memory_order_relaxed);
			do {
				//false parameter - we cannot insert invalidated element
				ASSERT(slot->SetNext(current_head, false)); //If failed: Somebody else has changed our slot that isn't in queue yet!
			} while (!_queue.compare_exchange_weak(current_head, slot, std::memory_order_relaxed));
		}

		/**
		 * Remove slot from the queue. Note that method must be called only
		 * under "lock" to eliminate concurrent queue modifications (! cannot be fully checked from method: only that lock is active !)
		 *
		 */
		void _DequeueSlot(OpNode* parent, OpNode* slot2remove) {
			// TODO: remove node from the queue
			// TODO: set pointer pare of "next" to null, DO NOT modify usage bit
			// TODO: if slot2remove isn't alive, delete pointer
			ASSERT(_IsLocked());
			ASSERT(slot2remove != _technical_element); //We cannot delete the last element
			ASSERT(slot2remove->Next() != nullptr);

			//purging the head of queue in destructor
			if (parent == nullptr && _queue.load(std::memory_order_relaxed) == nullptr) {
				if (!slot2remove->TryPurge()) {
					delete slot2remove;
				}
			}

			if (parent == nullptr) { //removes the head
				OpNode* current = slot2remove;
				if (!_queue.compare_exchange_strong(current, slot2remove->Next(), std::memory_order_relaxed)) { //somebody has changed the head!
					// we need to seek the parent from the start of queue
					current = _queue.load(std::memory_order_relaxed);
					while (current->Next() != slot2remove) {
						ASSERT(current->Next() != _technical_element); //slot2remove should be in queue
					}
					_DequeueSlot(current, slot2remove, std::memory_order_relaxed);
					return;
				}
			}
			else {
				ASSERT(parent->Next() != nullptr); //We cannot use this function for purged nodes

				if (!parent->SetNext(slot2remove->Next()), true) { //true parameter: we allow parent to be invalidated
					ASSERT(!parent->IsAlive()); //only one correct case: parent was invalidated in this moment
					ASSERT(parent->SetNext(slot2remove->Next()), true);
				}
			}

			if (!slot2remove->TryPurge()) {
				delete slot2remove;
			}
		}

		/**
		 * Function called once thread owning this slot is going to die or to
		 * destroy slot in some other way
		 * If thread is finishing nullptr will be set automaticly
		 *
		 * @param OpNode pointer to the slot is being to orphan
		 */
		static void _OrphanSlot(OpNode* node) {
			ASSERT(node != nullptr); //null slot cannot be deleted
			if (!node->Invalidate()) {
				delete node;
			}
		}

	private:
		static constexpr uint64_t _LCK_BIT_MASK = uint64_t(1) << 63L;
		static constexpr uint64_t _GEN_VAL_MASK = ~_LCK_BIT_MASK;

		// First bit is used to see if lock is acquired already or no. Rest of bits is
		// a counter showing how many "generation" has been passed. One generation is a
		// single call of flat_combine function.
		//
		// Based on that counter stale slots found and gets removed from the pending
		// operations queue
		std::atomic<uint64_t> _lock;
		std::atomic<bool> _is_alive; //sets to false on destroying

		std::function<void(FlatCombinerShotArrayType&)> _combiner;
		bool _need_sort_shot; //shows if combiner should sort pointers in shot with T::operator<

		std::atomic<OpNode*> _technical_element; //The last element in the queue

		// Pending operations queue. Each operation to be applied to the protected
		// data structure is ends up in this queue and then executed as a batch by
		// flat_combine method call
		std::atomic<OpNode*> _queue;

		// Slot of the current thread. If nullptr then cur thread gets access in the
		// first time or after a long period when slot has been deleted already
		ThreadLocal<OpNode> _slot;

		//Count of epochs before purging
		uint64_t _saving_time;
};

} //namespace Core
} //namespace Afina

#endif // AFINA_FLAT_COMBINER_HPP
