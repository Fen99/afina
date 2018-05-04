#include "MapBasedFCImpl.h"

namespace Afina {
namespace Backend {

MapBasedFCImpl::Operations::Operations() {
	operations[OperationTypes::PUT] = [](MapBasedFCImpl& container, CombinerType::OperationWrapperPtr wrapper) -> bool 
		{ wrapper->GetData().result = container.MapBasedImplementation::Put(wrapper->GetData().key, wrapper->GetData().value); };

	operations[OperationTypes::PUT_IF_ABSENT] = [](MapBasedFCImpl& container, CombinerType::OperationWrapperPtr wrapper) -> bool
		{ wrapper->GetData().result = container.MapBasedImplementation::PutIfAbsent(wrapper->GetData().key, wrapper->GetData().value); };

	operations[OperationTypes::SET] = [](MapBasedFCImpl& container, CombinerType::OperationWrapperPtr wrapper) -> bool
		{ wrapper->GetData().result = container.MapBasedImplementation::Set(wrapper->GetData().key, wrapper->GetData().value); };

	operations[OperationTypes::DELETE] = [](MapBasedFCImpl& container, CombinerType::OperationWrapperPtr wrapper) -> bool
		{ wrapper->GetData().result = container.MapBasedImplementation::Delete(wrapper->GetData().key); };

	operations[OperationTypes::GET] = [](MapBasedFCImpl& container, CombinerType::OperationWrapperPtr wrapper) -> bool
		{ wrapper->GetData().result = container.MapBasedImplementation::Get(wrapper->GetData().key, wrapper->GetData().value); };

	operations[OperationTypes::PRINT] = [](MapBasedFCImpl& container, CombinerType::OperationWrapperPtr wrapper) -> bool
		{ container.MapBasedImplementation::Print(); };
}

MapBasedFCImpl::MapBasedFCImpl(size_t max_size) : MapBasedImplementation(max_size), _flat_combiner(std::bind(&MapBasedFCImpl::_Combiner, this, _1), 0)
{}

MapBasedFCImpl::~MapBasedFCImpl() {
	_flat_combiner.DestroyCombiner();
	Clear();
}

//In our case: simply call function for each slot
void MapBasedFCImpl::_Combiner(CombinerType::FlatCombinerShotArrayType& arr) {
	//<Optimizations for equivalent keys can be here>
	for (CombinerType::OperationWrapperPtr operation : arr) {
		if (operation == nullptr) { break; }

		std::exception_ptr exc_ptr = nullptr;
		try {
			_operations.operations[operation->GetData().type](*this, operation);
		}
		catch (...) {
			exc_ptr = std::current_exception();
		}
		operation->OnExecutionComplete(exc_ptr);
	}
}

bool MapBasedFCImpl::_PrepareAndApplySlot(MapBasedFCImpl::OperationTypes type, const std::string& key, const std::string& value) {
	CombinerType::OperationWrapperPtr operation = _flat_combiner.GetThreadSlotOperation();
	DataForSlot new_data = {type, key, value, false};
	operation->SetOperation(new_data);

	_flat_combiner.ApplyThreadSlot(); //sets fence

	if (operation->GetException() != nullptr) {
		std::rethrow_exception(operation->GetException());
	}
	return operation->GetData().result;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedFCImpl::Put(const std::string &key, const std::string &value) {
	if (_GetElementSize(key, value) > GetMaxSize()) { return false; }
	return _PrepareAndApplySlot(OperationTypes::PUT, key, value);
}

// See MapBasedGlobalLockImpl.h
bool MapBasedFCImpl::PutIfAbsent(const std::string &key, const std::string &value) {
	if (_GetElementSize(key, value) > GetMaxSize()) { return false; }
	return _PrepareAndApplySlot(OperationTypes::PUT_IF_ABSENT, key, value);
}

// See MapBasedGlobalLockImpl.h
bool MapBasedFCImpl::Set(const std::string &key, const std::string &value) { 
	return _PrepareAndApplySlot(OperationTypes::SET, key, value);
}

// See MapBasedGlobalLockImpl.h
bool MapBasedFCImpl::Delete(const std::string &key) { 
	return _PrepareAndApplySlot(OperationTypes::DELETE, key, "");
}

// See MapBasedGlobalLockImpl.h
bool MapBasedFCImpl::Get(const std::string &key, std::string &value) {
	if (!_PrepareAndApplySlot(OperationTypes::GET, key, value)) { return false; }
	else {
		value = _flat_combiner.GetThreadSlotOperation()->GetData().value;
		return true;
	}
}

void MapBasedFCImpl::Print() {
	_PrepareAndApplySlot(OperationTypes::PRINT, "", "");
}

} // namespace Backend
} // namespace Afina
