#ifndef AFINA_STORAGE_MAP_BASED_FC_IMPL_H
#define AFINA_STORAGE_MAP_BASED_FC_IMPL_H

#include <array>
#include <functional>
#include <exception>

#include "MapBasedImplementation.h"
#include "./../core/multithreading/FlatCombiner.hpp"
#include <afina/core/Debug.h>

namespace Afina {
namespace Backend {

/**
 * # Map based implementation with flat combiner
 *
 *
 */

using namespace std::placeholders;

class MapBasedFCImpl : public MapBasedImplementation {
	private:
		enum OperationTypes
		{
			PUT = 0,
			PUT_IF_ABSENT = 1,
			SET = 2,
			DELETE = 3,
			GET = 4,
			PRINT = 5,
			
			CountOfTypes = 6
		};

		struct DataForSlot {
			OperationTypes type;
			std::string key;
			std::string value;

			bool result;

			//is needed from flat combiner
			bool operator<(const DataForSlot& data2) {
				return key < data2.key;
			}
		};

	private:
		using CombinerType = Core::FlatCombiner<DataForSlot>;

	private:
		struct Operations {
			std::array<std::function<void(MapBasedFCImpl&, CombinerType::OperationWrapperPtr)>, OperationTypes::CountOfTypes> operations;
			Operations();
		} _operations; //Static constructible object

	public:
		//max_size - in bytes
		MapBasedFCImpl(size_t max_size = std::numeric_limits<int>::max());
		~MapBasedFCImpl();

		// Implements Afina::Storage interface
		bool Put(const std::string &key, const std::string &value) override;

		// Implements Afina::Storage interface
		bool PutIfAbsent(const std::string &key, const std::string &value) override;

		// Implements Afina::Storage interface
		bool Set(const std::string &key, const std::string &value) override;

		// Implements Afina::Storage interface
		bool Delete(const std::string &key) override;

		// Implements Afina::Storage interface
		bool Get(const std::string &key, std::string &value) override;

		void Print();

	private:
		CombinerType _flat_combiner;

	private:
		void _Combiner(CombinerType::FlatCombinerShotArrayType& arr);
		bool _PrepareAndApplySlot(OperationTypes type, const std::string& key, const std::string& value);
};

} // namespace Backend
} // namespace Afina


#endif // AFINA_STORAGE_MAP_BASED_FC_IMPL_H
