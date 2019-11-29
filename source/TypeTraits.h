#ifndef TYPE_TRAITS_INCLUDE
#define TYPE_TRAITS_INCLUDE

#include <type_traits>
#include <string>
#include <vector>
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

////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
const size_t VectorSizeGetter(void* data)
{
	using vectorType = std::vector<T>;
	auto vectorPtr = reinterpret_cast<vectorType*>(data);
	const size_t size = (*vectorPtr).size();
	return size;
}

template<typename T>
void* VectorItemGetter(void* data, const size_t idx)
{
	using vectorType = std::vector<T>;
	auto vectorPtr = reinterpret_cast<vectorType*>(data);
	auto dataPtr = &((*vectorPtr)[idx]);
	return reinterpret_cast<void*>(dataPtr);
}

template<typename T>
void VectorSizeSetter(void* data, const size_t size)
{
	using vectorType = std::vector<T>;
	auto vectorPtr = reinterpret_cast<vectorType*>(data);
	(*vectorPtr).resize(size);
}

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