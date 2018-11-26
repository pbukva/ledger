#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "ledger/chain/consensus/consensus_miner_interface.hpp"

#include <random>

namespace fetch {
namespace chain {
namespace consensus {

class BadMiner : public ConsensusMinerInterface
{

public:
  BadMiner()  = default;
  ~BadMiner() = default;

  // Blocking mine
  void Mine(BlockType &block) override
  {
    block.body().nonce = 0;
    block.UpdateDigest();
  }

  // Mine for set number of iterations
  bool Mine(BlockType &block, uint64_t iterations) override
  {
    block.body().nonce = 0;
    block.UpdateDigest();
    return true;
  }
};
}  // namespace consensus
}  // namespace chain
}  // namespace fetch