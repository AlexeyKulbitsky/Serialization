#ifndef TYPE_TRAITS_INCLUDE
#define TYPE_TRAITS_INCLUDE

#include <type_traits>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>

template<typename T>
struct is_string : std::false_type {};

template<>
struct is_string<std::string> : std::true_type {};

template<>
struct is_string<const std::string> : std::true_type {};

template<typename T>
struct is_vector : std::false_type {};

template<typename T>
struct is_vector<std::vector<T>> : std::true_type {};

template<typename T>
struct is_vector<const std::vector<T>> : std::true_type {};

template<typename T>
struct is_map : std::false_type {};

template<typename KeyType, typename ValueType>
struct is_map<std::map<KeyType, ValueType>> : std::true_type {};

template<typename KeyType, typename ValueType>
struct is_map<const std::map<KeyType, ValueType>> : std::true_type {};

template<typename KeyType, typename ValueType>
struct is_map<std::unordered_map<KeyType, ValueType>> : std::true_type {};

template<typename KeyType, typename ValueType>
struct is_map<const std::unordered_map<KeyType, ValueType>> : std::true_type {};

template<typename T>
struct remove_vector_extent { using type = T; };

template<typename T>
struct remove_vector_extent<std::vector<T>> { using type = T; };

template<typename T>
struct remove_vector_extent<const std::vector<T>> { using type = T; };


template<typename T>
struct extract_key_value_from_map
{
	using key_type = T;
	using value_type = T;
};

template<typename Key, typename Value>
struct extract_key_value_from_map<std::map<Key, Value>>
{
	using key_type = Key;
	using value_type = Value;
};

template<typename Key, typename Value>
struct extract_key_value_from_map<std::unordered_map<Key, Value>>
{
	using key_type = Key;
	using value_type = Value;
};

////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
const size_t VectorSizeGetter(void* data)
{
	using VectorType = std::vector<T>;
	auto vectorPtr = reinterpret_cast<VectorType*>(data);
	const size_t size = (*vectorPtr).size();
	return size;
}

template<typename T>
void* VectorItemGetter(void* data, const size_t idx)
{
	using VectorType = std::vector<T>;
	auto vectorPtr = reinterpret_cast<VectorType*>(data);
	auto dataPtr = &((*vectorPtr)[idx]);
	return reinterpret_cast<void*>(dataPtr);
}

template<typename T>
void VectorSizeSetter(void* data, const size_t size)
{
	using VectorType = std::vector<T>;
	auto vectorPtr = reinterpret_cast<VectorType*>(data);
	(*vectorPtr).resize(size);
}

////////////////////////////////////////////////////////////////////////////////////////


/*template<typename T>
struct MapTypeWrapper;

template<typename Key, typename Value>
struct MapTypeWrapper<std::map<Key, Value>>
{
	using MapType = std::map<Key, Value>;
	using KeyType = Key;
	using ValueType = Value;
};

template<typename Key, typename Value>
struct MapTypeWrapper<std::unordered_map<Key, Value>>
{
	using MapType = std::unordered_map<Key, Value>;
	using KeyType = Key;
	using ValueType = Value;
};

template<typename T>
const size_t MapSizeGetter(void* data)
{
	return 0;
}

template<typename T>
const size_t MapSizeGetter<MapTypeWrapper<T>>(void* data)
{
	using MapType = MapTypeWrapper<T>::MapType;
	auto mapPtr = reinterpret_cast<MapType*>(data);
	const size_t size = (*mapPtr).size();
	return size;
}

template<typename T>
void* MapIteratorGetter(void* data)
{
	return nullptr;
}

template<typename T>
void* MapIteratorGetter<MapTypeWrapper<T>>(void* data)
{
	using MapType = MapTypeWrapper<T>::MapType;
	auto mapPtr = reinterpret_cast<MapType*>(data);
	auto it = (*mapPtr).begin();
	auto t = new decltype(it);
	(*t) = it;
	return t;
}

template<typename T>
const void* MapKeyGetter(void* data)
{
	return nullptr;
}

template<typename T>
const void* MapKeyGetter<MapTypeWrapper<T>>(void* data)
{
	using MapType = MapTypeWrapper<T>::MapType;
	using IteratorType = MapType::iterator;

	IteratorType* itPtr = reinterpret_cast<IteratorType*>(data);
	auto keyPtr = &((*itPtr)->first);

	auto dataPtr = reinterpret_cast<const void*>(keyPtr);
	return dataPtr;
}

template<typename T>
void* MapValueGetter(void* data)
{
	return nullptr;
}

template<typename T>
void* MapValueGetter<MapTypeWrapper<T>>(void* data)
{
	using MapType = MapTypeWrapper<T>::MapType;
	using IteratorType = MapType::iterator;

	IteratorType* itPtr = reinterpret_cast<IteratorType*>(data);
	auto valuePtr = &((*itPtr)->second);

	auto dataPtr = reinterpret_cast<void*>(valuePtr);
	return dataPtr;
}

template<typename T>
const bool MapIteratorValidator(void* iteratorRawPtr, void* mapRawPtr)
{
	return false;
}

template<typename T>
const bool MapIteratorValidator<MapTypeWrapper<T>>(void* iteratorRawPtr, void* mapRawPtr)
{
	using MapType = MapTypeWrapper<T>::MapType;
	using IteratorType = MapType::iterator;

	auto mapPtr = reinterpret_cast<MapType*>(mapRawPtr);
	auto itPtr = reinterpret_cast<IteratorType*>(iteratorRawPtr);

	const bool res = (*itPtr) != mapPtr->end();
	return res;
}


template<typename T>
void MapIteratorIncrementator(void* iteratorRawPtr)
{

}

template<typename T>
void MapIteratorIncrementator<MapTypeWrapper<T>>(void* iteratorRawPtr)
{
	using MapType = MapTypeWrapper<T>::MapType;
	using IteratorType = MapType::iterator;
	auto itPtr = reinterpret_cast<IteratorType*>(iteratorRawPtr);
	++(*itPtr);
}

template<typename T>
void MapKeyValueSetter(void* map, void* key, void* value) {}

template<typename T>
void MapKeyValueSetter<MapTypeWrapper<T>>(void* map, void* key, void* value)
{
	using MapType = MapTypeWrapper<T>::MapType;
	using KeyType = MapTypeWrapper<T>::KeyType;
	using ValueType = MapTypeWrapper<T>::ValueType;

	auto mapPtr = reinterpret_cast<MapType*>(map);
	auto keyPtr = reinterpret_cast<KeyType*>(key);
	auto valuePtr = reinterpret_cast<ValueType*>(value);

	(*mapPtr)[(*keyPtr)] = (*valuePtr);
}*/
////////////////////////////////////////////////////////////////////////////////////////

template<typename ObjectType, typename FieldType>
using ClassField = FieldType ObjectType::*;

template<typename T>
struct MemberAssigner;

template<typename ObjectType, typename FieldType>
struct MemberAssigner<ClassField<ObjectType, FieldType>>
{
	using ConcreteClassField = ClassField<ObjectType, FieldType>;
	MemberAssigner(ConcreteClassField field)
		: fieldPtr(field)
	{
	}

	void operator()(void* object, void* data)
	{
		ObjectType* concreteObject = reinterpret_cast<ObjectType*>(object);
		concreteObject->*fieldPtr = *(reinterpret_cast<FieldType*>(data));
	}

	ConcreteClassField fieldPtr = nullptr;
};

template<typename ObjectType, typename FieldType, size_t N>
struct MemberAssigner<ClassField<ObjectType, FieldType[N]>>
{ 
	using ConcreteClassField = ClassField<ObjectType, FieldType[N]>;
	MemberAssigner(ConcreteClassField field)
		: fieldPtr(field)
	{
	}

	void operator()(void* object, void* data)
	{
		ObjectType* concreteObject = reinterpret_cast<ObjectType*>(object);

		void* dstPtr = reinterpret_cast<void*>(concreteObject->*fieldPtr);

		std::memcpy(dstPtr, data, N);
	}

	ConcreteClassField fieldPtr = nullptr;
};

/*template<typename ObjectType, typename FieldType>
struct MemberAssigner<ClassField<ObjectType, std::vector<FieldType>>>
{
	using ConcreteClassField = ClassField<ObjectType, std::vector<FieldType>>;
	MemberAssigner(ConcreteClassField field)
		: fieldPtr(field)
	{
	}


	void operator()(void* object, void* data)
	{
		ObjectType* concreteObject = reinterpret_cast<ObjectType*>(object);
		concreteObject->*fieldPtr = *(reinterpret_cast<std::vector<FieldType>*>(data));
	}

	ConcreteClassField fieldPtr = nullptr;
};*/

#endif