// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include "variable.hpp"

/**
 * @brief Parsers of the non-volatile partition on BIOS flash.
 */
namespace nvram
{

/**
 * @brief Parse dump of firmware volume with NV variables.
 *
 * @param[in] file Path to the file to parse
 *
 * @return array of UEFI variables
 *
 * @throw std::system_error in case of file IO errors
 * @throw std::runtime_error in case of format errors
 */
Variables parseVolume(const std::filesystem::path& file);

/**
 * @brief Parse NVRAM dump.
 *
 * @param[in] data Pointer to the buffer to parse
 * @param[in] size Size of the buffer in bytes
 *
 * @return array of UEFI variables
 *
 * @throw std::runtime_error in case of format errors
 */
Variables parseNvram(const uint8_t* data, size_t size);

} // namespace nvram
