#ifndef TYPE_INFO_INCLUDE
#define TYPE_INFO_INCLUDE

#include "TypeTraits.h"

#include <cstdint>
#include <iostream>
#include <memory>
#include <functional>
#include <cstdint>
#include <cassert>

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

REGISTER_BASIC_TYPE(int8_t, 1, sizeof(int8_t))
REGISTER_BASIC_TYPE(int16_t, 2, sizeof(int16_t))
REGISTER_BASIC_TYPE(int32_t, 3, sizeof(int32_t))
REGISTER_BASIC_TYPE(int64_t, 4, sizeof(int64_t))
REGISTER_BASIC_TYPE(uint8_t, 5, sizeof(uint8_t))
REGISTER_BASIC_TYPE(uint16_t, 6, sizeof(uint16_t))
REGISTER_BASIC_TYPE(uint32_t, 7, sizeof(uint32_t))
REGISTER_BASIC_TYPE(uint64_t, 8, sizeof(uint64_t))
REGISTER_BASIC_TYPE(float, 9, sizeof(float))
REGISTER_BASIC_TYPE(double, 10, sizeof(double))

template <typename T>
struct GetTypeNameHelper
{
	static std::string GetTypeName()
	{
		return std::string(__FUNCTION__);
	}
};

template <typename T>
std::string GetTypeName()
{
	return GetTypeNameHelper<T>::GetTypeName();
}

enum BasicType
{
	BT_INT_8 = 1,
	BT_INT_16,
	BT_INT_32,
	BT_INT_64,
	BT_UNSIGNED_INT_8,
	BT_UNSIGNED_INT_16,
	BT_UNSIGNED_INT_32,
	BT_UNSIGNED_INT_64,
	BT_FLOAT,
	BT_DOUBLE
};

class TypeInfo;

template<typename T, typename Cond = void>
struct TypeFiller
{
	static void Fill(TypeInfo& typeInfo) {}
};

class TypeInfoCollection
{
public:
	static TypeInfoCollection& GetInstance();

	template<typename T>
	TypeInfo* RegisterTypeInfo();

	template<typename T>
	TypeInfo* GetTypeInfo();

	TypeInfo* GetTypeInfo(const std::string& typeName);

	template<typename T>
	bool IsTypeInfoRegistered() const;
private:
	std::unordered_map <std::string, TypeInfo> m_typeInfos;
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
	std::function<void(void*&, size_t&)> createDefaultValue;
	std::function<void(void*)> deleteValue;

	const std::string& GetName() const { return m_name; }

	template<typename ObjectType>
	void Init()
	{
		createDefaultValue = AllocateDefaultValue<ObjectType>;
		deleteValue = DeallocateValue<ObjectType>;

		if (std::is_fundamental<ObjectType>::value)
		{
			type = Fundamental;
			fundamentalTypeParams.typeIndex = TypeToId<ObjectType>();
			fundamentalTypeParams.typeSize = GetTypeSize<ObjectType>();
			
		}
		else if (std::is_enum<ObjectType>::value)
		{
			type = Enum;

			TypeFiller<ObjectType>::Fill(*this);
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

			auto& typeInfoCollection = TypeInfoCollection::GetInstance();
			if (!typeInfoCollection.IsTypeInfoRegistered<RawObjectType>())
			{
				typeInfoCollection.RegisterTypeInfo<RawObjectType>();
			}
			underlyingType = typeInfoCollection.GetTypeInfo<RawObjectType>();
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

		const std::string typeName = GetTypeName<ObjectType>();
	}

	Type type = Type::Undefined;
	uintptr_t objectDescId = 0U;
	TypeInfo* underlyingType = nullptr;
	std::string m_name;

	friend class TypeInfoCollection;
};

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

template<typename T>
struct TypeFiller<T, std::enable_if_t<std::is_enum<T>::value>>
{
	static void Fill(TypeInfo& typeInfo)
	{
		using UnderlyingType = std::underlying_type<T>::type;
		typeInfo.fundamentalTypeParams.typeIndex = TypeToId<UnderlyingType>();
		typeInfo.fundamentalTypeParams.typeSize = GetTypeSize<UnderlyingType>();
	}
};

template<typename T>
std::enable_if_t<std::is_default_constructible<T>::value>
AllocateDefaultValue(void*& data, size_t& size)
{
	data = reinterpret_cast<void*>(new T());
	size = sizeof(T);
}

template<typename T>
std::enable_if_t<!std::is_default_constructible<T>::value>
AllocateDefaultValue(void*& data, size_t& size)
{
	data = nullptr;
	size = 0U;
}

template<typename T>
std::enable_if_t<std::is_default_constructible<T>::value>
DeallocateValue(void* data)
{
	auto t = reinterpret_cast<T*>(data);
	delete t;
}

template<typename T>
std::enable_if_t<!std::is_default_constructible<T>::value>
DeallocateValue(void* data)
{
}

TypeInfoCollection& TypeInfoCollection::GetInstance()
{
	static TypeInfoCollection typeInfoCollection;
	return typeInfoCollection;
}

template<typename T>
TypeInfo* TypeInfoCollection::RegisterTypeInfo()
{
	const auto typeName = GetTypeName<T>();
	auto& typeInfo = m_typeInfos[typeName];
	typeInfo.Init<T>();
	typeInfo.m_name = typeName;

	return &typeInfo;
}

template<typename T>
TypeInfo* TypeInfoCollection::GetTypeInfo()
{
	const auto typeName = GetTypeName<T>();
	auto findResult = m_typeInfos.find(typeName);
	assert(findResult != m_typeInfos.end());

	return &(findResult->second);
}

TypeInfo* TypeInfoCollection::GetTypeInfo(const std::string& typeName)
{
	auto findResult = m_typeInfos.find(typeName);
	assert(findResult != m_typeInfos.end());

	return &(findResult->second);
}

template<typename T>
bool TypeInfoCollection::IsTypeInfoRegistered() const
{
	const auto typeName = GetTypeName<T>();
	auto findResult = m_typeInfos.find(typeName);
	return findResult != m_typeInfos.end();
}

#endif // !TYPE_INFO_INCLUDE

