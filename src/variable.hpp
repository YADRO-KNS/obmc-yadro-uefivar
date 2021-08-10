// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <uuid.h>

#include <filesystem>
#include <map>
#include <string>
#include <vector>

/**
 * @brief Unique variable key.
 */
struct VariableKey
{
    std::string name; ///< Variable name
    uuid_t guid;      ///< Vendor GUID

    /* Comparator for using as a key in map. */
    bool operator<(const VariableKey& rhs) const;
};

/**
 * @brief Value of UEFI variable.
 */
struct VariableValue
{
    uint32_t attributes;       ///< UEFI attributes
    std::vector<uint8_t> data; ///< Raw data
};

/**
 * @brief UEFI variables container.
 */
using Variables = std::map<VariableKey, VariableValue>;

/**
 * @brief Load variables from JSON file.
 *
 * @param[in] file Path to the JSON file to load
 * @return UEFI variables
 *
 * @throw std::runtime_error in case of errors
 */
Variables loadVariables(const std::filesystem::path& jsonFile);

/**
 * @brief Save variables to JSON file.
 *
 * @param[in] variables UEFI variables to save
 * @param[in] file Path to the JSON file to write
 *
 * @throw std::runtime_error in case of errors
 */
void saveVariables(const Variables& variables,
                   const std::filesystem::path& jsonFile);
