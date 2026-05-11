#pragma once
#include <qtdDefines.h>
#include "algorithm.h"
#include "vector.h"

#if QTD_IS_EASTL
#include <EASTL/iterator.h>
#include <EASTL/bonus/adaptors.h>
#else
#include <iterator>
#endif


namespace qd {

	// Actually implements signum (-1, 0, or 1).
	template <typename T>
	inline constexpr int sign(T val) {
		return (T(0) < val) - (val < T(0));
	}


	// CHECK IF STD::CONTAINER HAS ITEM
	template<typename T, class TList>
	inline bool is_has(const T& Inst, const TList& List) {
		return qtd::find(List.begin(), List.end(), (typename TList::value_type)Inst ) != List.end();
	}


	template< typename T >
	inline typename qtd::vector<T>::iterator insert_sorted( qtd::vector<T> & Vec, T&& /*const&*/ Item ) {
		return Vec.insert(qtd::upper_bound( Vec.begin(), Vec.end(), Item ), qtd::move(Item) );
	};


	template<typename T, typename TPred>
	inline typename qtd::vector<T>::iterator insert_sorted( qtd::vector<T>& Vec, T&& /*const&*/ Item, TPred pred ) {
		return Vec.insert( qtd::upper_bound( Vec.begin(), Vec.end(), Item, pred ), qtd::move(Item) );
	}


	//////////////////////////////////////////////////////////////////////////
	// Searches the given entry in the map by key, and if there is none, returns the default value
	//////////////////////////////////////////////////////////////////////////
	template<typename Map>
	inline typename Map::mapped_type find_in_map(const Map& mapKeyToValue, const typename Map::key_type& key, typename Map::mapped_type valueDefault) {
		typename Map::const_iterator it = mapKeyToValue.find(key);
		if ( it == mapKeyToValue.end() )
			return valueDefault;
		else
			return it->second;
	}

	//////////////////////////////////////////////////////////////////////////
	// Inserts and returns a reference to the given value in the map, or returns the current one if it's already there.
	//////////////////////////////////////////////////////////////////////////
	template<typename Map>
	inline typename Map::mapped_type& map_insert_or_get(Map& mapKeyToValue, const typename Map::key_type& key, const typename Map::mapped_type& defValue = typename Map::mapped_type()) {
		/*std::pair<typename Map::iterator, bool>*/ auto iresult = mapKeyToValue.insert(typename Map::value_type(key, defValue));
		return iresult.first->second;
	}


	//! Find and erase element from map.
	//! \return true if item was find and erased, false if item not found.
	template<class Container, class Key>
	inline bool map_find_and_erase(Container& container, const Key& key) {
		typename Container::iterator it = container.find(key);
		if ( it != container.end() ) {
			container.erase(it);
			return true;
		}
		return false;
	}


	template<typename Map, class Key>
	inline bool map_has(const Map& container, const Key& key) {
		typename Map::const_iterator it = container.find(key);
		return it != container.end();
	}


	template<typename TClass, typename... TArgs>
	inline TClass* new_(TArgs ...args) {
		return new TClass(args...);
	}

};
