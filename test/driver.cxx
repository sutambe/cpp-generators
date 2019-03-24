#include <iostream>
#include <fstream>
#include <boost/core/demangle.hpp>

#if _MSC_VER == 1900

#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

//#include <experimental/generator>
#include <msft_gen_mod.h>

#endif

#include "generators/generator.h"
#include "generators/type_generator.h"

#ifndef RANDOM_SEED
  #define RANDOM_SEED 0xAC0
#endif 

// Clang requires forward declarations for overloaded < operators.
// g++5 does not. Who's correct?

template <class... Args>
std::ostream & operator << (std::ostream & o, const std::tuple<Args...> & tuple);

template <class T>
std::ostream & operator << (std::ostream & o, const std::vector<T> & vector);

template <class T>
std::ostream & operator << (std::ostream & o, const boost::optional<T> & opt);

template <class T, class U>
std::ostream & operator << (std::ostream & o, const std::pair<T, U> & pair);

template <class T, size_t N>
std::ostream & operator << (std::ostream & o, const std::array<T, N> & arr)
{
  for (auto & elem : arr)
    o << elem;

  return o << "\n";
}

template <class T>
std::ostream & operator << (std::ostream & o, const std::vector<T> & vector)
{
  for (const auto & elem : vector)
    o << elem << " ";

  return o;
}

template <class T>
std::ostream & operator << (std::ostream & o, const boost::optional<T> & opt)
{
  if (opt)
    o << opt.get();

  return o;
}

template <class T, class U>
std::ostream & operator << (std::ostream & o, const std::pair<T, U> & pair)
{
  o << "pair.first = " << pair.first << "\n"
    << "pair.second = " << pair.second;

  return o;
}

template <class Tuple, size_t Size>
struct TuplePrinter
{
  static void print(std::ostream & o, const Tuple & tuple)
  {
    TuplePrinter<Tuple, Size-1>::print(o, tuple);
    o << std::get<Size-1>(tuple) << " ";
  }
};

template <class Tuple>
struct TuplePrinter<Tuple, 1>
{
  static void print(std::ostream & o, const Tuple & tuple)
  {
    o << std::get<0>(tuple) << " ";
  }
};

template <class Tuple>
struct TuplePrinter<Tuple, 0>
{
  static void print(std::ostream &, const Tuple &)
  {
    // no-op
  }
};

template <class... Args>
std::ostream & operator << (std::ostream & o, const std::tuple<Args...> & tuple)
{
  TuplePrinter<std::tuple<Args...>, sizeof...(Args)>::print(o, tuple);
  return o;
}

struct ShapeType
{
  int x, y, shapesize;
  std::string color;
};

std::ostream & operator << (std::ostream & o, const ShapeType & shape)
{
  o << "shape.x = "         << shape.x << "\n"
    << "shape.y = "         << shape.y << "\n"
    << "shape.shapesize = " << shape.shapesize << "\n"
    << "shape.color = "     << shape.color << "\n";

  return o;
}

auto test_shape_gen()
{
  auto xgen = gen::make_range_gen(0, 200);
  auto ygen = gen::make_range_gen(0, 200);
  auto sizegen = gen::make_constant_gen(30);
  auto colorgen = gen::make_oneof_gen({ "RED", "GREEN", "BLUE" });

  auto shapegen =
    gen::make_zip_gen(
      [](int x, int y, int size, const char * color) {
          return ShapeType { x, y, size, color };
      }, xgen, ygen, sizegen, colorgen);

  std::cout << shapegen.generate() << "\n";

  return shapegen;
}

void test_generators(void)
{
  gen::initialize();

  auto strgen =
    gen::make_string_gen(gen::make_printable_gen());

  std::cout << "size of strgen = " << sizeof(strgen) << "\n"
            << "string = " << strgen.generate() << "\n";

  auto vecgen =
    //gen::make_seq_gen<std::vector>(gen::GenFactory<int>::make(), 5, true);
    gen::GenFactory<std::vector<int>>::make(gen::GenFactory<int>::make(), 5, true);

  std::cout << "vector = " << vecgen.generate() << "\n";

  auto optgen = gen::make_optional_gen(strgen);
  std::cout << "optional string = " << optgen.generate() << "\n";

  auto pairgen = gen::make_pair_gen(strgen, vecgen);
  std::cout << pairgen.generate() << "\n";

  auto tuplegen = gen::make_composed_gen(strgen, vecgen);
  std::cout << tuplegen.generate() << "\n";

  auto shapegen = test_shape_gen();

  auto arraygen = gen::make_array_gen(shapegen, gen::dim_list<2, 2>());
  std::cout << arraygen.generate() << "\n";

  auto inordergen = 
    gen::make_inorder_gen({ 10, 20 });

  assert(inordergen.generate() == 10);
  assert(inordergen.generate() == 20);

  auto concatgen = 
    gen::make_inorder_gen({ 10, 20 })
       .concat(gen::make_inorder_gen({ 30 }));

  assert(concatgen.generate() == 10);
  assert(concatgen.generate() == 20);
  assert(concatgen.generate() == 30);

  auto v1 = gen::make_stepper_gen().take(5).to_vector();
  std::vector<int> v2 { 0, 1, 2, 3, 4 };
  assert(v1 == v2);

  std::cout << "All assertions satisfied\n";
}

void triangle()
{
  auto triangle_gen = 
    gen::make_inorder_gen({1,2,3,4,5,6,7,8,9,10,9,8,7,6,5,4,3,2,1})
        .concat_map([](int i) {
              std::cout << "\n";
              return gen::make_stepper_gen(1, i);
            });

  try {
    while(true)
      std::cout << triangle_gen.generate();
  }
  catch(std::out_of_range &) { 
  }
  std::cout << "\n";
}

template<class Gen1, class Gen2>
void is_same(Gen1 g1, Gen2 g2)
{
  assert(g1.to_vector() == g2.to_vector());
}

void monad_laws()
{
  auto f = [](int i) { return gen::make_stepper_gen(1, i); };
  auto g = [](int i) { return gen::make_stepper_gen(i, 1, -1); };
  auto M = gen::make_single_gen(300);

  // left identity
  is_same(M.concat_map(f), f(300));
  
  // right identity
  is_same(M.concat_map([](int i) { 
            return gen::make_single_gen(i); 
          }), M);
  
  // associativity
  is_same(M.concat_map(f).concat_map(g), 
          M.concat_map([f,g](int i) mutable { 
            return f(i).concat_map(g);
          }));
}

void test_typegen()
{
  std::cout << "\n Randomly generated std::tuple\n";
  typegen::RandomTuple<RANDOM_SEED>::type tuple;
  std::cout << boost::core::demangle(typeid(tuple).name());
}

void test_gen_iterator()
{
  const int SIZE = 6;
  int array [SIZE] = { 1, 2, 3, 4, 5, 6 };
  auto gen = gen::make_inorder_gen(array, array + SIZE);
  int i = 0;
  
  for (auto & g : gen)
  {
    assert(g == array[i++]);
  }
}

void test_priority_n()
{
  auto src1  = gen::make_stepper_gen(1).take(100);
  auto src2  = gen::make_stepper_gen(1).take(100);
  auto lown  = gen::make_lowest_n_gen(src1, 10);
  auto highn = gen::make_highest_n_gen(src2, 10);

  std::cout << "Lowest N: ";
  for (auto & val : lown)
  {
    std::cout << val << " ";
    assert(val >= 1 && val <= 10);
  }
  
  std::cout << std::endl << "Highest N: ";
  for (auto & val : highn)
  {
    std::cout << val << " ";
    assert(val >= 91 && val <= 100);
  }
  std::cout << std::endl;
}

#if _MSC_VER == 1900

std::experimental::generator<char> hello_world()
{
  yield 'H';
  yield 'e';
  yield 'l';
  yield 'l';
  yield 'o';
  yield ',';
  yield ' ';
  yield 'w';
  yield 'o';
  yield 'r';
  yield 'l';
  yield 'd';
}

void test_hello_world()
{
  for (char ch : hello_world())
    std::cout << ch << std::endl;

  std::cout << std::endl;
}

class MyFileStream : public std::ifstream
{
public:

  MyFileStream(const std::string & filename)
  : std::ifstream(filename)
  {}
  
  ~MyFileStream() {
    std::cout << "\nclosing filestream";
  }
};

MyFileStream & operator >> (MyFileStream & m, std::string & str)
{
  static_cast<std::ifstream &>(m) >> str;
  return m;
}

std::experimental::generator<std::string> read_file(const std::string & filename)
{
  MyFileStream in(filename);
  std::string str;
  while (in >> str)
    yield str;
}

void test_read_file(const std::string filename)
{
  {
    auto & generator = read_file(filename);
    for (auto & str : generator)
    {
      std::cout << str << " ";
    }
    std::cout << "\n";
  }
  std::cout << "finished test_read_file" << std::endl;
}

std::experimental::generator<char> msg(std::string s)
{
  for (auto ch : s)
    yield ch;
}

void test_multiple()
{
  auto a = msg("Hello");
  auto b = msg("World");

  auto i1 = begin(a);
  auto i2 = begin(b);

  while (i1 != end(a) && i2 != end(b))
  {
    std::cout << *i1 << *i2 << " ";
    ++i1;
    ++i2;
  }
  std::cout << std::endl;
}

std::experimental::generator<char> hello(int add = 0)
{
  for (auto ch : "HELLO, WORLD")
    yield ch + add;
}

void test_coroutine_gen()
{
  std::string msg = "HELLO, WORLD";

  auto gen = gen::make_coroutine_gen(hello, 32)
                .map([](char ch) { return char(ch - 32); })
                .take(msg.size());
  int i = 0;

  for (auto ch: gen)
  {
    std::cout << ch;
    //assert(ch == msg[i++]);
  }
  std::cout << std::endl;
}

std::experimental::generator<int> range(int start, int count)
{
  if (count > 0) {
    yield start;
    for (auto i : range(start + 1, count - 1))
      yield i;
  }
}

void test_recursive_coroutine()
{
  // deliberate leak. Is it the only one?
  new int;

  for (int i : range(5, 4)) // 4000 crash
    std::cout << i << " ";

  std::cout << std::endl;
}

void test_generator_iterator_category()
{
  using Gen = decltype(hello());
  using Category = Gen::iterator::iterator_category;
  const bool test = std::is_same<std::input_iterator_tag, Category>::value;

  static_assert(test, "");
}

#endif // _MSC_VER

int main(void)
{
  try {
    test_generators();
    test_typegen();
    monad_laws();
    triangle();
    test_gen_iterator();
    test_priority_n();

#if _MSC_VER == 1900
    //test_read_file("README.md");
    //test_hello_world();
    //test_multiple();
    //test_coroutine_gen();
    //test_recursive_coroutine();
    //test_generator_iterator_category();


    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_WNDW);
    _CrtDumpMemoryLeaks();

#endif // _MSC_VER
  }
  catch (std::out_of_range & ex)
  {
    std::cerr << "Caught std::out_of_range exception: " << ex.what() << std::endl;
  }
}
