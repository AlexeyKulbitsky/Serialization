#ifndef SERIALIZABLE_INCLUDE
#define SERIALIZABLE_INCLUDE

#define SOME_OBJECT(TypeName)					                    \
public:																\
using ClassType = TypeName;											\
static const char* GetTypeNameStatic() { return #TypeName; }		\
virtual const char* GetTypeName() override { return GetTypeNameStatic(); }

class Serializable
{
public:
    virtual const char* GetTypeName() = 0;
};

#endif
