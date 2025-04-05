#pragma once
#include <pthread.h>
#include "NoCopyable.h"

namespace nest
{
    namespace base
    {
        template <typename T>
        class Singleton: NonCopyable
        {
        public:
            Singleton() = delete;
            ~Singleton() = delete;

            static T*& Instance()
            {
                pthread_once(&ponce_, &Singleton::Init);
                return value_;
            }
        private:
            static void Init()
            {
                if (!value_) {
                    value_ = new T();
                }
            }

            static pthread_once_t ponce_;
            static T * value_; 
        };

        template <typename T>
        pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

        template <typename T>
        T * Singleton<T>::value_ = nullptr;
    }
}