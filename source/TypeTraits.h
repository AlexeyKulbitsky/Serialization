#ifndef TYPE_TRAITS_INCLUDE
#define TYPE_TRAITS_INCLUDE

#include <type_traits>
#include <string>
#include <vector>

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

#endif