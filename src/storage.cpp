// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include "nvram.hpp"
#include "storage.hpp"

#include <phosphor-logging/log.hpp>

#include <exception>

using namespace phosphor::logging;

// Special variable which contains default values for UEFI settings
static const VariableKey stdDefaults{"StdDefaults",
                                     {0x45, 0x99, 0xD2, 0x6F, 0x1A, 0x11, 0x49,
                                      0xB8, 0xB9, 0x1F, 0x85, 0x87, 0x45, 0xCF,
                                      0xF8, 0x24}};

Storage::Storage(const std::filesystem::path& varFile) : file(varFile)
{
    if (!std::filesystem::exists(file))
    {
        log<level::WARNING>("UEFI storage is empty",
                            entry("FILE=%s", file.c_str()));
    }
    else
    {
        variables = loadVariables(file);
        log<level::INFO>("UEFI settings loaded", entry("FILE=%s", file.c_str()),
                         entry("VARS=%u", variables.size()));
    }
}

bool Storage::empty() const
{
    return variables.empty();
}

std::optional<VariableValue> Storage::get(const VariableKey& key)
{
    auto it = variables.find(key);
    return it == variables.end() ? std::nullopt
                                 : std::optional<VariableValue>(it->second);
}

void Storage::set(const VariableKey& key, const VariableValue& value)
{
    const char* action = nullptr;
    auto existing = variables.find(key);
    if (existing == variables.end())
    {
        variables[key] = value;
        action = "Create";
    }
    else if (existing->second.attributes != value.attributes ||
             existing->second.data != value.data)
    {
        existing->second = value;
        action = "Change";
    }

    if (action)
    {
        saveVariables(variables, file);

        // Create audit record in log
        char uuid[UUID_STR_LEN];
        uuid_unparse_upper(key.guid, uuid);

        std::string msg = "AUDIT: ";
        msg += action;
        msg += " UEFI setting ";
        msg += key.name;
        log<level::INFO>(msg.c_str(), entry("NAME=%s", key.name.c_str()),
                         entry("GUID=%s", uuid));
    }
}

void Storage::remove(const VariableKey& key)
{
    auto existing = variables.find(key);
    if (existing != variables.end())
    {
        variables.erase(key);
        saveVariables(variables, file);

        // Create audit record in log
        char uuid[UUID_STR_LEN];
        uuid_unparse_upper(key.guid, uuid);
        std::string msg = "AUDIT: Remove UEFI setting ";
        msg += key.name;
        log<level::INFO>(msg.c_str(), entry("NAME=%s", key.name.c_str()),
                         entry("GUID=%s", uuid));
    }
}

std::optional<VariableKey> Storage::next(const VariableKey& key)
{
    if (key.name.empty())
    {
        // Request for the first variable
        return variables.empty()
                   ? std::nullopt
                   : std::optional<VariableKey>(variables.begin()->first);
    }

    auto existing = variables.find(key);
    return (existing != variables.end() && ++existing != variables.end())
               ? std::optional<VariableKey>(existing->first)
               : std::nullopt;
}

void Storage::reset()
{
    variables.clear();
    saveVariables(variables, file);
    log<level::INFO>("AUDIT: Reset UEFI settings");
}

void Storage::updateVars(const std::filesystem::path& newNvram)
{
    // get default variables to determine their new sizes
    const Variables newVars = nvram::parseVolume(newNvram);
    auto itDefaults = newVars.find(stdDefaults);
    if (itDefaults == newVars.end() || itDefaults->second.data.empty())
    {
        throw std::runtime_error("StdDefaults not found");
    }
    const Variables defVars = nvram::parseNvram(
        &itDefaults->second.data.front(), itDefaults->second.data.size());

    for (auto const& defVar : defVars)
    {
        auto existing = variables.find(defVar.first);
        if (existing != variables.end())
        {
            const VariableValue& newVar = defVar.second;
            VariableValue& oldVar = existing->second;
            oldVar.attributes = newVar.attributes;
            const size_t newDataSize = newVar.data.size();
            const size_t oldDataSize = oldVar.data.size();
            if (newDataSize > oldDataSize)
            {
                oldVar.data.insert(oldVar.data.end(),
                                   newVar.data.begin() + oldDataSize,
                                   newVar.data.end());
            }
            else if (newDataSize < oldDataSize)
            {
                oldVar.data.resize(newDataSize);
            }
        }
    }

    saveVariables(variables, file);

    log<level::INFO>("AUDIT: Update UEFI settings");
}

void Storage::importVars(const std::filesystem::path& oldNvram)
{
    const Variables oldVars = nvram::parseVolume(oldNvram);

    // unpack and put default variables
    auto itDefaults = oldVars.find(stdDefaults);
    if (itDefaults == oldVars.end() || itDefaults->second.data.empty())
    {
        throw std::runtime_error("StdDefaults not found");
    }
    variables = nvram::parseNvram(&itDefaults->second.data.front(),
                                  itDefaults->second.data.size());
    // put old variables
    for (const auto& var : oldVars)
    {
        if (var.first.name != stdDefaults.name)
        {
            variables[var.first] = var.second;
        }
    }

    saveVariables(variables, file);

    log<level::INFO>("AUDIT: Import UEFI settings");
}
