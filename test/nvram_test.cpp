// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include "nvram.hpp"

#include <gtest/gtest.h>

TEST(NvramParser, Volume)
{
    const Variables variables = nvram::parseVolume(TEST_DATA_DIR "/nvram.bin");

    EXPECT_EQ(variables.size(), 116);

    const VariableKey key{"NetworkStackVar",
                          {0xd1, 0x40, 0x5d, 0x16, 0x7a, 0xfc, 0x46, 0x95, 0xbb,
                           0x12, 0x41, 0x45, 0x9d, 0x36, 0x95, 0xa2}};
    auto var = variables.find(key);
    ASSERT_NE(var, variables.end());
    EXPECT_EQ(var->second.attributes, 3);
    EXPECT_EQ(var->second.data, std::vector<uint8_t>({0x01, 0x01, 0x00, 0x01,
                                                      0x00, 0x01, 0x00, 0x00}));

    const Variables dataVars = loadVariables(TEST_DATA_DIR "/nvram.json");
    for (auto& it : dataVars)
    {
        auto dit = variables.find(it.first);
        if (dit != variables.end())
        {
            EXPECT_EQ(it.second.data, dit->second.data);
        }
    }
}
