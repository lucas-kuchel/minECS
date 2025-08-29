#pragma once

namespace minECS
{
    template <typename T>
    class ReferenceResult
    {
    public:
        using Type = T;

        ReferenceResult() = delete;

        ReferenceResult(T* value, bool status)
            : Value(value), Status(status)
        {
        }

        bool Succeeded() const
        {
            return Status && Value != nullptr;
        }

        bool SoftFailed() const
        {
            return !Status && Value != nullptr;
        }

        bool Failed() const
        {
            return !Status && Value == nullptr;
        }

        T& GetValue()
        {
            return *Value;
        }

        const T& GetValue() const
        {
            return *Value;
        }

    private:
        T* Value;
        bool Status;
    };

    template <typename T>
    class ValueResult
    {
    public:
        using Type = T;

        ValueResult() = delete;

        ValueResult(T value, bool status)
            : Value(value), Status(status)
        {
        }

        bool Succeeded() const
        {
            return Status;
        }

        T& GetValue()
        {
            return Value;
        }

        const T& GetValue() const
        {
            return Value;
        }

    private:
        T Value;
        bool Status;
    };
}