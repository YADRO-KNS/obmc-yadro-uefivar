// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include "server.hpp"
#include "storage.hpp"

using Super = sdbusplus::server::object_t<
    sdbusplus::com::yadro::server::UefiVar>;

/**
 * @brief Implementation of xyz.openbmc_project.UefiVar interface.
 */
class DBus : public Super
{
  public:
    /** @brief D-Bus interface name. */
    static constexpr const char* interfaceName = "com.yadro.UefiVar";

    /** @brief D-Bus object path. */
    static constexpr const char* objectPath = "/com/yadro/uefivar";

    /**
     * @brief Constructor.
     *
     * @param[in] bus Bus to attach
     * @param[in] varStorage UEFI variable storage
     *
     * @throw std::exception in case of errors
     */
    DBus(sdbusplus::bus::bus& bus, Storage& varStorage);

    // Implementation of DBus methods
    std::tuple<uint32_t, std::vector<uint8_t>>
        getVariable(std::string name, std::vector<uint8_t> guid) override;

    void setVariable(std::string name, std::vector<uint8_t> guid,
                     uint32_t attributes, std::vector<uint8_t> data) override;

    std::tuple<std::string, std::vector<uint8_t>>
        nextVariable(std::string name, std::vector<uint8_t> guid) override;

    void removeVariable(std::string name, std::vector<uint8_t> guid);

    void reset() override;

    void updateVars(std::string file) override;

    void importVars(std::string file) override;

  private:
    /** @brief UEFI variables storage. */
    Storage& storage;
};
