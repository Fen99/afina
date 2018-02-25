#ifndef AFINA_STORAGE_LIST_H
#define AFINA_STORAGE_LIST_H

#include <iterator>
#include <exception>
#include <memory>
#include <iostream>

namespace Afina {
namespace Backend {

template <typename T>
class List
{
	private:
		struct Entry
		{
			T value;

			Entry* next;
			Entry* previous;

			Entry(const T& value, Entry* next, Entry* previous);
		};

		using EntryPtr = Entry*;

	public:
		class const_iterator : public std::iterator<std::bidirectional_iterator_tag, T>
		{
			protected:
				EntryPtr _current_entry;

			protected:
				explicit const_iterator(EntryPtr entry);
				friend const_iterator List<T>::_CreateConstIterator(EntryPtr);

				//For list operations
				friend EntryPtr List<T>::_GetEntryFromIterator(const_iterator);

			public:
				const T& operator*() const;
				
				const_iterator operator++();
				const_iterator operator++(int);
				const_iterator operator--();
				const_iterator operator--(int);

				bool operator==(const const_iterator& it) const;
				bool operator!=(const const_iterator& it) const;
		};

		class iterator : public const_iterator
		{
			protected:
				explicit iterator(EntryPtr entry);
				friend iterator List<T>::_CreateIterator(EntryPtr);
				explicit iterator(const_iterator it);
				friend iterator List<T>::remove_constness(const_iterator);

			public:
				T& operator*();

				iterator operator++();
				iterator operator++(int);
				iterator operator--();
				iterator operator--(int);
		};

	private:
		EntryPtr _head;
		EntryPtr _tail;

	private:
		iterator _CreateIterator(EntryPtr entry);
		const_iterator _CreateConstIterator(EntryPtr entry);
		EntryPtr _GetEntryFromIterator(const_iterator it);

	public:
		List();

		List(const List&) = delete;
		List& operator= (const List&) = delete;
		List(List&&) = delete;
		List& operator= (List&&) = delete;
		~List();

		const_iterator push_front(const T& value);
		void swap(const_iterator first, const_iterator second);
		void to_head(const_iterator it);
		void remove(const_iterator it);

		bool is_empty() const;

		iterator remove_constness(const_iterator it);

		iterator begin();
		iterator end();
		const_iterator cbegin();
		const_iterator cend();

		//"reverse" iterators. Not rbegin, because ++ means normal direction (oposite to STL)
		iterator rstart();
		iterator rfinish();
		const_iterator rcstart();
		const_iterator rcfinish();


		void PrintList();
};

// Add definitions

}
}

#endif // AFINA_STORAGE_LIST_H

#include "List.hpp"
