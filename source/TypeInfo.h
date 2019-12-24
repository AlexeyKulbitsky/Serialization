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

REGISTER_BASIC_TYPE(int, 1, sizeof (int))
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

	struct VectorParams
	{
		using SizeGetter = std::function<size_t(void*)>;
		using ItemGetter = std::function<void*(void*, const size_t)>;
		using SizeSetter = std::function<void(void*, size_t)>;

		SizeGetter getSize;
		ItemGetter getItem;
		SizeSetter setSize;

	} vectorParams;

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

	const Type GetType() const { return m_type; }
	const uintptr_t GetObjectDescId() const { return m_objectDescId; }
	const std::unique_ptr<TypeInfo>& GetUnderlyingType() const { return m_underlyingType; }
	const ArrayType GetArrayType() const { return m_arrayType; }
	const size_t GetElementsCount() const { return m_elementsCount; }
	const size_t GetElementSize() const { return m_elementSize; }
	const size_t GetTypeId() const { return m_typeIndex; }

	template<typename ObjectType>
	void Init()
	{
		if (std::is_fundamental<ObjectType>::value)
		{
			m_typeIndex = TypeToId<ObjectType>();
			m_elementSize = GetTypeSize<ObjectType>();
			m_type = Fundamental;
		}
		else if (std::is_enum<ObjectType>::value)
		{
			m_type = Enum;
		}
		else if (std::is_array<ObjectType>::value)
		{
			TypeDeducer<ObjectType> deducer;
			m_type = Array;
			m_arrayType = ArrayType::CStyleArray;

			// Get the size of array and the type of element
			m_elementsCount = deducer.elementsCount;

			auto typeInfo = std::make_unique<TypeInfo>();
			m_underlyingType = std::move(typeInfo);
			m_underlyingType->m_type = deducer.underlyingType;
			m_underlyingType->m_elementSize = deducer.elementSize;

			using ElementType = std::remove_extent<ObjectType>::type;
			m_underlyingType->Init<ElementType>();
		}
		else if (std::is_pointer<ObjectType>::value)
		{
			m_type = Pointer;
			using RawObjectType = std::remove_pointer<ObjectType>::type;

			auto typeInfo = std::make_unique<TypeInfo>();
			m_underlyingType = std::move(typeInfo);
			m_underlyingType->Init<RawObjectType>();
		}
		else if (is_string<ObjectType>::value)
		{
			m_type = String;
		}
		else if (is_vector<ObjectType>::value)
		{
			TypeDeducer<ObjectType> deducer;
			m_type = Array;
			m_arrayType = ArrayType::Vector;

			m_elementsCount = 0U;
			m_underlyingType = std::make_unique<TypeInfo>();
			m_underlyingType->m_type = deducer.underlyingType;
			m_underlyingType->m_elementSize = deducer.elementSize;

			using ElemetType = remove_vector_extent<ObjectType>::type;
			m_underlyingType->Init<ElemetType>();

			vectorParams.getSize = VectorSizeGetter<ElemetType>;
			vectorParams.getItem = VectorItemGetter<ElemetType>;
			vectorParams.setSize = VectorSizeSetter<ElemetType>;
		}
		else if (is_map<ObjectType>::value)
		{
			TypeDeducer<ObjectType> deducer;
			m_type = Map;
			mapParams.keyTypeInfo = std::make_unique<TypeInfo>();
			mapParams.valueTypeInfo = std::make_unique<TypeInfo>();

			mapParams.keyTypeInfo->m_type = deducer.keyType;
			mapParams.valueTypeInfo->m_type = deducer.valueType;

			using KeyType = extract_key_value_from_map<ObjectType>::key_type;
			using ValueType = extract_key_value_from_map<ObjectType>::value_type;
			/*mapParams.getSize = MapSizeGetter<ObjectType>;
			mapParams.getIterator = MapIteratorGetter<ObjectType>;
			mapParams.getKey = MapKeyGetter<ObjectType>;
			mapParams.getValue = MapValueGetter<ObjectType>;
			mapParams.isIteratorValid = MapIteratorValidator<ObjectType>;
			mapParams.incrementIterator = MapIteratorIncrementator<ObjectType>;
			mapParams.setKeyValue = MapKeyValueSetter<ObjectType>;*/
		}
		else if (std::is_class<ObjectType>::value)
		{
			const auto& objectFactory = ObjectFactory::GetInstance();
			if (objectFactory.IsObjectRegistered<ObjectType>())
			{
				m_type = Class;
				m_objectDescId = objectFactory.GetObjectId<ObjectType>();
			}
		}
	}

private:
	Type m_type = Type::Undefined;
	ArrayType m_arrayType = ArrayType::Undefined;
	uintptr_t m_objectDescId = 0U;
	size_t m_elementsCount = 0U;
	size_t m_elementSize = 0U;
	std::unique_ptr<TypeInfo> m_underlyingType;
	size_t m_typeIndex = 0U;
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

#endif // !TYPE_INFO_INCLUDE

