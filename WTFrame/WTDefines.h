#pragma once
#include <memory>

#define WTNAMESPACESTART namespace WT{
#define WTNAMESPACEEND }

#define FRAMENAMESPACESTART namespace Frame{
#define FRAMENAMESPACEEND }
#define USINGFRAMENAMESPACE using namespace WT::Frame;

#ifdef WT_Export
#define WTAPI _declspec(dllexport)
#else
#define WTAPI _declspec(dllimport)
#endif

// Examples:
// MyClassPtr = std::shared_ptr<MyClass>        (suitable for storage and passing as argument to function that needs to extend lifetime)
// SceneCPtr = std::shared_ptr<const Scene>     (suitable for storage and passing as const argument to function that needs to extend lifetime)
// SceneWeakPtr = std::weak_ptr<Scene>          (suitable for storage when you don't want to extend lifetime or incur circular references)
// SceneWeakCPtr = std::weak_ptr<const Scene>   (suitable for storage and passing as const argument to function when you don't want to extend lifetime)
//
//WT_SMART_POINTER_(struct, IPluginFactory);     用于结构体
//WT_SMART_POINTER(PluginFactoryBase);          用于类
#define WT_SMART_POINTER_(TYPE, CLASS)             \
    TYPE CLASS;                                         \
    using CLASS##Ptr = std::shared_ptr<CLASS>;          \
    using CLASS##CPtr = std::shared_ptr<const CLASS>;   \
    using CLASS##WeakPtr = std::weak_ptr<CLASS>;        \
    using CLASS##WeakCPtr = std::weak_ptr<const CLASS>

#define WT_SMART_POINTER(CLASS) WT_SMART_POINTER_(class, CLASS)