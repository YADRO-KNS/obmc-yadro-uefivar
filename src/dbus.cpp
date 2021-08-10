// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include "dbus.hpp"

#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

/**
 * @brief Make variable key.
 *
 * @param[in] name Variable name
 * @param[in] guid Vendor GUID
 *
 * @return variable key
 *
 * @throw InvalidArgument in case of errors
 */
static VariableKey makeKey(const std::string& name,
                           const std::vector<uint8_t>& guid)
{
    if (guid.size() != sizeof(uuid_t))
    {
        throw InvalidArgument();
    }
    VariableKey key;
    key.name = name;
    std::copy(guid.begin(), guid.end(), key.guid);
    return key;
}

DBus::DBus(sdbusplus::bus::bus& bus, Storage& varStorage) :
    Super(bus, objectPath), storage(varStorage)
{}

std::tuple<uint32_t, std::vector<uint8_t>>
    DBus::getVariable(std::string name, std::vector<uint8_t> guid)
{
    if (storage.empty())
    {
        throw NotAllowed(); // it should be "Unavailable", but we have too old
                            // DBus interfaces in the Vegman repo
    }
    const VariableKey key = makeKey(name, guid);
    auto variable = storage.get(key);
    if (!variable)
    {
        throw ResourceNotFound();
    }
    return std::make_tuple(variable->attributes, variable->data);
}

void DBus::setVariable(std::string name, std::vector<uint8_t> guid,
                       uint32_t attributes, std::vector<uint8_t> data)
{
    const VariableKey key = makeKey(name, guid);
    try
    {
        storage.set(key, VariableValue{attributes, data});
    }
    catch (const std::exception& ex)
    {
        log<level::ERR>("Error processing SetVariable method",
                        entry("EXCEPTION=%s", ex.what()));
        throw sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure();
    }
}

void DBus::removeVariable(std::string name, std::vector<uint8_t> guid)
{
    const VariableKey key = makeKey(name, guid);
    try
    {
        storage.remove(key);
    }
    catch (const std::exception& ex)
    {
        log<level::ERR>("Error processing SetVariable method",
                        entry("EXCEPTION=%s", ex.what()));
        throw sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure();
    }
}

std::tuple<std::string, std::vector<uint8_t>>
    DBus::nextVariable(std::string name, std::vector<uint8_t> guid)
{
    if (storage.empty())
    {
        throw NotAllowed();
    }
    const VariableKey key = makeKey(name, guid);
    auto variable = storage.next(key);
    if (!variable)
    {
        throw ResourceNotFound();
    }
    std::vector<uint8_t> vg(reinterpret_cast<const uint8_t*>(variable->guid),
                            reinterpret_cast<const uint8_t*>(variable->guid) +
                                sizeof(variable->guid));
    return std::make_tuple(variable->name, vg);
}

void DBus::reset()
{
    try
    {
        storage.reset();
    }
    catch (const std::exception& ex)
    {
        log<level::ERR>("Error processing Reset method",
                        entry("EXCEPTION=%s", ex.what()));
        throw sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure();
    }
}

void DBus::updateVars(std::string file)
{
    try
    {
        storage.updateVars(file);
    }
    catch (const std::exception& ex)
    {
        log<level::ERR>("Error processing UpdateVars method",
                        entry("EXCEPTION=%s", ex.what()));
        throw sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure();
    }
}

void DBus::importVars(std::string file)
{
    try
    {
        storage.importVars(file);
    }
    catch (const std::exception& ex)
    {
        log<level::ERR>("Error processing ImportVars method",
                        entry("EXCEPTION=%s", ex.what()));
        throw sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure();
    }
}
