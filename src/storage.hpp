// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include "variable.hpp"

#include <optional>

/**
 * @brief Storage for UEFI variables.
 */
class Storage final
{
  public:
    /** @brief Default path for UEFI storage file. */
    static constexpr const char* defaultFile = "/var/lib/uefivar.json";

    /**
     * @brief Constructor.
     *
     * @param[in] varFile Path to the variables storage file
     *
     * @throw std::runtime_error in case of errors
     */
    Storage(const std::filesystem::path& varFile);

    /**
     * @brief Check if storage is empty.
     *
     * @return true if storage is empty
     */
    bool empty() const;

    /**
     * @brief Get UEFI variable.
     *
     * @param[in] key Variable key
     *
     * @return variable value or nullopt if not found
     */
    std::optional<VariableValue> get(const VariableKey& key);

    /**
     * @brief Set UEFI variable.
     *
     * @param[in] key Variable key
     * @param[in] value Variable value
     *
     * @throw std::runtime_error in case of errors
     */
    void set(const VariableKey& key, const VariableValue& value);

    /**
     * @brief Remove UEFI variable.
     *
     * @param[in] key Variable key
     *
     * @throw std::runtime_error in case of errors
     */
    void remove(const VariableKey& key);

    /**
     * @brief Get next UEFI variable.
     *
     * @param[in] key The last variable key that was returned by next()
     *
     * @return next variable id or nullopt if not found
     */
    std::optional<VariableKey> next(const VariableKey& key);

    /**
     * @brief Reset UEFI setting by removing existing variables.
     *
     * @throw std::exception in case of errors
     */
    void reset();

    /**
     * @brief Merge UEFI setting to be consistent with the new variable format.
     *
     * @param[in] newNvram path to the file with dump of new version of NVRAM
     *
     * @throw std::exception in case of errors
     */
    void updateVars(const std::filesystem::path& newNvram);

    /**
     * @brief Import variables from existing NVRAM dump.
     *
     * @param[in] oldNvram path to the file with dump of old version of NVRAM
     *
     * @throw std::exception in case of errors
     */
    void importVars(const std::filesystem::path& oldNvram);

  private:
    /** @brief Container for variables. */
    Variables variables;
    /** @brief File used as persistent storage. */
    std::filesystem::path file;
};
