// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include "variable.hpp"

#include <json.h>

#include <cstring>
#include <stdexcept>

// Names of JSON field used to save/load variables
static const char* jsonRootNode = "variables";
static const char* jsonNameNode = "name";
static const char* jsonGuidNode = "guid";
static const char* jsonAttrNode = "attr";
static const char* jsonDataNode = "data";

bool VariableKey::operator<(const VariableKey& rhs) const
{
    const int gc = uuid_compare(guid, rhs.guid);
    return gc < 0 || (gc == 0 && name < rhs.name);
}

/**
 * @brief Convert binary array to hexadecimal string.
 *
 * @param[in] data source data
 *
 * @return hex string
 */
static std::string binToHex(const std::vector<uint8_t>& data)
{
    static const char hexMap[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    std::string result(data.size() * 2, 0);
    const size_t size = data.size();

    for (size_t idx = 0; idx < size; ++idx)
    {
        const uint8_t byte = data.at(idx);
        char* ptr = &result.at(idx * 2);
        *ptr++ = hexMap[byte >> 4];
        *ptr++ = hexMap[byte & 0x0f];
    }

    return result;
}

/**
 * @brief Convert hexadecimal string to binary array.
 *
 * @param[in] hex hex string
 *
 * @return binary array
 *
 * @throw std::invalid_argument input string is invalid
 */
static std::vector<uint8_t> hexToBin(const char* hex)
{
    std::vector<uint8_t> data;

    if (!hex || !*hex)
    {
        throw std::invalid_argument("Invalid hex format: string is empty");
    }

    data.reserve(strlen(hex) / 2);

    while (*hex)
    {
        // Skip spaces and dashes
        while (isspace(*hex) || *hex == '-')
        {
            ++hex;
        }
        if (!*hex)
        {
            break;
        }

        // Convert 2-bytes text hex to numeric
        uint8_t val = 0;
        for (uint8_t hb = 0; hb <= 1; ++hb)
        {
            const uint8_t ch = *hex;
            val <<= 4 * hb;
            if (ch >= '0' && ch <= '9')
                val |= ch - '0';
            else if (ch >= 'a' && ch <= 'f')
                val |= 0x0a + ch - 'a';
            else if (ch >= 'A' && ch <= 'F')
                val |= 0x0a + ch - 'A';
            else
                throw std::invalid_argument(
                    "Invalid hex format: unacceptable character");
            ++hex;
        }
        data.push_back(val);
    }

    return data;
}

Variables loadVariables(const std::filesystem::path& jsonFile)
{
    Variables variables;

    std::unique_ptr<json_object, decltype(&json_object_put)> jobj(
        json_object_from_file(jsonFile.c_str()), json_object_put);
    if (!jobj)
    {
        std::string msg;
        const char* err = json_util_get_last_err();
        if (err)
        {
            msg = err;
        }
        else
        {
            msg = "Unable to load file ";
            msg += jsonFile;
        }
        throw std::runtime_error(msg);
    }

    json_object* jvarlist;
    if (!json_object_object_get_ex(jobj.get(), jsonRootNode, &jvarlist))
    {
        throw std::runtime_error("JSON: root node not found");
    }

    int idx = json_object_array_length(jvarlist);
    while (--idx >= 0)
    {
        struct json_object* jvar = json_object_array_get_idx(jvarlist, idx);
        struct json_object* jname;
        struct json_object* jguid;
        struct json_object* jattr;
        struct json_object* jdata;
        if (!json_object_object_get_ex(jvar, jsonNameNode, &jname) ||
            !json_object_object_get_ex(jvar, jsonGuidNode, &jguid) ||
            !json_object_object_get_ex(jvar, jsonAttrNode, &jattr) ||
            !json_object_object_get_ex(jvar, jsonDataNode, &jdata))
        {
            throw std::runtime_error("JSON: incomplete variable");
        }

        VariableKey key;
        const char* name = json_object_get_string(jname);
        if (!name || !*name)
        {
            throw std::runtime_error("JSON: invalid variable name");
        }
        key.name = name;
        const char* guid = json_object_get_string(jguid);
        if (!guid || uuid_parse(guid, key.guid) != 0)
        {
            throw std::runtime_error("JSON: invalid variable GUID");
        }

        VariableValue value;
        value.attributes = static_cast<uint32_t>(json_object_get_int(jattr));
        if (value.attributes == 0 && errno == EINVAL)
        {
            throw std::runtime_error("JSON: invalid attribute");
        }
        const char* data = json_object_get_string(jdata);
        if (!data || !*data)
        {
            throw std::runtime_error("JSON: invalid data");
        }
        value.data = hexToBin(data);

        variables[key] = value;
    }

    return variables;
}

void saveVariables(const Variables& variables,
                   const std::filesystem::path& jsonFile)
{
    std::unique_ptr<json_object, decltype(&json_object_put)> jobj(
        json_object_new_object(), json_object_put);

    json_object* jvars = json_object_new_array();

    for (auto const& it : variables)
    {
        json_object* jvar = json_object_new_object();

        json_object_object_add(jvar, jsonNameNode,
                               json_object_new_string(it.first.name.c_str()));
        char uuid[UUID_STR_LEN];
        uuid_unparse_upper(it.first.guid, uuid);
        json_object_object_add(jvar, jsonGuidNode,
                               json_object_new_string(uuid));
        json_object_object_add(jvar, jsonAttrNode,
                               json_object_new_int64(it.second.attributes));
        const std::string data = binToHex(it.second.data);
        json_object_object_add(jvar, jsonDataNode,
                               json_object_new_string(data.c_str()));

        json_object_array_add(jvars, jvar);
    }

    json_object_object_add(jobj.get(), jsonRootNode, jvars);

    std::filesystem::create_directories(jsonFile.parent_path());

    if (json_object_to_file_ext(jsonFile.c_str(), jobj.get(),
                                JSON_C_TO_STRING_PRETTY |
                                    JSON_C_TO_STRING_SPACED) == -1)
    {
        std::string msg;
        const char* err = json_util_get_last_err();
        if (err)
        {
            msg = err;
        }
        else
        {
            msg = "Unable to write file ";
            msg += jsonFile;
        }
        throw std::runtime_error(msg);
    }
}
