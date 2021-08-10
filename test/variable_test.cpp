// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include "variable.hpp"

#include <fstream>

#include <gtest/gtest.h>

namespace fs = std::filesystem;

// clang-format off
#define GUID1 { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 }
#define GUID2 { 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 }
// clang-format on

TEST(VariableKeyTest, Less)
{
    const char* nameL = "Abc";
    const char* nameG = "Def";

    EXPECT_TRUE((VariableKey{nameL, GUID1}) < (VariableKey{nameG, GUID2}));
    EXPECT_FALSE((VariableKey{nameG, GUID2}) < (VariableKey{nameL, GUID1}));
    EXPECT_TRUE((VariableKey{nameL, GUID1}) < (VariableKey{nameL, GUID2}));
    EXPECT_TRUE((VariableKey{nameL, GUID1}) < (VariableKey{nameG, GUID1}));
}

TEST(VariablesTest, LoadSave)
{
    fs::path file = fs::temp_directory_path() / "uefivar.json";
    const char* name = "TestVariable";
    const uint32_t attr = 0x12345678;
    const std::vector<uint8_t> data{0x01, 0x02, 0x03, 0x04};
    const VariableKey key{name,
                          {0x96, 0x46, 0xa1, 0x0c, 0x1f, 0xd0, 0x4b, 0xdc, 0x9a,
                           0x59, 0xab, 0x5b, 0x17, 0x5b, 0x57, 0x9e}};

    Variables variables;
    variables[key] = VariableValue{attr, data};
    saveVariables(variables, file);

    variables = loadVariables(file);
    auto var = variables.find(key);
    ASSERT_NE(var, variables.end());
    EXPECT_EQ(var->second.attributes, attr);
    EXPECT_EQ(var->second.data, data);

    fs::remove(file);
}

TEST(VariablesTest, LoadFull)
{
    Variables variables = loadVariables(TEST_DATA_DIR "/nvram.json");
    EXPECT_EQ(variables.size(), 128);

    const VariableKey key{"NetworkStackVar",
                          {0xd1, 0x40, 0x5d, 0x16, 0x7a, 0xfc, 0x46, 0x95, 0xbb,
                           0x12, 0x41, 0x45, 0x9d, 0x36, 0x95, 0xa2}};
    auto var = variables.find(key);
    ASSERT_NE(var, variables.end());
    EXPECT_EQ(var->second.attributes, 3);
    EXPECT_EQ(var->second.data, std::vector<uint8_t>({0x01, 0x01, 0x00, 0x01,
                                                      0x00, 0x01, 0x00, 0x00}));
}
