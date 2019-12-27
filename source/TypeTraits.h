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
struct is_static_array : std::false_type {};

template<typename T, size_t N>
struct is_static_array<std::array<T, N>> : std::true_type {};

template<typename T, size_t N>
struct is_static_array<const std::array<T, N>> : std::true_type {};

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
struct remove_static_array_extent { using type = T; };

template<typename T, size_t N>
struct remove_static_array_extent<std::array<T, N>> { using type = T; };

template<typename T, size_t N>
struct remove_static_array_extent<const std::array<T, N>> { using type = T; };


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