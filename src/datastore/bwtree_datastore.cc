// Copyright (c) 2017, Los Alamos National Security, LLC.
// All rights reserved.
#include "bwtree_datastore.h"
#include "kv-config.h"
#include <chrono>
#include <iostream>
#include <boost/filesystem.hpp>

using namespace std::chrono;

BwTreeDataStore::BwTreeDataStore() :
  AbstractDataStore(Duplicates::IGNORE, false, false) {
  _tree = NULL;
};

BwTreeDataStore::BwTreeDataStore(Duplicates duplicates, bool eraseOnGet, bool debug) :
  AbstractDataStore(duplicates, eraseOnGet, debug) {
  _tree = NULL;
};

BwTreeDataStore::~BwTreeDataStore() {
  // deleting BwTree can cause core dump
#if 0 // letting leak, for now
    delete _tree;
#endif
};

void BwTreeDataStore::createDatabase(std::string db_name) {
  _tree = new BwTree<ds_bulk_t, ds_bulk_t, 
		     ds_bulk_less, ds_bulk_equal, ds_bulk_hash,
		     ds_bulk_equal, ds_bulk_hash>();
  if (_debug) {
    _tree->SetDebugLogging(1);
  }
  else {
    _tree->SetDebugLogging(0);
  }
  _tree->UpdateThreadLocal(1);
  _tree->AssignGCID(0);
};

bool BwTreeDataStore::put(const ds_bulk_t &key, const ds_bulk_t &data) {
  std::vector<ds_bulk_t> values;
  bool success = false;

  if(!_tree) return false;
  
  if (_duplicates == Duplicates::ALLOW) {
    success = _tree->Insert(key, data);
  }
  else if (_duplicates == Duplicates::IGNORE) {
    _tree->GetValue(key, values);
    bool duplicate_key = (values.size() != 0);
    if (duplicate_key) {
      // silently ignore
      success = true;
    }
    else {
      success = _tree->Insert(key, data);
    }
  }
  else {
    std::cerr << "BwTreeDataStore::put: Unexpected Duplicates option = " << int32_t(_duplicates) << std::endl;
  }
  
  return success;
};

bool BwTreeDataStore::get(const ds_bulk_t &key, ds_bulk_t &data) {
  std::vector<ds_bulk_t> values;
  bool success = false;

  _tree->GetValue(key, values);
  if (values.size() == 1) {
    data = std::move(values.front());
    success = true;
  }
  else if (values.size() > 1) {
    // this should only happen if duplicates are allowed
    if (_duplicates == Duplicates::ALLOW) {
      data = std::move(values.front()); // caller is asking for just 1
      success = true;
    }
  }

  if (success && _eraseOnGet) {
    bool status = _tree->Delete(key, data);
    if (!status) {
      success = false;
      std::cerr << "BwTreeDataStore::get: BwTree error on delete (eraseOnGet) = " << status << std::endl;
    }
  }

  return success;
};

bool BwTreeDataStore::get(const ds_bulk_t &key, std::vector<ds_bulk_t> &data) {
  std::vector<ds_bulk_t> values;
  bool success = false;

  _tree->GetValue(key, values);
  if (values.size() > 1) {
    // this should only happen if duplicates are allowed
    if (_duplicates == Duplicates::ALLOW) {
      data = std::move(values);
      success = true;
    }
  }
  else {
    data = std::move(values);
    success = true;
  }
  
  return success;
};

void BwTreeDataStore::BwTreeDataStore::set_in_memory(bool enable)
{};

std::vector<ds_bulk_t> BwTreeDataStore::BwTreeDataStore::list(const ds_bulk_t &start, size_t count)
{
    std::vector<ds_bulk_t> keys;
    auto it = _tree->Begin(start);
    while (it.IsEnd() == false) {
	/* BUG: bwtree doesn't support "list keys" or "get a key" */
	//keys.push_back(it.GetLeafNode());
    }

    return keys;
}