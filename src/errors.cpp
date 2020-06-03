#include "errors.hpp"

namespace cdb {

DEFINE_RUNTIME_ERROR(config_error);
DEFINE_RUNTIME_ERROR(parse_incomplete_error);
DEFINE_RUNTIME_ERROR(parse_syntax_error);
DEFINE_RUNTIME_ERROR(server_error);
DEFINE_RUNTIME_ERROR(log_error);

}