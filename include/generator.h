#pragma once

#include <string>
#include <cstdlib>
#include <vector>
#include <queue>
#include <list>
#include <array>
#include <ctime>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>

#include <boost/optional.hpp>

namespace gen {

  constexpr unsigned int DEFAULT_MAX_STR_LEN = 10;
  constexpr unsigned int DEFAULT_MAX_SEQ_LEN = 10;

  template <class T, class Gen>
  class gen_iterator;

#ifdef WIN32
  void initialize(unsigned int seed = 0)
  {
    if (seed == 0)
      seed = (unsigned) time(NULL);
    
    srand(seed);
  }

  int32_t random_int32()
  {
    return rand();
  }

#else
  void initialize(unsigned int seed = 0)
  {
    if (seed == 0)
      seed = (unsigned) time(NULL);

    srandom(seed);
  }

  int random_int32()
  {
    return static_cast<int>(random());
  }
#endif // WIN32

  template <class GenFunc>
  auto make_gen_from(GenFunc&& func);

  template <class T>
  struct GenFactory
  {
    static auto make();
  };

  template <class T, class GenFunc>
  class Gen : GenFunc
  {
  public:
    typedef T value_type;

    explicit Gen(GenFunc& func)
      : GenFunc(func)
    { }

    explicit Gen(GenFunc&& func)
      : GenFunc(std::move(func))
    { }

    T generate()
    {
      return GenFunc::operator()();
    }

    gen_iterator<T, Gen> begin() 
    {
      return gen_iterator<T, Gen>(*this, false);
    }

    gen_iterator<T, Gen> end() 
    {
      return gen_iterator<T, Gen>(*this, true);
    }

    template <class Func>
    auto map(Func&& func)
    {
      return make_gen_from(
                [self = std::move(*this),
                 func = std::forward<Func>(func)]() mutable {
                    return func(self.generate());
                });
    }

    template <class Zipper, class... GenList>
    auto zip_with(Zipper&& func, GenList&&... genlist)
    {
      return make_gen_from(
        [self = std::move(*this),
         genlist...,
         func = std::forward<Zipper>(func)]() mutable {
            return func(self.generate(), genlist.generate()...);
      });
    }

    template <class UGen>
    auto amb(UGen&& ugen) 
    {
      return make_gen_from(
         [self = std::move(*this),
          ugen = std::forward<UGen>(ugen)]() mutable {
             return (random_int32() % 2) ? self.generate() : ugen.generate();
      });
    }

    auto take(size_t count) 
    {
      return make_gen_from(
             [self = std::move(*this), count]() mutable {
                if(count-- > 0)
                {
                  return self.generate();
                }
                else
                  throw std::out_of_range("take: generate exceeded take");
          });
    }

    template <class ReducerFunc, class Seed>
    auto reduce(ReducerFunc&& reducer, Seed&& seed) 
    {
      return make_gen_from(
               [self = std::move(*this),
                reducer = std::forward<ReducerFunc>(reducer), 
                seed = std::forward<Seed>(seed),
                done = false]() mutable {
                  try {
                    while(!done)
                      seed = reducer(seed, self.generate());
                  }
                  catch(std::out_of_range &) {
                    done = true;
                    return seed;
                  }
                  throw std::out_of_range("reduction completed");
                  return seed;
          });
    }

    template <class UGen>
    auto append(UGen&& ugen) 
    {
      return concat(std::forward<UGen>(ugen));
    }

    template <class UGen>
    auto concat(UGen&& ugen) 
    {
      return make_gen_from(
          [tgen = std::move(*this),
           ugen = std::forward<UGen>(ugen),
           tdone = false,
           udone = false]() mutable {
             if(!tdone)
             {
               try {
                  return tgen.generate();
               } 
               catch(std::out_of_range &) {
                 tdone = true;
               }
             }

             if(!udone)
             {
               try {
                  return ugen.generate();
               } 
               catch(std::out_of_range &) {
                 udone = true;
               }
             }
             
             throw std::out_of_range("concat: both generators completed!");
          });
    }

    template <class UGenFunc>
    auto concat_map(UGenFunc&& ugenfunc) 
    {
      typedef decltype(ugenfunc(std::declval<T>())) UGenType;
      return make_gen_from(
          [tgen = std::move(*this),
           ugenfunc = std::forward<UGenFunc>(ugenfunc),
           //ugenopt  = boost::optional<UGenType>(),
           ugenvec  = std::vector<UGenType>(),
           tdone = false,
           udone = true]() mutable {
             if(!udone)
             {
               try {
                 return ugenvec[0].generate();
                 //return ugenopt->generate();
               }
               catch(std::out_of_range &) {
                 udone = true;
               }
             }
             while(!tdone && udone)
             {
               try {
                 //ugenopt.emplace(ugenfunc(tgen.generate()));
                 ugenvec.clear();
                 ugenvec.emplace_back(ugenfunc(tgen.generate()));
                 udone = false;
               }
               catch(std::out_of_range &) {
                 tdone = true;
                 throw;
               }

               if(!udone)
               {
                 try {
                   return ugenvec[0].generate();
                   //return ugenopt->generate();
                 }
                 catch(std::out_of_range &) {
                   udone = true;
                 }
               }
             }
             throw std::out_of_range("concat_map completed");
          });
    }

    std::vector<T> to_vector() 
    {
      std::vector<T> v;
      try {
        while(true)
          v.push_back(this->generate());
      }
      catch(std::out_of_range &)
      {
        return v;
      }
      return v;
    }

  };

  template <class GenFunc>
  auto make_gen_from(GenFunc&& func)
  {
    return Gen<decltype(func()), GenFunc>(std::forward<GenFunc>(func));
  }

  template <class T>
  auto make_constant_gen(T&& t)
  {
    return make_gen_from([t = std::forward<T>(t)]() { return t; });
  }

  template <class T>
  auto make_empty_gen()
  {
    return make_gen_from([]() -> T { 
        throw std::out_of_range("empty generator!"); 
        //return std::declval<T>(); //did not work for int!!
        return *static_cast<T *>(nullptr);
      });
  }

  template <class T>
  auto make_single_gen(T&& t)
  {
    return make_gen_from([t = std::forward<T>(t), done = false]() mutable {
        if(!done)
        {
          done = true;
          return t; 
        }
        else
          throw std::out_of_range("single generator completed!");
      });
  }

  template <class Integer>
  auto make_range_gen(Integer lo, Integer hi)
  {
    return make_gen_from([lo, hi]() { 
      return static_cast<Integer>(lo + random_int32() % (hi - lo)); 
    });
  }

  auto make_printable_gen()
  {
    return make_gen_from([]() {
      auto space = 32;
      auto tilde = 126;
      return static_cast<char>(space + random_int32() % (tilde + 1 - space));
    });
  }

  auto make_ascii_gen()
  {
    return make_gen_from([]() {
              return static_cast<char>(random_int32() % 128);
           });
  }

  auto make_lowercase_gen()
  {
    return make_gen_from([]() {
      return static_cast<char>('a' + random_int32() % ('z' + 1 - 'a'));
    });
  }

  auto make_uppercase_gen()
  {
    return make_gen_from([]() {
      return static_cast<char>('A' + random_int32() % ('Z' + 1 - 'A'));
    });
  }

  auto make_alpha_gen()
  {
    return make_gen_from([]() {
      return (random_int32() % 2) ?
        static_cast<char>('a' + random_int32() % ('z' + 1 - 'a')) :
        static_cast<char>('A' + random_int32() % ('Z' + 1 - 'A'));
    });
  }

  auto make_digit_gen()
  {
    return make_gen_from([]() {
      return static_cast<char>('0' + random_int32() % ('9' + 1 - '0'));
    });
  }

  auto make_alphanum_gen()
  {
    return make_gen_from([]() {
      if (random_int32() % 2)
      {
        return (random_int32() % 2) ?
          static_cast<char>('a' + random_int32() % ('z' + 1 - 'a')) :
          static_cast<char>('A' + random_int32() % ('Z' + 1 - 'A'));
      }
      else
        return static_cast<char>('0' + random_int32() % ('9' + 1 - '0'));
    });
  }

  template <class CharGen>
  auto make_string_gen(CharGen&& chargen = make_printable_gen(), 
                       unsigned int maxlen = DEFAULT_MAX_STR_LEN, 
                       bool possibly_empty = false)
  {
    return make_gen_from(
      [chargen=std::forward<CharGen>(chargen),
       maxlen,
       possibly_empty]() mutable {
        std::string str;
        int length =
          possibly_empty ? (random_int32() % (maxlen+1)) :
                           (random_int32() % maxlen) + 1;
        str.reserve(length);
        for (int i = 0; i < length; ++i)
          str.push_back(chargen.generate());
        
        return str;
    });
  }

  template <template <class, class> class Container, class ElemGen>
  auto make_seq_gen(ElemGen&& elemgen, 
                    unsigned int maxlen = DEFAULT_MAX_SEQ_LEN,
                    bool possibly_empty = false)
  {
    return make_gen_from([elemgen=std::forward<ElemGen>(elemgen),
                          maxlen,
                          possibly_empty]() mutable 
    {
      typedef typename std::remove_reference<ElemGen>::type ElemGenT;
      typedef typename ElemGenT::value_type ElemType;
      Container<ElemType, std::allocator<ElemType>> container;

      int length =
        possibly_empty ? (random_int32() % (maxlen + 1)) :
                         (random_int32() % maxlen) + 1;
      container.reserve(length);
      for (int i = 0; i < length; ++i)
        container.push_back(elemgen.generate());

      return container;
    });
  }

  template <class Zipper, class... GenList>
  auto make_zip_gen(Zipper&& func, GenList&&... genlist)
  {
    return make_gen_from(
      [genlist...,
       func = std::forward<Zipper>(func)]() mutable {
          return func(genlist.generate()...);
    });
  }

  template<size_t... Dims>
  struct dim_list {
    enum { size = sizeof...(Dims) };
  };

  namespace detail {

    template <class ElemGen, size_t DimListSize, class DimList>
    struct ArrayGen;

    template <class ElemGen, size_t DimListSize, size_t Head, size_t... Tail>
    struct ArrayGen<ElemGen, DimListSize, dim_list<Head, Tail...>>
    {
      static auto make(ElemGen & elemgen)
      {
        auto innergen =
          ArrayGen<ElemGen, DimListSize - 1, dim_list<Tail...>>::make(elemgen);
        
        return make_gen_from([innergen=std::move(innergen)]() mutable {

          std::array<decltype(innergen.generate()), Head> arr;
          for (auto & elem : arr)
            elem = innergen.generate();

          return arr;
        });
      }
    };
    
    template <class ElemGen, size_t Dim>
    struct ArrayGen<ElemGen, 1, dim_list<Dim>>
    {
      static auto make(ElemGen & elemgen)
      {
        return make_gen_from([elemgen]() mutable {
          typedef typename std::remove_reference<ElemGen>::type ElemGenT;
          std::array<typename ElemGenT::value_type, Dim> arr;
          for (auto & elem : arr)
            elem = elemgen.generate();

          return arr;
        });
      }
    };

    template <class... Args>
    struct TupleGen;

    template <class... Args>
    struct TupleGen<std::tuple<Args...>>
    {
      static auto make()
      {
        return make_zip_gen([](auto&&... args) {
          return std::make_tuple(std::forward<decltype(args)>(args)...);
        }, GenFactory<Args>::make()...);
      }
    };

  } // anonymous namespace

  template <class ElemGen, class DimList>
  auto make_array_gen(ElemGen&& elemgen, DimList)
  {
    typedef typename std::remove_reference<ElemGen>::type ElemGenT;
    return detail::ArrayGen<ElemGenT, DimList::size, DimList>::make(elemgen);
  }

  template <class ElemGen>
  auto make_optional_gen(ElemGen&& elemgen)
  {
    typedef typename std::remove_reference<ElemGen>::type ElemGenT;
    typedef typename boost::optional<typename ElemGenT::value_type> Opt;

    return make_gen_from([elemgen=std::forward<ElemGen>(elemgen)]() mutable {
      return (random_int32() % 2) ? Opt() : Opt(elemgen.generate());
    });
  }

  template <class Iter>
  auto make_inorder_gen(Iter begin, Iter end)
  {
    std::vector<typename std::iterator_traits<Iter>::value_type> 
      range(begin, end);
    
    /*
    auto iter = range.begin();
    return make_gen_from(
        [range = std::move(range),
         iter = std::move(iter)]() mutable {
           if(iter != range.end())
             return *iter++;
           else
             throw std::out_of_range("in_order_gen: Range traversal completed!");
    });*/
    return make_gen_from(
        [range = std::move(range),
         i = 0u]() mutable {
           if(i < range.size())
             return range[i++];
           else
             throw std::out_of_range("in_order_gen: Range traversal completed!");
    });
  }

  template <class T>
  auto make_inorder_gen(std::initializer_list<T> list)
  {
    return make_inorder_gen(list.begin(), list.end());
  }

  template <class T>
  auto make_oneof_gen(std::initializer_list<T> list)
  {
    std::vector<T> options(list);
    return make_gen_from([options = std::move(options)]() {
      return *(options.begin() + random_int32() % options.size());
    });
  }

  template <class T>
  auto make_oneof_gen(std::vector<T> options)
  {
    return make_gen_from([options = std::move(options)]() {
      return *(options.begin() + random_int32() % options.size());
    });
  }

  template <class UGen, class VGen>
  auto make_pair_gen(UGen&& ugen, VGen&& vgen)
  {
    return make_gen_from(
      [ugen = std::forward<UGen>(ugen),
       vgen = std::forward<VGen>(vgen)]() mutable {
          return std::make_pair(ugen.generate(), vgen.generate());
    });
  }

  template <class... GenList>
  auto make_composed_gen(GenList&&... genlist)
  {
    return make_gen_from(
      [genlist...]() mutable {
      return std::make_tuple(genlist.generate()...);
    });
  }

  template <class Tuple>
  auto make_tuple_gen()
  {
    return detail::TupleGen<Tuple>::make();
  }

  auto make_stepper_gen(int start = 0, 
                        int max = std::numeric_limits<int>::max(), 
                        int step = 1, 
                        bool cycle = false)
  {
    bool empty = false;
    if(step >= 0)
    {
      if(start > max)
        empty = true;
    }
    else
    {
      if(start < max)
        empty = true;
    }

    auto current = start;
    auto init = false;

    return make_gen_from([start, max, step, cycle, current, init, empty]() mutable 
           {
               if(empty)
                 throw std::out_of_range("stepper: steps over!");

               if(init==false)
                 init = true;
               else
               {
                 if(step >= 0)
                 {
                   if(current + step <= max)
                      current = current + step;
                   else
                   {
                     if(cycle)
                       current = start;
                     else
                       throw std::out_of_range("stepper: steps over!");
                   }
                 }
                 else
                 {
                   if(current + step >= max)
                      current = current + step;
                   else
                   {
                     if(cycle)
                       current = start;
                     else
                       throw std::out_of_range("stepper: stpes over!");
                   }
                 }
               }
               
               return current;
           });
  }

  namespace detail {

    template <class Gen, class HeapComp>
    auto make_priority_n_gen(Gen&& src_gen, size_t n, HeapComp comp)
    {
      using SrcType = decltype(src_gen.generate());
      std::priority_queue<SrcType, std::vector<SrcType>, HeapComp> heap;

      while (1)
      {
        try {
          auto && val = src_gen.generate();
          if (heap.size() < n)
          {
            heap.push(std::move(val));
          }
          else if (comp(val, heap.top()))
          {
            heap.push(std::move(val));
            heap.pop();
          }
        }
        catch (std::out_of_range &)
        {
          break;
        }
      }

      return make_gen_from([heap = std::move(heap)]() mutable {
        if (heap.empty())
          throw std::out_of_range("empty heap");
        else
        {
          SrcType val = heap.top();
          heap.pop();
          return val;
        }
      });
    }

  } // namespace detail

  template <class Gen>
  auto make_lowest_n_gen(Gen&& src_gen, size_t n)
  {
    using MaxHeapComp = std::less<>;
    return detail::make_priority_n_gen(std::forward<Gen>(src_gen), n, MaxHeapComp());
  }

  template <class Gen>
  auto make_highest_n_gen(Gen&& src_gen, size_t n)
  {
    using MinHeapComp = std::greater<>;
    return detail::make_priority_n_gen(std::forward<Gen>(src_gen), n, MinHeapComp());
  }

#if _MSC_VER == 1900
  
  template <class Func, class... Args>
  auto make_coroutine_gen(Func&& f, Args... args)
  {
    using gentype = decltype(f(args...));
    std::vector<gentype> gen_v;
    std::vector<decltype(std::declval<gentype>().begin())> iter_v;

    gen_v.reserve(1);
    iter_v.reserve(2);
    bool first = true;

    return make_gen_from(
      [first, 
       gen_v=std::move(gen_v), 
       iter_v=std::move(iter_v), 
       f=std::move(f), 
       args...]() mutable {
      if (first)
      {
        first = false;
        gen_v.emplace_back(f(args...));
        iter_v.emplace_back(gen_v[0].begin());
        iter_v.emplace_back(gen_v[0].end());
      }
      else
        ++iter_v[0];
      
      if (iter_v[0] == iter_v[1])
        throw std::out_of_range("coroutine completed");
      else
      {
        return *iter_v[0];
      }
    });
  }
  
#endif // __MSC_VER

} // namespace gen

#include "gen_factory.h"
#include "gen_iterator.h"