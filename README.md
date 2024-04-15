# Red Lib

Header-only library to help create [RED4ext](https://github.com/WopsS/RED4ext.SDK) plugins.

## Integration

### CMake

```cmake
add_compile_definitions(NOMINMAX)
add_subdirectory(vendor/RedLib)
target_link_libraries(Project PRIVATE RedLib)
```

```cpp
#include <RedLib.hpp>
```

### Namespaces

The namespace of the library `Red` is also an alias for `RED4ext`.
You can use whichever one you prefer (for example, `RED4ext::IScriptable` vs `Red::IScriptable`).

## Building type info

### Global functions

```cpp
Red::DynArray<int32_t> MakeArray(int32_t n)
{
    Red::DynArray<int32_t> array;
    array.Reserve(n);

    while (--n >= 0)
    {
        array.PushBack(std::rand());
    }

    return array;
}

void SortArray(Red::ScriptRef<Red::DynArray<int32_t>>& array)
{
    std::sort(array.ref->begin(), array.ref->end());
}

void Swap(int32_t* a, int32_t* b)
{
    std::swap(*a, *b);
}

RTTI_DEFINE_GLOBALS({
    RTTI_FUNCTION(MakeArray);
    RTTI_FUNCTION(SortArray);
    RTTI_FUNCTION(Swap);
});
```

```swift
public static native func MakeArray(n: Int32) -> array<Int32>
public static native func SortArray(array: script_ref<array<Int32>>)
public static native func Swap(a: Int32, b: Int32)
```

```swift
public static func TestGlobals() {
  let array = MakeArray(5);
  SortArray(array);
  for item in array {
    LogChannel(n"DEBUG", ToString(item));
  }

  let a = 3;
  let b = 7;
  Swap(a, b);
  LogChannel(n"DEBUG", s"a=\(a) b=\(b)");
}
```

### Enum definitions

```cpp
enum class MyEnum
{
    OptionA,
    OptionB,
    OptionC,
};

enum class MyFlags
{
    FlagA = 1 << 0,
    FlagB = 1 << 1,
    FlagC = 1 << 2,
};

RTTI_DEFINE_ENUM(MyEnum);
RTTI_DEFINE_FLAGS(MyFlags);
```

```swift
enum MyEnum {
  OptionA = 0,
  OptionB = 1,
  OptionC = 2,
}

enum MyFlags {
  FlagA = 1,
  FlagB = 2,
  FlagC = 4,
}
```

### Class definitions

#### Structs

```cpp
struct MyStruct
{
    int32_t Inc(Red::Optional<int32_t, 1> step)
    {
        value += step;
        return value;
    }

    int32_t value;
};

RTTI_DEFINE_CLASS(MyStruct, {
    RTTI_METHOD(Inc);
    RTTI_PROPERTY(value);
});
```

Note that redscript structs can't have instance methods.
All non-static methods are converted to static methods in redscript by the lib,
so you can conveniently use instance methods in C++ despite the limitation.

```swift
public native struct MyStruct {
  native let value: Int32;

  public static native func Inc(self: script_ref<MyStruct>, opt step: Int32) -> Int32
}
```

```swift
public static func TestStruct() {
  let x = new MyStruct(10);
  MyStruct.Inc(x);
  MyStruct.Inc(x, 5);

  LogChannel(n"DEBUG", ToString(x.value)); // 16
}
```

#### IScriptables

```cpp
struct MyClass : Red::IScriptable
{
    void AddItem(MyEnum item)
    {
        items.EmplaceBack(item);
    }
    
    void AddFrom(const Red::Handle<MyClass>& other)
    {
        for (const auto& item : other->items)
        {
            items.EmplaceBack(item);
        }
    }
    
    inline static Red::Handle<MyClass> Create()
    {
        return Red::MakeHandle<MyClass>();
    }

    Red::DynArray<MyEnum> items;

    RTTI_IMPL_TYPEINFO(MyClass);
    RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(MyClass, {
    RTTI_METHOD(AddItem);
    RTTI_METHOD(AddFrom);
    RTTI_METHOD(Create);
    RTTI_GETTER(items);
});
```

```swift
public native class MyClass {
  public native func AddItem(item: MyEnum)
  public native func AddFrom(other: ref<MyClass>)
  public native func GetItems() -> array<MyEnum>

  public static native func Create() -> ref<MyClass>
}
```

```swift
public static func TestClass() {
  let a = new MyClass();
  a.AddItem(MyEnum.OptionA);
  a.AddItem(MyEnum.OptionC);
    
  let b = MyClass.Create();
  b.AddItem(MyEnum.OptionB);
  b.AddFrom(a);
  
  for item in b.GetItems() {
    LogChannel(n"DEBUG", ToString(item));
  }
}
```

#### Inheritance

```cpp
struct ClassA : Red::IScriptable
{
    RTTI_IMPL_TYPEINFO(ClassA);
    RTTI_IMPL_ALLOCATOR();
};

struct ClassB : ClassA
{
    RTTI_IMPL_TYPEINFO(ClassB);
    RTTI_IMPL_ALLOCATOR();
};

struct ClassC : ClassB
{
    RTTI_IMPL_TYPEINFO(ClassC);
    RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(ClassA, "A", {
    RTTI_ABSTRACT();
});

RTTI_DEFINE_CLASS(ClassB, "B", {
    RTTI_PARENT(ClassA);
});

RTTI_DEFINE_CLASS(ClassC, "C", {
    RTTI_PARENT(ClassB);
});
```

```swift
public abstract native class A {}
public native class B extends A {}
public native class C extends B {}
```

#### Persistence

```cpp
struct MyData
{
    int32_t first;
    int32_t second;
};

RTTI_DEFINE_CLASS(MyData, {
    RTTI_PERSISTENT(first);
    RTTI_PROPERTY(second);
});
```

```swift
public native struct MyData {
  native persistent let first: Int32;
  native let second: Int32;
}

public class MySystem extends ScriptableSystem {
  private persistent let data: MyData;
    
  private func OnAttach() {
    this.data.first += 1; // Will be added to a save file and restored on load
    this.data.second += 1; // Will reset on every load

    LogChannel(n"DEBUG", s"MyData: \(this.data.first) / \(this.data.second)");
  }
}
```

#### Game systems

When you define `IGameSystem` class, it will be automatically registered in game instance.

```cpp
class MyGameSystem : public Red::IGameSystem
{
public:
    bool IsAttached() const
    {
        return attached;
    }

private:
    void OnWorldAttached(Red::world::RuntimeScene* scene) override
    {
        attached = true;
    }

    void OnWorldDetached(Red::world::RuntimeScene* scene) override
    {
        attached = false;
    }

    bool attached{};

    RTTI_IMPL_TYPEINFO(MyGameSystem);
    RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(MyGameSystem, {
    RTTI_METHOD(IsAttached);
});
```

```swift
public native class MyGameSystem extends IGameSystem {
  public native func IsAttached() -> Bool
}

@addMethod(GameInstance)
public static native func GetMyGameSystem() -> ref<MyGameSystem>
```

```swift
public static func TestGameSystem() {
  let system = GameInstance.GetMyGameSystem();
  
  LogChannel(n"DEBUG", s"Attached = \(system.IsAttached())");
}
```

#### Scripted classes

In some cases game expects scripted classes and/or functions, and rejects native members.
You can create scripted members backed by native code.
In particular, it allows you to create scriptable systems.

```cpp
struct MyScriptableSystem : Red::ScriptableSystem
{
    void OnAttach()
    {
        Red::Log::Debug("Attached");
    }

    void OnDetach()
    {
        Red::Log::Debug("Detached");
    }

    void OnRestored(int32_t saveVersion, int32_t gameVersion)
    {
        Red::Log::Debug("Restored save={} game={}", saveVersion, gameVersion);
    }

    RTTI_IMPL_TYPEINFO(MyScriptableSystem);
    RTTI_FWD_CONSTRUCTOR();
};

RTTI_DEFINE_CLASS(MyScriptableSystem, {
    RTTI_SCRIPT_METHOD(OnAttach);
    RTTI_SCRIPT_METHOD(OnDetach);
    RTTI_SCRIPT_METHOD(OnRestored);
});
```

#### Incomplete classes

Using `RTTI_FWD_CONSTRUCTOR()` as in the previous example, 
you can inherit partially dedcoded classes and delegate construction and destruction to RTTI system. 

### Alternative naming

You can use other names for RTTI definitions instead of the original C++ identifiers: 

```cpp
RTTI_DEFINE_ENUM(MyEnum, "Xyzzy");

RTTI_DEFINE_CLASS(MyStruct, "Foo", {
    RTTI_PROPERTY(value, "bar");
    RTTI_METHOD(Inc, "Baz");
});
```

```swift
enum Xyzzy {
  OptionA = 0,
  OptionB = 1,
  OptionC = 2,
}

public native struct Foo {
  native let bar: Int32;

  public static native func Baz(self: script_ref<Foo>, opt step: Int32) -> Int32
}
```

### Class extensions

You can add methods to already defined classes.

```cpp
struct MyExtension : Red::GameObject
{
    void AddTag(Red::CName tag)
    {
        tags.Add(tag);
    }
};

RTTI_EXPAND_CLASS(Red::GameObject, {
    RTTI_METHOD_FQN(MyExtension::AddTag);
});
```

```swift
@addMethod(GameObject)
public native func AddTag(tag: CName)
```

```swift
public static func TestExtension(game: GameInstance) {
  let player = GetPlayer(game);
  LogChannel(n"DEBUG", s"HasTag = \(player.HasTag(n"Test"))");
    
  player.AddTag(n"Test");
  LogChannel(n"DEBUG", s"HasTag = \(player.HasTag(n"Test"))");
}
```

Properties cannot be added to existing classes.

### Raw native handlers

```cpp
struct RawExample : RED4ext::IScriptable
{
    inline static void Add(RawExample* self, RED4ext::CStackFrame* frame, 
                           int32_t* out, RED4ext::CBaseRTTIType*)
    {
        int32_t a;
        int32_t b;
        
        RED4ext::GetParameter(frame, &a);
        RED4ext::GetParameter(frame, &b);
        ++frame->code;
        
        if (out)
        {
            *out = a + b;
        }
        
        // If this function was called from scripts, 
        // then stak frame should contain the caller
        if (frame->func)
        {
            self->caller = frame->func->shortName;
        }
    }
    
    RED4ext::CName caller;
    
    RTTI_IMPL_TYPEINFO(RawExample);
};

RTTI_DEFINE_CLASS(RawExample, {
    RTTI_METHOD(Add);
    RTTI_GETTER(caller);
});
```

```swift
public native class RawExample {
  public native func Add(a: Int32, b: Int32) -> Int32
  public native func GetCaller() -> CName
}
```

```swift
public static func TestRaw() {
  let obj = new RawExample();
  let sum = obj.Add(2, 5);

  LogChannel(n"DEBUG", s"Sum = \(sum)");
  LogChannel(n"DEBUG", s"Called from \(obj.GetCaller())");
}
```

If you need access to stack frame, alternatively you can just add it as a param to a regular method:

```cpp
struct RawExample : Red::IScriptable
{
    int32_t Add(int32_t a, int32_t b, Red::CStackFrame* frame)
    {
        if (frame->func)
        {
            caller = frame->func->shortName;
        }
        
        return a + b;
    }
    
    Red::CName caller;
    
    RTTI_IMPL_TYPEINFO(RawExample);
};
```

### Registration

To register your definitions you have to call `TypeInfoRegistrar::RegisterDiscovered()`.

```cpp
RED4EXT_C_EXPORT bool RED4EXT_CALL Main(RED4ext::PluginHandle aHandle, RED4ext::EMainReason aReason,
                                        const RED4ext::Sdk* aSdk)
{
    if (aReason == RED4ext::EMainReason::Load)
    {
        Red::TypeInfoRegistrar::RegisterDiscovered();
    }

    return true;
}
```

## Accessing type info

At compile time you can convert any C++ type to a corresponding RTTI type name:

```cpp
// CName("Uint64")
constexpr auto name = Red::GetTypeName<uint64_t>();

// CName("String")
constexpr auto name = Red::GetTypeName<Red::CString>();

// CName("array:handle:MyClass")
constexpr auto name = Red::GetTypeName<Red::DynArray<Red::Handle<MyClass>>>();

// std::array<char, 7> = "String\0"
constexpr auto name = Red::GetTypeNameStr<RED4ext::CString>();
```

At runtime you can get `CBaseRTTIType` and `CClass` based on C++ types:

```cpp
auto stringType = Red::GetType<Red::CString>();
auto enumArrayType = Red::GetType<Red::DynArray<MyEnum>>();
auto entityClass = Red::GetClass<Red::Entity>();
```

## Calling functions

```cpp
float a = 13, b = 78, max;
Red::CallGlobal("MaxF", max, a, b); // max = MaxF(a, b)
```

```cpp
Red::Vector4 vec{};
Red::CallStatic("Vector4", "Rand", vec); // vec = Vector4.Rand()
```

```cpp
Red::ScriptGameInstance game;
Red::Handle<Red::PlayerSystem> system;
Red::Handle<Red::GameObject> player;

// system = GameInstance.GetPlayerSystem(game)
Red::CallStatic("ScriptGameInstance", "GetPlayerSystem", system, game);

// player = system.GetLocalPlayerControlledGameObject()
Red::CallVirtual(system, "GetLocalPlayerControlledGameObject", player);

// player.Revive(100.0)
Red::CallVirtual(player, "Revive", 100.0f);

```

Whether a class / global function is defined in RTTI system? 
> It can be used to test if another mod is present or not, when it defines new 
> classes / global functions.
```cpp
bool hasRedFileSystem = Red::HasClass("FileSystem");
bool hasRedData = Red::Detail::HasGlobalFunction("ParseJson");
```

## Accessing game systems

```cpp
auto system = Red::GetGameSystem<Red::IPersistencySystem>();
auto status = system->GetEntityStatus(1ULL);
```

## Printing to game log

```cpp
const auto projectName = "MyMod";

Red::Log::Debug("Hello from {}", projectName);
```

## Examples

- [Codeware](https://github.com/psiberx/cp2077-codeware)
