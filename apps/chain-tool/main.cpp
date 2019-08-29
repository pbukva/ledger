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

Block::Hash GetHeadHash()
{
  std::fstream  head_store;
  head_store.open("chain.head.db",
                  std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);

  byte_array::ByteArray buffer;

  // determine is the hash has already been stored once
  head_store_.seekg(0, std::ios::end);
  auto const file_size = head_store.tellg();

  if (file_size == 32)
  {
    buffer.Resize(32);

    // return to the beginning and overwrite the hash
    head_store.seekg(0);
    head_store.read(reinterpret_cast<char *>(buffer.pointer()),
                     static_cast<std::streamsize>(buffer.size()));
  }

  return buffer;
}

int main(int argc, char **argv)
{
  std::string dir{};
  TxStores    tx_stores;

  // build the parser
  commandline::Params parser{};
  parser.description(
    "Tool for consistency check & analysis of fetch block-chain & transaction storage files.");
  parser.add(dir, "directory", "Directory where data-files are located", std::string{"."});

  // parse the command line
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

  for (auto const &tx_lane_store : tx_stores)
  {
    std::cout << "Lane" << tx_lane_store.first << ": Tx Count: " << tx_lane_store.second.size()
              << std::endl;
  }

  auto const blockchain_head_hash{GetHeadHash()};
  auto       block_hash{blockchain_head_hash};
  ledger::BlockDbRecord block;
  //for (;;)
  //{
    if (!block_store.Get(block_head, block))
    {
      std::cerr << "ERROR: Unable to fetch " << block_hash.ToHex() << " block" << std::endl;
      return -4;
    }
  //};

  return EXIT_SUCCESS;
}

