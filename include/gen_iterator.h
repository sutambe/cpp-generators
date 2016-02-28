#pragma once

#include <iterator>

namespace gen {

  template <class T, class Gen>
  class gen_iterator :
    public std::iterator<std::input_iterator_tag, T>
  {
    Gen * gen_;
    bool end_;
    T current_;

  public:

    typedef T  value_type;
    typedef const T & reference;
    typedef const T * pointer;

    gen_iterator(Gen & gen, bool end = false) 
      : gen_(&gen),
        end_(end)
    {
      if(!end_)
        ++(*this);
    }

    reference operator *() const {
      return current_;
    }

    pointer operator ->() const {
      return &current_;
    }

    gen_iterator & operator ++ () {
      if (end_)
        throw std::out_of_range("gen_iterator incremented beyond the end");

      try {
          current_ = gen_->generate();
      }
      catch (std::out_of_range &) {
          end_ = true;
      }
      return *this;
    }

    gen_iterator operator ++ (int) {
      gen_iterator temp(*this);
      ++(*this);
      return temp;
    }

    bool operator == (gen_iterator const & gi) const {
      if ((this->gen_ == gi.gen_) &&
          (this->end_ == gi.end_))
        return true;
      else
        return false;
    }

    bool operator != (gen_iterator const & gi) const {
      return !(*this == gi);
    }
  };

} // namespace gen