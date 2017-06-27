// Умный указатель с value semantics. Exception safe, правильно стоят noexcept. Поддерживаются рекурсивные структуры данных. Дефолтный конструктор, деструктор и прочее все не являются тривиальными

// value_ptr работает даже если T - incomplete type. Это позволяет создавать с его помощью рекурсивные структуры данных. Но при этом value_ptr имеет баг: std::is_copy_constructible_v<value_ptr<std::unique_ptr<int>>> - это true (такой же баг есть у std::vector в libstdc++: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70472 , к настоящему моменту этот баг libstdc++ может быть пофиксен). Я не знаю способа опционально отключить конструтор копирования с помощью std::enable_if и тому подобных техник. Единственный известный мне способ: унаследовать value_ptr от класса, который будет либо иметь, либо не иметь конструктор копирования в зависимости от переданных к нему шаблонных параметров. Но тогда в случае рекурсивного типа данных std::is_copy_constructible_v<T> будет вычисляться в тот момент, когда T - incomplete type, а потому std::is_copy_constructible_v будет false. Так что остаётся единственный известный мне способ решить проблему с std::is_copy_constructible_v: просто вручную определить std::is_copy_constructible для value_ptr. TO DO: всё же попробовать опционально отключить конструтор копирования и оператор присваивания с помощью enable_if и тому подобного, определить traits std::is_copy_constructible и тому подобные

// В коде предусмотрены те вещи, которые нужно предусматривать при написании кода такого рода, т. е. правило пяти, exception safety и так далее. Тем не менее, много дополнительных вещей (которые есть у стандартного std::unique_ptr и прочих) не предусмотрено, например, аллокаторы, массивы и так далее. TO DO: предусмотреть их, также дополнить функциональность по аналогии с std::unique_ptr

// Требуем, чтобы T был std::is_nothrow_destructible_v. Требование не выражено в коде. Если попытаться сделать ~value_ptr () noexcept (std::is_nothrow_destructible_v<T>) или ~value_ptr () noexcept (noexcept (p->~T ())), то имеем ошибку компиляции на g++ 6.3.0 (-std=c++17) для рекурсивных типов данных. Если попытаться опционально отключить деструктор (с помощью трюка с наследованием) или сам класс, то получим невозможность использования для incomplete types. Я не знаю других способов выразить в коде требование std::is_nothrow_destructible_v

// Код написан на C++11 и исходит из ограничений языка на момент актуальности этого стандарта

// value_ptr имеет полную value semantics (отсюда название). В частности, если в будущем будет добавлен operator==, то он должен вызывать operator== для значения. Я не могу придумать никакого применения для копирующего умного указателя без полной value semantics. В частности, поэтому operator* протаскивает const

#ifndef _VALUE_PTR_VALUE_PTR_HPP
#define _VALUE_PTR_VALUE_PTR_HPP

#include <cstddef>
#include <utility>

namespace value_ptr_ns
{
  template <typename T> class value_ptr
  {
    T *p;

  public:
    value_ptr () noexcept : p (nullptr)
    {
    }

    value_ptr (std::nullptr_t) noexcept : p (nullptr)
    {
    }

    explicit value_ptr (T *a) noexcept : p (a)
    {
    }

    value_ptr (value_ptr &&a) noexcept : p (a.p)
    {
      a.p = nullptr;
    }

    value_ptr (const value_ptr &a) : p (a.p == nullptr ? nullptr : new T (*a.p))
    {
    }

    value_ptr &
    operator= (std::nullptr_t) noexcept
    {
      delete p;
      p = nullptr;
    }

    // DR 2468 says we should leave object in valid but unspecified state in case of self-move. And we do this
    value_ptr &
    operator= (value_ptr &&a) noexcept
    {
      delete p;
      p = a.p;
      a.p = nullptr;
    }

    /*
     * Alternative implementation may look like this:
     *
     * value_ptr &
     * operator= (value_ptr &&a) noexcept
     * {
     *   std::swap (this->p, a.p);
     * }
     *
     * But such implementation will not do what user want. He wants "p" to be deleted right now
     */

    value_ptr &
    operator= (const value_ptr &a)
    {
      if (this != &a)
        {
          if (a.p == nullptr)
            {
              delete p;
              p = nullptr;
            }
          else
            {
              if (p == nullptr)
                {
                  p = new T (*a.p);
                }
              else
                {
                  *p = *a.p;
                }
            }
        }

      return *this;
    }

    ~value_ptr () noexcept
    {
      delete p;
    }

    void
    swap (value_ptr &a) noexcept
    {
      std::swap (p, a.p);
    }

    const T &
    operator* () const & noexcept
    {
      return *p;
    }

    T &
    operator* () & noexcept
    {
      return *p;
    }

    const T &&
    operator* () const && noexcept
    {
      return std::move (*p);
    }

    T &&
    operator* () && noexcept
    {
      return std::move (*p);
    }

    const T *
    operator-> () const noexcept
    {
      return p;
    }

    T *
    operator-> () noexcept
    {
      return p;
    }
  };

  template <typename T> inline void
  swap (value_ptr<T> &a, value_ptr<T> &b) noexcept
  {
    a.swap (b);
  }

  template <typename T, typename... U> inline value_ptr<T>
  make_value (U &&... a)
  {
    return value_ptr<T> (new T (std::forward<U> (a)...));
  }
}

#endif // ! _VALUE_PTR_VALUE_PTR_HPP
