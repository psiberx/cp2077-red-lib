#pragma once

namespace Red
{
class TypeInfoRegistrar
{
public:
    using Callback = void(*)();

    TypeInfoRegistrar(Callback aRegister, Callback aDescribe)
    {
        AddRegisterCallback(aRegister);
        AddDescribeCallback(aDescribe);
    }

    static inline void RegisterDiscovered()
    {
        QueuePendingRegisterCallbacks();
        QueuePendingDescribeCallbacks();
    }

    // For hosts where the game does not run the RTTI register callbacks by itself (the macOS game): the plugin
    // calls this once the RTTI system is ready, and again whenever a describe callback re-queued itself.
    static inline void RunPending()
    {
        ProcessPendingRegisterCallbacks();
        ProcessPendingDescriberCallbacks();
    }

    static inline void AddRegisterCallback(Callback aRegister)
    {
        if (aRegister)
        {
            s_registerCallbacks.push_back(aRegister);
        }
    }

    static inline void AddDescribeCallback(Callback aDescribe)
    {
        if (aDescribe)
        {
            s_describeCallbacks.push_back(aDescribe);
        }
    }

private:
    static inline void QueuePendingRegisterCallbacks()
    {
        if (!s_registerCallbacks.empty())
        {
            CRTTISystem::Get()->AddRegisterCallback(&OnRegister);
        }
    }

    static inline void QueuePendingDescribeCallbacks()
    {
        if (!s_describeCallbacks.empty())
        {
            CRTTISystem::Get()->AddPostRegisterCallback(&OnDescribe);
        }
    }

    static inline void ProcessPendingRegisterCallbacks()
    {
        auto callbacks = std::move(s_registerCallbacks);
        for (const auto& callback :callbacks)
        {
            callback();
        }
    }

    static inline void ProcessPendingDescriberCallbacks()
    {
        auto callbacks = std::move(s_describeCallbacks);
        for (const auto& callback :callbacks)
        {
            callback();
        }
    }

    static inline void OnRegister()
    {
        ProcessPendingRegisterCallbacks();
        QueuePendingRegisterCallbacks();
    }

    static inline void OnDescribe()
    {
        ProcessPendingDescriberCallbacks();
        QueuePendingDescribeCallbacks();
    }

    static inline std::vector<Callback> s_registerCallbacks;
    static inline std::vector<Callback> s_describeCallbacks;
};
}
