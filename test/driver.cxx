#include <iostream>
#include <fstream>
//#include <boost/archive/text_oarchive.hpp>
//#include <boost/archive/text_iarchive.hpp>

#include "generator.h"
#include "type_generator.h"
//#include "serialize_tuple.h"

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
}

int main(void)
{
  test_generators();
  triangle();
}
