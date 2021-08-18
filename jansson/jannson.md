# Jansson简介

Jansson 是一个用于编码、解码和操作 JSON 数据的 C 库。其主要特点和设计原则是： 

- 简单直观的 API 和数据模型 
- 综合文档 
- 不依赖其他库 
- 完全 Unicode 支持 (UTF-8) 
- 广泛的测试套件 

Jansson 在 MIT 许可下获得许可；有关详细信息，请参阅源代码分发中的许可证。 Jansson 用于生产，其 API 稳定。它适用于众多平台，包括众多类 Unix 系统和 Windows。它适用于任何系统，包括台式机、服务器和小型嵌入式系统。

实际上就是处理json数据，原因是网络中是json字符串传输。

```c
#include <jansson.h>
```

JSON 规范 (RFC 4627) 定义了以下数据类型：**对象、数组、字符串、数字、布尔值和 null**。 JSON 类型是动态使用的；**数组和对象**可以保存任何其他数据类型，包括它们自己。因此，Jansson 的类型系统本质上也是动态的。有一种 C 类型来表示所有 JSON 值，这个结构知道它所保存的 JSON 值的类型。 

json_t 

该数据结构在整个库中用于表示所有 JSON 值。它始终包含它所持有的 JSON 值的类型以及该值的引用计数。其余的取决于值的类型。 json_t 的对象总是通过**<font color='red'>指针</font>**使用。有用于查询类型、操作引用计数以及构造和操作不同类型值的 API。 

```c
typedef struct json_t {
    json_type type;
    size_t refcount;
} json_t;
typedef enum {
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_INTEGER,
    JSON_REAL,
    JSON_TRUE,
    JSON_FALSE,
    JSON_NULL
} json_type;
```

所有定义都以json_t开始，之后再赋值类型。

除非另有说明，否则所有 API 函数都会在发生错误时返回**错误值**。根据函数的签名，错误值为 NULL 或 -1。无效的参数或无效的输入是错误的明显来源。内存分配和 I/O 操作也可能导致错误。

<font color='red'>这里有一个问题是：我们的值是存在哪里的？</font>

以两个小例子看一下：

example 1：实数

```c
json_t *json_real(double value);
该函数会根据value值返回一个JSON值。
json_t指向的一个结构体指针，结构体中只有类型和引用计数。拿书数据在哪？
看源码实现~

typedef struct {
    json_t json;
    char *value;
    size_t length;
} json_string_t;

typedef struct {
    json_t json;
    double value;
} json_real_t;

typedef struct {
    json_t json;
    json_int_t value;
} json_integer_t;


json_t *json_real(double value)
{
    json_real_t *real;			//首先分配了一个json_real_t结构体指针

    if(isnan(value) || isinf(value))
        return NULL;

    real = jsonp_malloc(sizeof(json_real_t));	//分配json_real_t内存
    if(!real)
        return NULL;
    json_init(&real->json, JSON_REAL);

    real->value = value;			//real结构体中有存值的地方~
    return &real->json;				//返回的是real->json结构体的地址，但是其实就是json_real_t的首地址，所有的结构体首地址既是json_t结构体地址。
}
```

example 2：数组

```c
/*** array ***/

typedef struct {
    json_t json;
    size_t size;
    size_t entries;
    json_t **table;
    int visited;
} json_array_t;

json_t *json_array(void)
{
    json_array_t *array = jsonp_malloc(sizeof(json_array_t));
    if(!array)
        return NULL;
    json_init(&array->json, JSON_ARRAY);

    array->entries = 0;
    array->size = 8;			//默认大小是8个size_t

    array->table = jsonp_malloc(array->size * sizeof(json_t *));	
    if(!array->table) {
        jsonp_free(array);
        return NULL;
    }

    array->visited = 0;

    return &array->json;		//该地址既是数组结构体首地址
}
```

<font color='red'>因此json_t可以看做一个小的结构体作为大结构体的头部，头部之后才是数据，因此给了大结构体的首地址，也是小结构体的首地址。</font>

<font color='red'>数据管理实际是函数内部在帮我们管理，编程者只需要考虑数据的插入与取出。</font>

## 数据类型

- JSON_OBJECT 		对象
- JSON_ARRAY          数组
- JSON_STRING        字符串
- JSON_INTEGER      整形
- JSON_REAL             数字
- JSON_TRUE              布尔true
- JSON_FALSE             布尔false
- JSON_NULL               null

这些对应于 JSON 对象、数组、字符串、数字、布尔值和 null。数字由 JSON_INTEGER 类型或 JSON_REAL 类型的值表示。 true 布尔值由 JSON_TRUE 类型的值表示，false 由 JSON_FALSE 类型的值表示。

判断类型：

```c
int json_typeof(const json_t *json)
```

返回 JSON 值的类型（一个 json_type 转换为 int）。 json 不能为 NULL。

```c
int json_is_object(const json_t *json)
int json_is_array(const json_t *json)
int json_is_string(const json_t *json)
int json_is_integer(const json_t *json)
int json_is_real(const json_t *json)
int json_is_true(const json_t *json)
int json_is_false(const json_t *json)
int json_is_null(const json_t *json)
```

这些函数（实际上是宏）为给定类型的值返回真（非零），为其他类型的值和 NULL 返回假（零）。

```c
int json_is_number(const json_t *json)
```

对于 JSON_INTEGER 和 JSON_REAL 类型的值返回 true，对于其他类型和 NULL 返回 false。

```c
int json_is_boolean(const json_t *json)
```

对于 JSON_TRUE 和 JSON_FALSE 类型返回 true，对于其他类型的值和 NULL 返回 false。

```c
int json_boolean_value(const json_t *json)
```

json_is_true() 的别名，即 JSON_TRUE 返回 1，否则返回 0。

## 引用计数

引用计数被用于跟踪一个值是否仍然在使用或不使用。当创建一个value，它的引用计数设置为1。如果某个值的参考保持（例如值存储以备以后使用的地方），它的引用计数递增，而当不再需要的值时，参考计数递减。当引用计数下降到零，没有剩余的引用，并且该值可以被摧毁。

```c
json_t *json_incref(json_t *json)
```

如果 json 不为 NULL，则增加 json 的引用计数。返回 json。

```c
void json_decref(json_t *json)
```

减少json的引用计数。一旦对 json_decref() 的调用将引用计数降至零，该值就会被销毁并且无法再使用。

创建新 JSON 值的函数将引用计数设置为 1。据说这些函数返回一个新引用。返回（现有）JSON 值的其他函数通常不会增加引用计数。据说这些函数返回一个借用的引用。因此，如果用户将持有对作为借用引用返回的值的引用，则他必须调用 json_incref()。一旦不再需要该值，就应调用 json_decref() 以释放引用。 

通常，所有接受 JSON 值作为参数的函数都会管理引用，即根据需要增加和减少引用计数。但是，某些函数会窃取引用，即它们的结果与用户在调用函数后立即对参数调用 json_decref() 的结果相同。这些函数以__new为后缀或在其名称中的某处带有 _new\__。

例如，以下代码创建一个新的 JSON 数组并向其附加一个整数：

```c
json_t *array, *integer;

array = json_array();
integer = json_integer(42);

json_array_append(array, integer);
json_decref(integer);
```

注意调用者如何通过调用 json_decref() 来释放对整数值的引用。通过使用引用窃取函数 json_array_append_new() 而不是 json_array_append() ，代码变得更加简单：

```c
json_t *array = json_array();
json_array_append_new(array, json_integer(42));
```

在这种情况下，用户不必显式释放对整数值的引用，因为 json_array_append_new() 在将值附加到数组时会窃取引用。 在下面的部分中，清楚地记录了一个函数是返回一个新的或借用的引用还是窃取对其参数的引用。

## 循环引用

当对象或数组直接或间接插入到自身内部时，就会创建循环引用。直接的情况很简单：

```c
json_t *obj = json_object();
json_object_set(obj, "foo", obj);
```

Jansson 将拒绝这样做，并且 json_object_set()（以及所有其他用于对象和数组的此类函数）将返回错误状态。间接的情况是危险的情况：

```c
json_t *arr1 = json_array(), *arr2 = json_array();
json_array_append(arr1, arr2);
json_array_append(arr2, arr1);
```

在此示例中，数组 arr2 包含在数组 arr1 中，反之亦然。 Jansson 无法在不影响性能的情况下检查这种间接循环引用，因此用户可以避免它们。 

如果创建了循环引用，则 json_decref() 无法释放值消耗的内存。引用计数永远不会降到零，因为这些值会保持对彼此的引用。此外，尝试使用任何编码函数对值进行编码都会失败。编码器检测循环引用并返回错误状态。

## 范围基础引用

可以使用 json_auto_t 类型在范围末尾自动取消引用值。例如：

```c
void function(void) {
  json_auto_t *value = NULL;
  value = json_string("foo");
  /* json_decref(value) is automatically called. */
}
```

此功能仅在 GCC 和 Clang 上可用。因此，如果您的项目对其他编译器有可移植性要求，则应避免使用此功能。 此外，与往常一样，将值传递给窃取引用的函数时应该小心。

## True, False and Null

这三个值被实现为单例，因此返回的指针不会在这些函数的调用之间改变。

```c
json_t *json_true(void)
```

返回true

```c
json_t *json_false(void)
```

返回false

```c
json_t *json_boolean(val)
```

根据val返回treu或者false，0返回false，其他值返回true。类似三元表达式： val ? json_true() : json_false().

```c
json_t *json_null(void)
```

返回null

## string

Jansson 使用 UTF-8 作为字符编码。所有 JSON 字符串都必须是有效的 UTF-8（或 ASCII，因为它是 UTF-8 的子集）。允许所有 Unicode 代码点 U+0000 到 U+10FFFF，但如果您希望在字符串中嵌入空字节，则必须使用长度感知函数。

```c
json_t *json_string(const char *value)
```

返回一个新的 JSON 字符串，或者在出错时返回 NULL。值必须是有效的空终止 UTF-8 编码的 Unicode 字符串。

```c
json_t *json_stringn(const char *value, size_t len)
```

与 json_string() 类似，但具有明确的长度，因此值可能包含空字符或不以空字符结尾

```c
json_t *json_string_nocheck(const char *value)
json_t *json_stringn_nocheck(const char *value, size_t len)
```

不检测

```c
const char *json_string_value(const json_t *string)
```

以空终止的 UTF-8 编码字符串形式返回字符串的关联值，如果字符串不是 JSON 字符串，则返回 NULL。 返回值是只读的，用户不得修改或释放。只要字符串存在，即只要它的引用计数没有降到零，它就是有效的。

```c
size_t json_string_length(const json_t *string)
```

返回字符串在其 UTF-8 表示中的长度，如果字符串不是 JSON 字符串，则返回零。

```c
int json_string_set(json_t *string, const char *value)
```

将字符串的关联值设置为值。值必须是有效的 UTF-8 编码的 Unicode 字符串。成功时返回 0，错误时返回 -1。

```c
int json_string_setn(json_t *string, const char *value, size_t len)
```

与 json_string_set() 类似，但具有明确的长度，因此值可能包含空字符或不以空字符结尾。

```c
int json_string_set_nocheck(json_t *string, const char *value)
```

像json_string_set（），但不检查值是有效的UTF-8。这真的是情况仅当您确定此功能（例如你已经检查了通过其他方式）

```c
int json_string_setn_nocheck(json_t *string, const char *value, size_t len)
```

与 json_string_set_nocheck() 类似，但具有明确的长度，因此值可能包含空字符或不以空字符结尾。

```c
json_t *json_sprintf(const char *format, ...)
json_t *json_vsprintf(const char *format, va_list ap)
```

从格式字符串和可变参数构造一个 JSON 字符串，就像 printf() 一样。

## 数字

JSON 规范仅包含一种数字类型“数字”。 C 编程语言对整数和浮点数有不同的类型，因此出于实际原因，Jansson 对这两者也有不同的类型。它们分别被称为“整数integer”和“实数real”。

json_int_t

这是用于存储 JSON 整数值的 C 类型。它代表系统上可用的最宽整数类型。在实践中，如果你的编译器支持它，它只是一个 long long 的 typedef，否则就是 long。

```c
json_t *json_integer(json_int_t value)
```

返回一个新的 JSON 整数，或者在出错时返回 NULL。

```c
json_int_tjson_integer_value(const json_t *integer)
```

返回整数的关联值，如果 json 不是 JSON 整数，则返回 0。

```c
int json_integer_set(const json_t *integer, json_int_t value)
```

将 integer 的关联值设置为 value。成功时返回 0，如果整数不是 JSON 整数，则返回 -1。

```c
json_t *json_real(double value)
```

返回一个新的 JSON 实数，或者在出错时返回 NULL。

```c
double json_real_value(const json_t *real)
```

返回 real 的关联值，如果 real 不是 JSON real，则返回 0.0。

```c
int json_real_set(const json_t *real, double value)
```

将 real 的关联值设置为 value。成功时返回 0，如果 real 不是 JSON 实数，则返回 -1。

```c
double json_number_value(const json_t *json)
```

返回 JSON 整数或 JSON 实数 json 的关联值，无论实际类型如何，都转换为 double。如果 json 既不是 JSON 实数也不是 JSON 整数，则返回 0.0。

## 数组

JSON 数组是其他 JSON 值的有序集合。

```c
json_t *json_array(void)
```

返回一个新的 JSON 数组，或者在出错时返回 NULL。最初，该数组为空。

```c
size_t json_array_size(const json_t *array)
```

返回数组中的元素个数，如果数组为 NULL 或不是 JSON 数组，则返回 0。

```c
int json_array_set(json_t *array, size_t index, json_t *value)
```

用值替换数组中位置索引处的元素。 index 的有效范围是从 0 到 json_array_size() 的返回值减去 1。成功时返回 0，错误时返回 -1。

```c
json_t *json_array_get(const json_t *array, size_t index)
```

返回数组中索引位置的元素。 index 的有效范围是从 0 到 json_array_size() 的返回值减 1。如果 array 不是 JSON 数组，如果 array 为 NULL，或者如果 index 超出范围，则返回 NULL。

```c
int json_array_set_new(json_t *array, size_t index, json_t *value)
```

类似于 json_array_set() 但窃取了对 value 的引用。当值是新创建的并且在调用后不使用时，这很有用。

```c
int json_array_append(json_t *array, json_t *value)
```

将值附加到数组的末尾，将数组的大小增加 1。成功时返回 0，错误时返回 -1。

```c
int json_array_append_new(json_t *array, json_t *value)
```

类似于 json_array_append() 但窃取了对 value 的引用。当值是新创建的并且在调用后不使用时，这很有用。

```c
int json_array_insert(json_t *array, size_t index, json_t *value)
```

将值插入到数组的 index 位置，将 index 处的元素向数组末尾移动一个位置。成功时返回 0，错误时返回 -1。

```c
int json_array_insert_new(json_t *array, size_t index, json_t *value)
```

类似于 json_array_insert() 但窃取了对 value 的引用。当值是新创建的并且在调用后不使用时，这很有用。

```c
int json_array_remove(json_t *array, size_t index)
```

移除数组中 index 位置的元素，将 index 之后的元素向数组开头移动一个位置。成功时返回 0，错误时返回 -1。删除值的引用计数递减。

```c
int json_array_clear(json_t *array)
```

从数组中删除所有元素。成功时返回 0，错误时返回 -1。所有删除值的引用计数都会递减。

```c
int json_array_extend(json_t *array, json_t *other_array)
```

将 other_array 中的所有元素附加到数组的末尾。成功时返回 0，错误时返回 -1。

```c
json_array_foreach(array, index, value)
```

迭代数组的每个元素，每次运行后面的代码块，并将适当的值设置为变量 index 和 value，分别为 size_t 和 json_t * 类型。

项目以递增的索引顺序返回。 该宏在预处理时扩展为普通的 for 语句，因此其性能相当于使用数组访问函数的手写代码。这个宏的主要优点是它抽象了复杂性，并使代码更加简洁和可读。

实例：

```c
/* array is a JSON array */
size_t index;
json_t *value;

json_array_foreach(array, index, value) {
    /* block of code that uses index and value */
}
```

## 对象

JSON 对象是键值对的字典，其中键是 Unicode 字符串，值是任何 JSON 值。 尽管字符串值中允许使用空字节，但对象键中不允许使用空字节。

```c
json_t *json_object(void)
```

返回一个新的 JSON 对象，或者在出错时返回 NULL。最初，对象是空的。

```c
size_t json_object_size(const json_t *object)
```

返回对象中的元素个数，如果对象不是 JSON 对象，则返回 0。

```c
json_t *json_object_get(const json_t *object, const char *key)
```

从对象中获取与键对应的值。如果未找到键且发生错误，则返回 NULL。

```c
int json_object_set(json_t *object, const char *key, json_t *value)
```

将键的值设置为对象中的值。键必须是有效的空终止 UTF-8 编码的 Unicode 字符串。如果 key 已经有一个值，它将被新值替换。成功时返回 0，错误时返回 -1。

```c
int json_object_set_nocheck(json_t *object, const char *key, json_t *value)
```

与 json_object_set() 类似，但不检查密钥是否为有效的 UTF-8。仅当您确定确实如此（例如，您已经通过其他方式检查过）时才使用此功能。

```c
int json_object_set_new(json_t *object, const char *key, json_t *value)
```

类似于 json_object_set() 但窃取了对 value 的引用。当值是新创建的并且在调用后不使用时，这很有用。

```c
int json_object_set_new_nocheck(json_t *object, const char *key, json_t *value)
```

与 json_object_set_new() 类似，但不检查密钥是否为有效的 UTF-8。仅当您确定确实如此（例如，您已经通过其他方式检查过）时才使用此功能。

```c
int json_object_del(json_t *object, const char *key)
```

从对象中删除键（如果存在）。成功时返回 0，如果未找到键，则返回 -1。删除值的引用计数递减。

```c
int json_object_clear(json_t *object)
```

从对象中删除所有元素。成功时返回 0，如果对象不是 JSON 对象，则返回 -1。所有删除值的引用计数都会递减。

```c
int json_object_update(json_t *object, json_t *other)
```

使用其他键值对更新对象，覆盖现有键。成功时返回 0，错误时返回 -1。

```c
int json_object_update_existing(json_t *object, json_t *other)
```

与 json_object_update() 类似，但仅更新现有键的值。不会创建新密钥。成功时返回 0，错误时返回 -1。

```c
int json_object_update_missing(json_t *object, json_t *other)
```

与 json_object_update() 类似，但只创建新键。任何现有键的值都不会更改。成功时返回 0，错误时返回 -1。

```c
int json_object_update_new(json_t *object, json_t *other)
```

与 json_object_update() 类似，但窃取了对 other 的引用。当 other 是新创建的并且在调用后不使用时，这很有用。

```c
int json_object_update_existing_new(json_t *object, json_t *other)
```

与 json_object_update_new() 类似，但仅更新现有键的值。不会创建新密钥。成功时返回 0，错误时返回 -1。

```c
int json_object_update_missing_new(json_t *object, json_t *other)
```

与 json_object_update_new() 类似，但只创建新的键。任何现有键的值都不会更改。成功时返回 0，错误时返回 -1。

```c
int json_object_update_recursive(json_t *object, json_t *other)
```

与 json_object_update() 类似，但是如果 other 中的对象值也是对象，则它们将与 object 中的相应值递归合并，而不是覆盖它们。成功时返回 0，错误时返回 -1。

```c
json_object_foreach(object, key, value)
```

迭代对象的每个键值对，每次运行后面的代码块，并将正确的值设置为变量 key 和 value，分别为 const char * 和 json_t * 类型。例子：

```c
/* obj is a JSON object */
const char *key;
json_t *value;

json_object_foreach(obj, key, value) {
    /* block of code that uses key and value */
}
```

项目按它们插入对象的顺序返回。 注意：在迭代过程中调用 json_object_del(object, key) 是不安全的。如果需要，请改用 json_object_foreach_safe()。 该宏在预处理时扩展为普通的 for 语句，因此其性能相当于使用对象迭代协议的手写迭代代码（见下文）。这个宏的主要优点是它抽象了迭代背后的复杂性，并使代码更加简洁和可读。

```c
json_object_foreach_safe(object, tmp, key, value)
```

与 json_object_foreach() 类似，但在迭代期间调用 json_object_del(object, key) 是安全的。您需要传递一个额外的 void * 参数 tmp 用于临时存储。

以下函数可用于遍历对象中的所有键值对。项目按它们插入对象的顺序返回。

```c
void *json_object_iter(json_t *object)
```

返回一个不透明的迭代器，可用于迭代对象中的所有键值对，如果对象为空，则返回 NULL。

```c
void *json_object_iter_at(json_t *object, const char *key)
```

与 json_object_iter() 类似，但返回一个迭代器，指向对象中键等于键的键值对，如果在对象中找不到键，则返回 NULL。如果键恰好是底层哈希表中的第一个键，则向前迭代到对象的末尾只会产生对象的所有键值对。

```c
void *json_object_iter_next(json_t *object, void *iter)
```

返回一个迭代器，该迭代器指向 iter 之后的对象中的下一个键值对，如果整个对象已被迭代，则返回 NULL。

```c
const char *json_object_iter_key(void *iter)
```

从 iter 中提取关联的键。

```c
json_t *json_object_iter_value(void *iter)
```

从 iter 中提取关联的值。

```c
int json_object_iter_set(json_t *object, void *iter, json_t *value)
```

将 iter 指向的 object 中键值对的值设置为 value。

```c
int json_object_iter_set_new(json_t *object, void *iter, json_t *value)
```

类似于 json_object_iter_set()，但窃取了对 value 的引用。当值是新创建的并且在调用后不使用时，这很有用。

```c
void *json_object_key_to_iter(const char *key)
```

像 json_object_iter_at()，但要快得多。仅适用于 json_object_iter_key() 返回的值。使用其他键会导致段错误。该函数在内部用于实现 json_object_foreach()。例子：

```c
/* obj is a JSON object */
const char *key;
json_t *value;

void *iter = json_object_iter(obj);
while(iter)
{
    key = json_object_iter_key(iter);
    value = json_object_iter_value(iter);
    /* use key and value ... */
    iter = json_object_iter_next(obj, iter);
}
```

```c
void json_object_seed(size_t seed)
```

 为 Jansson 的哈希表实现中使用的哈希函数设定种子。种子用于随机化散列函数，以便攻击者无法控制其输出。

 如果种子为 0，Jansson 通过从操作系统的熵源读取随机数据来生成种子本身。如果没有可用的熵源，则回退到使用当前时间戳（如果可能，精度为微秒）和进程 ID 的组合。 

如果完全调用，则必须在任何显式或隐式调用 json_object() 之前调用此函数。如果用户未调用此函数，则对 json_object() 的第一次调用（显式或隐式）会为散列函数提供种子。有关线程安全的说明，请参阅线程安全。 

如果需要可重复的结果，例如单元测试，散列函数可以通过在程序启动时使用常量值调用 json_object_seed() 来“非随机化”，例如json_object_seed(1)。

## 错误上报

 实例：

```
int main() {
    json_t *json;
    json_error_t error;

    json = json_load_file("/path/to/file.json", 0, &error);
    if(!json) {
        /* the error variable contains error information */
    }
    ...
}
```

json_error_t

错误信息包括：

char text[]：错误消息（UTF-8 格式），如果消息不可用，则为空字符串。 该数组的最后一个字节包含一个数字错误代码。使用 json_error_code() 提取此代码。

char source[]：错误的来源。这可以是（一部分）文件名或尖括号中的特殊标识符（例如 <string>）。

int line：发生错误的行号。

int column：发生错误的列。请注意，这是字符列，而不是字节列，即多字节 UTF-8 字符计为一列。

int position：从输入开始的位置（以字节为单位）。这对于调试 Unicode 编码问题很有用。

另请注意，如果调用成功（上例中为 json != NULL），则错误的内容通常未指定。解码函数也会在成功时写入位置成员。

所有函数也接受 NULL 作为 json_error_t 指针，在这种情况下，不会向调用者返回错误信息。

enum json_error_code

包含数字错误代码的枚举。当前定义了以下错误：

- json_error_unknown：未知错误。这应该只为非错误的 json_error_t 结构返回。
- json_error_out_of_memory：库无法分配任何堆内存。
- json_error_stack_overflow：堆栈溢出、嵌套太深
- json_error_cannot_open_file：无法打开文件文件
- json_error_invalid_argument：参数非法
- json_error_invalid_utf8：输入字符串不是UTF-8格式
- json_error_premature_end_of_input：输入在 JSON 值的中间结束。
- json_error_end_of_input_expected：JSON 值结束后有一些文本。请参阅 JSON_DISABLE_EOF_CHECK 标志。
- json_error_invalid_syntax：JSON 语法错误。
- json_error_invalid_format：用于打包或解包的格式字符串无效。
- json_error_wrong_type：打包或解包时，值的实际类型与格式字符串中指定的类型不同。
- json_error_null_character：在 JSON 字符串中检测到空字符。
- json_error_null_value：打包或解包时，某些键或值为 NULL。
- json_error_null_byte_in_key：对象键包含一个空字节。 Jansson 不能代表这样的键；
- json_error_duplicate_key：对象中的重复键。
- json_error_numeric_overflow：将 JSON 数字转换为 C 数字类型时，检测到数字溢出。
- json_error_item_not_found：对象中无该键key
- json_error_index_out_of_range：数组索引超出范围

```c
enum json_error_code json_error_code(const json_error_t *error)
```

返回嵌入在 error->text 中的错误代码(<font color='red'>2.10的jansson用不了</font>)



<font size=6 color='red'>注意：</font>

字符串：这个很好解释，指使用“”双引号或’’单引号包括的字符。例如：var comStr = 'this is string';
json字符串：指的是符合json格式要求的js字符串。例如：var jsonStr = "{StudentID:'100',Name:'tmac',Hometown:'usa'}";
json对象：指符合json格式要求的js对象。例如：var jsonObj = { StudentID: "100", Name: "tmac", Hometown: "usa" };

## 编码

本节介绍可用于将<font color='red'>JSON数据（包括对象、数组、true、int、real等所有数据结构）数值转化为JSON字符串</font>的函数。默认情况下，只有对象和数组可以直接编码，因为它们是 JSON 文本的唯一有效根值。要对任何 JSON 值进行编码，请使用 JSON_ENCODE_ANY 标志（见下文）。 

默认情况下，输出没有换行符，并且在数组和对象元素之间使用空格以获得可读输出。可以使用下面描述的 JSON_INDENT 和 JSON_COMPACT 标志来更改此行为。换行符永远不会附加到编码的 JSON 数据的末尾。 

每个函数都有一个标志参数，用于控制数据编码方式的某些方面。其默认值为 0。可以将以下宏进行 OR 运算以获取标志。

JSON_INDENT(n)

漂亮地打印结果，在数组和对象项之间使用换行符，并用 n 个空格缩进。 n 的有效范围在 0 到 31（含）之间，其他值导致未定义的输出。如果未使用 JSON_INDENT 或 n 为 0，则不会在数组和对象项之间插入换行符。 JSON_MAX_INDENT 常量定义了可以使用的最大缩进，其值为 31。

JSON_COMPACT

此标志启用紧凑表示，即将数组和对象项之间的分隔符设置为“,”，并将对象键和值之间的分隔符设置为“:”。如果没有这个标志，相应的分隔符是“,”和“:”以获得更可读的输出。

JSON_ENSURE_ASCII

如果使用此标志，则保证输出仅包含 ASCII 字符。这是通过转义 ASCII 范围之外的所有 Unicode 字符来实现的。

JSON_SORT_KEYS

如果使用此标志，则输出中的所有对象都按关键字排序。这很有用，例如如果两个 JSON 文本不同或在视觉上比较。

JSON_PRESERVE_ORDER

<font color='red'>2.8 版后已弃用</font>：始终保留对象键的顺序。 2.8 版之前：如果使用此标志，则输出中的对象键将按照它们首次插入对象的相同顺序进行排序。例如，解码 JSON 文本然后使用此标志进行编码会保留对象键的顺序。

.JSON_ENCODE_ANY

指定此标志可以自行编码任何 JSON 值。没有它，只能将对象和数组作为 json 值传递给编码函数。

 注意：在某些情况下编码任何值可能很有用，但通常不鼓励这样做，因为它违反了与 RFC 4627 的严格兼容性。如果您使用此标志，请不要指望与其他 JSON 系统的互操作性。

JSON_ESCAPE_SLASH

用 \/ 转义字符串中的 / 字符。

JSON_REAL_PRECISION(n)

输出精度最多为 n 位的所有实数。 n 的有效范围在 0 到 31（含）之间，其他值会导致未定义的行为。 默认情况下，精度为 17，以正确无损地编码所有 IEEE 754 双精度浮点数。

JSON_EMBED

如果使用此标志，则在编码过程中会忽略顶级数组（‘[‘、‘]’）或对象（‘{‘、‘}’）的开始和结束字符。当将多个数组或对象连接到一个流中时，此标志很有用。

以下函数输出UTF-8字符串：

dump：dump在计算机科学中一个广泛运用的动词、名词。 作为动词：一般指将数据<font color='red'>导出、转存成文件或静态形式</font>。 比如可以理解成：把内存某一时刻的内容，dump（转存，导出，保存）成文件。 作为名词：一般特指上述过程中所得到的文件或者静态形式。

```c
char *json_dumps(const json_t *json, size_t flags) 
```

以字符串形式返回 json 的 JSON 表示形式，或在出错时返回 NULL。标志如上所述。返回值必须由调用者使用 free() 释放。请注意，如果您已调用 json_set_alloc_funcs() 来覆盖 free()，则应调用自定义的 free 函数来释放返回值。 

```c
size_t json_dumpb(const json_t *json, char *buffer, size_t size, size_t flags) 
```

将 json 的 JSON 表示写入size字节的缓冲区。返回将写入的字节数或错误时返回 0。标志如上所述。缓冲区不是空终止的。 此函数永远不会写入超过 size 个字节。如果返回值大于 size，则缓冲区的内容未定义。此行为使您能够指定 NULL 缓冲区以确定编码的长度。例如： 

```c
size_t size = json_dumpb(json, NULL, 0, 0);
if (size == 0)
    return -1;

char *buf = alloca(size);

size = json_dumpb(json, buf, size, 0);
```



```c
int json_dumpf(const json_t *json, FILE *output, size_t flags) 
```

将 json数据写入流输出。标志如上所述。成功时返回 0，错误时返回 -1。如果发生错误，则可能已将某些内容写入输出。在这种情况下，输出未定义并且很可能不是有效的 JSON。

```c
int json_dumpfd（const json_t *json，int output，size_t flags） 
```

将 json数据写入流输出。标志如上所述。成功时返回 0，错误时返回 -1。如果发生错误，则可能已将某些内容写入输出。在这种情况下，输出未定义并且很可能不是有效的 JSON。 需要注意的是，此函数只能在<font color='red'>流文件描述符</font>（例如 SOCK_STREAM）上成功。在非流文件描述符上使用此函数将导致未定义的行为。对于非流文件描述符，请参阅 json_dumpb()。 此功能需要 POSIX 并且在所有非 POSIX 系统上失败。 

```c
int json_dump_file(const json_t *json, const char *path, size_t flags) 
```

将 json 的 JSON 表示写入文件路径。如果路径已经存在，它会被覆盖。标志如上所述。成功时返回 0，错误时返回 -1。

```c
 json_dump_callback_t 
```

由 json_dump_callback() 调用的回调函数： 

```c
typedef int (*json_dump_callback_t)(const char *buffer, size_t size, void *data); 
```

buffer 指向一个包含输出块的缓冲区，size 是缓冲区的长度，而 data 是传递的相应 json_dump_callback() 参数。 

buffer 保证是有效的 UTF-8 字符串（即保留多字节代码单元序列）。缓冲区从不包含嵌入的空字节。 出错时，该函数应返回 -1 以停止编码过程。成功时，它应该返回 0。

```c
int json_dump_callback(const json_t *json, json_dump_callback_t callback, void *data, size_t flags) 
```

重复调用回调，每次传递 json 的一部分 JSON 表示。标志如上所述。成功时返回 0，错误时返回 -1。

## 解码

<font color='red'>实际上是将网络中传输的JSON格式字符串形式的数据解码为JSON格式的数据结构（对象、数组等）</font>

本节介绍可用于将 JSON 文本解码为 JSON 数据的 Jansson 表示的函数。<font color='red'>这里的JSON文本实际就是JSON字符串</font> JSON 规范要求 JSON 文本是序列化数组或对象，并且此要求也通过以下函数强制执行。换句话说，正在解码的 JSON 文本中的顶级值必须是数组或对象。要解码任何 JSON 值，请使用 JSON_DECODE_ANY 标志（见下文）。 

有关 Jansson 对 JSON 规范的一致性的讨论，请参阅 RFC Conformance。它解释了许多特别影响解码器行为的设计决策。 

每个函数都有一个标志参数，可用于控制解码器的行为。其默认值为 0。可以将以下宏进行 OR 运算以获取标志。

JSON_REJECT_DUPLICATES

如果输入文本中的任何 JSON 对象包含重复键，则发出解码错误。如果没有这个标志，每个键最后一次出现的值最终会出现在结果中。密钥等价性是逐字节检查的，没有特殊的 Unicode 比较算法。

JSON_DECODE_ANY

默认情况下，解码器需要一个数组或对象作为输入。启用此标志后，解码器接受任何有效的 JSON 值。 

注意：解码任何值在某些情况下可能很有用，但通常不鼓励这样做，因为它违反了与 RFC 4627 的严格兼容性。如果您使用此标志，请不要指望与其他 JSON 系统的互操作性。

JSON_DISABLE_EOF_CHECK

默认情况下，解码器期望它的整个输入构成一个有效的 JSON 文本，如果在其他有效的 JSON 输入之后有额外的数据，则会发出错误。启用此标志后，解码器在解码有效的 JSON 数组或对象后停止，因此允许在 JSON 文本之后添加额外数据。 通常，当遇到 JSON 输入中的最后一个 ] 或 } 时，读取将停止。如果同时使用 JSON_DISABLE_EOF_CHECK 和 JSON_DECODE_ANY 标志，解码器可能会读取一个额外的 UTF-8 代码单元（最多 4 个字节的输入）。例如，解码 4true 正确解码整数 4，但也读取 t。因此，如果读取多个不是数组或对象的连续值，则应至少用一个空格字符分隔它们。

JSON_DECODE_INT_AS_REAL

JSON 只定义了一种数字类型。 Jansson 区分整数和实数。有关更多信息，请参阅实数与整数。启用此标志后，解码器将所有数字解释为实数值。没有精确双精度表示的整数将默默地导致精度损失。导致双溢出的整数将导致错误。

JSON_ALLOW_NUL

允许 \u0000 在字符串值内转义。这是一项安全措施；如果您知道您的输入可以包含空字节，请使用此标志。如果你不使用这个标志，你就不必担心字符串中的空字节，除非你通过使用例如显式地创建它们自己。 json_stringn() 或 s# json_pack() 格式说明符。 即使使用此标志，对象键也不能嵌入空字节。

每个函数还采用一个可选的 json_error_t 参数，如果解码失败，该参数将填充错误信息。它还更新了成功；输入读取的字节数写入其位置字段。这在使用 JSON_DISABLE_EOF_CHECK 读取多个连续 JSON 文本时特别有用。 

2.3 新版功能：输入读取的字节数写入 json_error_t 结构的位置字段。

 如果不需要错误或位置信息，则可以传递 NULL。

```c
json_t *json_loads(const char *input, size_t flags, json_error_t *error)
```

解码 JSON 字符串输入并返回它包含的数组或对象，或者在出错时返回 NULL，在这种情况下，error 将填充有关错误的信息。标志如上所述。<font color='red'>将静态格式中的格式化东西去除，返回纯数据</font>

```c
json_t *json_loadb(const char *buffer, size_t buflen, size_t flags, json_error_t *error)
```

解码 JSON 字符串缓冲区，其长度为 buflen，并返回它包含的数组或对象，或错误时返回 NULL，在这种情况下，error 将填充有关错误的信息。这与 json_loads() 类似，只是字符串不需要以空字符结尾。标志如上所述。

```c
json_t *json_loadf(FILE *input, size_t flags, json_error_t *error)
```

解码流输入中的 JSON 文本并返回它包含的数组或对象，或者在出错时返回 NULL，在这种情况下，error 将填充有关错误的信息。标志如上所述。 此函数将从输入文件所在的任何位置开始读取输入，而不会尝试先查找。如果发生错误，文件位置将不确定。成功时，文件位置将位于 EOF，除非使用 JSON_DISABLE_EOF_CHECK 标志。在这种情况下，文件位置将位于 JSON 输入中最后一个 ] 或 } 之后的第一个字符。这允许在同一个 FILE 对象上多次调用 json_loadf()，如果输入由连续的 JSON 文本组成，可能由空格分隔。

```c
json_t *json_loadfd(int input, size_t flags, json_error_t *error)
```

解码流输入中的 JSON 文本并返回它包含的数组或对象，或者在出错时返回 NULL，在这种情况下，error 将填充有关错误的信息。标志如上所述。

 此函数将从输入文件描述符所在的任何位置开始读取输入，而不会尝试先查找。如果发生错误，文件位置将不确定。成功时，文件位置将位于 EOF，除非使用 JSON_DISABLE_EOF_CHECK 标志。在这种情况下，文件描述符的位置将位于 JSON 输入中最后一个 ] 或 } 之后的第一个字符。

这允许在同一个文件描述符上多次调用 json_loadfd()，如果输入由连续的 JSON 文本组成，可能由空格分隔。 需要注意的是，此函数只能在流文件描述符（例如 SOCK_STREAM）上成功。在非流文件描述符上使用此函数将导致未定义的行为。对于非流文件描述符，请参阅 json_loadb()。另外，请注意该函数不能用于非阻塞文件描述符（例如非阻塞套接字）。在非阻塞文件描述符上使用这个函数有很高的数据丢失风险，因为它不支持恢复。 此功能需要 POSIX 并且在所有非 POSIX 系统上失败。

```c
json_t *json_load_file(const char *path, size_t flags, json_error_t *error)
```

解码文件路径中的 JSON 文本并返回它包含的数组或对象，或者在错误时返回 NULL，在这种情况下，错误会填充有关错误的信息。标志如上所述。

json_load_callback_t

由 json_load_callback() 调用以读取输入数据块的函数的 typedef：

```c
typedef size_t (*json_load_callback_t)(void *buffer, size_t buflen, void *data);
```

buffer 指向一个 buflen 字节的缓冲区，而 data 是传递过来的对应的 json_load_callback() 参数。 成功时，该函数最多应将 buflen 字节写入缓冲区，并返回写入的字节数；返回值 0 表示未生成数据并且已到达文件末尾。出错时，该函数应返回 (size_t)-1 以中止解码过程。 

在 UTF-8 中，一些代码点被编码为多字节序列。回调函数不需要担心这个，因为 Jansson 在更高级别处理它。例如，您可以安全地从网络连接读取固定数量的字节，而不必关心被块边界分开的代码单元序列。

```c
json_t *json_load_callback(json_load_callback_t callback, void *data, size_t flags, json_error_t *error)
```

对重复调用 callback 产生的 JSON 文本进行解码，并返回它包含的数组或对象，或者在出错时返回 NULL，在这种情况下，error 将填充有关错误的信息。每次调用时都会将数据传递给回调。标志如上所述。

## 打包

本节介绍有助于创建或打包复杂 JSON 值（尤其是嵌套对象和数组）的函数。价值构建基于用于告诉函数有关预期参数的格式字符串。

例如，格式字符串“i”指定单个整数值，而格式字符串“[ssb]”或等效的“[s, s, b]”指定一个数组值，其中包含两个字符串和一个布尔值作为其项：

```c
/* Create the JSON integer 42 */
json_pack("i", 42);

/* Create the JSON array ["foo", "bar", true] */
json_pack("[ssb]", "foo", "bar", 1);
```

这是格式说明符的完整列表。括号中的类型表示生成的 JSON 类型，括号中的类型（如果有）表示预期作为相应参数的 C 类型。

**s (string) [const char *]**

将空终止的 UTF-8 字符串转换为 JSON 字符串。

**s? (string) [const char *]**

与 s 类似，但如果参数为 NULL，则输出 JSON 空值。

**s* (string) [const char *]**

与s类似，但如果参数为NULL，则不输出任何值。这种格式只能在对象或数组中使用。如果在对象内部使用，则在省略值时会额外抑制相应的键。请参阅下面的示例。

**s# (string) [const char *, int]**

将给定长度的 UTF-8 缓冲区转换为 JSON 字符串。

**s% (string) [const char *, size_t]**

与 s# 类似，但长度参数的类型为 size_t

**\+ [const char *]**

类似于 s，但连接到前一个字符串。仅在 s、s#、+ 或 +# 之后有效。

**\+# [const char *, int]**

类似于 s#，但连接到前一个字符串。仅在 s、s#、+ 或 +# 之后有效。

**\+% (string) [const char *, size_t]**

与 +# 类似，但长度参数的类型为 size_t。

**n (null)**

输出一个 JSON 空值。不消耗任何参数

**b (boolean) [int]**

将 C int 转换为 JSON 布尔值。零转换为假，非零转换为真。

**i (integer) [int]**

将 C int 转换为 JSON 整数。

**I (integer) [json_int_t]**

将 C json_int_t 转换为 JSON 整数。

**f (real) [double]**

将 C double 转换为 JSON real。

**o (any value) [json_t *]**

按原样输出任何给定的 JSON 值。如果将值添加到数组或对象中，则容器会窃取传递给 o 的值的引用。

**O (any value) [json_t *]**

与 o 类似，但参数的引用计数增加。如果您打包到数组或对象中并希望将 O 消耗的 JSON 值的引用保留给自己，这将非常有用。

**o?, O? (any value) [json_t *]**

分别类似于 o 和 O，但如果参数为 NULL，则输出 JSON 空值。

**o\*, O\* (any value) [json_t *]**

分别像 o 和 O，但如果参数为 NULL，则不输出任何值。这种格式只能在对象或数组中使用。如果在对象内部使用，相应的键会被额外抑制。请参阅下面的示例。

**[fmt] (array)**

使用内部格式字符串 fmt 中的内容构建一个对象。第一个、第三个等格式说明符代表一个键，并且必须是一个字符串（参见上面的 s、s#、+ 和 +#），因为对象键总是字符串。第二、第四等格式说明符表示一个值。任何值都可以是对象或数组，即支持递归值构建。

**{fmt} (object)**

使用内部格式字符串 fmt 中的内容构建一个对象。第一个、第三个等格式说明符代表一个键，并且必须是一个字符串（参见上面的 s、s#、+ 和 +#），因为对象键总是字符串。第二、第四等格式说明符表示一个值。任何值都可以是对象或数组，即支持递归值构建。

```c
json_t *json_pack(const char *fmt, ...)
```

根据格式字符串 fmt 构建一个新的 JSON 值。对于每个格式说明符（{}[]n 除外），使用一个或多个参数并用于构建相应的值。出错时返回 NULL。

```c
json_t *json_pack_ex(json_error_t *error, size_t flags, const char *fmt, ...)

json_t *json_vpack_ex(json_error_t *error, size_t flags, const char *fmt, va_list ap)
```

与 json_pack() 类似，但在出现错误的情况下，如果它不是 NULL，则会将错误消息写入 error。 flags 参数当前未使用，应设置为 0。 由于打包程序只能捕获格式字符串中的错误（和内存不足错误），因此这两个函数很可能仅用于调试格式字符串。

示例：

```c
/* Build an empty JSON object */
json_pack("{}");

/* Build the JSON object {"foo": 42, "bar": 7} */
json_pack("{sisi}", "foo", 42, "bar", 7);

/* Like above, ':', ',' and whitespace are ignored */
json_pack("{s:i, s:i}", "foo", 42, "bar", 7);

/* Build the JSON array [[1, 2], {"cool": true}] */
json_pack("[[i,i],{s:b}]", 1, 2, "cool", 1);

/* Build a string from a non-null terminated buffer */
char buffer[4] = {'t', 'e', 's', 't'};
json_pack("s#", buffer, 4);

/* Concatenate strings together to build the JSON string "foobarbaz" */
json_pack("s++", "foo", "bar", "baz");

/* Create an empty object or array when optional members are missing */
json_pack("{s:s*,s:o*,s:O*}", "foo", NULL, "bar", NULL, "baz", NULL);
json_pack("[s*,o*,O*]", NULL, NULL, NULL);
```



## 解包

本节介绍有助于验证复杂值并从中提取或解包数据的函数。与构建值一样，这也基于格式字符串。 

在解压 JSON 值时，会检查格式字符串中指定的类型以匹配 JSON 值的类型。这是过程的验证部分。除此之外，解包函数还可以检查数组和对象的所有项目是否都被解包。使用格式说明符启用此检查！或使用标志 JSON_STRICT。

详情请见下文。 这是格式说明符的完整列表。括号中的类型表示 JSON 类型，括号中的类型（如果有）表示应传递其地址的 C 类型。

**s (string) [const char *]**

将 JSON 字符串转换为指向以空字符结尾的 UTF-8 字符串的指针。生成的字符串是通过内部使用 json_string_value() 提取的，因此只要仍然存在对相应 JSON 字符串的引用，它就存在。

**s% (string) [const char *, size_t *]**

将 JSON 字符串转换为指向以空字符结尾的 UTF-8 字符串及其长度的指针。

**n (null)**

期望 JSON 空值。什么都没有提取。

**b (boolean) [int]**

将 JSON 布尔值转换为 C int，以便将 true 转换为 1，将 false 转换为 0。

**i (integer) [int]**

将 JSON 整数转换为 C int。

**I (integer) [json_int_t]**

将 JSON 整数转换为 C json_int_t。

**f (real) [double]**

将 JSON real 转换为 C double。

**F (integer or real) [double]**

将 JSON 数字（整数或实数）转换为 C double。

**o (any value) [json_t *]**

存储不转换为 json_t 指针的 JSON 值。

**O (any value) [json_t *]**

与 o 类似，但 JSON 值的引用计数递增。在使用解包之前，存储指针应该被初始化为 NULL。调用者负责释放所有通过解包递增的引用，即使发生错误也是如此。

**[fmt] (array)**

根据内部格式字符串转换 JSON 数组中的每个项目。 fmt 可能包含对象和数组，即支持递归值提取。

**{fmt} (object)**

根据内部格式字符串 fmt 转换 JSON 对象中的每个项目。第一个、第三个等格式说明符代表一个键，必须是s。解包函数的相应参数被读取为对象键。第二个、第四个等格式说明符表示一个值并写入作为相应参数给出的地址。请注意，读取每个其他参数并写入每个其他参数。 fmt 可能包含对象和数组作为值，即支持递归值提取。 2.3 新版功能：任何表示键的 s 都可以后缀 ?使密钥可选。如果未找到密钥，则不会提取任何内容。请参阅下面的示例。

**!**

这个特殊的格式说明符用于启用检查所有对象和数组项是否被访问，基于每个值。它必须出现在数组或对象中，作为结束括号或大括号之前的最后一个格式说明符。要全局启用检查，请使用 JSON_STRICT 解包标志。

**\***

这个特殊的格式说明符与 ! 相反。如果使用 JSON_STRICT 标志，* 可用于禁用基于每个值的严格检查。它必须出现在数组或对象中，作为结束括号或大括号之前的最后一个格式说明符。

空格, : 和 ,被忽略。

```c
int json_unpack(json_t *root, const char *fmt, ...)
```

根据格式字符串 fmt 验证并解压 JSON 值root。成功时返回 0，失败时返回 -1。

```c
int json_unpack_ex(json_t *root, json_error_t *error, size_t flags, const char *fmt, ...)

int json_vunpack_ex(json_t *root, json_error_t *error, size_t flags, const char *fmt, va_list ap)
```

根据格式字符串 fmt 验证并解压 JSON 值根。如果发生错误且error 不为NULL，则将错误信息写入error。标志可用于控制解包器的行为，有关标志，请参见下文。成功时返回 0，失败时返回 -1。

注：

所有解包函数的第一个参数是 json_t *root 而不是 const json_t *root，因为使用 O 格式说明符会导致 root 的引用计数或从 root 可达的某些值增加。此外，o 格式说明符可用于按原样提取值，这允许修改可从根访问的值的结构或内容。

 如果不使用 O 和 o 格式说明符，则在与这些函数一起使用时将 const json_t * 变量强制转换为纯 json_t * 是完全安全的。

以下解包标志可用：

JSON_STRICT——启用额外的验证步骤，检查所有对象和数组项是否已解包。这相当于附加格式说明符！到格式字符串中每个数组和对象的末尾。

JSON_VALIDATE_ONLY——不要提取任何数据，只需根据给定的格式字符串验证 JSON 值。请注意，仍然必须在格式字符串之后指定对象键。

示例：

```c
/* root is the JSON integer 42 */
int myint;
json_unpack(root, "i", &myint);
assert(myint == 42);

/* root is the JSON object {"foo": "bar", "quux": true} */
const char *str;
int boolean;
json_unpack(root, "{s:s, s:b}", "foo", &str, "quux", &boolean);
assert(strcmp(str, "bar") == 0 && boolean == 1);

/* root is the JSON array [[1, 2], {"baz": null} */
json_error_t error;
json_unpack_ex(root, &error, JSON_VALIDATE_ONLY, "[[i,i], {s:n}]", "baz");
/* returns 0 for validation success, nothing is extracted */

/* root is the JSON array [1, 2, 3, 4, 5] */
int myint1, myint2;
json_unpack(root, "[ii!]", &myint1, &myint2);
/* returns -1 for failed validation */

/* root is an empty JSON object */
int myint = 0, myint2 = 0, myint3 = 0;
json_unpack(root, "{s?i, s?[ii]}",
            "foo", &myint1,
            "bar", &myint2, &myint3);
/* myint1, myint2 or myint3 is no touched as "foo" and "bar" don't exist */
```

assert()：“断言”在语文中的意思是“断定”、“十分肯定地说”，在编程中是指对某种假设条件进行检测，如果条件成立就不进行任何操作，如果条件不成立就捕捉到这种错误，并打印出错误信息，终止程序执行。

## 相等

通常，无法使用 == 运算符来测试两个 JSON 值是否相等。 == 运算符方面的相等性表示两个 json_t 指针指向完全相同的 JSON 值。但是，两个 JSON 值不仅可以是完全相同的值，还可以是它们具有相同的“内容”： 

- 如果两个整数或实数值包含的数值相等，则它们相等。但是，整数值永远不会等于实际值。 
- 如果两个字符串包含的 UTF-8 字符串逐字节相等，则它们相等。未实现 Unicode 比较算法。 
- 如果两个数组具有相同数量的元素，并且第一个数组中的每个元素都等于第二个数组中的相应元素，则它们相等。
-  如果两个对象具有完全相同的键，并且第一个对象中每个键的值等于第二个对象中相应键的值，则两个对象相等。 
- 两个 true、false 或 null 值没有“内容”，因此如果它们的类型相等，则它们相等。 （因为这些值是单例，它们的相等性实际上可以用 == 来测试。）

```c
int json_equal(json_t *value1, json_t *value2)
```

如果 value1 和 value2 相等，则返回 1，如上所述。如果它们不相等或一个或两个指针为 NULL，则返回 0。

## 数值拷贝

由于引用计数，传递 JSON 值不需要复制它们。但有时需要一份 JSON 值的新副本。例如，如果您需要修改一个数组，但之后仍想使用原始数组，则应先复制它。 

Jansson 支持两种复制：浅拷贝和深拷贝。这些方法之间的区别仅适用于数组和对象。浅复制仅复制第一级值（数组或对象）并在复制值中使用相同的子值。深度复制也会对子值进行新的复制。此外，所有子值都以递归方式进行深度复制。

 复制对象会保留键的插入顺序。

```c
json_t *json_copy(json_t *value)
```

返回值的浅拷贝，错误时返回 NULL。

```c
json_t *json_deep_copy(const json_t *value)
```

返回值的深层副本，或错误时返回 NULL。

## 内存分配

默认情况下，Jansson 使用 malloc() 和 free() 进行内存分配。如果需要自定义行为，可以覆盖这些函数。

json_malloc_t：

带有 malloc() 签名的函数指针的 typedef：typedef void *(*json_malloc_t)(size_t);

json_free_t：

一种用于与游离（）的签名函数指针的typedef：typedef void (*json_free_t)(void *);

```c
void json_set_alloc_funcs(json_malloc_t malloc_fn, json_free_t free_fn)
```

使用 malloc_fn 代替 malloc() 和 free_fn 代替 free()。此函数必须在任何其他 Jansson 的 API 函数之前调用，以确保所有内存操作使用相同的函数。

```c
void json_get_alloc_funcs(json_malloc_t *malloc_fn, json_free_t *free_fn)
```

获取当前使用的 malloc_fn 和 free_fn。任何一个参数都可以为 NULL。

示例：

通过使用应用程序的 malloc() 和 free() 在 Windows 上解决不同 CRT 堆的问题：

```c
json_set_alloc_funcs(malloc, free);
```

使用 Boehm 的保守垃圾收集器进行内存操作：

```c
json_set_alloc_funcs(GC_malloc, GC_free);
```

通过在释放时将所有内存归零，允许在 JSON 结构中存储敏感数据（例如密码或加密密钥）：

```
static void *secure_malloc(size_t size)
{
    /* Store the memory area size in the beginning of the block */
    void *ptr = malloc(size + 8);
    *((size_t *)ptr) = size;
    return ptr + 8;
}

static void secure_free(void *ptr)
{
    size_t size;

    ptr -= 8;
    size = *((size_t *)ptr);

    guaranteed_memset(ptr, 0, size + 8);
    free(ptr);
}

int main()
{
    json_set_alloc_funcs(secure_malloc, secure_free);
    /* ... */
}
```



demo：

\#include <stdio.h>

\#include <stdlib.h>

\#include <string.h>

\#include <pthread.h>

\#include <unistd.h>

\#include <assert.h>



\#include <jansson.h>



int main(int argc, char **argv)

{

  json_t *obj = NULL;

  json_t *group = NULL;

  json_t *root = NULL;

  json_t *value;   //若在if，for等函数里面定义，仅在相关函数里面有用

  //FILE * fp = NULL;

  json_error_t err;

  char *buf = NULL;

  int value1 = 0;

  const char *key;



  //fp = fopen("jsondata.txt", "r+");

  //encoding

  obj = json_load_file("jsondata.txt", 0, &err);



  printf("%s", err.text);

  //assert(obj);

  if ( !obj )

  {

​    fprintf( stderr, "err: on line %d: %s\n", err.line, err.text );

​    return -1;

  }

  else

  {

​    group = json_object_get( obj, "group" );

​    assert( obj );



​    printf( "group:%s\n", json_string_value( group ) );

​    //decoding

​    buf = json_dumps( obj, JSON_INDENT(4) );

​    printf( "%s\n", buf );



​    //packing

​    root = json_pack("{s:s, s:i}", "frute", "apple", "weight", 1);

​    buf = NULL;

​    buf = json_dumps( root, JSON_INDENT(4) ); 

​    printf( "%s\n", buf );

​    buf = NULL;



​    //unpacking

​    json_unpack(root, "{s:s, s:i}", "frute", &buf, "weight", &value1);

​    printf( "%s:%d\n", buf, value1 );

​    //printf( "%s\n", err.text );





​    //iter

​    

​    void *iter = json_object_iter(root);

​    while (iter)

​    {

​      key = json_object_iter_key(iter);

​      value = json_object_iter_value(iter);

​      switch(json_typeof(value))

​      {

​        case JSON_STRING:

​          printf( "%s:%s\n", key, json_string_value(value));

​          break;

​        case JSON_INTEGER:

​          json_unpack(value, "i", &value1);

​          printf( "%s:%d\n", key, value1);

​          break;

​      }

​      iter = json_object_iter_next(root, iter);

​    }

​    

​    

  }



  

  free(buf);

  json_decref( group );

  json_decref( value );

  json_decref( obj );

  json_decref( root );

  return 0;

}
