# C++ 四种 Cast 学习笔记

C++ 提供了四种显式类型转换：

- `static_cast`
- `const_cast`
- `reinterpret_cast`
- `dynamic_cast`

相比 C 风格转换：

```cpp
(T)value
```

C++ 的 cast 将不同类型转换行为拆分，使代码能够明确表达转换目的，提高可读性和安全性。

## 1. `static_cast`

### 1.1 基本概念

`static_cast` 用于编译期类型转换，适用于具有明确类型关系，并且编译器能够检查合法性的转换。

它不会进行运行时类型检查。

### 1.2 常见使用场景

#### 1. 内置数值类型转换

例如：

- `double -> int`
- `int -> double`
- `float -> long`
- `char -> int`
- `bool -> int`

示例：

```cpp
double value = 3.14;
int number = static_cast<int>(value);
```

结果：

```cpp
number = 3;
```

注意：

`static_cast` 不改变转换规则，只是将隐式转换显式化。

例如：

```cpp
int a = value;
int b = static_cast<int>(value);
```

两者转换结果相同。

区别在于：

`static_cast<int>(value)` 明确表达了：

这里的类型转换是程序员主动要求的。

### 1.3 枚举转换

支持：

- `enum -> int`
- `int -> enum`

例如：

```cpp
enum class Color
{
    Red = 1,
    Green = 2
};

int value = static_cast<int>(Color::Green);
```

注意：

整数转换为 `enum` 时不会检查是否合法。

例如：

```cpp
Color c = static_cast<Color>(100);
```

编译可以通过。

### 1.4 `void*` 转换

`static_cast` 可以完成：

- `void* -> T*`
- `T* -> void*`

例如：

```cpp
int value = 100;
void* ptr = &value;
int* p = static_cast<int*>(ptr);
```

这在 C 接口中比较常见。

### 1.5 继承体系转换

#### 向上转型

`Derived -> Base`

```cpp
class Base
{
};

class Derived : public Base
{
};

Derived d;
Base* p = static_cast<Base*>(&d);
```

向上转型一定安全。

因为：

```text
Derived
├── Base
└── Derived成员
```

`Derived` 一定包含 `Base`。

#### 向下转型

`Base -> Derived`

```cpp
Base* base = &derived;
Derived* child = static_cast<Derived*>(base);
```

可以编译。

但是：

`static_cast` 不检查真实对象类型。

例如：

```cpp
Base base;
Base* p = &base;

Derived* d = static_cast<Derived*>(p);
```

编译通过。

但是对象实际不是 `Derived`。

访问：

```cpp
d->xxx;
```

会导致：

未定义行为（Undefined Behavior, UB）。

如果需要运行时检查，应使用：

`dynamic_cast`

### 1.6 特点总结

- 编译期完成
- 不进行运行时类型检查
- 不能移除 `const` / `volatile`
- 不能转换完全无关的类型

适用原则：

类型之间本身存在合法转换关系，并且不需要运行时检查时，优先使用 `static_cast`。

---

## 2. `const_cast`

### 2.1 基本概念

`const_cast` 用于修改：

- `const`
- `volatile`

限定符。

它不能改变对象类型。

### 2.2 常见使用场景

#### `const T*` 与 `T*`

例如：

```cpp
int value = 100;
const int* p = &value;

int* q = const_cast<int*>(p);
*q = 200;
```

这是合法的。

原因：

对象本身是：

```cpp
int value;
```

并不是 `const`。

只是通过：

```cpp
const int*
```

访问。

### 2.3 真正 `const` 对象禁止修改

例如：

```cpp
const int value = 100;

int* p = const_cast<int*>(&value);
*p = 200;
```

虽然可以转换。

但是修改 `const` 对象属于：

未定义行为（UB）。

### 2.4 `mutable`

`mutable` 是解决 `const` 成员修改问题的正规方式。

例如：

```cpp
class Cache
{
public:
    int Get() const
    {
        m_cache = 100;
        return m_cache;
    }

private:
    mutable int m_cache = 0;
};
```

`mutable` 表示：

即使对象是 `const`，该成员也允许修改。

常用于：

- 缓存
- 延迟初始化
- 统计信息
- `mutex`

### 2.5 特点总结

- 只能修改 `const` / `volatile`
- 不能改变类型
- 修改真正 `const` 对象属于 UB

适用原则：

仅在对象本身不是 `const`，只是接口要求 `const` 时使用。

---

## 3. `reinterpret_cast`

### 3.1 基本概念

`reinterpret_cast` 表示：

重新解释一块内存的含义。

它不会进行真正的数据转换。

只是改变编译器理解这块内存的方式。

### 3.2 常见使用场景

#### 1. 不同类型指针转换

例如：

```cpp
int value = 100;

char* p = reinterpret_cast<char*>(&value);
```

内存没有变化。

只是：

```text
int*
 |
 v
char*
```

#### 2. 查看对象字节

例如：

```cpp
int value = 0x12345678;

unsigned char* bytes =
    reinterpret_cast<unsigned char*>(&value);
```

可以读取对象底层字节。

#### 3. 指针和整数转换

例如：

```cpp
uintptr_t addr =
    reinterpret_cast<uintptr_t>(&value);
```

常用于：

- 内核
- 驱动
- 内存映射

### 3.3 特点总结

- 不进行类型检查
- 不进行运行时检查
- 不改变内存内容
- 风险最高

错误使用容易导致：

未定义行为（UB）。

适用原则：

仅用于底层开发，需要直接操作内存表示时使用。

普通业务代码尽量避免。

---

## 4. `dynamic_cast`

### 4.1 基本概念

`dynamic_cast` 用于：

具有继承关系的类之间进行安全的运行时类型转换。

主要用于：

```text
Base*
  ↓
Derived*
```

向下转型。

### 4.2 为什么需要虚函数？

`dynamic_cast` 依赖：

RTTI（Runtime Type Information，运行时类型信息）。

当类具有虚函数时：

```cpp
class Base
{
public:
    virtual ~Base() = default;
};
```

编译器会：

- 创建虚函数表（vtable）
- 对象内部保存 `vptr`
- `vtable` 关联 RTTI 信息

运行时流程大致如下：

```text
Base*
   |
dynamic_cast
   |
通过 vptr
   |
查询 RTTI
   |
判断真实对象类型
   |
+----------------+
| Derived ?      |
+----------------+

是：返回 Derived*
否：返回 nullptr
```

### 4.3 指针转换

成功：

```cpp
Base* base = new Derived();

Derived* d =
    dynamic_cast<Derived*>(base);
```

返回：

`Derived*`

失败：

```cpp
Base* base = new OtherDerived();

Derived* d =
    dynamic_cast<Derived*>(base);
```

返回：

`nullptr`

### 4.4 引用转换

成功：

```cpp
Derived& d =
    dynamic_cast<Derived&>(baseRef);
```

失败：

抛出：

`std::bad_cast`

### 4.5 与 `static_cast` 区别

#### `static_cast`

- 编译期转换
- 不检查真实类型
- 错误的 `Base* -> Derived*` 可能导致未定义行为

#### `dynamic_cast`

- 运行时检查 RTTI
- 失败可检测
- 更加安全

---

## 四种 Cast 总结

| Cast               | 作用                      | 检查时机 | 安全性   |
| ------------------ | ------------------------- | -------- | -------- |
| `static_cast`      | 普通类型转换、继承转换    | 编译期   | 中       |
| `const_cast`       | 修改 `const` / `volatile` | 编译期   | 条件安全 |
| `reinterpret_cast` | 重新解释内存              | 无检查   | 最危险   |
| `dynamic_cast`     | 安全向下转型              | 运行时   | 最高     |

## 最终记忆

### `static_cast`

我知道类型关系，编译器帮我检查。

### `const_cast`

我只想修改 `const` 属性。

### `reinterpret_cast`

不管类型，把这块内存按照另一种方式解释。

### `dynamic_cast`

我不确定真实类型，让运行时帮我检查。

## 推荐使用顺序

正常 C++ 开发中，通常建议按以下倾向使用：

```text
static_cast
    ↓
const_cast
    ↓
dynamic_cast
    ↓
reinterpret_cast
```

其中：

- `static_cast` 最常用
- `dynamic_cast` 用于多态安全转换
- `const_cast` 应尽量少用
- `reinterpret_cast` 只用于底层代码

