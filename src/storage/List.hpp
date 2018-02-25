#ifndef AFINA_STORAGE_LIST_HPP
#define AFINA_STORAGE_LIST_HPP

#include "List.h"

namespace Afina {
namespace Backend {

template <typename T>
List<T>::Entry::Entry(const T& value, EntryPtr next, EntryPtr previous) : 
	value(value), next(next), previous(previous)
{}

//========================================================================================

template <typename T>
List<T>::const_iterator::const_iterator(EntryPtr entry) : _current_entry(entry)
{}

template <typename T>
const T& List<T>::const_iterator::operator*() const
{
	if (_current_entry == nullptr)
	{
		throw std::out_of_range("Cannot get a value from the end iterator");
	}
	return _current_entry->value;
}

template <typename T>
typename List<T>::const_iterator List<T>::const_iterator::operator++()
{
	if (_current_entry == nullptr)
	{
		throw std::out_of_range("Cannot get next from end iterator");
	}

	_current_entry = _current_entry->next;
	return *this;
}

template <typename T>
typename List<T>::const_iterator List<T>::const_iterator::operator++(int)
{
	const_iterator current_it(*this);
	++(*this);
	return current_it;
}

template <typename T>
typename List<T>::const_iterator List<T>::const_iterator::operator--()
{
	if (_current_entry->previous == nullptr)
	{
		throw std::out_of_range("Cannot get previous from begin iterator");
	}

	_current_entry = _current_entry->previous;
	return *this;
}

template <typename T>
typename List<T>::const_iterator List<T>::const_iterator::operator--(int)
{
	const_iterator current_it(*this);
	--(*this);
	return current_it;
}

template <typename T>
bool List<T>::const_iterator::operator==(const const_iterator& it) const
{
	return _current_entry == it._current_entry;
}

template <typename T>
bool List<T>::const_iterator::operator!=(const const_iterator& it) const
{
	return !(*this == it);
}

//========================================================================================

template <typename T>
List<T>::iterator::iterator(EntryPtr entry) : const_iterator(entry)
{}

template <typename T>
List<T>::iterator::iterator(const_iterator it) : const_iterator(it)
{}

template <typename T>
T& List<T>::iterator::operator*()
{
	return const_cast<T&>(const_iterator::operator*());
}

template <typename T>
typename List<T>::iterator List<T>::iterator::operator++()
{
	return const_iterator::operator++();
}

template <typename T>
typename List<T>::iterator List<T>::iterator::operator++(int p)
{
	return const_iterator::operator++(p);
}

template <typename T>
typename List<T>::iterator List<T>::iterator::operator--()
{
	return const_iterator::operator--();
}

template <typename T>
typename List<T>::iterator List<T>::iterator::operator--(int p)
{
	return const_iterator::operator--(p);
}

//========================================================================================

template <typename T>
List<T>::List() : _head(nullptr), _tail(nullptr)
{}

template <typename T>
List<T>::~List()
{
	Entry* current_entry = _head;
	while (current_entry != nullptr)
	{
		Entry* next = current_entry->next;
		delete current_entry;
		current_entry = next;
	}
}

template <typename T>
typename List<T>::iterator List<T>::_CreateIterator(EntryPtr entry)
{
	return iterator(entry);
}

template <typename T>
typename List<T>::const_iterator List<T>::_CreateConstIterator(EntryPtr entry)
{
	return const_iterator(entry);
}

template <typename T>
typename List<T>::EntryPtr List<T>::_GetEntryFromIterator(const_iterator it)
{
	return it._current_entry;
}

template <typename T>
typename List<T>::iterator List<T>::remove_constness(const_iterator it)
{
	return iterator(it);
}

template <typename T>
typename List<T>::const_iterator List<T>::cbegin() 
{
	return _CreateConstIterator(_head);
}

template <typename T>
typename List<T>::const_iterator List<T>::cend()
{
	return _CreateConstIterator(nullptr);
}

template <typename T>
typename List<T>::iterator List<T>::begin()
{
	return remove_constness(cbegin());
}

template <typename T>
typename List<T>::iterator List<T>::end()
{
	return remove_constness(cend());
}

template <typename T>
typename List<T>::const_iterator List<T>::rcstart() 
{
	return _CreateConstIterator(_tail);
}

template <typename T>
typename List<T>::const_iterator List<T>::rcfinish()
{
	return _CreateConstIterator(nullptr);
}

template <typename T>
typename List<T>::iterator List<T>::rstart()
{
	return remove_constness(rcstart());
}

template <typename T>
typename List<T>::iterator List<T>::rfinish()
{
	return remove_constness(rcfinish());
}

template <typename T>
bool List<T>::is_empty() const
{
	return (_head == nullptr);
}

template <typename T>
typename List<T>::const_iterator List<T>::push_front(const T& value)
{
	EntryPtr new_entry = new Entry(value, _head, nullptr);

	//Empty list
	if (_tail == nullptr) { _tail = new_entry; }
	//Not emptry
	if (_head != nullptr) { _head->previous = new_entry; }
	
	_head = new_entry;

	return _CreateConstIterator(_head);
}

template <typename T>
void List<T>::swap(const_iterator first, const_iterator second)
{
	if (first == cend() || second == cend())
	{
		throw std::out_of_range("Cannot change place of the end iterator");
	}

	EntryPtr first_entry = _GetEntryFromIterator(first);
	EntryPtr second_entry = _GetEntryFromIterator(second);

	EntryPtr prev_tmp = first_entry->previous;
	EntryPtr next_tmp = first_entry->next;

	// Other elements
	if (first_entry != _head) { first_entry->previous->next = second_entry; }
	if (first_entry != _tail) { first_entry->next->previous = second_entry; }
	if (second_entry != _head) { second_entry->previous->next = first_entry; }
	if (second_entry != _tail) { second_entry->next->previous = first_entry; }

	first_entry->next = second_entry->next;
	first_entry->previous = second_entry->previous;
	second_entry->next = next_tmp;
	first_entry->previous = prev_tmp;

	if (first_entry == _head)  { _head = second_entry; }
	if (first_entry == _tail)  { _tail = second_entry; }
	if (second_entry == _head) { _head = first_entry;  }
        if (second_entry == _tail) { _tail = first_entry;  } 	
}

template <typename T>
void List<T>::to_head(const_iterator it)
{
	EntryPtr entry = _GetEntryFromIterator(it);
	if (entry == _head) { return; }
	
	entry->previous->next = entry->next; //Not head
	if (entry != _tail) { entry->next->previous = entry->previous; }
	if (entry == _tail) { _tail = entry->previous; } //2 minimum

	//it != begin => 2 elements minimum
	_head->previous = entry;
	entry->next = _head;
	entry->previous = nullptr;
	_head = entry;
}

template <typename T>
void List<T>::remove(const_iterator it)
{
	if (it == cend())
	{
		throw std::out_of_range("Cannot remove the end iterator");
	}

	EntryPtr current_entry = _GetEntryFromIterator(it);
	if (current_entry != _head) { current_entry->previous->next = current_entry->next; }
	if (current_entry != _tail) { current_entry->next->previous = current_entry->previous; }

	if (current_entry == _head && current_entry == _tail) //One element
	{
	       	_head = nullptr;
	       	_tail = nullptr; 
       	}
	if (current_entry == _head) { _head = _head->next; }
	if (current_entry == _tail) { _tail = _tail->previous; }

	delete current_entry;
}

template <typename T>
void List<T>::PrintList()
{
	Entry* current_entry = _head;
	int i = 0;
	while (current_entry != nullptr)
	{
		std::cout << i << " " << current_entry << ": " << "current_entry->value" << "; next = " << current_entry->next << "; prev = " << current_entry->previous << std::endl;
		current_entry = current_entry->next;
		i++;
	}
}

}
}


#endif // AFINA_STORAGE_LIST_HPP
