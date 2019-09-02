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

#include "core/commandline/params.hpp"
#include "ledger/chain/block_db_record.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chain/transaction_layout.hpp"
#include "ledger/chain/transaction_rpc_serializers.hpp"
#include "meta/log2.hpp"

#include "storage/object_store.hpp"

#include <fstream>
#include <regex>

#include <dirent.h>


using namespace fetch;
using namespace fetch::storage;

struct DIRDeleter;

using LaneIdx       = uint64_t;
using BlockStore    = ObjectStore<ledger::BlockDbRecord>;
using TxStore       = ObjectStore<ledger::Transaction>;
using TxStores      = std::unordered_map<LaneIdx, TxStore>;
using BlockStorePtr = std::unique_ptr<BlockStore>;
using DIRPtr        = std::unique_ptr<DIR, DIRDeleter>;

struct DIRDeleter
{
  void operator() (DIR* ptr)
  {
    if (ptr)
    {
      closedir(ptr);
    }
  }
};

ledger::Block::Hash GetHeadHash()
{
  std::fstream  head_store;
  head_store.open("chain.head.db",
                  std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);

  byte_array::ByteArray buffer;

  // determine is the hash has already been stored once
  head_store.seekg(0, std::ios::end);
  auto const file_size = head_store.tellg();

  if (file_size == 32)
  {
    buffer.Resize(32);

    // return to the beginning and overwrite the hash
    head_store.seekg(0);
    head_store.read(reinterpret_cast<char *>(buffer.pointer()),
                     static_cast<std::streamsize>(buffer.size()));
  }

  return std::move(buffer);
}

int main(int argc, char **argv)
{
  std::string dir{};
  TxStores    tx_stores;

  commandline::Params parser{};
  parser.description(
    "Tool for consistency check & analysis of fetch block-chain & transaction storage files.");
  parser.add(dir, "directory", "Directory where data-files are located", std::string{"."});
  parser.Parse(argc, argv);

  DIRPtr dp{opendir(dir.c_str())};
  if (dp)
  {
    std::regex const rex("node_storage_lane([0-9]+)_transaction\\.db");
    std::smatch      match;

    LaneIdx lane_idx_min{std::numeric_limits<LaneIdx>::max()};
    LaneIdx lane_idx_max{0};

    dirent *entry{nullptr};
    while ((entry = readdir(dp.get())))
    {
      std::string const name{entry->d_name};
      if (!std::regex_match(name, match, rex) || match.size() != 2)
      {
        continue;
      }

      std::istringstream reader(match[1].str());
      LaneIdx            idx;
      reader >> idx;

      if (idx > lane_idx_max)
      {
        lane_idx_max = idx;
      }
      else if (idx < lane_idx_min)
      {
        lane_idx_min = idx;
      }

      if (tx_stores.find(idx) != tx_stores.end())
      {
        std::cerr << "ERROR: The \"" << name << "\" file with index '" << match[1].str()
                  << "` has been already inserted before!" << std::endl;
        return -1;
      }
      tx_stores[idx].Load(name, "node_storage_lane" + match[1].str() + "_transaction_index.db",
                          false);

      std::cout << "DEBUG: " << name << ": " << match[1].str() << " -> " << idx << std::endl;
    }

    if (lane_idx_min != 0 || (lane_idx_max + 1 - lane_idx_min) != tx_stores.size())
    {
      std::cerr
        << "ERROR: Files with \"node_storage_lane[0-9]+_transaction(_index)?\\.db\" name format have inconsistent(non-continuous) numbering -> there are missing files for one or more indexes."
        << std::endl;
      return -2;
    }

    if (!meta::IsLog2(tx_stores.size()))
    {
      std::cerr << "ERROR: Inferred number of lanes " << tx_stores.size()
                << " (= number of file indexes) MUST be power of 2." << std::endl;
      return -3;
    }

    std::cout << "INFO: Inferred number of lanes: " << tx_stores.size() << std::endl;
  }


  BlockStore block_store;
  block_store.Load("chain.db", "chain.index.db", false);

  std::cout << "Blocks count: " << block_store.size() << std::endl;

  uint64_t count_of_all_tx_in_db{0};
  for (auto const &tx_lane_store : tx_stores)
  {
    count_of_all_tx_in_db += tx_lane_store.second.size();
    std::cout << "Lane" << tx_lane_store.first << ": Tx Count: " << tx_lane_store.second.size()
              << std::endl;
  }

  auto block_hash{GetHeadHash()};
  ledger::BlockDbRecord block;
  uint64_t reference_block_index{0};
  uint64_t tx_count_in_blockchain{0};
  uint64_t tx_count_missing{0};

  do
  {
    if (!block_store.Get(storage::ResourceID{block_hash}, block))
    {
      std::cerr << "ERROR: Unable to fetch " << block_hash.ToHex() << " block" << std::endl;
      return -4;
    }

    if (block.block.body.hash != block_hash)
    {
      std::cerr << "INCONSISTENCY: Block hash " << block.block.body.hash.ToHex()
                << " does not match the hash " << block_hash.ToHex()
                << " used to fetch block from db."
                << std::endl;
    }

    //if (block.block.body.block_number != reference_block_index)
    //{
    //  std::cerr << "INCONSISTENCY: Block number " << block.block.body.block_number
    //            << " does not match the expected reference block number " << reference_block_index
    //            << std::endl;
    //}

    block_hash = block.block.body.previous_hash;
    ++reference_block_index;

    std::cout << "INFO: Checking block[" << block.block.body.block_number
              << "] " << block.block.body.hash.ToHex()
              << std::endl;
    std::cerr.flush();
    std::cout.flush();

    uint64_t slice_idx{0};
    for (auto const &slice : block.block.body.slices)
    {
      tx_count_in_blockchain += slice.size();

      uint64_t tx_idx_in_slice{0};
      for (auto const &tx_layout : slice)
      {
        ledger::Transaction tx;

        bool res{false};

        try
        {
          res = tx_stores[0].Get(storage::ResourceID{tx_layout.digest()}, tx);
        }
        catch(...)
        {
          std::cerr << "EXCEPTION: Tx fetch from db failed:"
                    << " tx hash = " << tx_layout.digest().ToHex()
                    << std::endl;
          std::cerr.flush();
        }

        if (!res)
        {
          ++tx_count_missing;
          std::cerr << "INCONSISTENCY: Tx fetch from db failed: block[" << block.block.body.block_number
                    << "] " << block.block.body.hash.ToHex()
                    << ", slice = " << slice_idx
                    << ", tx index in slice = " << tx_idx_in_slice
                    << ", tx hash = " << tx_layout.digest().ToHex()
                    << std::endl;
          std::cerr.flush();
        }

        ++tx_idx_in_slice;
      }
      ++slice_idx;
    }
  } while (block_hash != ledger::GENESIS_DIGEST);

  if (tx_count_in_blockchain > count_of_all_tx_in_db)
  {
    std::cerr << "INCONSISTENCY: Less transactions present in db store " << count_of_all_tx_in_db
              << " than transactions required by blockchain " << tx_count_in_blockchain
              << std::endl;
  }

  if (tx_count_missing > 0)
  {
    std::cerr << "INCONSISTENCY: " << tx_count_missing
              << " transactions required by blockchain are missing in tx db store"
              << std::endl;
  }

  std::cout << "Counted blocks: " << reference_block_index << std::endl;
  return EXIT_SUCCESS;
}
