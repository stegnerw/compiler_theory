#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include <memory>
#include <string>
#include <unordered_map>

#include "environment.h"
#include "token.h"

class TypeChecker {
public:
	TypeChecker();
	bool areCompatible(std::shared_ptr<Token>, const TypeMark&);
	bool areCompatible(std::shared_ptr<Token>, const TypeMark&,
			const TypeMark&);
	bool areCompatible(const TypeMark&, const TypeMark&);

private:
};

#endif // TYPE_CHECKER_H
