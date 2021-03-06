#ifndef AFINA_EXECUTE_ADD_H
#define AFINA_EXECUTE_ADD_H

#include <cstdint>
#include <string>

#include "InsertCommand.h"

namespace Afina {
namespace Execute {

/**
 * # Adds new key to cache
 * Stores given key/value association if it is not present yet.
 *
 * Command must write result to the output, which could be:
 * - "STORED", to indicate success.
 * - "NOT_STORED" to indicate the data was not stored, but not because of an
 * error. This normally means that the condition for the command wasn't met.
 */
class Add : public InsertCommand {
public:
    void Execute(Storage &storage, const std::string &args, std::string &out) const override;
};

} // namespace Execute
} // namespace Afina

#endif // AFINA_EXECUTE_ADD_H
