#include "JsonSerializer.h"

#include <iostream>

template<typename T>
struct Getter
{
	static std::string Get()
	{
		return std::string(__FUNCTION__);
	}
};

struct Vec
{
	virtual ~Vec() {}
};

struct Vec1 : Vec
{
	float x = 0.0f;
};

struct Vec2 : Vec1
{
	float y = 0.0f;
};

struct Vec3 : Vec2
{
    float z = 2.3f;
};

struct Vec4 : Vec3
{
	float w = 0.0f;
};

enum class MyEnum
{
	First,
	Second,
	Third
};

class BaseClass
{
public:
	int a = 0;
	float b = 0.0f;
};

class DerivedClass : public BaseClass
{
public:
	float c = 0.0f;
};

struct TestStruct
{
	int intValue = 5;

	int GetValue() const { return intValue; }
	void SetValue(const int val) { intValue = val; }
	float floatValue = 7.0f;
	int someArray[5] = { 1, 4, 5, 8, 9 };
	int someMatrix[3][3] = { {1, 2, 3}, {3, 3, 3}, {6, 6, 6} };
	std::vector<int> someVector;
	std::array<int, 3> someStaticArray;
	std::map<std::string, float> someMap;
	Vec3 vec3;
	Vec* vec = nullptr;
	MyEnum someEnum;
	std::string someString;

	Vec3 privateVec3;
	const Vec3 GetVec3() const { return privateVec3; }
	void SetVec3(const Vec3& value) { privateVec3 = value; }
};

struct TestStruct2
{
	int intVal = 1;
	Vec2* vec2 = nullptr;
};

struct TestStruct3
{
	float floatVal = 9.0f;
	Vec2* vec2 = nullptr;
};

int main()
{
	class_<Vec>("Vec");

	class_<Vec1, Vec>("Vec1")
		.AddProperty("x", &Vec1::x);

	class_<Vec2, Vec1>("Vec2")
		.AddProperty("y", &Vec2::y);

	class_<Vec3, Vec2>("Vec3")
		.AddProperty("z", &Vec3::z);

	class_<Vec4, Vec3>("Vec4")
		.AddProperty("w", &Vec4::w);

	class_<TestStruct>("TestStruct")
		.AddProperty("intValue", &TestStruct::GetValue, &TestStruct::SetValue)
		.AddProperty("floatValue", &TestStruct::floatValue)
		.AddProperty("someArray", &TestStruct::someArray)
		.AddProperty("someStaticArray", &TestStruct::someStaticArray)
		.AddProperty("someMatrix", &TestStruct::someMatrix)
		.AddProperty("someVector", &TestStruct::someVector)
		.AddProperty("vec3", &TestStruct::vec3)
		.AddProperty("someMap", &TestStruct::someMap)
		.AddProperty("vec", &TestStruct::vec)
		.AddProperty("someEnum", &TestStruct::someEnum)
		.AddProperty("someString", &TestStruct::someString)
		.AddProperty("Vec3Accessor", &TestStruct::GetVec3, &TestStruct::SetVec3)
		;

	class_<TestStruct2>("TestStruct2")
		.AddProperty("intValue", &TestStruct2::intVal)
		.AddProperty("vec2", &TestStruct2::vec2)
		;

	class_<TestStruct3>("TestStruct3")
		.AddProperty("floatValue", &TestStruct3::floatVal)
		.AddProperty("vec2", &TestStruct3::vec2)
		;

	class_<BaseClass>("Base")
		.AddProperty("a", &BaseClass::a)
		.AddProperty("b", &BaseClass::b)
		;

	class_<DerivedClass, BaseClass>("Derived")
		.AddProperty("c", &DerivedClass::c)
		;

	Vec3* vec3 = new Vec3();
	vec3->x = 555.7f;
	vec3->y = 6.2f;
	vec3->z = 7777.0f;

	TestStruct objectToSerialize_1, objectToSerialize_2;
	objectToSerialize_1.intValue = 7;
	objectToSerialize_1.floatValue = 23.5f;
	objectToSerialize_1.someStaticArray[1] = 100;
	objectToSerialize_1.someArray[3] = 900;
	objectToSerialize_1.someArray[4] = 7788;
	objectToSerialize_1.someMatrix[1][1] = 333333;
	objectToSerialize_1.someMap["One"] = 3.0f;
	objectToSerialize_1.someMap["BBBB"] = 444.4f;
	objectToSerialize_1.someEnum = MyEnum::Second;
	objectToSerialize_1.someString = "AAA";
	objectToSerialize_1.vec3.z = 111.555f;
	objectToSerialize_1.vec = vec3;

	objectToSerialize_2.vec = vec3;

	TestStruct2 objectOfTestStruct2, objectDesOfTestStruct2;
	objectOfTestStruct2.vec2 = vec3;
	objectOfTestStruct2.intVal = 999;

	TestStruct3 objectOfTestStruct3, objectDesOfTestStruct3;
	objectOfTestStruct3.vec2 = vec3;
	objectOfTestStruct3.floatVal = 444.3f;

	JsonSerializer serializer("test.json");
 	serializer.Serialize(objectToSerialize_1);
	serializer.Serialize(objectOfTestStruct2);
	serializer.Serialize(objectOfTestStruct3);
	serializer.SerializePointers();

	TestStruct objectToDeserialize;
	serializer.Deserialize(objectToDeserialize);
	serializer.Deserialize(objectDesOfTestStruct2);
	serializer.Deserialize(objectDesOfTestStruct3);
	serializer.DeserializePointers();

	serializer.Clear();

	auto r = Getter<int>::Get();

    return 0;
}