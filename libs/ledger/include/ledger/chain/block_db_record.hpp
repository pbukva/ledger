#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "ledger/chain/block.hpp"
#include "ledger/chain/constants.hpp"

namespace fetch {
namespace ledger {

/**
 * The class represents block structure in permanent chain storage file.
 */
struct BlockDbRecord
{
  Block block;
  // genesis (hopefully) cannot be next hash so is used as undefined value
  Block::Hash next_hash = GENESIS_DIGEST;

  Block::Hash hash() const
  {
    return block.body.hash;
  }
};

/**
 * Serializer for the BlockDbRecord
 *
 * @tparam T The serializer type
 * @param serializer The reference to hte serializer
 * @param dbRecord The reference to the DbRecord to be serialised
 */
template <typename T>
void Serialize(T &serializer, BlockDbRecord const &dbRecord)
{
  serializer << dbRecord.block << dbRecord.next_hash;
}

/**
 * Deserializer for the BlockDbRecord
 *
 * @tparam T The serializer type
 * @param serializer The reference to the serializer
 * @param dbRecord The reference to the output dbRecord to be populated
 */
template <typename T>
void Deserialize(T &serializer, BlockDbRecord &dbRecord)
{
  serializer >> dbRecord.block >> dbRecord.next_hash;
}

}  // namespace ledger
}  // namespace fetch
