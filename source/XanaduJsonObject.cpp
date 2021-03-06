//
// Created by Administrator on 2021/1/14.
//

#include <XanaduJson/XanaduJsonObject.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>

static const char* _StaticErrorPtr = nullptr;
static const unsigned char _StaticFirstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

// JSON String
static int XJson_strcasecmp(const char* _String1, const char* _String2)
{
	if(nullptr == _String1)
	{
		return (_String1 == _String2) ? 0 : 1;
	}
	if(nullptr == _String2)
	{
		return 1;
	}
	for(; tolower(*_String1) == tolower(*_String2); ++_String1, ++_String2)
	{
		if(*_String1 == 0)
		{
			return 0;
		}
	}
	return tolower(*(const unsigned char*)_String1) - tolower(*(const unsigned char*)_String2);
};

// JSON String
static char* XJson_strdup(const char* _String)
{
	auto		vLength = Xanadu::strlen(_String) + 1;
	auto		vCopy = (char*)Xanadu::malloc(vLength);
	if(nullptr == vCopy)
	{
		return nullptr;
	}
	Xanadu::memcpy(vCopy, _String, vLength);
	return vCopy;
};

// 内部构造函数
XANADU_JSON_INFO* XJson_New_Item()
{
	auto		vNode = (XANADU_JSON_INFO*)Xanadu::malloc(sizeof(XANADU_JSON_INFO));
	if(vNode)
	{
		Xanadu::memset(vNode, 0, sizeof(XANADU_JSON_INFO));
	}
	return vNode;
}

// 解析输入文本以生成一个数字，并将结果填充到item中
static const char* parse_number(XANADU_JSON_INFO* _Item, const char* _Num)
{
	long double n = 0, vScale = 0;
	int subscale = 0, signsubscale = 1;
	_Item->sign = 1;

	/* Could use sscanf for this? */
	if(*_Num == '-')
	{
		_Item->sign = -1, _Num++; /* Has sign? */
	}
	if(*_Num == '0')
	{
		_Num++; /* is zero */
	}
	if(*_Num >= '1' && *_Num <= '9')
	{
		do
		{
			n = (n * 10.0) + (*_Num++ - '0');
		}
		while(*_Num >= '0' && *_Num <= '9'); /* Number? */
	}
	if(*_Num == '.' && _Num[1] >= '0' && _Num[1] <= '9')
	{
		_Num++;
		do
		{
			n = (n * 10.0) + (*_Num++ - '0'), vScale--;
		}while(*_Num >= '0' && *_Num <= '9');
	} /* Fractional part? */
	if(*_Num == 'e' || *_Num == 'E') /* Exponent? */
	{
		_Num++;
		if(*_Num == '+')
		{
			_Num++;
		}
		else if(*_Num == '-')
		{
			signsubscale = -1, _Num++; /* With sign? */
		}
		while(*_Num >= '0' && *_Num <= '9')
		{
			subscale = (subscale * 10) + (*_Num++ - '0'); /* Number? */
		}
	}

	if(vScale == 0 && subscale == 0)
	{
		_Item->valuedouble = (double)(_Item->sign * (int64U)n);
		_Item->valueint = (int64U)(_Item->sign * (int64U)n);
		_Item->type = XJson_Int;
	}
	else
	{
		n = _Item->sign * n * pow(10.0, (double)(vScale + subscale * signsubscale)); /* number = +/- number.fraction * 10^+/- exponent */
		_Item->valuedouble = (double)n;
		_Item->valueint = (int64U)n;
		_Item->type = XJson_Double;
	}
	return _Num;
}

// 将给定项中的数字精确地呈现为字符串
static char* print_double(XANADU_JSON_INFO* _Item)
{
	auto		vValue = _Item->valuedouble;
	auto		vString = (char*)Xanadu::malloc(64); /* This is a nice tradeoff. */
	if(vString)
	{
		if(fabs(floor(vValue) - vValue) <= DBL_EPSILON)
		{
			sprintf(vString, "%.0f", vValue);
		}
		else if(fabs(vValue) < 1.0e-6 || fabs(vValue) > 1.0e9)
		{
			sprintf(vString, "%lf", vValue);
		}
		else
		{
			sprintf(vString, "%f", vValue);
		}
	}
	return vString;
}

// 将给定项中的数字精确地呈现为字符串
static char* print_int(XANADU_JSON_INFO* _Item)
{
	auto		vString = (char*)Xanadu::malloc(22); /* 2^64+1 can be represented in 21 chars. */
	if(vString)
	{
		if(_Item->sign == -1)
		{
			if(_Item->valueint <= (int64S)INT_MAX && _Item->valueint >= (int64S)INT_MIN)
			{
				sprintf(vString, "%d", (int32S)_Item->valueint);
			}
			else
			{
				sprintf(vString, "%lld", (int64S)_Item->valueint);
			}
		}
		else
		{
			if(_Item->valueint <= (int64U)UINT_MAX)
			{
				sprintf(vString, "%u", (int32U)_Item->valueint);
			}
			else
			{
				sprintf(vString, "%llu", _Item->valueint);
			}
		}
	}
	return vString;
}

// 将输入文本解析为未转义的cstring，并填充项
static const char* parse_string(XANADU_JSON_INFO* _Item, const char* _String)
{
	auto		vPtr = _String + 1;
	auto		vPtr2 = static_cast<char*>(nullptr);
	auto		vOut = static_cast<char*>(nullptr);
	auto		vLength = static_cast<int>(0);
	auto		vUC1 = static_cast<unsigned int>(0);;
	auto		vUC2 = static_cast<unsigned int>(0);;
	if(*_String != '\"')
	{
		_StaticErrorPtr = _String;
		return nullptr;
	} /* not a string! */

	while(*vPtr != '\"' && *vPtr && ++vLength)
	{
		if(*vPtr++ == '\\')
		{
			vPtr++; /* Skip escaped quotes. */
		}
	}

	vOut = (char*)Xanadu::malloc(vLength + 1); /* This is how long we need for the string, roughly. */
	if(!vOut)
	{
		return nullptr;
	}

	vPtr = _String + 1;
	vPtr2 = vOut;
	while(*vPtr != '\"' && *vPtr)
	{
		if(*vPtr != '\\')
		{
			*vPtr2++ = *vPtr++;
		}
		else
		{
			vPtr++;
			switch(*vPtr)
			{
				case 'b':
					*vPtr2++ = '\b';
					break;
				case 'f':
					*vPtr2++ = '\f';
					break;
				case 'n':
					*vPtr2++ = '\n';
					break;
				case 'r':
					*vPtr2++ = '\r';
					break;
				case 't':
					*vPtr2++ = '\t';
					break;
				case 'u': /* transcode utf16 to utf8. */
					sscanf(vPtr + 1, "%4x", &vUC1);
					vPtr += 4; /* get the unicode char. */

					if((vUC1 >= 0xDC00 && vUC1 <= 0xDFFF) || vUC1 == 0)
					{
						break;	// check for invalid.
					}

					if(vUC1 >= 0xD800 && vUC1 <= 0xDBFF)	// UTF16 surrogate pairs.
					{
						if(vPtr[1] != '\\' || vPtr[2] != 'u')
						{
							break;	// missing second-half of surrogate.
						}
						sscanf(vPtr + 3, "%4x", &vUC2);
						vPtr += 6;
						if(vUC2 < 0xDC00 || vUC2 > 0xDFFF)
						{
							break;	// invalid second-half of surrogate.
						}
						vUC1 = 0x10000 | ((vUC1 & 0x3FF) << 10) | (vUC2 & 0x3FF);
					}

					vLength = 4;
					if(vUC1 < 0x80)
					{
						vLength = 1;
					}
					else if(vUC1 < 0x800)
					{
						vLength = 2;
					}
					else if(vUC1 < 0x10000)
					{
						vLength = 3;
					}
					vPtr2 += vLength;

					switch(vLength)
					{
						case 4:
							*--vPtr2 = ((vUC1 | 0x80) & 0xBF);
							vUC1 >>= 6;
						case 3:
							*--vPtr2 = ((vUC1 | 0x80) & 0xBF);
							vUC1 >>= 6;
						case 2:
							*--vPtr2 = ((vUC1 | 0x80) & 0xBF);
							vUC1 >>= 6;
						case 1:
							*--vPtr2 = (char)(vUC1 | _StaticFirstByteMark[vLength]);
					}
					vPtr2 += vLength;
					break;
				default:
					*vPtr2++ = *vPtr;
					break;
			}
			vPtr++;
		}
	}
	*vPtr2 = 0;
	if(*vPtr == '\"')
	{
		vPtr++;
	}
	_Item->valuestring = vOut;
	_Item->type = XJson_String;
	return vPtr;
}

// 将提供的cstring呈现为可打印的转义版本
static char* print_string_ptr(const char* _String)
{
	auto		vPtr1 = static_cast<const char*>(nullptr);
	auto		vPtr2 = static_cast<char*>(nullptr);
	auto		vOut = static_cast<char*>(nullptr);
	auto		vLength = static_cast<int>(0);
	auto		vToken = static_cast<unsigned char>(0);

	if(nullptr == _String)
	{
		return XJson_strdup("");
	}
	vPtr1 = _String;
	vToken = *vPtr1;
	while(vToken && ++vLength)
	{
		if(Xanadu::strchr("\"\\\b\f\n\r\t", vToken))
		{
			vLength++;
		}
		else if(vToken < 32)
		{
			vLength += 5;
		}
		vPtr1++;
		vToken = *vPtr1;
	}

	vOut = (char*)Xanadu::malloc(vLength + 3);
	if(!vOut)
	{
		return nullptr;
	}

	vPtr2 = vOut;
	vPtr1 = _String;
	*vPtr2++ = '\"';
	while(*vPtr1)
	{
		if((unsigned char)*vPtr1 > 31 && *vPtr1 != '\"' && *vPtr1 != '\\')
		{
			*vPtr2++ = *vPtr1++;
		}
		else
		{
			*vPtr2++ = '\\';
			switch(vToken = *vPtr1++)
			{
				case '\\':
					*vPtr2++ = '\\';
					break;
				case '\"':
					*vPtr2++ = '\"';
					break;
				case '\b':
					*vPtr2++ = 'b';
					break;
				case '\f':
					*vPtr2++ = 'f';
					break;
				case '\n':
					*vPtr2++ = 'n';
					break;
				case '\r':
					*vPtr2++ = 'r';
					break;
				case '\t':
					*vPtr2++ = 't';
					break;
				default:
					sprintf(vPtr2, "u%04x", vToken);
					vPtr2 += 5;
					break; /* escape and print */
			}
		}
	}
	*vPtr2++ = '\"';
	*vPtr2++ = 0;
	return vOut;
}

// 项目上的Invote print_string_ptr（这很有用）
static char* print_string(XANADU_JSON_INFO* _Item)
{
	return print_string_ptr(_Item->valuestring);
}


// 预先公布这些原型
static const char* parse_value(XANADU_JSON_INFO* _Item, const char* _Value);

// 预先公布这些原型
static char* print_value(XANADU_JSON_INFO* _Item, int depth, int _Format);

// 预先公布这些原型
static const char* parse_array(XANADU_JSON_INFO* _Item, const char* _Value);

// 预先公布这些原型
static char* print_array(XANADU_JSON_INFO* _Item, int depth, int _Format);

// 预先公布这些原型
static const char* parse_object(XANADU_JSON_INFO* _Item, const char* _Value);

// 预先公布这些原型
static char* print_object(XANADU_JSON_INFO* _Item, int depth, int _Format);

// 跳转空白和 CR/LF
static const char* skip(const char* _In)
{
	while(_In && *_In && (unsigned char)*_In <= 32)
	{
		_In++;
	}
	return _In;
}


// 解析器核心-当遇到文本时，适当处理
static const char* parse_value(XANADU_JSON_INFO* _Item, const char* _Value)
{
	if(nullptr == _Value)
	{
		return nullptr; /* Fail on null. */
	}
	if(0 == Xanadu::strncmp(_Value, "null", 4))
	{
		_Item->type = XJson_NULL;
		return _Value + 4;
	}
	if(0 == Xanadu::strncmp(_Value, "false", 5))
	{
		_Item->type = XJson_False;
		return _Value + 5;
	}
	if(0 == Xanadu::strncmp(_Value, "true", 4))
	{
		_Item->type = XJson_True;
		_Item->valueint = 1;
		return _Value + 4;
	}
	if(*_Value == '\"')
	{
		return parse_string(_Item, _Value);
	}
	if(*_Value == '-' || (*_Value >= '0' && *_Value <= '9'))
	{
		return parse_number(_Item, _Value);
	}
	if(*_Value == '[')
	{
		return parse_array(_Item, _Value);
	}
	if(*_Value == '{')
	{
		return parse_object(_Item, _Value);
	}

	_StaticErrorPtr = _Value;
	return 0; /* failure. */
}

// 将值呈现为文本
static char* print_value(XANADU_JSON_INFO* _Item, int _Depth, int _Format)
{
	auto		vOut = static_cast<char*>(nullptr);
	if(nullptr == _Item)
	{
		return nullptr;
	}
	switch((_Item->type) & 255)
	{
		case XJson_NULL:
			vOut = XJson_strdup("null");
			break;
		case XJson_False:
			vOut = XJson_strdup("false");
			break;
		case XJson_True:
			vOut = XJson_strdup("true");
			break;
		case XJson_Int:
			vOut = print_int(_Item);
			break;
		case XJson_Double:
			vOut = print_double(_Item);
			break;
		case XJson_String:
			vOut = print_string(_Item);
			break;
		case XJson_Array:
			vOut = print_array(_Item, _Depth, _Format);
			break;
		case XJson_Object:
			vOut = print_object(_Item, _Depth, _Format);
			break;
	}
	return vOut;
}

// 从输入文本生成数组
static const char* parse_array(XANADU_JSON_INFO* _Item, const char* _Value)
{
	auto		vChild = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(*_Value != '[')
	{
		_StaticErrorPtr = _Value;
		return 0;
	} /* not an array! */

	_Item->type = XJson_Array;
	_Value = skip(_Value + 1);
	if(*_Value == ']')
	{
		return _Value + 1; /* empty array. */
	}

	_Item->child = vChild = XJson_New_Item();
	if(!_Item->child)
	{
		return 0; /* memory fail */
	}
	_Value = skip(parse_value(vChild, skip(_Value))); /* skip any spacing, get the value. */
	if(!_Value)
	{
		return 0;
	}

	while(*_Value == ',')
	{
		XANADU_JSON_INFO* new_item = XJson_New_Item();
		if(!new_item)
		{
			return 0; /* memory fail */
		}
		vChild->next = new_item;
		new_item->prev = vChild;
		vChild = new_item;
		_Value = skip(parse_value(vChild, skip(_Value + 1)));
		if(!_Value)
		{
			return 0; /* memory fail */
		}
	}

	if(*_Value == ']')
	{
		return _Value + 1; /* end of array */
	}
	_StaticErrorPtr = _Value;
	return 0; /* malformed. */
}

// 将数组呈现为文本
static char* print_array(XANADU_JSON_INFO* _Item, int _Depth, int _Format)
{
	char** entries;
	char* out = 0, * ptr, * ret;
	int len = 5;
	auto		vChild = _Item->child;
	int numentries = 0, i = 0, fail = 0;

	/* How many entries in the array? */
	while(vChild)
	{
		numentries++;
		vChild = vChild->next;
	}
	/* Allocate an array to hold the values for each */
	entries = (char**)Xanadu::malloc(numentries * sizeof(char*));
	if(!entries)
	{
		return 0;
	}
	Xanadu::memset(entries, 0, numentries * sizeof(char*));
	/* Retrieve all the results: */
	vChild = _Item->child;
	while(vChild && !fail)
	{
		ret = print_value(vChild, _Depth + 1, _Format);
		entries[i++] = ret;
		if(ret)
			len += Xanadu::strlen(ret) + 2 + (_Format ? 1 : 0);
		else
			fail = 1;
		vChild = vChild->next;
	}

	/* If we didn't fail, try to malloc the output string */
	if(!fail)
	{
		out = (char*)Xanadu::malloc(len);
	}
	/* If that fails, we fail. */
	if(!out)
	{
		fail = 1;
	}

	/* Handle failure. */
	if(fail)
	{
		for(i = 0; i < numentries; i++)
		{
			if(entries[i])
			{
				Xanadu::free(entries[i]);
			}
		}
		Xanadu::free(entries);
		return 0;
	}

	/* Compose the output array. */
	*out = '[';
	ptr = out + 1;
	*ptr = 0;
	for(i = 0; i < numentries; i++)
	{
		Xanadu::strcpy(ptr, entries[i]);
		ptr += Xanadu::strlen(entries[i]);
		if(i != numentries - 1)
		{
			*ptr++ = ',';
			if(_Format)
			{
				*ptr++ = ' ';
			}
			*ptr = 0;
		}
		Xanadu::free(entries[i]);
	}
	Xanadu::free(entries);
	*ptr++ = ']';
	*ptr++ = 0;
	return out;
}

// 从文本生成对象
static const char* parse_object(XANADU_JSON_INFO* _Item, const char* _Value)
{
	auto		vChild = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(*_Value != '{')
	{
		_StaticErrorPtr = _Value;
		return 0;
	} /* not an object! */

	_Item->type = XJson_Object;
	_Value = skip(_Value + 1);
	if(*_Value == '}')
	{
		return _Value + 1; /* empty array. */
	}

	_Item->child = vChild = XJson_New_Item();
	if(!_Item->child)
	{
		return 0;
	}
	_Value = skip(parse_string(vChild, skip(_Value)));
	if(!_Value)
	{
		return 0;
	}
	vChild->string = vChild->valuestring;
	vChild->valuestring = 0;
	if(*_Value != ':')
	{
		_StaticErrorPtr = _Value;
		return 0;
	} /* fail! */
	_Value = skip(parse_value(vChild, skip(_Value + 1))); /* skip any spacing, get the value. */
	if(!_Value)
	{
		return 0;
	}

	while(*_Value == ',')
	{
		XANADU_JSON_INFO* new_item = XJson_New_Item();
		if(!new_item)
		{
			return 0; /* memory fail */
		}
		vChild->next = new_item;
		new_item->prev = vChild;
		vChild = new_item;
		_Value = skip(parse_string(vChild, skip(_Value + 1)));
		if(!_Value)
		{
			return 0;
		}
		vChild->string = vChild->valuestring;
		vChild->valuestring = 0;
		if(*_Value != ':')
		{
			_StaticErrorPtr = _Value;
			return 0;
		} /* fail! */
		_Value = skip(parse_value(vChild, skip(_Value + 1))); /* skip any spacing, get the value. */
		if(!_Value)
		{
			return 0;
		}
	}

	if(*_Value == '}')
	{
		return _Value + 1; /* end of array */
	}
	_StaticErrorPtr = _Value;
	return 0; /* malformed. */
}

// 将对象呈现为文本
static char* print_object(XANADU_JSON_INFO* _Item, int _Depth, int _Format)
{
	auto		entries = static_cast<char**>(nullptr);
	auto		names = static_cast<char**>(nullptr);
	auto		out = static_cast<char*>(nullptr);
	auto		ptr = static_cast<char*>(nullptr);
	auto		ret = static_cast<char*>(nullptr);
	auto		str = static_cast<char*>(nullptr);
	auto		len = static_cast<int>(7);
	auto		i = static_cast<int>(0);
	auto		j = static_cast<int>(0);
	auto		vChild = _Item->child;
	auto		numentries = static_cast<int>(0);
	auto		fail = static_cast<int>(0);

	// 计算条目的数量
	while(vChild)
	{
		numentries++;
		vChild = vChild->next;
	}

	// 为名称和对象分配空间
	entries = (char**)Xanadu::malloc(numentries * sizeof(char*));
	if(!entries)
	{
		return nullptr;
	}
	names = (char**)Xanadu::malloc(numentries * sizeof(char*));
	if(!names)
	{
		Xanadu::free(entries);
		return nullptr;
	}
	Xanadu::memset(entries, 0, sizeof(char*) * numentries);
	Xanadu::memset(names, 0, sizeof(char*) * numentries);

	// 将所有结果收集到数组中
	vChild = _Item->child;
	_Depth++;
	if(_Format)
	{
		len += _Depth;
	}
	while(vChild)
	{
		names[i] = str = print_string_ptr(vChild->string);
		entries[i++] = ret = print_value(vChild, _Depth, _Format);
		if(str && ret)
		{
			len += Xanadu::strlen(ret) + Xanadu::strlen(str) + 2 + (_Format ? 2 + _Depth : 0);
		}
		else
		{
			fail = 1;
		}
		vChild = vChild->next;
	}

	// 尝试分配输出字符串
	if(!fail)
	{
		out = (char*)Xanadu::malloc(len);
	}
	if(!out)
	{
		fail = 1;
	}

	// 失败处理
	if(fail)
	{
		for(i = 0; i < numentries; i++)
		{
			if(names[i])
			{
				Xanadu::free(names[i]);
			}
			if(entries[i])
			{
				Xanadu::free(entries[i]);
			}
		}
		Xanadu::free(names);
		Xanadu::free(entries);
		return nullptr;
	}

	/* Compose the output: */
	*out = '{';
	ptr = out + 1;
	if(_Format)
	{
		*ptr++ = '\n';
	}
	*ptr = 0;
	for(i = 0; i < numentries; i++)
	{
		if(_Format)
		{
			for(j = 0; j < _Depth; j++)
			{
				*ptr++ = '\t';
			}
		}
		Xanadu::strcpy(ptr, names[i]);
		ptr += Xanadu::strlen(names[i]);
		*ptr++ = ':';
		if(_Format)
		{
			*ptr++ = '\t';
		}
		Xanadu::strcpy(ptr, entries[i]);
		ptr += Xanadu::strlen(entries[i]);
		if(i != numentries - 1)
		{
			*ptr++ = ',';
		}
		if(_Format)
		{
			*ptr++ = '\n';
		}
		*ptr = 0;
		Xanadu::free(names[i]);
		Xanadu::free(entries[i]);
	}

	Xanadu::free(names);
	Xanadu::free(entries);
	if(_Format)
	{
		for(i = 0; i < _Depth - 1; i++)
		{
			*ptr++ = '\t';
		}
	}
	*ptr++ = '}';
	*ptr++ = 0;
	return out;
}


// 数组列表处理
static void suffix_object(XANADU_JSON_INFO* _Prev, XANADU_JSON_INFO* _Item)
{
	_Prev->next = _Item;
	_Item->prev = _Prev;
}

// 处理参考文献
static XANADU_JSON_INFO* create_reference(XANADU_JSON_INFO* _Item)
{
	auto		vReference = XJson_New_Item();
	if(!vReference)
	{
		return 0;
	}
	Xanadu::memcpy(vReference, _Item, sizeof(XANADU_JSON_INFO));
	vReference->string = 0;
	vReference->type |= XJson_IsReference;
	vReference->next = vReference->prev = 0;
	return vReference;
}


// 提供一个JSON块，这将返回一个可以查询的XANADU_JSON_INFO对象。完成后请调用XJson_Delete
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_Parse(const char* _Value) noexcept
{
	auto		vChild = XJson_New_Item();
	_StaticErrorPtr = nullptr;
	if(nullptr == vChild)
	{
		return nullptr;
	}

	if(nullptr == parse_value(vChild, skip(_Value)))
	{
		XJson_Delete(vChild);
		return nullptr;
	}
	return vChild;
}

// 将XANADU_JSON_INFO实体呈现为文本以进行传输/存储。完成后释放字符
XANADU_JSON_EXPORT char* XJson_Print(XANADU_JSON_INFO* _Item) noexcept
{
	return print_value(_Item, 0, 1);
}

// 将XANADU_JSON_INFO实体呈现为文本，以便传输/存储，而无需任何格式。完成后释放字符
XANADU_JSON_EXPORT char* XJson_PrintUnformatted(XANADU_JSON_INFO* _Item) noexcept
{
	return print_value(_Item, 0, 0);
}

// 删除XANADU_JSON_INFO实体及其所有子实体
XANADU_JSON_EXPORT void XJson_Delete(XANADU_JSON_INFO* _Json) noexcept
{
	auto		vNext = static_cast<XANADU_JSON_INFO*>(nullptr);
	while(_Json)
	{
		vNext = _Json->next;
		if(!(_Json->type & XJson_IsReference) && _Json->child)
		{
			XJson_Delete(_Json->child);
			_Json->child = nullptr;
		}
		if(!(_Json->type & XJson_IsReference) && _Json->valuestring)
		{
			Xanadu::free(_Json->valuestring);
			_Json->valuestring = nullptr;
		}
		if(_Json->string)
		{
			Xanadu::free(_Json->string);
			_Json->string = nullptr;
		}
		Xanadu::free(_Json);
		_Json = vNext;
	}
}


// 返回数组（或对象）中的项数
XANADU_JSON_EXPORT int XJson_GetArraySize(XANADU_JSON_INFO* _Array) noexcept
{
	auto		vChild = _Array->child;
	auto		vIndex = static_cast<int>(0);
	while(vChild)
	{
		vIndex++; vChild = vChild->next;
	}
	return vIndex;
}

// 从数组“array”中检索项目编号“item”。如果不成功，则返回NULL
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_GetArrayItem(XANADU_JSON_INFO* _Array, int _Item) noexcept
{
	auto		vChild = _Array->child;
	while(vChild && _Item > 0)
	{
		_Item--;
		vChild = vChild->next;
	}
	return vChild;
}


// 从对象获取项“string”。不区分大小写
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_GetObjectItem(XANADU_JSON_INFO* _Object, const char* _String) noexcept
{
	auto		vChild = _Object->child;
	while(vChild && XJson_strcasecmp(vChild->string, _String))
	{
		vChild = vChild->next;
	}
	return vChild;
}


// 用于分析失败的原因。这将返回一个指向解析错误的指针。当XJson_Parse（）返回0时定义
XANADU_JSON_EXPORT const char* XJson_GetErrorPtr() noexcept
{
	return _StaticErrorPtr;
}


// 这些调用创建适当类型的XANADU_JSON_INFO项
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_CreateNull() noexcept
{
	auto		vItem = XJson_New_Item();
	if(vItem)
	{
		vItem->type = XJson_NULL;
	}
	return vItem;
}

// 这些调用创建适当类型的XANADU_JSON_INFO项
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_CreateTrue() noexcept
{
	auto		vItem = XJson_New_Item();
	if(vItem)
	{
		vItem->type = XJson_True;
	}
	return vItem;
}

// 这些调用创建适当类型的XANADU_JSON_INFO项
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_CreateFalse() noexcept
{
	auto		vItem = XJson_New_Item();
	if(vItem)
	{
		vItem->type = XJson_False;
	}
	return vItem;
}

// 这些调用创建适当类型的XANADU_JSON_INFO项
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_CreateBool(int _Number) noexcept
{
	auto		vItem = XJson_New_Item();
	if(vItem)
	{
		vItem->type = _Number ? XJson_True : XJson_False;
	}
	return vItem;
}

// 这些调用创建适当类型的XANADU_JSON_INFO项
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_CreateDouble(double _Number, int _Sign) noexcept
{
	auto		vItem = XJson_New_Item();
	if(vItem)
	{
		vItem->type = XJson_Double;
		vItem->valuedouble = _Number;
		vItem->valueint = (int64U)_Number;
		vItem->sign = _Sign;
	}
	return vItem;
}

// 这些调用创建适当类型的XANADU_JSON_INFO项
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_CreateInt(int64U _Number, int _Sign) noexcept
{
	auto		vItem = XJson_New_Item();
	if(vItem)
	{
		vItem->type = XJson_Int;
		vItem->valuedouble = (double)_Number;
		vItem->valueint = (int64U)_Number;
		vItem->sign = _Sign;
	}
	return vItem;
}

// 这些调用创建适当类型的XANADU_JSON_INFO项
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_CreateString(const char* _String) noexcept
{
	auto		vItem = XJson_New_Item();
	if(vItem)
	{
		vItem->type = XJson_String;
		vItem->valuestring = XJson_strdup(_String);
	}
	return vItem;
}

// 这些调用创建适当类型的XANADU_JSON_INFO项
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_CreateArray() noexcept
{
	auto		vItem = XJson_New_Item();
	if(vItem)
	{
		vItem->type = XJson_Array;
	}
	return vItem;
}

// 这些调用创建适当类型的XANADU_JSON_INFO项
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_CreateObject() noexcept
{
	auto		vItem = XJson_New_Item();
	if(vItem)
	{
		vItem->type = XJson_Object;
	}
	return vItem;
}


// 创建int数组
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_CreateIntArray(int* _Numbers, int _Sign, int _Count) noexcept
{
	auto		vCreate = static_cast<XANADU_JSON_INFO*>(nullptr);
	auto		vPrev = static_cast<XANADU_JSON_INFO*>(nullptr);
	auto		vArray = XJson_CreateArray();
	for(auto vIndex = 0; vArray && vIndex < _Count; vIndex++)
	{
		vCreate = XJson_CreateDouble((double)(long double)((unsigned int)_Numbers[vIndex]), _Sign);
		if(vIndex)
		{
			suffix_object(vPrev, vCreate);
		}
		else
		{
			vArray->child = vCreate;
		}
		vPrev = vCreate;
	}
	return vArray;
}

// 创建float数组
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_CreateFloatArray(float* _Numbers, int _Count) noexcept
{
	auto		vCreate = static_cast<XANADU_JSON_INFO*>(nullptr);
	auto		vPrev = static_cast<XANADU_JSON_INFO*>(nullptr);
	auto		vArray = XJson_CreateArray();
	for(auto vIndex = 0; vArray && vIndex < _Count; vIndex++)
	{
		vCreate = XJson_CreateDouble((double)(long double)_Numbers[vIndex], -1);
		if(vIndex)
		{
			suffix_object(vPrev, vCreate);
		}
		else
		{
			vArray->child = vCreate;
		}
		vPrev = vCreate;
	}
	return vArray;
}

// 创建double数组
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_CreateDoubleArray(double* _Numbers, int _Count) noexcept
{
	auto		vCreate = static_cast<XANADU_JSON_INFO*>(nullptr);
	auto		vPrev = static_cast<XANADU_JSON_INFO*>(nullptr);
	auto		vArray = XJson_CreateArray();
	for(auto vIndex = 0; vArray && vIndex < _Count; vIndex++)
	{
		vCreate = XJson_CreateDouble((double)(long double)_Numbers[vIndex], -1);
		if(vIndex)
		{
			suffix_object(vPrev, vCreate);
		}
		else
		{
			vArray->child = vCreate;
		}
		vPrev = vCreate;
	}
	return vArray;
}

// 创建const char*数组
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_CreateStringArray(const char** _Strings, int _Count) noexcept
{
	auto		vCreate = static_cast<XANADU_JSON_INFO*>(nullptr);
	auto		vPrev = static_cast<XANADU_JSON_INFO*>(nullptr);
	auto		vArray = XJson_CreateArray();
	for(auto vIndex = 0; vArray && vIndex < _Count; vIndex++)
	{
		vCreate = XJson_CreateString(_Strings[vIndex]);
		if(vIndex)
		{
			suffix_object(vPrev, vCreate);
		}
		else
		{
			vArray->child = vCreate;
		}
		vPrev = vCreate;
	}
	return vArray;
}


// 将项附加到指定的数组/对象
XANADU_JSON_EXPORT void XJson_AddItemToArray(XANADU_JSON_INFO* _Array, XANADU_JSON_INFO* _Item) noexcept
{
	auto		vChild = _Array->child;
	if(nullptr == _Item)
	{
		return;
	}
	if(vChild)
	{
		while(vChild && vChild->next)
		{
			vChild = vChild->next;
		}
		suffix_object(vChild, _Item);
	}
	else
	{
		_Array->child = _Item;
	}
}

// 将项附加到指定的数组/对象
XANADU_JSON_EXPORT void XJson_AddItemToArrayHead(XANADU_JSON_INFO* _Array, XANADU_JSON_INFO* _Item) noexcept
{
	auto		vChild = _Array->child;
	if(nullptr == _Item)
	{
		return;
	}
	if(vChild)
	{
		_Item->prev = vChild->prev;
		_Item->next = vChild;
		vChild->prev = _Item;
		_Array->child = _Item;
	}
	else
	{
		_Array->child = _Item;
	}
}

// 将项附加到指定的数组/对象
XANADU_JSON_EXPORT void XJson_AddItemToObject(XANADU_JSON_INFO* _Object, const char* _String, XANADU_JSON_INFO* _Item) noexcept
{
	if(nullptr == _Item)
	{
		return;
	}
	if(_Item->string)
	{
		Xanadu::free(_Item->string);
	}
	_Item->string = XJson_strdup(_String);
	XJson_AddItemToArray(_Object, _Item);
}


// 将对项的引用附加到指定的数组/对象。如果要将现有的XANADU_JSON_INFO添加到新的XANADU_JSON_INFO，但不希望损坏现有的XANADU_JSON_INFO，请使用此选项
XANADU_JSON_EXPORT void XJson_AddItemReferenceToArray(XANADU_JSON_INFO* _Array, XANADU_JSON_INFO* _Item) noexcept
{
	XJson_AddItemToArray(_Array, create_reference(_Item));
}

// 将对项的引用附加到指定的数组/对象。如果要将现有的XANADU_JSON_INFO添加到新的XANADU_JSON_INFO，但不希望损坏现有的XANADU_JSON_INFO，请使用此选项
XANADU_JSON_EXPORT void XJson_AddItemReferenceToObject(XANADU_JSON_INFO* _Object, const char* _String, XANADU_JSON_INFO* _Item) noexcept
{
	XJson_AddItemToObject(_Object, _String, create_reference(_Item));
}


// 从数组/对象中移除/取消匹配项
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_DetachItemFromArray(XANADU_JSON_INFO* _Array, int _Which) noexcept
{
	auto		vChild = _Array->child;
	while(vChild && _Which > 0)
	{
		vChild = vChild->next;
		_Which--;
	}
	if(nullptr == vChild)
	{
		return 0;
	}
	if(vChild->prev)
	{
		vChild->prev->next = vChild->next;
	}
	if(vChild->next)
	{
		vChild->next->prev = vChild->prev;
	}
	if(vChild == _Array->child)
	{
		_Array->child = vChild->next;
	}
	vChild->prev = vChild->next = 0;
	return vChild;
}

// 从数组/对象中移除/取消匹配项
XANADU_JSON_EXPORT void XJson_DeleteItemFromArray(XANADU_JSON_INFO* _Array, int _Which) noexcept
{
	XJson_Delete(XJson_DetachItemFromArray(_Array, _Which));
}

// 从数组/对象中移除/取消匹配项
XANADU_JSON_EXPORT XANADU_JSON_INFO* XJson_DetachItemFromObject(XANADU_JSON_INFO* _Object, const char* _String) noexcept
{
	auto		vIndex = static_cast<int>(0);
	auto		vChild = _Object->child;
	while(vChild && XJson_strcasecmp(vChild->string, _String))
	{
		vIndex++;
		vChild = vChild->next;
	}
	if(vChild)
	{
		return XJson_DetachItemFromArray(_Object, vIndex);
	}
	return 0;
}

// 从数组/对象中移除/取消匹配项
XANADU_JSON_EXPORT void XJson_DeleteItemFromObject(XANADU_JSON_INFO* _Object, const char* _String) noexcept
{
	XJson_Delete(XJson_DetachItemFromObject(_Object, _String));
}

// 更新数组项
XANADU_JSON_EXPORT void XJson_ReplaceItemInArray(XANADU_JSON_INFO* _Array, int _Which, XANADU_JSON_INFO* _NewItem) noexcept
{
	auto		vChild = _Array->child;
	while(vChild && _Which > 0)
	{
		vChild = vChild->next;
		_Which--;
	}
	if(nullptr == vChild)
	{
		return;
	}
	_NewItem->next = vChild->next;
	_NewItem->prev = vChild->prev;
	if(_NewItem->next)
	{
		_NewItem->next->prev = _NewItem;
	}
	if(vChild == _Array->child)
	{
		_Array->child = _NewItem;
	}
	else
	{
		_NewItem->prev->next = _NewItem;
	}
	vChild->next = vChild->prev = 0;
	XJson_Delete(vChild);
}

// 更新数组项
XANADU_JSON_EXPORT void XJson_ReplaceItemInObject(XANADU_JSON_INFO* _Object, const char* _String, XANADU_JSON_INFO* _NewItem) noexcept
{
	auto		vIndex = static_cast<int>(0);
	auto		vChild = _Object->child;
	while(vChild && XJson_strcasecmp(vChild->string, _String))
	{
		vIndex++;
		vChild = vChild->next;
	}
	if(vChild)
	{
		_NewItem->string = XJson_strdup(_String);
		XJson_ReplaceItemInArray(_Object, vIndex, _NewItem);
	}
}





XJsonObject::XJsonObject(XANADU_JSON_INFO* _JsonData) noexcept : _ThisJsonData(nullptr), _ThisExternJsonDataRef(_JsonData)
{
	this->_ThisJsonArrayRef = XANADU_NEW std::map<unsigned int, XJsonObject*>();
	this->_ThisJsonObjectRef = XANADU_NEW std::map<UString, XJsonObject*>();
}

XJsonObject::XJsonObject() noexcept : _ThisJsonData(nullptr), _ThisExternJsonDataRef(nullptr)
{
	this->_ThisJsonArrayRef = XANADU_NEW std::map<unsigned int, XJsonObject*>();
	this->_ThisJsonObjectRef = XANADU_NEW std::map<UString, XJsonObject*>();
}

XJsonObject::XJsonObject(const UString& _JsonString) noexcept : _ThisJsonData(nullptr), _ThisExternJsonDataRef(nullptr)
{
	this->_ThisJsonArrayRef = XANADU_NEW std::map<unsigned int, XJsonObject*>();
	this->_ThisJsonObjectRef = XANADU_NEW std::map<UString, XJsonObject*>();
	Parse(_JsonString);
}

XJsonObject::XJsonObject(const XJsonObject* _JsonObject) noexcept : _ThisJsonData(nullptr), _ThisExternJsonDataRef(nullptr)
{
	this->_ThisJsonArrayRef = XANADU_NEW std::map<unsigned int, XJsonObject*>();
	this->_ThisJsonObjectRef = XANADU_NEW std::map<UString, XJsonObject*>();
	if(_JsonObject)
	{
		Parse(_JsonObject->ToString());
	}
}

XJsonObject::XJsonObject(const XJsonObject& _JsonObject) noexcept : _ThisJsonData(nullptr), _ThisExternJsonDataRef(nullptr)
{
	this->_ThisJsonArrayRef = XANADU_NEW std::map<unsigned int, XJsonObject*>();
	this->_ThisJsonObjectRef = XANADU_NEW std::map<UString, XJsonObject*>();
	Parse(_JsonObject.ToString());
}

XJsonObject::~XJsonObject() noexcept
{
	Clear();
	XANADU_DELETE_PTR(this->_ThisJsonArrayRef);
	XANADU_DELETE_PTR(this->_ThisJsonObjectRef);
}



XJsonObject& XJsonObject::operator=(const XJsonObject& _Value) noexcept
{
	Parse(_Value.ToString().data());
	return *this;
}

bool XJsonObject::operator==(const XJsonObject& _Value) const noexcept
{
	return(this->ToString() == _Value.ToString());
}

bool XJsonObject::AddEmptySubObject(const UString& _Key) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateObject();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Object)
	{
		_ThisErrorMessage = L"not a json object! json array?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateObject();
	if(nullptr == vJsonStruct)
	{
		_ThisErrorMessage = L"create sub empty object error!";
		return false;
	}
	XJson_AddItemToObject(vFocusData, _Key.data(), vJsonStruct);
	return true;
}

bool XJsonObject::AddEmptySubArray(const UString& _Key) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateObject();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Object)
	{
		_ThisErrorMessage = L"not a json object! json array?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateArray();
	if(nullptr == vJsonStruct)
	{
		_ThisErrorMessage = L"create sub empty array error!";
		return false;
	}
	XJson_AddItemToObject(vFocusData, _Key.data(), vJsonStruct);
	return true;
}

XJsonObject& XJsonObject::operator[](const UString& _Key) noexcept
{
	auto		vIterator = _ThisJsonObjectRef->find(_Key);
	if(vIterator == _ThisJsonObjectRef->end())
	{
		auto		vJsonStruct = static_cast<XANADU_JSON_INFO*>(nullptr);
		if(_ThisJsonData)
		{
			if(_ThisJsonData->type == XJson_Object)
			{
				vJsonStruct = XJson_GetObjectItem(_ThisJsonData, _Key.data());
			}
		}
		else if(_ThisExternJsonDataRef)
		{
			if(_ThisExternJsonDataRef->type == XJson_Object)
			{
				vJsonStruct = XJson_GetObjectItem(_ThisExternJsonDataRef, _Key.data());
			}
		}
		if(vJsonStruct)
		{
			auto		vJsonObject = new XJsonObject(vJsonStruct);
			_ThisJsonObjectRef->insert(std::pair<UString, XJsonObject*>(_Key, vJsonObject));
			return(*vJsonObject);
		}
		else
		{
			auto		vJsonObject = new XJsonObject();
			_ThisJsonObjectRef->insert(std::pair<UString, XJsonObject*>(_Key, vJsonObject));
			return(*vJsonObject);
		}
	}
	else
	{
		return(*(vIterator->second));
	}
}

XJsonObject& XJsonObject::operator[](unsigned int _Which) noexcept
{
	auto		vIterator = _ThisJsonArrayRef->find(_Which);
	if(vIterator == _ThisJsonArrayRef->end())
	{
		auto		vJsonStruct = static_cast<XANADU_JSON_INFO*>(nullptr);
		if(_ThisJsonData)
		{
			if(_ThisJsonData->type == XJson_Array)
			{
				vJsonStruct = XJson_GetArrayItem(_ThisJsonData, _Which);
			}
		}
		else if(_ThisExternJsonDataRef)
		{
			if(_ThisExternJsonDataRef->type == XJson_Array)
			{
				vJsonStruct = XJson_GetArrayItem(_ThisExternJsonDataRef, _Which);
			}
		}
		if(vJsonStruct)
		{
			auto		vJsonObject = new XJsonObject(vJsonStruct);
			_ThisJsonArrayRef->insert(std::pair<unsigned int, XJsonObject*>(_Which, vJsonObject));
			return(*vJsonObject);
		}
		else
		{
			auto		vJsonObject = new XJsonObject();
			_ThisJsonArrayRef->insert(std::pair<unsigned int, XJsonObject*>(_Which, vJsonObject));
			return(*vJsonObject);
		}
	}
	else
	{
		return(*(vIterator->second));
	}
}



UString XJsonObject::operator()(const UString& _Key) const noexcept
{
	auto		vJsonStruct = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		if(_ThisJsonData->type == XJson_Object)
		{
			vJsonStruct = XJson_GetObjectItem(_ThisJsonData, _Key.data());
		}
	}
	else if(_ThisExternJsonDataRef)
	{
		if(_ThisExternJsonDataRef->type == XJson_Object)
		{
			vJsonStruct = XJson_GetObjectItem(_ThisExternJsonDataRef, _Key.data());
		}
	}
	if(vJsonStruct == nullptr)
	{
		return "";
	}
	if(vJsonStruct->type == XJson_String)
	{
		return vJsonStruct->valuestring;
	}
	else if(vJsonStruct->type == XJson_Int)
	{
		char		vBuffer[128] = { 0 };
		if(vJsonStruct->sign == -1)
		{
			if((int64S)vJsonStruct->valueint <= (int64S)INT_MAX && (int64S)vJsonStruct->valueint >= (int64S)INT_MIN)
			{
				snprintf(vBuffer, sizeof(vBuffer), "%d", (int32S)vJsonStruct->valueint);
			}
			else
			{
				snprintf(vBuffer, sizeof(vBuffer), "%lld", (int64S)vJsonStruct->valueint);
			}
		}
		else
		{
			if(vJsonStruct->valueint <= (int64U)UINT_MAX)
			{
				snprintf(vBuffer, sizeof(vBuffer), "%u", (int32U)vJsonStruct->valueint);
			}
			else
			{
				snprintf(vBuffer, sizeof(vBuffer), "%llu", vJsonStruct->valueint);
			}
		}
		return vBuffer;
	}
	else if(vJsonStruct->type == XJson_Double)
	{
		char		vBuffer[128] = { 0 };
		if(fabs(vJsonStruct->valuedouble) < 1.0e-6 || fabs(vJsonStruct->valuedouble) > 1.0e9)
		{
			snprintf(vBuffer, sizeof(vBuffer), "%e", vJsonStruct->valuedouble);
		}
		else
		{
			snprintf(vBuffer, sizeof(vBuffer), "%f", vJsonStruct->valuedouble);
		}
		return vBuffer;
	}
	else if(vJsonStruct->type == XJson_False)
	{
		return "false";
	}
	else if(vJsonStruct->type == XJson_True)
	{
		return "true";
	}
	return "";
}

UString XJsonObject::operator()(unsigned int _Which) const noexcept
{
	auto		vJsonStruct = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		if(_ThisJsonData->type == XJson_Array)
		{
			vJsonStruct = XJson_GetArrayItem(_ThisJsonData, _Which);
		}
	}
	else if(_ThisExternJsonDataRef)
	{
		if(_ThisExternJsonDataRef->type == XJson_Array)
		{
			vJsonStruct = XJson_GetArrayItem(_ThisExternJsonDataRef, _Which);
		}
	}
	if(vJsonStruct == nullptr)
	{
		return "";
	}
	if(vJsonStruct->type == XJson_String)
	{
		return vJsonStruct->valuestring;
	}
	else if(vJsonStruct->type == XJson_Int)
	{
		char		vBuffer[128] = { 0 };
		if(vJsonStruct->sign == -1)
		{
			if((int64S)vJsonStruct->valueint <= (int64S)INT_MAX && (int64S)vJsonStruct->valueint >= (int64S)INT_MIN)
			{
				snprintf(vBuffer, sizeof(vBuffer), "%d", (int32S)vJsonStruct->valueint);
			}
			else
			{
				snprintf(vBuffer, sizeof(vBuffer), "%lld", (int64S)vJsonStruct->valueint);
			}
		}
		else
		{
			if(vJsonStruct->valueint <= (int64U)UINT_MAX)
			{
				snprintf(vBuffer, sizeof(vBuffer), "%u", (int32U)vJsonStruct->valueint);
			}
			else
			{
				snprintf(vBuffer, sizeof(vBuffer), "%llu", vJsonStruct->valueint);
			}
		}
		return vBuffer;
	}
	else if(vJsonStruct->type == XJson_Double)
	{
		char		vBuffer[128] = { 0 };
		if(fabs(vJsonStruct->valuedouble) < 1.0e-6 || fabs(vJsonStruct->valuedouble) > 1.0e9)
		{
			snprintf(vBuffer, sizeof(vBuffer), "%e", vJsonStruct->valuedouble);
		}
		else
		{
			snprintf(vBuffer, sizeof(vBuffer), "%f", vJsonStruct->valuedouble);
		}
		return vBuffer;
	}
	else if(vJsonStruct->type == XJson_False)
	{
		return "false";
	}
	else if(vJsonStruct->type == XJson_True)
	{
		return "true";
	}
	return "";
}

bool XJsonObject::Parse(const UString& strJson) noexcept
{
	Clear();
	_ThisJsonData = XJson_Parse(strJson.data());
	if(nullptr == _ThisJsonData)
	{
		// _ThisErrorMessage = UString("prase json string error at ") + XJson_GetErrorPtr();
		_ThisErrorMessage = L"prase json string error at ";
		return false;
	}
	return true;
}

void XJsonObject::Clear() noexcept
{
	_ThisExternJsonDataRef = nullptr;
	if(_ThisJsonData)
	{
		XJson_Delete(_ThisJsonData);
		_ThisJsonData = nullptr;
	}
	for(auto vIterator = _ThisJsonArrayRef->begin(); vIterator != _ThisJsonArrayRef->end(); ++vIterator)
	{
		if(vIterator->second)
		{
			delete (vIterator->second);
			vIterator->second = nullptr;
		}
	}
	_ThisJsonArrayRef->clear();
	for(auto vIterator = _ThisJsonObjectRef->begin(); vIterator != _ThisJsonObjectRef->end(); ++vIterator)
	{
		if(vIterator->second)
		{
			delete (vIterator->second);
			vIterator->second = nullptr;
		}
	}
	_ThisJsonObjectRef->clear();
}

bool XJsonObject::IsEmpty() const noexcept
{
	if(_ThisJsonData)
	{
		return false;
	}
	else if(_ThisExternJsonDataRef)
	{
		return false;
	}
	return true;
}

bool XJsonObject::IsArray() const noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}

	if(vFocusData == nullptr)
	{
		return false;
	}
	if(vFocusData->type == XJson_Array)
	{
		return true;
	}
	else
	{
		return false;
	}
}

UString XJsonObject::ToString() const noexcept
{
	auto		vJsonString = static_cast<char*>(nullptr);
	auto		vJsonData = UString("");
	if(_ThisJsonData)
	{
		vJsonString = XJson_PrintUnformatted(_ThisJsonData);
	}
	else if(_ThisExternJsonDataRef)
	{
		vJsonString = XJson_PrintUnformatted(_ThisExternJsonDataRef);
	}
	if(vJsonString)
	{
		vJsonData = vJsonString;
		Xanadu::free(vJsonString);
	}
	return vJsonData;
}

XString XJsonObject::ToXString() const noexcept
{
	return XString::fromUString(ToString());
}

UString XJsonObject::ToFormattedString() const noexcept
{
	auto		vJsonString = static_cast<char*>(nullptr);
	auto		vJsonData = UString("");
	if(_ThisJsonData)
	{
		vJsonString = XJson_Print(_ThisJsonData);
	}
	else if(_ThisExternJsonDataRef)
	{
		vJsonString = XJson_Print(_ThisExternJsonDataRef);
	}
	if(vJsonString)
	{
		vJsonData = vJsonString;
		Xanadu::free(vJsonString);
	}
	return vJsonData;
}

const XString& XJsonObject::GetErrMsg() const noexcept
{
	return _ThisErrorMessage;
}



bool XJsonObject::Get(const UString& _Key, XJsonObject& _Value) const noexcept
{
	auto		vJsonStruct = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		if(_ThisJsonData->type == XJson_Object)
		{
			vJsonStruct = XJson_GetObjectItem(_ThisJsonData, _Key.data());
		}
	}
	else if(_ThisExternJsonDataRef)
	{
		if(_ThisExternJsonDataRef->type == XJson_Object)
		{
			vJsonStruct = XJson_GetObjectItem(_ThisExternJsonDataRef, _Key.data());
		}
	}
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	auto		vJsonString = XJson_Print(vJsonStruct);

	// Fix : basic_string::_M_construct null not valid
	auto		vJsonData = UString(vJsonString ? vJsonString : "");
	Xanadu::free(vJsonString);
	if(_Value.Parse(vJsonData))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool XJsonObject::Get(const UString& _Key, UString& _Value) const noexcept
{
	auto		vJsonStruct = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		if(_ThisJsonData->type == XJson_Object)
		{
			vJsonStruct = XJson_GetObjectItem(_ThisJsonData, _Key.data());
		}
	}
	else if(_ThisExternJsonDataRef)
	{
		if(_ThisExternJsonDataRef->type == XJson_Object)
		{
			vJsonStruct = XJson_GetObjectItem(_ThisExternJsonDataRef, _Key.data());
		}
	}
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	if(vJsonStruct->type != XJson_String)
	{
		return false;
	}
	_Value = vJsonStruct->valuestring;
	return true;
}

bool XJsonObject::Get(const UString& _Key, int32S& _Value) const noexcept
{
	auto 		vJsonStruct = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		if(_ThisJsonData->type == XJson_Object)
		{
			vJsonStruct = XJson_GetObjectItem(_ThisJsonData, _Key.data());
		}
	}
	else if(_ThisExternJsonDataRef)
	{
		if(_ThisExternJsonDataRef->type == XJson_Object)
		{
			vJsonStruct = XJson_GetObjectItem(_ThisExternJsonDataRef, _Key.data());
		}
	}
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	if(vJsonStruct->type != XJson_Int)
	{
		return false;
	}
	_Value = (int32S)(vJsonStruct->valueint);
	return true;
}

bool XJsonObject::Get(const UString& _Key, int32U& _Value) const noexcept
{
	auto		vJsonStruct = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		if(_ThisJsonData->type == XJson_Object)
		{
			vJsonStruct = XJson_GetObjectItem(_ThisJsonData, _Key.data());
		}
	}
	else if(_ThisExternJsonDataRef)
	{
		if(_ThisExternJsonDataRef->type == XJson_Object)
		{
			vJsonStruct = XJson_GetObjectItem(_ThisExternJsonDataRef, _Key.data());
		}
	}
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	if(vJsonStruct->type != XJson_Int)
	{
		return false;
	}
	_Value = (int32U)(vJsonStruct->valueint);
	return true;
}

bool XJsonObject::Get(const UString& _Key, int64S& _Value) const noexcept
{
	auto		vJsonStruct = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		if(_ThisJsonData->type == XJson_Object)
		{
			vJsonStruct = XJson_GetObjectItem(_ThisJsonData, _Key.data());
		}
	}
	else if(_ThisExternJsonDataRef)
	{
		if(_ThisExternJsonDataRef->type == XJson_Object)
		{
			vJsonStruct = XJson_GetObjectItem(_ThisExternJsonDataRef, _Key.data());
		}
	}
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	if(vJsonStruct->type != XJson_Int)
	{
		return false;
	}
	_Value = (int64S)vJsonStruct->valueint;
	return true;
}

bool XJsonObject::Get(const UString& _Key, int64U& _Value) const noexcept
{
	auto		vJsonStruct = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		if(_ThisJsonData->type == XJson_Object)
		{
			vJsonStruct = XJson_GetObjectItem(_ThisJsonData, _Key.data());
		}
	}
	else if(_ThisExternJsonDataRef)
	{
		if(_ThisExternJsonDataRef->type == XJson_Object)
		{
			vJsonStruct = XJson_GetObjectItem(_ThisExternJsonDataRef, _Key.data());
		}
	}
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	if(vJsonStruct->type != XJson_Int)
	{
		return false;
	}
	_Value = (int64U)vJsonStruct->valueint;
	return true;
}

bool XJsonObject::Get(const UString& _Key, bool& _Value) const noexcept
{
	auto		vJsonStruct = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		if(_ThisJsonData->type == XJson_Object)
		{
			vJsonStruct = XJson_GetObjectItem(_ThisJsonData, _Key.data());
		}
	}
	else if(_ThisExternJsonDataRef)
	{
		if(_ThisExternJsonDataRef->type == XJson_Object)
		{
			vJsonStruct = XJson_GetObjectItem(_ThisExternJsonDataRef, _Key.data());
		}
	}
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	if(vJsonStruct->type > XJson_True)
	{
		return false;
	}
	_Value = vJsonStruct->type ? true : false;
	return true;
}

bool XJsonObject::Get(const UString& _Key, float& _Value) const noexcept
{
	auto		vJsonStruct = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		if(_ThisJsonData->type == XJson_Object)
		{
			vJsonStruct = XJson_GetObjectItem(_ThisJsonData, _Key.data());
		}
	}
	else if(_ThisExternJsonDataRef)
	{
		if(_ThisExternJsonDataRef->type == XJson_Object)
		{
			vJsonStruct = XJson_GetObjectItem(_ThisExternJsonDataRef, _Key.data());
		}
	}
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	if(vJsonStruct->type != XJson_Double)
	{
		return false;
	}
	_Value = (float)(vJsonStruct->valuedouble);
	return true;
}

bool XJsonObject::Get(const UString& _Key, double& _Value) const noexcept
{
	auto		vJsonStruct = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		if(_ThisJsonData->type == XJson_Object)
		{
			vJsonStruct = XJson_GetObjectItem(_ThisJsonData, _Key.data());
		}
	}
	else if(_ThisExternJsonDataRef)
	{
		if(_ThisExternJsonDataRef->type == XJson_Object)
		{
			vJsonStruct = XJson_GetObjectItem(_ThisExternJsonDataRef, _Key.data());
		}
	}
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	if(vJsonStruct->type != XJson_Double && vJsonStruct->type != XJson_Int)
	{
		return false;
	}
	_Value = vJsonStruct->valuedouble;
	return true;
}



bool XJsonObject::Add(const UString& _Key, const XJsonObject& _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateObject();
		vFocusData = _ThisJsonData;
	}
	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Object)
	{
		_ThisErrorMessage = L"not a json object! json array?";
		return false;
	}

	auto		vJsonStruct = XJson_Parse(_Value.ToString().data());
	if(nullptr == vJsonStruct)
	{
		///_ThisErrorMessage = UString("prase json string error at ") + XJson_GetErrorPtr();
		_ThisErrorMessage = L"prase json string error";
		return false;
	}
	XJson_AddItemToObject(vFocusData, _Key.data(), vJsonStruct);
	if(nullptr == XJson_GetObjectItem(vFocusData, _Key.data()))
	{
		return false;
	}
	auto		vIterator = _ThisJsonObjectRef->find(_Key);
	if(vIterator != _ThisJsonObjectRef->end())
	{
		if(vIterator->second)
		{
			delete (vIterator->second);
			vIterator->second = nullptr;
		}
		_ThisJsonObjectRef->erase(vIterator);
	}
	return true;
}

bool XJsonObject::Add(const UString& _Key, const UString& _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateObject();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Object)
	{
		_ThisErrorMessage = L"not a json object! json array?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateString(_Value.data());
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	XJson_AddItemToObject(vFocusData, _Key.data(), vJsonStruct);
	if(nullptr == XJson_GetObjectItem(vFocusData, _Key.data()))
	{
		return false;
	}
	return true;
}

bool XJsonObject::Add(const UString& _Key, const XString& _Value) noexcept
{
	return Add(_Key, _Value.toUString());
}

bool XJsonObject::Add(const UString& _Key, int32S _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateObject();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Object)
	{
		_ThisErrorMessage = L"not a json object! json array?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateInt((int64U)_Value, -1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	XJson_AddItemToObject(vFocusData, _Key.data(), vJsonStruct);
	if(nullptr == XJson_GetObjectItem(vFocusData, _Key.data()))
	{
		return false;
	}
	return true;
}

bool XJsonObject::Add(const UString& _Key, int32U _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateObject();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Object)
	{
		_ThisErrorMessage = L"not a json object! json array?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateInt((int64U)_Value, 1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	XJson_AddItemToObject(vFocusData, _Key.data(), vJsonStruct);
	if(nullptr == XJson_GetObjectItem(vFocusData, _Key.data()))
	{
		return false;
	}
	return true;
}

bool XJsonObject::Add(const UString& _Key, int64S _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateObject();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Object)
	{
		_ThisErrorMessage = L"not a json object! json array?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateInt((int64U)_Value, -1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	XJson_AddItemToObject(vFocusData, _Key.data(), vJsonStruct);
	if(nullptr == XJson_GetObjectItem(vFocusData, _Key.data()))
	{
		return false;
	}
	return true;
}

bool XJsonObject::Add(const UString& _Key, int64U _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateObject();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Object)
	{
		_ThisErrorMessage = L"not a json object! json array?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateInt(_Value, 1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	XJson_AddItemToObject(vFocusData, _Key.data(), vJsonStruct);
	if(nullptr == XJson_GetObjectItem(vFocusData, _Key.data()))
	{
		return false;
	}
	return true;
}

bool XJsonObject::Add(const UString& _Key, bool _Value, bool _Again) noexcept
{
	XANADU_UNPARAMETER(_Again);

	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateObject();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Object)
	{
		_ThisErrorMessage = L"not a json object! json array?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateBool(_Value);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	XJson_AddItemToObject(vFocusData, _Key.data(), vJsonStruct);
	if(nullptr == XJson_GetObjectItem(vFocusData, _Key.data()))
	{
		return false;
	}
	return true;
}

bool XJsonObject::Add(const UString& _Key, float _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateObject();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Object)
	{
		_ThisErrorMessage = L"not a json object! json array?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateDouble((double)_Value, -1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	XJson_AddItemToObject(vFocusData, _Key.data(), vJsonStruct);
	if(nullptr == XJson_GetObjectItem(vFocusData, _Key.data()))
	{
		return false;
	}
	return true;
}

bool XJsonObject::Add(const UString& _Key, double _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateObject();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Object)
	{
		_ThisErrorMessage = L"not a json object! json array?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateDouble((double)_Value, -1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	XJson_AddItemToObject(vFocusData, _Key.data(), vJsonStruct);
	if(nullptr == XJson_GetObjectItem(vFocusData, _Key.data()))
	{
		return false;
	}
	return true;
}



bool XJsonObject::Append(const UString& _Key, const XJsonObject& _Value) noexcept
{
	return Add(_Key, _Value);
}

bool XJsonObject::Append(const UString& _Key, const XByteArray& _Value) noexcept
{
	return Add(_Key, UString(_Value.data(), _Value.size()));
}

bool XJsonObject::Append(const UString& _Key, const XString& _Value) noexcept
{
	return Add(_Key, _Value.toUString());
}

bool XJsonObject::Append(const UString& _Key, int32S _Value) noexcept
{
	return Add(_Key, _Value);
}

bool XJsonObject::Append(const UString& _Key, int32U _Value) noexcept
{
	return Add(_Key, _Value);
}

bool XJsonObject::Append(const UString& _Key, int64S _Value) noexcept
{
	return Add(_Key, _Value);
}

bool XJsonObject::Append(const UString& _Key, int64U _Value) noexcept
{
	return Add(_Key, _Value);
}

bool XJsonObject::Append(const UString& _Key, bool _Value, bool _Again) noexcept
{
	return Add(_Key, _Value, _Again);
}

bool XJsonObject::Append(const UString& _Key, float _Value) noexcept
{
	return Add(_Key, _Value);
}

bool XJsonObject::Append(const UString& _Key, double _Value) noexcept
{
	return Add(_Key, _Value);
}



bool XJsonObject::Delete(const UString& _Key) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(nullptr == _ThisJsonData)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		vFocusData = _ThisJsonData;
	}
	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Object)
	{
		_ThisErrorMessage = L"not a json object! json array?";
		return false;
	}
	XJson_DeleteItemFromObject(vFocusData, _Key.data());
	auto		vIterator = _ThisJsonObjectRef->find(_Key);
	if(vIterator != _ThisJsonObjectRef->end())
	{
		if(vIterator->second)
		{
			delete (vIterator->second);
			vIterator->second = nullptr;
		}
		_ThisJsonObjectRef->erase(vIterator);
	}
	return true;
}



bool XJsonObject::Replace(const UString& _Key, const XJsonObject& _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(nullptr == _ThisJsonData)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		vFocusData = _ThisJsonData;
	}
	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Object)
	{
		_ThisErrorMessage = L"not a json object! json array?";
		return false;
	}
	auto		vJsonStruct = XJson_Parse(_Value.ToString().data());
	if(nullptr == vJsonStruct)
	{
		// _ThisErrorMessage = UString("prase json string error at ") + XJson_GetErrorPtr();
		_ThisErrorMessage = L"prase json string error at ";
		return false;
	}
	XJson_ReplaceItemInObject(vFocusData, _Key.data(), vJsonStruct);
	if(nullptr == XJson_GetObjectItem(vFocusData, _Key.data()))
	{
		return false;
	}
	auto		vIterator = _ThisJsonObjectRef->find(_Key);
	if(vIterator != _ThisJsonObjectRef->end())
	{
		if(vIterator->second)
		{
			delete (vIterator->second);
			vIterator->second = nullptr;
		}
		_ThisJsonObjectRef->erase(vIterator);
	}
	return true;
}

bool XJsonObject::Replace(const UString& _Key, const UString& _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(nullptr == _ThisJsonData)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		vFocusData = _ThisJsonData;
	}
	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Object)
	{
		_ThisErrorMessage = L"not a json object! json array?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateString(_Value.data());
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	XJson_ReplaceItemInObject(vFocusData, _Key.data(), vJsonStruct);
	if(nullptr == XJson_GetObjectItem(vFocusData, _Key.data()))
	{
		return false;
	}
	return true;
}

bool XJsonObject::Replace(const UString& _Key, int32S _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(nullptr == _ThisJsonData)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		vFocusData = _ThisJsonData;
	}
	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Object)
	{
		_ThisErrorMessage = L"not a json object! json array?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateInt((int64U)_Value, -1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	XJson_ReplaceItemInObject(vFocusData, _Key.data(), vJsonStruct);
	if(nullptr == XJson_GetObjectItem(vFocusData, _Key.data()))
	{
		return false;
	}
	return true;
}

bool XJsonObject::Replace(const UString& _Key, int32U _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(nullptr == _ThisJsonData)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		vFocusData = _ThisJsonData;
	}
	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Object)
	{
		_ThisErrorMessage = L"not a json object! json array?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateInt((int64U)_Value, 1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	XJson_ReplaceItemInObject(vFocusData, _Key.data(), vJsonStruct);
	if(nullptr == XJson_GetObjectItem(vFocusData, _Key.data()))
	{
		return false;
	}
	return true;
}

bool XJsonObject::Replace(const UString& _Key, int64S _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(nullptr == _ThisJsonData)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		vFocusData = _ThisJsonData;
	}
	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Object)
	{
		_ThisErrorMessage = L"not a json object! json array?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateInt((int64U)_Value, -1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	XJson_ReplaceItemInObject(vFocusData, _Key.data(), vJsonStruct);
	if(nullptr == XJson_GetObjectItem(vFocusData, _Key.data()))
	{
		return false;
	}
	return true;
}

bool XJsonObject::Replace(const UString& _Key, int64U _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(nullptr == _ThisJsonData)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		vFocusData = _ThisJsonData;
	}
	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Object)
	{
		_ThisErrorMessage = L"not a json object! json array?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateInt((int64U)_Value, 1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	XJson_ReplaceItemInObject(vFocusData, _Key.data(), vJsonStruct);
	if(nullptr == XJson_GetObjectItem(vFocusData, _Key.data()))
	{
		return false;
	}
	return true;
}

bool XJsonObject::Replace(const UString& _Key, bool _Value, bool _Again) noexcept
{
	XANADU_UNPARAMETER(_Again);

	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(nullptr == _ThisJsonData)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		vFocusData = _ThisJsonData;
	}
	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Object)
	{
		_ThisErrorMessage = L"not a json object! json array?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateBool(_Value);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	XJson_ReplaceItemInObject(vFocusData, _Key.data(), vJsonStruct);
	if(nullptr == XJson_GetObjectItem(vFocusData, _Key.data()))
	{
		return false;
	}
	return true;
}

bool XJsonObject::Replace(const UString& _Key, float _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(nullptr == _ThisJsonData)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		vFocusData = _ThisJsonData;
	}
	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Object)
	{
		_ThisErrorMessage = L"not a json object! json array?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateDouble((double)_Value, -1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	XJson_ReplaceItemInObject(vFocusData, _Key.data(), vJsonStruct);
	if(nullptr == XJson_GetObjectItem(vFocusData, _Key.data()))
	{
		return false;
	}
	return true;
}

bool XJsonObject::Replace(const UString& _Key, double _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(nullptr == _ThisJsonData)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		vFocusData = _ThisJsonData;
	}
	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Object)
	{
		_ThisErrorMessage = L"not a json object! json array?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateDouble((double)_Value, -1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	XJson_ReplaceItemInObject(vFocusData, _Key.data(), vJsonStruct);
	if(nullptr == XJson_GetObjectItem(vFocusData, _Key.data()))
	{
		return false;
	}
	return true;
}



XJsonObject XJsonObject::ToObject(UString _Key) const noexcept
{
	auto		vValue = XJsonObject();
	Get(_Key, vValue);
	return vValue;
}

UString XJsonObject::ToString(UString _Key) const noexcept
{
	auto		vValue = UString("");
	Get(_Key, vValue);
	return vValue;
}

XByteArray XJsonObject::ToBytes(const UString& _Key) const noexcept
{
	auto		vValue = UString("");
	Get(_Key, vValue);
	return XByteArray(vValue.data(), vValue.size());
}

XString XJsonObject::ToXString(UString _Key) const noexcept
{
	auto		vValue = UString("");
	Get(_Key, vValue);
	return XString::fromUString(vValue);
}

double XJsonObject::ToDouble(UString _Key) const noexcept
{
	auto		vValue = static_cast<double>(0.0f);
	Get(_Key, vValue);
	return vValue;
}

int32S XJsonObject::ToInt(UString _Key) const noexcept
{
	auto		vValue = static_cast<int32S>(0);
	Get(_Key, vValue);
	return vValue;
}

int32U XJsonObject::ToUint(UString _Key) const noexcept
{
	auto		vValue = static_cast<int32U>(0);
	Get(_Key, vValue);
	return vValue;
}

int64S XJsonObject::ToLong(UString _Key) const noexcept
{
	auto		vValue = static_cast<int64S>(0);
	Get(_Key, vValue);
	return vValue;
}

int64U XJsonObject::ToUlong(UString _Key) const noexcept
{
	auto		vValue = static_cast<int64U>(0);
	Get(_Key, vValue);
	return vValue;
}

bool XJsonObject::ToBool(UString _Key) const noexcept
{
	auto		vValue = false;
	Get(_Key, vValue);
	return vValue;
}



int XJsonObject::GetArraySize() const noexcept
{
	if(_ThisJsonData)
	{
		if(_ThisJsonData->type == XJson_Array)
		{
			return(XJson_GetArraySize(_ThisJsonData));
		}
	}
	else if(_ThisExternJsonDataRef)
	{
		if(_ThisExternJsonDataRef->type == XJson_Array)
		{
			return(XJson_GetArraySize(_ThisExternJsonDataRef));
		}
	}
	return 0;
}



bool XJsonObject::Get(int _Which, XJsonObject& _Value) const noexcept
{
	auto		vJsonStruct = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		if(_ThisJsonData->type == XJson_Array)
		{
			vJsonStruct = XJson_GetArrayItem(_ThisJsonData, _Which);
		}
	}
	else if(_ThisExternJsonDataRef)
	{
		if(_ThisExternJsonDataRef->type == XJson_Array)
		{
			vJsonStruct = XJson_GetArrayItem(_ThisExternJsonDataRef, _Which);
		}
	}
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	auto		vJsonString = XJson_Print(vJsonStruct);
	auto		vJsonData = UString(vJsonString);
	Xanadu::free(vJsonString);
	if(_Value.Parse(vJsonData))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool XJsonObject::Get(int _Which, UString& _Value) const noexcept
{
	auto		vJsonStruct = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		if(_ThisJsonData->type == XJson_Array)
		{
			vJsonStruct = XJson_GetArrayItem(_ThisJsonData, _Which);
		}
	}
	else if(_ThisExternJsonDataRef)
	{
		if(_ThisExternJsonDataRef->type == XJson_Array)
		{
			vJsonStruct = XJson_GetArrayItem(_ThisExternJsonDataRef, _Which);
		}
	}
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	if(vJsonStruct->type != XJson_String)
	{
		return false;
	}
	_Value = vJsonStruct->valuestring;
	return true;
}

bool XJsonObject::Get(int _Which, int32S& _Value) const noexcept
{
	auto		vJsonStruct = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		if(_ThisJsonData->type == XJson_Array)
		{
			vJsonStruct = XJson_GetArrayItem(_ThisJsonData, _Which);
		}
	}
	else if(_ThisExternJsonDataRef)
	{
		if(_ThisExternJsonDataRef->type == XJson_Array)
		{
			vJsonStruct = XJson_GetArrayItem(_ThisExternJsonDataRef, _Which);
		}
	}
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	if(vJsonStruct->type != XJson_Int)
	{
		return false;
	}
	_Value = (int32S)(vJsonStruct->valueint);
	return true;
}

bool XJsonObject::Get(int _Which, int32U& _Value) const noexcept
{
	auto		vJsonStruct = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		if(_ThisJsonData->type == XJson_Array)
		{
			vJsonStruct = XJson_GetArrayItem(_ThisJsonData, _Which);
		}
	}
	else if(_ThisExternJsonDataRef)
	{
		if(_ThisExternJsonDataRef->type == XJson_Array)
		{
			vJsonStruct = XJson_GetArrayItem(_ThisExternJsonDataRef, _Which);
		}
	}
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	if(vJsonStruct->type != XJson_Int)
	{
		return false;
	}
	_Value = (int32U)(vJsonStruct->valueint);
	return true;
}

bool XJsonObject::Get(int _Which, int64S& _Value) const noexcept
{
	auto		vJsonStruct = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		if(_ThisJsonData->type == XJson_Array)
		{
			vJsonStruct = XJson_GetArrayItem(_ThisJsonData, _Which);
		}
	}
	else if(_ThisExternJsonDataRef)
	{
		if(_ThisExternJsonDataRef->type == XJson_Array)
		{
			vJsonStruct = XJson_GetArrayItem(_ThisExternJsonDataRef, _Which);
		}
	}
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	if(vJsonStruct->type != XJson_Int)
	{
		return false;
	}
	_Value = (int64S)vJsonStruct->valueint;
	return true;
}

bool XJsonObject::Get(int _Which, int64U& _Value) const noexcept
{
	auto		vJsonStruct = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		if(_ThisJsonData->type == XJson_Array)
		{
			vJsonStruct = XJson_GetArrayItem(_ThisJsonData, _Which);
		}
	}
	else if(_ThisExternJsonDataRef)
	{
		if(_ThisExternJsonDataRef->type == XJson_Array)
		{
			vJsonStruct = XJson_GetArrayItem(_ThisExternJsonDataRef, _Which);
		}
	}
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	if(vJsonStruct->type != XJson_Int)
	{
		return false;
	}
	_Value = (int64U)vJsonStruct->valueint;
	return true;
}

bool XJsonObject::Get(int _Which, bool& _Value) const noexcept
{
	auto		vJsonStruct = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		if(_ThisJsonData->type == XJson_Array)
		{
			vJsonStruct = XJson_GetArrayItem(_ThisJsonData, _Which);
		}
	}
	else if(_ThisExternJsonDataRef)
	{
		if(_ThisExternJsonDataRef->type == XJson_Array)
		{
			vJsonStruct = XJson_GetArrayItem(_ThisExternJsonDataRef, _Which);
		}
	}
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	if(vJsonStruct->type > XJson_True)
	{
		return false;
	}
	_Value = vJsonStruct->type ? true : false;
	return true;
}

bool XJsonObject::Get(int _Which, float& _Value) const noexcept
{
	auto		vJsonStruct = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		if(_ThisJsonData->type == XJson_Array)
		{
			vJsonStruct = XJson_GetArrayItem(_ThisJsonData, _Which);
		}
	}
	else if(_ThisExternJsonDataRef)
	{
		if(_ThisExternJsonDataRef->type == XJson_Array)
		{
			vJsonStruct = XJson_GetArrayItem(_ThisExternJsonDataRef, _Which);
		}
	}
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	if(vJsonStruct->type != XJson_Double)
	{
		return false;
	}
	_Value = (float)(vJsonStruct->valuedouble);
	return true;
}

bool XJsonObject::Get(int _Which, double& _Value) const noexcept
{
	auto		vJsonStruct = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		if(_ThisJsonData->type == XJson_Array)
		{
			vJsonStruct = XJson_GetArrayItem(_ThisJsonData, _Which);
		}
	}
	else if(_ThisExternJsonDataRef)
	{
		if(_ThisExternJsonDataRef->type == XJson_Array)
		{
			vJsonStruct = XJson_GetArrayItem(_ThisExternJsonDataRef, _Which);
		}
	}
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	if(vJsonStruct->type != XJson_Double)
	{
		return false;
	}
	_Value = vJsonStruct->valuedouble;
	return true;
}



bool XJsonObject::Add(const XJsonObject& _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateArray();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_Parse(_Value.ToString().data());
	if(nullptr == vJsonStruct)
	{
		// _ThisErrorMessage = UString("prase json string error at ") + XJson_GetErrorPtr();
		_ThisErrorMessage = L"prase json string error";
		return false;
	}
	auto		vArraySizeBeforeAdd = XJson_GetArraySize(vFocusData);
	XJson_AddItemToArray(vFocusData, vJsonStruct);
	auto		vArraySizeAfterAdd = XJson_GetArraySize(vFocusData);
	if(vArraySizeAfterAdd == vArraySizeBeforeAdd)
	{
		return false;
	}
	auto		vLastIndex = (unsigned int)XJson_GetArraySize(vFocusData) - 1;
	for(auto vIterator = _ThisJsonArrayRef->begin(); vIterator != _ThisJsonArrayRef->end();)
	{
		if(vIterator->first >= vLastIndex)
		{
			if(vIterator->second)
			{
				delete (vIterator->second);
				vIterator->second = nullptr;
			}
			_ThisJsonArrayRef->erase(vIterator++);
		}
		else
		{
			vIterator++;
		}
	}
	return true;
}

bool XJsonObject::Add(const UString& _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateArray();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateString(_Value.data());
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	auto		vArraySizeBeforeAdd = XJson_GetArraySize(vFocusData);
	XJson_AddItemToArray(vFocusData, vJsonStruct);
	auto		vArraySizeAfterAdd = XJson_GetArraySize(vFocusData);
	if(vArraySizeAfterAdd == vArraySizeBeforeAdd)
	{
		return false;
	}
	return true;
}

bool XJsonObject::Add(int32S _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateArray();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateInt((int64U)_Value, -1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	auto		vArraySizeBeforeAdd = XJson_GetArraySize(vFocusData);
	XJson_AddItemToArray(vFocusData, vJsonStruct);
	auto		vArraySizeAfterAdd = XJson_GetArraySize(vFocusData);
	if(vArraySizeAfterAdd == vArraySizeBeforeAdd)
	{
		return false;
	}
	return true;
}

bool XJsonObject::Add(int32U _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateArray();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateInt((int64U)_Value, 1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	auto		vArraySizeBeforeAdd = XJson_GetArraySize(vFocusData);
	XJson_AddItemToArray(vFocusData, vJsonStruct);
	auto		vArraySizeAfterAdd = XJson_GetArraySize(vFocusData);
	if(vArraySizeAfterAdd == vArraySizeBeforeAdd)
	{
		return false;
	}
	return true;
}

bool XJsonObject::Add(int64S _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateArray();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateInt((int64U)_Value, -1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	auto		vArraySizeBeforeAdd = XJson_GetArraySize(vFocusData);
	XJson_AddItemToArray(vFocusData, vJsonStruct);
	auto		vArraySizeAfterAdd = XJson_GetArraySize(vFocusData);
	if(vArraySizeAfterAdd == vArraySizeBeforeAdd)
	{
		return false;
	}
	return true;
}

bool XJsonObject::Add(int64U _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateArray();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateInt((int64U)_Value, 1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	auto		vArraySizeBeforeAdd = XJson_GetArraySize(vFocusData);
	XJson_AddItemToArray(vFocusData, vJsonStruct);
	auto		vArraySizeAfterAdd = XJson_GetArraySize(vFocusData);
	if(vArraySizeAfterAdd == vArraySizeBeforeAdd)
	{
		return false;
	}
	return true;
}

bool XJsonObject::Add(int _Anywhere, bool _Value) noexcept
{
	XANADU_UNPARAMETER(_Anywhere);

	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateArray();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateBool(_Value);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	auto		vArraySizeBeforeAdd = XJson_GetArraySize(vFocusData);
	XJson_AddItemToArray(vFocusData, vJsonStruct);
	auto		vArraySizeAfterAdd = XJson_GetArraySize(vFocusData);
	if(vArraySizeAfterAdd == vArraySizeBeforeAdd)
	{
		return false;
	}
	return true;
}

bool XJsonObject::Add(float _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateArray();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateDouble((double)_Value, -1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	auto		vArraySizeBeforeAdd = XJson_GetArraySize(vFocusData);
	XJson_AddItemToArray(vFocusData, vJsonStruct);
	auto		vArraySizeAfterAdd = XJson_GetArraySize(vFocusData);
	if(vArraySizeAfterAdd == vArraySizeBeforeAdd)
	{
		return false;
	}
	return true;
}

bool XJsonObject::Add(double _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateArray();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateDouble((double)_Value, -1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	auto		vArraySizeBeforeAdd = XJson_GetArraySize(vFocusData);
	XJson_AddItemToArray(vFocusData, vJsonStruct);
	auto		vArraySizeAfterAdd = XJson_GetArraySize(vFocusData);
	if(vArraySizeAfterAdd == vArraySizeBeforeAdd)
	{
		return false;
	}
	return true;
}



bool XJsonObject::AddAsFirst(const XJsonObject& _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateArray();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_Parse(_Value.ToString().data());
	if(nullptr == vJsonStruct)
	{
		// _ThisErrorMessage = UString("prase json string error at ") + XJson_GetErrorPtr();
		_ThisErrorMessage = L"prase json string error at ";
		return false;
	}
	auto		vArraySizeBeforeAdd = XJson_GetArraySize(vFocusData);
	XJson_AddItemToArrayHead(vFocusData, vJsonStruct);
	auto		vArraySizeAfterAdd = XJson_GetArraySize(vFocusData);
	if(vArraySizeAfterAdd == vArraySizeBeforeAdd)
	{
		return false;
	}
	for(auto vIterator = _ThisJsonArrayRef->begin(); vIterator != _ThisJsonArrayRef->end();)
	{
		if(vIterator->second)
		{
			delete (vIterator->second);
			vIterator->second = nullptr;
		}
		_ThisJsonArrayRef->erase(vIterator++);
	}
	return true;
}

bool XJsonObject::AddAsFirst(const UString& _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateArray();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateString(_Value.data());
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	auto		vArraySizeBeforeAdd = XJson_GetArraySize(vFocusData);
	XJson_AddItemToArrayHead(vFocusData, vJsonStruct);
	auto		vArraySizeAfterAdd = XJson_GetArraySize(vFocusData);
	if(vArraySizeAfterAdd == vArraySizeBeforeAdd)
	{
		return false;
	}
	return true;
}

bool XJsonObject::AddAsFirst(int32S _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateArray();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateInt((int64U)_Value, -1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	auto		vArraySizeBeforeAdd = XJson_GetArraySize(vFocusData);
	XJson_AddItemToArrayHead(vFocusData, vJsonStruct);
	auto		vArraySizeAfterAdd = XJson_GetArraySize(vFocusData);
	if(vArraySizeAfterAdd == vArraySizeBeforeAdd)
	{
		return false;
	}
	return true;
}

bool XJsonObject::AddAsFirst(int32U _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateArray();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateInt((int64U)_Value, -1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	auto		vArraySizeBeforeAdd = XJson_GetArraySize(vFocusData);
	XJson_AddItemToArrayHead(vFocusData, vJsonStruct);
	auto		vArraySizeAfterAdd = XJson_GetArraySize(vFocusData);
	if(vArraySizeAfterAdd == vArraySizeBeforeAdd)
	{
		return false;
	}
	return true;
}

bool XJsonObject::AddAsFirst(int64S _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateArray();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateInt((int64U)_Value, -1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	auto		vArraySizeBeforeAdd = XJson_GetArraySize(vFocusData);
	XJson_AddItemToArrayHead(vFocusData, vJsonStruct);
	auto		vArraySizeAfterAdd = XJson_GetArraySize(vFocusData);
	if(vArraySizeAfterAdd == vArraySizeBeforeAdd)
	{
		return false;
	}
	return true;
}

bool XJsonObject::AddAsFirst(int64U _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateArray();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateInt((int64U)_Value, -1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	auto		vArraySizeBeforeAdd = XJson_GetArraySize(vFocusData);
	XJson_AddItemToArrayHead(vFocusData, vJsonStruct);
	auto		vArraySizeAfterAdd = XJson_GetArraySize(vFocusData);
	if(vArraySizeAfterAdd == vArraySizeBeforeAdd)
	{
		return false;
	}
	return true;
}

bool XJsonObject::AddAsFirst(int _Anywhere, bool _Value) noexcept
{
	XANADU_UNPARAMETER(_Anywhere);

	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateArray();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateBool(_Value);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	auto		vArraySizeBeforeAdd = XJson_GetArraySize(vFocusData);
	XJson_AddItemToArrayHead(vFocusData, vJsonStruct);
	auto		vArraySizeAfterAdd = XJson_GetArraySize(vFocusData);
	if(vArraySizeAfterAdd == vArraySizeBeforeAdd)
	{
		return false;
	}
	return true;
}

bool XJsonObject::AddAsFirst(float _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateArray();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateDouble((double)_Value, -1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	auto		vArraySizeBeforeAdd = XJson_GetArraySize(vFocusData);
	XJson_AddItemToArrayHead(vFocusData, vJsonStruct);
	auto		vArraySizeAfterAdd = XJson_GetArraySize(vFocusData);
	if(vArraySizeAfterAdd == vArraySizeBeforeAdd)
	{
		return false;
	}
	return true;
}

bool XJsonObject::AddAsFirst(double _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(_ThisJsonData)
	{
		vFocusData = _ThisJsonData;
	}
	else if(_ThisExternJsonDataRef)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		_ThisJsonData = XJson_CreateArray();
		vFocusData = _ThisJsonData;
	}

	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateDouble((double)_Value, -1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	auto		vArraySizeBeforeAdd = XJson_GetArraySize(vFocusData);
	XJson_AddItemToArrayHead(vFocusData, vJsonStruct);
	auto		vArraySizeAfterAdd = XJson_GetArraySize(vFocusData);
	if(vArraySizeAfterAdd == vArraySizeBeforeAdd)
	{
		return false;
	}
	return true;
}



bool XJsonObject::Delete(int _Which) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(nullptr == _ThisJsonData)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		vFocusData = _ThisJsonData;
	}
	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	XJson_DeleteItemFromArray(vFocusData, _Which);
	for(auto vIterator = _ThisJsonArrayRef->begin(); vIterator != _ThisJsonArrayRef->end();)
	{
		if(vIterator->first >= (unsigned int)_Which)
		{
			if(vIterator->second)
			{
				delete (vIterator->second);
				vIterator->second = nullptr;
			}
			_ThisJsonArrayRef->erase(vIterator++);
		}
		else
		{
			vIterator++;
		}
	}
	return true;
}



bool XJsonObject::Replace(int _Which, const XJsonObject& _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(nullptr == _ThisJsonData)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		vFocusData = _ThisJsonData;
	}
	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_Parse(_Value.ToString().data());
	if(nullptr == vJsonStruct)
	{
		// _ThisErrorMessage = UString("prase json string error at ") + XJson_GetErrorPtr();
		return false;
	}
	XJson_ReplaceItemInArray(vFocusData, _Which, vJsonStruct);
	if(nullptr == XJson_GetArrayItem(vFocusData, _Which))
	{
		return false;
	}
	auto		vIterator = _ThisJsonArrayRef->find(_Which);
	if(vIterator != _ThisJsonArrayRef->end())
	{
		if(vIterator->second)
		{
			delete (vIterator->second);
			vIterator->second = nullptr;
		}
		_ThisJsonArrayRef->erase(vIterator);
	}
	return true;
}

bool XJsonObject::Replace(int _Which, const UString& _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(nullptr == _ThisJsonData)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		vFocusData = _ThisJsonData;
	}
	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateString(_Value.data());
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	XJson_ReplaceItemInArray(vFocusData, _Which, vJsonStruct);
	if(XJson_GetArrayItem(vFocusData, _Which) == NULL)
	{
		return false;
	}
	return true;
}

bool XJsonObject::Replace(int _Which, int32S _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(nullptr == _ThisJsonData)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		vFocusData = _ThisJsonData;
	}
	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateInt((int64U)_Value, -1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	XJson_ReplaceItemInArray(vFocusData, _Which, vJsonStruct);
	if(XJson_GetArrayItem(vFocusData, _Which) == NULL)
	{
		return false;
	}
	return true;
}

bool XJsonObject::Replace(int _Which, int32U _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(nullptr == _ThisJsonData)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		vFocusData = _ThisJsonData;
	}
	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateInt((int64U)_Value, 1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	XJson_ReplaceItemInArray(vFocusData, _Which, vJsonStruct);
	if(XJson_GetArrayItem(vFocusData, _Which) == NULL)
	{
		return false;
	}
	return true;
}

bool XJsonObject::Replace(int _Which, int64S _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(nullptr == _ThisJsonData)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		vFocusData = _ThisJsonData;
	}
	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateInt((int64U)((int64U)_Value), -1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	XJson_ReplaceItemInArray(vFocusData, _Which, vJsonStruct);
	if(XJson_GetArrayItem(vFocusData, _Which) == NULL)
	{
		return false;
	}
	return true;
}

bool XJsonObject::Replace(int _Which, int64U _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(nullptr == _ThisJsonData)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		vFocusData = _ThisJsonData;
	}
	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateInt((int64U)_Value, 1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	XJson_ReplaceItemInArray(vFocusData, _Which, vJsonStruct);
	if(XJson_GetArrayItem(vFocusData, _Which) == NULL)
	{
		return false;
	}
	return true;
}

bool XJsonObject::Replace(int _Which, bool _Value, bool _Again) noexcept
{
	XANADU_UNPARAMETER(_Again);

	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(nullptr == _ThisJsonData)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		vFocusData = _ThisJsonData;
	}
	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateBool(_Value);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	XJson_ReplaceItemInArray(vFocusData, _Which, vJsonStruct);
	if(XJson_GetArrayItem(vFocusData, _Which) == NULL)
	{
		return false;
	}
	return true;
}

bool XJsonObject::Replace(int _Which, float _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(nullptr == _ThisJsonData)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		vFocusData = _ThisJsonData;
	}
	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateDouble((double)_Value, -1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	XJson_ReplaceItemInArray(vFocusData, _Which, vJsonStruct);
	if(nullptr == XJson_GetArrayItem(vFocusData, _Which))
	{
		return false;
	}
	return true;
}

bool XJsonObject::Replace(int _Which, double _Value) noexcept
{
	auto		vFocusData = static_cast<XANADU_JSON_INFO*>(nullptr);
	if(nullptr == _ThisJsonData)
	{
		vFocusData = _ThisExternJsonDataRef;
	}
	else
	{
		vFocusData = _ThisJsonData;
	}
	if(nullptr == vFocusData)
	{
		_ThisErrorMessage = L"json data is null!";
		return false;
	}
	if(vFocusData->type != XJson_Array)
	{
		_ThisErrorMessage = L"not a json array! json object?";
		return false;
	}
	auto		vJsonStruct = XJson_CreateDouble((double)_Value, -1);
	if(nullptr == vJsonStruct)
	{
		return false;
	}
	XJson_ReplaceItemInArray(vFocusData, _Which, vJsonStruct);
	if(nullptr == XJson_GetArrayItem(vFocusData, _Which))
	{
		return false;
	}
	return true;
}
