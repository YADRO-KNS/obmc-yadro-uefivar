// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include "storage.hpp"

#include <fstream>

#include <gtest/gtest.h>

namespace fs = std::filesystem;

// clang-format off
#define GUID1 { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 }
#define GUID2 { 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 }
// clang-format on

/**
 * @brief UEFI storage tests.
 */
class StorageTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        fs::remove(file);
    }

    void TearDown() override
    {
        fs::remove(file);
    }

    const fs::path file = fs::temp_directory_path() / "uefivar.json";
};

TEST_F(StorageTest, SetAndGet)
{
    Storage storage(file);

    EXPECT_FALSE(storage.get(VariableKey{"TestVariable", GUID1}));

    storage.set(VariableKey{"TestVariable", GUID1},
                VariableValue{42, {1, 2, 3}});
    auto var = storage.get(VariableKey{"TestVariable", GUID1});
    ASSERT_TRUE(var);
    EXPECT_EQ(var->attributes, 42);
    EXPECT_EQ(var->data, (std::vector<uint8_t>{1, 2, 3}));

    EXPECT_FALSE(storage.get(VariableKey{"NotFound", GUID1}));
    EXPECT_FALSE(storage.get(VariableKey{"TestVariable", GUID2}));
}

TEST_F(StorageTest, Remove)
{
    Storage storage(file);
    storage.set(VariableKey{"TestVariable", GUID1},
                VariableValue{42, {1, 2, 3}});

    EXPECT_TRUE(storage.get(VariableKey{"TestVariable", GUID1}));

    storage.remove(VariableKey{"TestVariable", GUID1});
    EXPECT_FALSE(storage.get(VariableKey{"TestVariable", GUID1}));
}

TEST_F(StorageTest, GetNext)
{
    Storage storage(file);

    EXPECT_FALSE(storage.next(VariableKey{}));

    storage.set(VariableKey{"TestVariable1", GUID1}, VariableValue{0, {0}});
    storage.set(VariableKey{"TestVariable2", GUID1}, VariableValue{0, {0}});

    auto var1 = storage.next(VariableKey{});
    ASSERT_TRUE(var1);
    EXPECT_EQ(var1->name, "TestVariable1");

    auto var2 = storage.next(*var1);
    ASSERT_TRUE(var2);
    EXPECT_EQ(var2->name, "TestVariable2");

    EXPECT_FALSE(storage.next(*var2));
}

TEST_F(StorageTest, MergeUpgrade)
{
    const VariableKey netVar{"NetworkStackVar",
                             {0xd1, 0x40, 0x5d, 0x16, 0x7a, 0xfc, 0x46, 0x95,
                              0xbb, 0x12, 0x41, 0x45, 0x9d, 0x36, 0x95, 0xa2}};
    Storage storage(file);
    storage.set(netVar, VariableValue{3, {1, 2, 3}});

    storage.updateVars(TEST_DATA_DIR "/nvram.bin");

    auto var = storage.get(netVar);
    ASSERT_TRUE(var);
    EXPECT_EQ(var->data, std::vector<uint8_t>({1, 2, 3, 1, 0, 1, 1, 1}));
}

TEST_F(StorageTest, MergeDowngrade)
{
    const VariableKey netVar{"NetworkStackVar",
                             {0xd1, 0x40, 0x5d, 0x16, 0x7a, 0xfc, 0x46, 0x95,
                              0xbb, 0x12, 0x41, 0x45, 0x9d, 0x36, 0x95, 0xa2}};
    Storage storage(file);
    storage.set(netVar, VariableValue{3, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}});

    storage.updateVars(TEST_DATA_DIR "/nvram.bin");

    auto var = storage.get(netVar);
    ASSERT_TRUE(var);
    EXPECT_EQ(var->data, std::vector<uint8_t>({1, 2, 3, 4, 5, 6, 7, 8}));
}

TEST_F(StorageTest, ImportVars)
{
    Storage storage(file);

    const VariableKey stdVar{"StdDefaults",
                             {0x45, 0x99, 0xD2, 0x6F, 0x1A, 0x11, 0x49, 0xB8,
                              0xB9, 0x1F, 0x85, 0x87, 0x45, 0xCF, 0xF8, 0x24}};
    const VariableKey netVar{"NetworkStackVar",
                             {0xd1, 0x40, 0x5d, 0x16, 0x7a, 0xfc, 0x46, 0x95,
                              0xbb, 0x12, 0x41, 0x45, 0x9d, 0x36, 0x95, 0xa2}};

    storage.importVars(TEST_DATA_DIR "/nvram.bin");

    EXPECT_FALSE(storage.get(stdVar));
    EXPECT_TRUE(storage.get(netVar));
}

TEST_F(StorageTest, Reset)
{
    Storage storage(file);

    EXPECT_TRUE(storage.empty());
    storage.set(VariableKey{"TestVariable", GUID1}, VariableValue{0, {0}});
    EXPECT_FALSE(storage.empty());
    storage.reset();
    EXPECT_TRUE(storage.empty());
}
