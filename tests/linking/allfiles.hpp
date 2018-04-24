#include "abstract_mutex.hpp"
#include "assert.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/fnv.hpp"
#include "crypto/hash.hpp"
#include "crypto/prover.hpp"
#include "crypto/sha256.hpp"
#include "crypto/stream_hasher.hpp"
#include "crypto/verifier.hpp"
#include "logger.hpp"
#include "memory/array.hpp"
#include "memory/shared_array.hpp"
#include "mutex.hpp"
#include "protocols.hpp"
#include "serializer/byte_array_buffer.hpp"
#include "serializer/counter.hpp"
#include "serializer/exception.hpp"
#include "serializer/referenced_byte_array.hpp"
#include "serializer/serializable_exception.hpp"
#include "serializer/stl_types.hpp"
#include "serializer/type_register.hpp"
#include "serializer/typed_byte_array_buffer.hpp"

#include "chain/block.hpp"
#include "chain/block_generator.hpp"
#include "chain/consensus/proof_of_work.hpp"
#include "chain/transaction.hpp"
#include "commandline/parameter_parser.hpp"
#include "commandline/vt100.hpp"
#include "image/image.hpp"
#include "image/load_png.hpp"
#include "math/bignumber.hpp"
#include "math/exp.hpp"
#include "math/linalg/matrix.hpp"
#include "math/log.hpp"
#include "math/spline/linear.hpp"
#include "memory/rectangular_array.hpp"
#include "meta/log2.hpp"
#include "network/message.hpp"
#include "network/tcp/abstract_connection.hpp"
#include "network/tcp/abstract_server.hpp"
#include "network/tcp/client_connection.hpp"
#include "network/tcp/client_manager.hpp"
#include "network/tcp_client.hpp"
#include "network/tcp_server.hpp"
#include "network/thread_manager.hpp"
#include "script/variant.hpp"
#include "storage/versioned_random_access_stack.hpp"
//#include "storage/file_object.hpp"
#include "storage/random_access_stack.hpp"
#include "storage/variant_stack.hpp"
//#include "storage/indexed_document_store.hpp"
#include "byte_array/basic_byte_array.hpp"
#include "byte_array/const_byte_array.hpp"
#include "byte_array/consumers.hpp"
#include "byte_array/decoders.hpp"
#include "byte_array/details/encode_decode.hpp"
#include "byte_array/encoders.hpp"
#include "byte_array/referenced_byte_array.hpp"
#include "byte_array/tokenizer/token.hpp"
#include "byte_array/tokenizer/tokenizer.hpp"
#include "json/document.hpp"
#include "json/exceptions.hpp"

#include "http/abstract_connection.hpp"
#include "http/abstract_server.hpp"
#include "http/connection.hpp"
#include "http/header.hpp"
#include "http/http_connection_manager.hpp"
#include "http/key_value_set.hpp"
#include "http/method.hpp"
#include "http/middleware.hpp"
#include "http/middleware/allow_origin.hpp"
#include "http/middleware/color_log.hpp"
#include "http/mime_types.hpp"
#include "http/module.hpp"
#include "http/query.hpp"
#include "http/request.hpp"
#include "http/response.hpp"
#include "http/route.hpp"
#include "http/server.hpp"
#include "http/session.hpp"
#include "http/status.hpp"
#include "http/view_parameters.hpp"

#include "containers/vector.hpp"
#include "string/trim.hpp"

#include "service/abstract_callable.hpp"
#include "service/abstract_publication_feed.hpp"
#include "service/callable_class_member.hpp"
#include "service/client.hpp"
#include "service/client_interface.hpp"
#include "service/error_codes.hpp"
#include "service/feed_subscription_manager.hpp"
#include "service/function.hpp"
#include "service/message_types.hpp"
#include "service/promise.hpp"
#include "service/protocol.hpp"
#include "service/publication_feed.hpp"
#include "service/server.hpp"
#include "service/server_interface.hpp"
#include "service/types.hpp"

#include "random/bitgenerator.hpp"
#include "random/bitmask.hpp"
#include "random/lcg.hpp"
#include "random/lfg.hpp"

#include "protocols/chain_keeper.hpp"
#include "protocols/chain_keeper/chain_manager.hpp"
#include "protocols/chain_keeper/commands.hpp"
#include "protocols/chain_keeper/controller.hpp"
#include "protocols/chain_keeper/protocol.hpp"
#include "protocols/chain_keeper/transaction_manager.hpp"
#include "protocols/fetch_protocols.hpp"
#include "protocols/swarm.hpp"
#include "protocols/swarm/commands.hpp"
#include "protocols/swarm/controller.hpp"
#include "protocols/swarm/entry_point.hpp"
#include "protocols/swarm/node_details.hpp"
#include "protocols/swarm/protocol.hpp"
#include "protocols/swarm/serializers.hpp"

#include "optimisation/abstract_spinglass_solver.hpp"
#include "optimisation/brute_force.hpp"
#include "optimisation/instance/binary_problem.hpp"
#include "optimisation/instance/load_txt.hpp"
#include "optimisation/simulated_annealing/reference_annealer.hpp"
#include "optimisation/simulated_annealing/sparse_annealer.hpp"
