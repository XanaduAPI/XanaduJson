//
// Created by Administrator on 2021/1/14.
//

#ifndef			_XANADU_JSON_OBJECT_H_
#define			_XANADU_JSON_OBJECT_H_

#include <XanaduJson/XanaduJsonHeader.h>

/// Json objects from the Xanadu family
/// JSON TYPE
#define	XJson_False		0
#define XJson_True		1
#define XJson_NULL		2
#define XJson_Int		3
#define XJson_Double		4
#define XJson_String		5
#define XJson_Array		6
#define XJson_Object		7

#define XJson_IsReference	256

/// XANADU_JSON_INFO结构
typedef struct _XANADU_JSON_INFO
{
	struct _XANADU_JSON_INFO*	prev;		/// next/prev允许您遍历数组/对象链。或者，使用GetArraySize/GetArrayItem/GetObjectItem
	struct _XANADU_JSON_INFO*	next;		/// next/prev允许您遍历数组/对象链。或者，使用GetArraySize/GetArrayItem/GetObjectItem
	struct _XANADU_JSON_INFO*	child;		/// 数组或对象项将有一个子指针指向数组/对象中的链表
	int				type;		/// 项目的类型
	char*				valuestring;	/// 字符串，如果type==XJson_String
	int64U				valueint;	/// 整型，如果type==XJson_Number
	double				valuedouble;	/// 浮点数，如果type==XJson_Number
	int				sign;		/// valueint的符号，1（无符号），-1（有符号）
	char*				string;		/// 项的名称字符串，如果此项是对象的子项，或位于对象的子项列表中
}XANADU_JSON_INFO;


/// 提供一个JSON块，这将返回一个可以查询的XANADU_JSON_INFO对象。完成后请调用XJson_Delete
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_Parse(const char* value) XANADU_NOTHROW;

/// 将XANADU_JSON_INFO实体呈现为文本以进行传输/存储。完成后释放字符
XANADU_JSON_EXPORT char* XJson_Print(XANADU_JSON_INFO* item) XANADU_NOTHROW;

/// 将XANADU_JSON_INFO实体呈现为文本，以便传输/存储，而无需任何格式。完成后释放字符
XANADU_JSON_EXPORT char* XJson_PrintUnformatted(XANADU_JSON_INFO* item) XANADU_NOTHROW;

/// 删除XANADU_JSON_INFO实体及其所有子实体
XANADU_JSON_EXPORT void XJson_Delete(XANADU_JSON_INFO* _Json) XANADU_NOTHROW;


/// 返回数组（或对象）中的项数
XANADU_JSON_EXPORT int XJson_GetArraySize(XANADU_JSON_INFO* array) XANADU_NOTHROW;


/// 从数组“array”中检索项目编号“item”。如果不成功，则返回NULL
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_GetArrayItem(XANADU_JSON_INFO* array, int item) XANADU_NOTHROW;


/// 从对象获取项“string”。不区分大小写
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_GetObjectItem(XANADU_JSON_INFO* object, const char* string) XANADU_NOTHROW;


/// 用于分析失败的原因。这将返回一个指向解析错误的指针。当XJson_Parse（）返回0时定义
XANADU_JSON_EXPORT const char* XJson_GetErrorPtr() XANADU_NOTHROW;


/// 这些调用创建适当类型的XANADU_JSON_INFO项
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_CreateNull() XANADU_NOTHROW;

/// 这些调用创建适当类型的XANADU_JSON_INFO项
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_CreateTrue() XANADU_NOTHROW;

/// 这些调用创建适当类型的XANADU_JSON_INFO项
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_CreateFalse() XANADU_NOTHROW;

/// 这些调用创建适当类型的XANADU_JSON_INFO项
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_CreateBool(int b) XANADU_NOTHROW;

/// 这些调用创建适当类型的XANADU_JSON_INFO项
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_CreateDouble(double num, int sign) XANADU_NOTHROW;

/// 这些调用创建适当类型的XANADU_JSON_INFO项
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_CreateInt(int64U num, int sign) XANADU_NOTHROW;

/// 这些调用创建适当类型的XANADU_JSON_INFO项
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_CreateString(const char* string) XANADU_NOTHROW;

/// 这些调用创建适当类型的XANADU_JSON_INFO项
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_CreateArray() XANADU_NOTHROW;

/// 这些调用创建适当类型的XANADU_JSON_INFO项
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_CreateObject() XANADU_NOTHROW;


/// 创建int数组
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_CreateIntArray(int* numbers, int sign, int count) XANADU_NOTHROW;

/// 创建float数组
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_CreateFloatArray(float* numbers, int count) XANADU_NOTHROW;

/// 创建double数组
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_CreateDoubleArray(double* numbers, int count) XANADU_NOTHROW;

/// 创建const char*数组
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_CreateStringArray(const char** strings, int count) XANADU_NOTHROW;


/// 将项附加到指定的数组/对象
XANADU_JSON_EXPORT void XJson_AddItemToArray(XANADU_JSON_INFO* array, XANADU_JSON_INFO* item) XANADU_NOTHROW;

/// 将项附加到指定的数组/对象
XANADU_JSON_EXPORT void XJson_AddItemToArrayHead(XANADU_JSON_INFO* array, XANADU_JSON_INFO* item) XANADU_NOTHROW;

/// 将项附加到指定的数组/对象
XANADU_JSON_EXPORT void XJson_AddItemToObject(XANADU_JSON_INFO* object, const char* string, XANADU_JSON_INFO* item) XANADU_NOTHROW;


/// 将对项的引用附加到指定的数组/对象。如果要将现有的XANADU_JSON_INFO添加到新的XANADU_JSON_INFO，但不希望损坏现有的XANADU_JSON_INFO，请使用此选项
XANADU_JSON_EXPORT void XJson_AddItemReferenceToArray(XANADU_JSON_INFO* array, XANADU_JSON_INFO* item) XANADU_NOTHROW;

/// 将对项的引用附加到指定的数组/对象。如果要将现有的XANADU_JSON_INFO添加到新的XANADU_JSON_INFO，但不希望损坏现有的XANADU_JSON_INFO，请使用此选项
XANADU_JSON_EXPORT void XJson_AddItemReferenceToObject(XANADU_JSON_INFO* object, const char* string, XANADU_JSON_INFO* item) XANADU_NOTHROW;


/// 从数组/对象中移除/取消匹配项
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_DetachItemFromArray(XANADU_JSON_INFO* array, int which) XANADU_NOTHROW;

/// 从数组/对象中移除/取消匹配项
XANADU_JSON_EXPORT void XJson_DeleteItemFromArray(XANADU_JSON_INFO* array, int which) XANADU_NOTHROW;

/// 从数组/对象中移除/取消匹配项
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_DetachItemFromObject(XANADU_JSON_INFO* object, const char* string) XANADU_NOTHROW;

/// 从数组/对象中移除/取消匹配项
XANADU_JSON_EXPORT void XJson_DeleteItemFromObject(XANADU_JSON_INFO* object, const char* string) XANADU_NOTHROW;


/// 更新数组项
XANADU_JSON_EXPORT void XJson_ReplaceItemInArray(XANADU_JSON_INFO* array, int which, XANADU_JSON_INFO* newitem) XANADU_NOTHROW;

/// 更新数组项
XANADU_JSON_EXPORT void XJson_ReplaceItemInObject(XANADU_JSON_INFO* object, const char* string, XANADU_JSON_INFO* newitem) XANADU_NOTHROW;



#define XJson_AddNullToObject(_Object,_Name)			XJson_AddItemToObject(_Object, _Name, XJson_CreateNull())
#define XJson_AddTrueToObject(_Object,_Name)			XJson_AddItemToObject(_Object, _Name, XJson_CreateTrue())
#define XJson_AddFalseToObject(_Object,_Name)			XJson_AddItemToObject(_Object, _Name, XJson_CreateFalse())
#define XJson_AddNumberToObject(_Object,_Name,_Number)		XJson_AddItemToObject(_Object, _Name, XJson_CreateInt(_Number))
#define XJson_AddStringToObject(_Object,_Name,_String)		XJson_AddItemToObject(_Object, _Name, XJson_CreateString(_String))




/// Xanadu Class Json
class XANADU_JSON_EXPORT XJsonObject
{
private:
	XJsonObject(XANADU_JSON_INFO* _JsonData) XANADU_NOTHROW;

public:
	XJsonObject() XANADU_NOTHROW;

	XJsonObject(const UString& _JsonString) XANADU_NOTHROW;

	XJsonObject(const XJsonObject* _JsonObject) XANADU_NOTHROW;

	XJsonObject(const XJsonObject& _JsonObject) XANADU_NOTHROW;

	virtual ~XJsonObject() XANADU_NOTHROW;

public:
	virtual XJsonObject& operator=(const XJsonObject& _Value) XANADU_NOTHROW;

	virtual bool operator==(const XJsonObject& _Value) const XANADU_NOTHROW;

	virtual bool Parse(const UString& strJson) XANADU_NOTHROW;

	virtual void Clear() XANADU_NOTHROW;

	virtual bool IsEmpty() const XANADU_NOTHROW;

	virtual bool IsArray() const XANADU_NOTHROW;

	virtual UString ToString() const XANADU_NOTHROW;

	virtual XString ToXString() const XANADU_NOTHROW;

	virtual UString ToFormattedString() const XANADU_NOTHROW;

	virtual const XString& GetErrMsg() const XANADU_NOTHROW;

public:
	virtual bool AddEmptySubObject(const UString& _Key) XANADU_NOTHROW;

	virtual bool AddEmptySubArray(const UString& _Key) XANADU_NOTHROW;

	virtual XJsonObject& operator[](const UString& _Key) XANADU_NOTHROW;

	virtual UString operator()(const UString& _Key) const XANADU_NOTHROW;

private:
	virtual bool Get(const UString& _Key, XJsonObject& _Value) const XANADU_NOTHROW;

	virtual bool Get(const UString& _Key, UString& _Value) const XANADU_NOTHROW;

	virtual bool Get(const UString& _Key, int32S& _Value) const XANADU_NOTHROW;

	virtual bool Get(const UString& _Key, int32U& _Value) const XANADU_NOTHROW;

	virtual bool Get(const UString& _Key, int64S& _Value) const XANADU_NOTHROW;

	virtual bool Get(const UString& _Key, int64U& _Value) const XANADU_NOTHROW;

	virtual bool Get(const UString& _Key, bool& _Value) const XANADU_NOTHROW;

	virtual bool Get(const UString& _Key, float& _Value) const XANADU_NOTHROW;

	virtual bool Get(const UString& _Key, double& _Value) const XANADU_NOTHROW;

private:
	virtual bool Add(const UString& _Key, const XJsonObject& _Value) XANADU_NOTHROW;

	virtual bool Add(const UString& _Key, const UString& _Value) XANADU_NOTHROW;

	virtual bool Add(const UString& _Key, const XString& _Value) XANADU_NOTHROW;

	virtual bool Add(const UString& _Key, int32S _Value) XANADU_NOTHROW;

	virtual bool Add(const UString& _Key, int32U _Value) XANADU_NOTHROW;

	virtual bool Add(const UString& _Key, int64S _Value) XANADU_NOTHROW;

	virtual bool Add(const UString& _Key, int64U _Value) XANADU_NOTHROW;

	virtual bool Add(const UString& _Key, bool _Value, bool _Again) XANADU_NOTHROW;

	virtual bool Add(const UString& _Key, float _Value) XANADU_NOTHROW;

	virtual bool Add(const UString& _Key, double _Value) XANADU_NOTHROW;

public:
	virtual bool Append(const UString& _Key, const XJsonObject& _Value) XANADU_NOTHROW;

	virtual bool Append(const UString& _Key, const XByteArray& _Value) XANADU_NOTHROW;

	virtual bool Append(const UString& _Key, const XString& _Value) XANADU_NOTHROW;

	virtual bool Append(const UString& _Key, int32S _Value) XANADU_NOTHROW;

	virtual bool Append(const UString& _Key, int32U _Value) XANADU_NOTHROW;

	virtual bool Append(const UString& _Key, int64S _Value) XANADU_NOTHROW;

	virtual bool Append(const UString& _Key, int64U _Value) XANADU_NOTHROW;

	virtual bool Append(const UString& _Key, bool _Value, bool _Again) XANADU_NOTHROW;

	virtual bool Append(const UString& _Key, float _Value) XANADU_NOTHROW;

	virtual bool Append(const UString& _Key, double _Value) XANADU_NOTHROW;

public:
	virtual bool Delete(const UString& _Key) XANADU_NOTHROW;

public:
	virtual bool Replace(const UString& _Key, const XJsonObject& _Value) XANADU_NOTHROW;

	virtual bool Replace(const UString& _Key, const UString& _Value) XANADU_NOTHROW;

	virtual bool Replace(const UString& _Key, int32S _Value) XANADU_NOTHROW;

	virtual bool Replace(const UString& _Key, int32U _Value) XANADU_NOTHROW;

	virtual bool Replace(const UString& _Key, int64S _Value) XANADU_NOTHROW;

	virtual bool Replace(const UString& _Key, int64U _Value) XANADU_NOTHROW;

	virtual bool Replace(const UString& _Key, bool _Value, bool _Again) XANADU_NOTHROW;

	virtual bool Replace(const UString& _Key, float _Value) XANADU_NOTHROW;

	virtual bool Replace(const UString& _Key, double _Value) XANADU_NOTHROW;

public:
	virtual XJsonObject ToObject(UString _Key) const XANADU_NOTHROW;

	virtual UString ToString(UString _Key) const XANADU_NOTHROW;

	virtual XByteArray ToBytes(const UString& _Key) const XANADU_NOTHROW;

	virtual XString ToXString(UString _Key) const XANADU_NOTHROW;

	virtual double ToDouble(UString _Key) const XANADU_NOTHROW;

	virtual int32S ToInt(UString _Key) const XANADU_NOTHROW;

	virtual int32U ToUint(UString _Key) const XANADU_NOTHROW;

	virtual int64S ToLong(UString _Key) const XANADU_NOTHROW;

	virtual int64U ToUlong(UString _Key) const XANADU_NOTHROW;

	virtual bool ToBool(UString _Key) const XANADU_NOTHROW;

public:
	virtual int GetArraySize() const XANADU_NOTHROW;

	virtual XJsonObject& operator[](unsigned int _Which) XANADU_NOTHROW;

	virtual UString operator()(unsigned int _Which) const XANADU_NOTHROW;

public:
	virtual bool Get(int _Which, XJsonObject& _Value) const XANADU_NOTHROW;

	virtual bool Get(int _Which, UString& _Value) const XANADU_NOTHROW;

	virtual bool Get(int _Which, int32S& _Value) const XANADU_NOTHROW;

	virtual bool Get(int _Which, int32U& _Value) const XANADU_NOTHROW;

	virtual bool Get(int _Which, int64S& _Value) const XANADU_NOTHROW;

	virtual bool Get(int _Which, int64U& _Value) const XANADU_NOTHROW;

	virtual bool Get(int _Which, bool& _Value) const XANADU_NOTHROW;

	virtual bool Get(int _Which, float& _Value) const XANADU_NOTHROW;

	virtual bool Get(int _Which, double& _Value) const XANADU_NOTHROW;

public:
	virtual bool Add(const XJsonObject& _Value) XANADU_NOTHROW;

	virtual bool Add(const UString& _Value) XANADU_NOTHROW;

	virtual bool Add(int32S _Value) XANADU_NOTHROW;

	virtual bool Add(int32U _Value) XANADU_NOTHROW;

	virtual bool Add(int64S _Value) XANADU_NOTHROW;

	virtual bool Add(int64U _Value) XANADU_NOTHROW;

	virtual bool Add(int _Anywhere, bool _Value) XANADU_NOTHROW;

	virtual bool Add(float _Value) XANADU_NOTHROW;

	virtual bool Add(double _Value) XANADU_NOTHROW;

public:
	virtual bool AddAsFirst(const XJsonObject& _Value) XANADU_NOTHROW;

	virtual bool AddAsFirst(const UString& _Value) XANADU_NOTHROW;

	virtual bool AddAsFirst(int32S _Value) XANADU_NOTHROW;

	virtual bool AddAsFirst(int32U _Value) XANADU_NOTHROW;

	virtual bool AddAsFirst(int64S _Value) XANADU_NOTHROW;

	virtual bool AddAsFirst(int64U _Value) XANADU_NOTHROW;

	virtual bool AddAsFirst(int _Anywhere, bool _Value) XANADU_NOTHROW;

	virtual bool AddAsFirst(float _Value) XANADU_NOTHROW;

	virtual bool AddAsFirst(double _Value) XANADU_NOTHROW;


public:
	virtual bool Delete(int _Which) XANADU_NOTHROW;

public:
	virtual bool Replace(int _Which, const XJsonObject& _Value) XANADU_NOTHROW;

	virtual bool Replace(int _Which, const UString& _Value) XANADU_NOTHROW;

	virtual bool Replace(int _Which, int32S _Value) XANADU_NOTHROW;

	virtual bool Replace(int _Which, int32U _Value) XANADU_NOTHROW;

	virtual bool Replace(int _Which, int64S _Value) XANADU_NOTHROW;

	virtual bool Replace(int _Which, int64U _Value) XANADU_NOTHROW;

	virtual bool Replace(int _Which, bool _Value, bool _Again) XANADU_NOTHROW;

	virtual bool Replace(int _Which, float _Value) XANADU_NOTHROW;

	virtual bool Replace(int _Which, double _Value) XANADU_NOTHROW;

private:
	XANADU_JSON_INFO*					_ThisJsonData;

	XANADU_JSON_INFO*					_ThisExternJsonDataRef;

	XString							_ThisErrorMessage;

	std::map<unsigned int, XJsonObject*>*			_ThisJsonArrayRef;

	std::map<UString, XJsonObject*>*			_ThisJsonObjectRef;
};

#endif /// _XANADU_JSON_OBJECT_H_
