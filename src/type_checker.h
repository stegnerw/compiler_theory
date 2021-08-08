#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include <memory>
#include <string>
#include <unordered_map>

#include "token.h"

namespace type_checker {
  bool checkCompatible(std::shared_ptr<Token>, const TypeMark&);
  bool checkCompatible(std::shared_ptr<Token>, const TypeMark&,
      const TypeMark&);
  bool checkCompatible(const TypeMark&, const TypeMark&);
  bool checkArrayIndex(const TypeMark&);
  bool checkArraySize(std::shared_ptr<Token>, const int&);
  bool checkArraySize(std::shared_ptr<Token>, const int&, const int&);
};

#endif // TYPE_CHECKER_H
