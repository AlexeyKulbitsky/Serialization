#ifndef TYPE_INFO_INCLUDE
#define TYPE_INFO_INCLUDE

#include "TypeTraits.h"

#include <cstdint>
#include <iostream>
#include <memory>
#include <functional>


template<size_t index>
using TypeId_ = std::integral_constant<size_t, index>;

template<typename T>
constexpr size_t TypeToId(T = T{}) noexcept
{
	return 0;
}
template<typename T>
constexpr size_t GetTypeSize(T = T{}) noexcept
{
	return 0;
}

#define REGISTER_BASIC_TYPE(Type, Index, Size)		\
template<>											\
constexpr size_t TypeToId<Type>(Type) noexcept		\
{													\
	return Index;									\
}													\
constexpr Type IdToType(TypeId_<Index>) noexcept	\
{													\
	return Type();									\
}													\
template<>											\
constexpr size_t GetTypeSize<Type>(Type) noexcept	\
{													\
	return Size;									\
}

REGISTER_BASIC_TYPE(int, 1, sizeof(int))
REGISTER_BASIC_TYPE(long int, 2, sizeof(long int))
REGISTER_BASIC_TYPE(long long int, 3, sizeof(long long int))
REGISTER_BASIC_TYPE(short, 4, sizeof(short))
REGISTER_BASIC_TYPE(char, 5, sizeof(char))
REGISTER_BASIC_TYPE(unsigned int, 6, sizeof(unsigned int))
REGISTER_BASIC_TYPE(unsigned long int, 7, sizeof(unsigned long int))
REGISTER_BASIC_TYPE(unsigned long long int, 8, sizeof(unsigned long long int))
REGISTER_BASIC_TYPE(unsigned short, 9, sizeof(unsigned short))
REGISTER_BASIC_TYPE(unsigned char, 10, sizeof(unsigned char))
REGISTER_BASIC_TYPE(float, 11, sizeof(float))
REGISTER_BASIC_TYPE(double, 12, sizeof(double))

class TypeInfo;

template<typename T, typename Cond = void>
struct TypeFiller
{
	static void Fill(TypeInfo& typeInfo) {}
};


class TypeInfo
{
public:
	enum Type
	{
		Fundamental,
		Class,
		Array,
		Pointer,
		Enum,
		Map,
		String,

		Undefined
	};

	enum class ArrayType
	{
		CStyleArray,
		StaticArray,
		Vector,

		Undefined
	};

	struct FundamentalTypeParams
	{
		size_t typeSize;
		size_t typeIndex;
	} fundamentalTypeParams;

	struct ArrayParams
	{
		using SizeGetter = std::function<size_t(void*)>;
		using ItemGetter = std::function<void*(void*, const size_t)>;
		using SizeSetter = std::function<void(void*, size_t)>;

		SizeGetter getSize;
		ItemGetter getItem;
		SizeSetter setSize;

		ArrayType arrayType;
		std::unique_ptr<TypeInfo> elementTypeInfo;
		// For C-style and static arrays
		size_t elementsCount = 0U;

	} arrayParams;

	struct MapParams
	{
		using SizeGetter = std::function<size_t(void*)>;
		using IteratorGetter = std::function<void*(void*)>;
		using KeyGetter = std::function<const void*(void*)>;
		using ValueGetter = std::function<void*(void*)>;
		using IteratorValidator = std::function<const bool(void*, void*)>;
		using IteratorIncrementator = std::function<void(void*)>;
		using KeyValueSetter = std::function<void(void*, void*, void*)>;

		std::unique_ptr<TypeInfo> keyTypeInfo;
		std::unique_ptr<TypeInfo> valueTypeInfo;

		SizeGetter getSize;
		IteratorGetter getIterator;
		KeyGetter getKey;
		ValueGetter getValue;
		IteratorValidator isIteratorValid;
		IteratorIncrementator incrementIterator;
		KeyValueSetter setKeyValue;

	} mapParams;

	const uintptr_t GetObjectDescId() const { return objectDescId; }
	//const std::unique_ptr<TypeInfo>& GetUnderlyingType() const { return underlyingType; }

	template<typename ObjectType>
	void Init()
	{
		if (std::is_fundamental<ObjectType>::value)
		{
			type = Fundamental;
			fundamentalTypeParams.typeIndex = TypeToId<ObjectType>();
			fundamentalTypeParams.typeSize = GetTypeSize<ObjectType>();
		}
		else if (std::is_enum<ObjectType>::value)
		{
			type = Enum;
		}
		else if (is_vector<ObjectType>::value)
		{
			type = Array;
			arrayParams.arrayType = ArrayType::Vector;

			TypeFiller<ObjectType>::Fill(*this);
		}
		else if (is_static_array<ObjectType>::value)
		{
			type = Array;
			arrayParams.arrayType = ArrayType::StaticArray;

			TypeFiller<ObjectType>::Fill(*this);
		}
		else if (std::is_array<ObjectType>::value)
		{
			type = Array;
			arrayParams.arrayType = ArrayType::CStyleArray;

			TypeFiller<ObjectType>::Fill(*this);
		}
		else if (std::is_pointer<ObjectType>::value)
		{
			type = Pointer;
			using RawObjectType = std::remove_pointer<ObjectType>::type;

			auto typeInfo = std::make_unique<TypeInfo>();
			underlyingType = std::move(typeInfo);
			underlyingType->Init<RawObjectType>();
		}
		else if (is_string<ObjectType>::value)
		{
			type = String;
		}
		
		else if (is_map<ObjectType>::value)
		{
			type = Map;
			TypeFiller<ObjectType>::Fill(*this);
		}
		else if (std::is_class<ObjectType>::value)
		{
			const auto& objectFactory = ObjectFactory::GetInstance();
			if (objectFactory.IsObjectRegistered<ObjectType>())
			{
				type = Class;
				objectDescId = objectFactory.GetObjectId<ObjectType>();
			}
		}
	}

	Type type = Type::Undefined;
	uintptr_t objectDescId = 0U;
	std::unique_ptr<TypeInfo> underlyingType;
};

template<typename ObjectType>
struct TypeDeducer
{
	TypeDeducer()
	{
		if (std::is_fundamental<ObjectType>::value)
		{
			type = TypeInfo::Fundamental;
		}
		else if (std::is_class<ObjectType>::value)
		{
			type = TypeInfo::Class;
		}
		else if (std::is_pointer<ObjectType>::value)
		{
			type = TypeInfo::Pointer;
		}
	}

	TypeInfo::Type keyType = TypeInfo::Undefined;
	TypeInfo::Type valueType = TypeInfo::Undefined;
	TypeInfo::Type type = TypeInfo::Undefined;
	TypeInfo::Type underlyingType = TypeInfo::Undefined;
	size_t elementsCount = 0U;
	size_t elementSize = 0U;
};

template<typename ObjectType, size_t N>
struct TypeDeducer<ObjectType[N]>
{
	TypeDeducer()
	{
		elementsCount = N;
		elementSize = sizeof(ObjectType);
		if (std::is_fundamental<ObjectType>::value)
		{
			underlyingType = TypeInfo::Fundamental;
		}
		else if (std::is_class<ObjectType>::value)
		{
			underlyingType = TypeInfo::Class;
		}
		else if (std::is_array<ObjectType>::value)
		{
			TypeDeducer<ObjectType> deducer;
			underlyingType = TypeInfo::Array;
		}
		else if (std::is_pointer<ObjectType>::value)
		{
			underlyingType = TypeInfo::Pointer;
		}
	}

	TypeInfo::Type keyType = TypeInfo::Undefined;
	TypeInfo::Type valueType = TypeInfo::Undefined;
	TypeInfo::Type type = TypeInfo::Array;
	TypeInfo::Type underlyingType = TypeInfo::Undefined;
	size_t elementsCount = 0U;
	size_t elementSize = 0U;
};


template<typename ObjectType>
struct TypeDeducer<std::vector<ObjectType>>
{
	TypeDeducer()
	{
		elementsCount = 0;
		elementSize = sizeof(ObjectType);
		if (std::is_fundamental<ObjectType>::value)
		{
			underlyingType = TypeInfo::Fundamental;
		}
		else if (std::is_class<ObjectType>::value)
		{
			underlyingType = TypeInfo::Class;
		}
		else if (std::is_array<ObjectType>::value)
		{
			TypeDeducer<ObjectType> deducer;
			underlyingType = TypeInfo::Array;
		}
		else if (std::is_pointer<ObjectType>::value)
		{
			underlyingType = TypeInfo::Pointer;
		}
	}

	TypeInfo::Type keyType = TypeInfo::Undefined;
	TypeInfo::Type valueType = TypeInfo::Undefined;
	TypeInfo::Type type = TypeInfo::Array;
	TypeInfo::Type underlyingType = TypeInfo::Undefined;
	size_t elementsCount = 0U;
	size_t elementSize = 0U;
};


template<typename KeyType, typename ValueType>
struct TypeDeducer<std::map<KeyType, ValueType>>
{
	TypeDeducer()
	{
		TypeDeducer<KeyType> keyTypeDeducer;
		TypeDeducer<ValueType> valueTypeDeducer;

		keyType = keyTypeDeducer.type;
		valueType = valueTypeDeducer.type;
	}

	TypeInfo::Type keyType = TypeInfo::Undefined;
	TypeInfo::Type valueType = TypeInfo::Undefined;
	TypeInfo::Type underlyingType = TypeInfo::Undefined;
	size_t elementsCount = 0U;
	size_t elementSize = 0U;
};

template<typename KeyType, typename ValueType>
struct TypeDeducer<std::unordered_map<KeyType, ValueType>>
{
	TypeDeducer()
	{
		TypeDeducer<KeyType> keyTypeDeducer;
		TypeDeducer<ValueType> valueTypeDeducer;

		keyType = keyTypeDeducer.type;
		valueType = valueTypeDeducer.type;
	}

	TypeInfo::Type type = TypeInfo::Map;

	TypeInfo::Type keyType = TypeInfo::Undefined;
	TypeInfo::Type valueType = TypeInfo::Undefined;

	TypeInfo::Type underlyingType = TypeInfo::Undefined;
	size_t elementsCount = 0U;
	size_t elementSize = 0U;
};


//////////////////////////////////////////////////////////////////////////////////////////////////

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


template<typename T>
void* ArrayItemGetter(void* data, const size_t idx)
{
	auto arrayPtr = reinterpret_cast<T*>(data);
	auto dataPtr = &(arrayPtr[idx]);
	return reinterpret_cast<void*>(dataPtr);
}

template<typename T, size_t N>
void* StaticArrayItemGetter(void* data, const size_t idx)
{
	using ArrayType = std::array<T, N>;
	auto arrayPtr = reinterpret_cast<ArrayType*>(data);
	auto dataPtr = &((*arrayPtr)[idx]);
	return reinterpret_cast<void*>(dataPtr);
}

template<typename T>
struct TypeFiller<T, std::enable_if_t<is_vector<T>::value>>
{
	static void Fill(TypeInfo& typeInfo)
	{
		typeInfo.arrayParams.elementTypeInfo = std::make_unique<TypeInfo>();
		typeInfo.arrayParams.getSize = VectorSizeGetter<T>;
		typeInfo.arrayParams.getItem = VectorItemGetter<T>;
		typeInfo.arrayParams.setSize = VectorSizeSetter<T>;

		using ElementType = remove_vector_extent<T>::type;
		typeInfo.arrayParams.elementTypeInfo->Init<ElementType>();
	}
};

template<typename T>
struct ArraySize{};

template<typename T, size_t N>
struct ArraySize<T[N]>
{
	static constexpr size_t size = N;
};

template<typename T, size_t N>
struct ArraySize<std::array<T, N>>
{
	static constexpr size_t size = N;
};

template<typename T>
struct TypeFiller<T, std::enable_if_t<is_static_array<T>::value>>
{
	static void Fill(TypeInfo& typeInfo)
	{
		using ElementType = remove_static_array_extent<T>::type;
		typeInfo.arrayParams.getItem = StaticArrayItemGetter<ElementType, ArraySize<T>::size>;
		typeInfo.arrayParams.elementsCount = ArraySize<T>::size;

		typeInfo.arrayParams.elementTypeInfo = std::make_unique<TypeInfo>();
		typeInfo.arrayParams.elementTypeInfo->Init<ElementType>();
	}
};

template<typename T>
struct TypeFiller<T, std::enable_if_t<std::is_array<T>::value>>
{
	static void Fill(TypeInfo& typeInfo)
	{
		using ElementType = std::remove_extent<T>::type;
		typeInfo.arrayParams.getItem = ArrayItemGetter<ElementType>;
		typeInfo.arrayParams.elementsCount = ArraySize<T>::size;	

		typeInfo.arrayParams.elementTypeInfo = std::make_unique<TypeInfo>();
		typeInfo.arrayParams.elementTypeInfo->Init<ElementType>();
	}
};

////////////////////////////////////////////////////////////////////////////////////////


template<typename T>
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

template<typename MapType>
const size_t MapSizeGetter(void* data)
{
	auto mapPtr = reinterpret_cast<MapType*>(data);
	const size_t size = (*mapPtr).size();
	return size;
}

template<typename MapType>
void* MapIteratorGetter(void* data)
{
	auto mapPtr = reinterpret_cast<MapType*>(data);
	auto it = (*mapPtr).begin();
	auto t = new decltype(it);
	(*t) = it;
	return t;
}

template<typename MapType>
const void* MapKeyGetter(void* data)
{
	using IteratorType = MapType::iterator;

	IteratorType* itPtr = reinterpret_cast<IteratorType*>(data);
	auto keyPtr = &((*itPtr)->first);

	auto dataPtr = reinterpret_cast<const void*>(keyPtr);
	return dataPtr;
}

template<typename MapType>
void* MapValueGetter(void* data)
{
	using IteratorType = MapType::iterator;

	IteratorType* itPtr = reinterpret_cast<IteratorType*>(data);
	auto valuePtr = &((*itPtr)->second);

	auto dataPtr = reinterpret_cast<void*>(valuePtr);
	return dataPtr;
}

template<typename MapType>
const bool MapIteratorValidator(void* iteratorRawPtr, void* mapRawPtr)
{
	using IteratorType = MapType::iterator;

	auto mapPtr = reinterpret_cast<MapType*>(mapRawPtr);
	auto itPtr = reinterpret_cast<IteratorType*>(iteratorRawPtr);

	const bool res = (*itPtr) != mapPtr->end();
	return res;
}

template<typename MapType>
void MapIteratorIncrementator(void* iteratorRawPtr)
{
	using IteratorType = MapType::iterator;
	auto itPtr = reinterpret_cast<IteratorType*>(iteratorRawPtr);
	++(*itPtr);
}

template<typename MapType, typename KeyType, typename ValueType>
void MapKeyValueSetter(void* map, void* key, void* value)
{
	auto mapPtr = reinterpret_cast<MapType*>(map);
	auto keyPtr = reinterpret_cast<KeyType*>(key);
	auto valuePtr = reinterpret_cast<ValueType*>(value);

	(*mapPtr)[(*keyPtr)] = (*valuePtr);
}


template<typename T>
struct TypeFiller<T, std::enable_if_t<is_map<T>::value>>
{
	static void Fill(TypeInfo& typeInfo)
	{
		using MapType = MapTypeWrapper<T>::MapType;
		using KeyType = MapTypeWrapper<T>::KeyType;
		using ValueType = MapTypeWrapper<T>::ValueType;
		
		typeInfo.mapParams.getSize = MapSizeGetter<MapType>;
		typeInfo.mapParams.getIterator = MapIteratorGetter<MapType>;
		typeInfo.mapParams.getKey = MapKeyGetter<MapType>;
		typeInfo.mapParams.getValue = MapValueGetter<MapType>;
		typeInfo.mapParams.isIteratorValid = MapIteratorValidator<MapType>;
		typeInfo.mapParams.incrementIterator = MapIteratorIncrementator<MapType>;
		typeInfo.mapParams.setKeyValue = MapKeyValueSetter<MapType, KeyType, ValueType>;

		typeInfo.mapParams.keyTypeInfo = std::make_unique<TypeInfo>();
		typeInfo.mapParams.keyTypeInfo->Init<KeyType>();
		typeInfo.mapParams.valueTypeInfo = std::make_unique<TypeInfo>();
		typeInfo.mapParams.valueTypeInfo->Init<ValueType>();
	}
};

#endif // !TYPE_INFO_INCLUDE

