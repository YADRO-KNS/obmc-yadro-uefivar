// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include "edk.hpp"
#include "nvram.hpp"

#include <endian.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>
#include <system_error>

namespace nvram
{

/** @brief GUID wrapper. */
class Guid
{
  public:
    /**
     * @brief Constructor. EFI stores GUID in the structure which is not
     *        compatible with uuid_t (flat array).
     *
     * @param[in] guid GUID in EFI format
     */
    Guid(const EFI_GUID& guid)
    {
        *reinterpret_cast<uint32_t*>(&uuid[0]) = htobe32(guid.Data1);
        *reinterpret_cast<uint16_t*>(&uuid[4]) = htobe16(guid.Data2);
        *reinterpret_cast<uint16_t*>(&uuid[6]) = htobe16(guid.Data3);
        memcpy(&uuid[8], &guid.Data4, sizeof(guid.Data4));
    }

    bool operator!=(const Guid& rhs) const
    {
        return uuid_compare(uuid, rhs.uuid) != 0;
    }

    uuid_t uuid;
};

/** @brief Map file to memory. */
class FileMapper
{
  public:
    /** @brief Destructor. */
    ~FileMapper()
    {
        if (data != MAP_FAILED)
        {
            munmap(data, size);
        }
    }

    /**
     * @brief Load file.
     *
     * @param[in] file Path to the file to load
     *
     * @throw std::system_error in case of errors
     */
    void load(const std::filesystem::path& file)
    {
        // open file
        const int fd = open(file.c_str(), O_RDONLY);
        if (fd == -1)
        {
            throw std::system_error(errno, std::generic_category());
        }

        // get file size
        struct stat st;
        if (fstat(fd, &st) == -1)
        {
            const int err = errno;
            close(fd);
            throw std::system_error(err, std::generic_category());
        }
        size = st.st_size;

        // map file to memory
        data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (data == MAP_FAILED)
        {
            const int err = errno;
            close(fd);
            throw std::system_error(err, std::generic_category());
        }
        close(fd);
    }

    void* data = MAP_FAILED;
    size_t size = 0;
};

/** @brief NVRAM parser wrapper. */
class Nvram
{
  public:
    // clang-format off
    static constexpr EFI_GUID volumeGuid =
        { 0xfa4974fc, 0xaf1d, 0x4e5d, { 0xbd, 0xc5, 0xda, 0xcd, 0x6d, 0x27, 0xba, 0xec } };
    static constexpr EFI_GUID ffsGuid =
        { 0xcef5b9a3, 0x476d, 0x497f, { 0x9f, 0xdc, 0xe9, 0x81, 0x43, 0xe0, 0x42, 0x2c } };
    // clang-format on

    static constexpr uint32_t nvarSignature =
        'N' | ('V' << 8) | ('A' << 16) | ('R' << 24);

    static constexpr uint32_t flagRuntime = 0b00000001;
    static constexpr uint32_t flagDataOnly = 0b00001000;
    static constexpr uint32_t flagHwError = 0b00100000;
    static constexpr uint32_t flagAuthWrite = 0b01000000;
    static constexpr uint32_t flagValid = 0b10000000;

    static constexpr uint32_t lastNodeId = 0x00ffffff;

    struct NodeHeader
    {
        uint32_t signature;
        uint16_t size;
        uint8_t next[3];
        uint8_t flags;
    } __attribute__((packed));

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
    Variables parse(const uint8_t* data, size_t size)
    {
        if (size < sizeof(NodeHeader))
        {
            throw std::runtime_error("Not enough data in NVRAM");
        }

        dumpStart = data;
        dumpSize = size;

        Variables variables;

        const NodeHeader* node = reinterpret_cast<const NodeHeader*>(data);
        while (isPtrValid(node, sizeof(*node)) &&
               node->signature == nvarSignature)
        {
            if ((node->flags & flagValid) && !(node->flags & flagDataOnly))
            {
                auto [key, value] = readVariable(node);
                variables.insert(std::make_pair(key, value));
            }
            // move to the next node
            node = reinterpret_cast<const NodeHeader*>(
                reinterpret_cast<const uint8_t*>(node) + node->size);
        }
        return variables;
    }

  private:
    /**
     * @brief Construct variable from node.
     *
     * @param[in] node variable node
     *
     * @return variable key and value
     *
     * @throw std::runtime_error in case of format errors
     */
    std::tuple<VariableKey, VariableValue>
        readVariable(const NodeHeader* node) const
    {
        VariableKey key;
        VariableValue value;

        if (node->size < sizeof(NodeHeader))
        {
            throw std::runtime_error("Invalid header");
        }
        const uint8_t* payloadStart =
            reinterpret_cast<const uint8_t*>(node) + sizeof(NodeHeader);
        const uint8_t* payloadEnd =
            reinterpret_cast<const uint8_t*>(node) + node->size;
        if (!isPtrValid(payloadStart, 1) || !isPtrValid(payloadEnd - 1, 1))
        {
            throw std::runtime_error("Invalid header");
        }

        // vendor guid
        const uint8_t guidIndex = *payloadStart;
        getGuid(guidIndex, key.guid);
        ++payloadStart; // skip GUID index

        // variable name
        const char* name = reinterpret_cast<const char*>(payloadStart);
        while (*payloadStart)
        {
            ++payloadStart;
            if (payloadStart >= payloadEnd)
            {
                throw std::runtime_error("Variable name too long");
            }
        }
        key.name.assign(name, reinterpret_cast<const char*>(payloadStart));
        ++payloadStart; // skip last null

        // variable attributes
        value.attributes = getAttributes(node->flags);

        // value data
        const NodeHeader* dataNode = getLastNode(node);
        if (!dataNode)
        {
            throw std::runtime_error("Data not found");
        }
        if (dataNode != node)
        {
            payloadStart =
                reinterpret_cast<const uint8_t*>(dataNode) + sizeof(NodeHeader);
            payloadEnd =
                reinterpret_cast<const uint8_t*>(dataNode) + dataNode->size;
            if (!isPtrValid(payloadStart, 1) || !isPtrValid(payloadEnd - 1, 1))
            {
                throw std::runtime_error("Data out of range");
            }
        }
        if (payloadStart >= payloadEnd)
        {
            throw std::runtime_error("Value data is empty");
        }
        value.data.assign(payloadStart, payloadEnd);

        return std::make_tuple(key, value);
    }

    /**
     * @brief Get last node from linked list.
     *
     * @param[in] node start node
     *
     * @return last node or nullptr if not found
     */
    const NodeHeader* getLastNode(const NodeHeader* node) const
    {
        uint32_t next =
            *reinterpret_cast<const uint32_t*>(node->next) & 0x00ffffff;

        while (next != lastNodeId)
        {
            if (next == 0)
            {
                return nullptr;
            }

            node = reinterpret_cast<const NodeHeader*>(
                reinterpret_cast<const uint8_t*>(node) + next);

            if (!isPtrValid(node, sizeof(*node)) ||
                node->signature != nvarSignature)
            {
                return nullptr;
            }

            next = *reinterpret_cast<const uint32_t*>(node->next) & 0x00ffffff;
        }

        return node;
    }

    /**
     * @brief Get GUID from index.
     *
     * @param[in] index GUID index
     * @param[out] guid destination buffer
     *
     * @throw std::runtime_error in case of format errors
     */
    void getGuid(uint8_t index, uuid_t guid) const
    {
        const EFI_GUID* zeroGuid = reinterpret_cast<const EFI_GUID*>(
            dumpStart + dumpSize - sizeof(EFI_GUID));
        const EFI_GUID* indexGuid = zeroGuid - index;
        if (!isPtrValid(indexGuid, sizeof(EFI_GUID)))
        {
            throw std::runtime_error("GUID not found");
        }
        Guid id(*indexGuid);
        uuid_copy(guid, id.uuid);
    }

    /**
     * @brief Get variable attributes from node flags.
     *
     * @param[in] flags variable flags
     *
     * @return variable attributes
     */
    uint32_t getAttributes(uint32_t flags) const
    {
        uint32_t attr =
            EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS;

        if (flags & flagRuntime)
            attr |= EFI_VARIABLE_RUNTIME_ACCESS;
        if (flags & flagHwError)
            attr |= EFI_VARIABLE_HARDWARE_ERROR_RECORD;
        if (flags & flagAuthWrite)
            attr |= EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS;

        return attr;
    }

    /**
     * @brief Validate pointer.
     *
     * @param[in] ptr pointer to check
     * @param[in] size size of data
     *
     * @return true if data is in proper range
     */
    bool isPtrValid(const void* ptr, size_t size = sizeof(void*)) const
    {
        const uint8_t* addr = reinterpret_cast<const uint8_t*>(ptr);
        return addr >= dumpStart && addr + size <= dumpStart + dumpSize;
    }

  private:
    const uint8_t* dumpStart;
    size_t dumpSize;
};

Variables parseVolume(const std::filesystem::path& file)
{
    FileMapper fileMap;
    fileMap.load(file);
    const uint8_t* data = reinterpret_cast<const uint8_t*>(fileMap.data);

    // Unpack volume

    if (fileMap.size < sizeof(EFI_FIRMWARE_VOLUME_HEADER))
        throw std::runtime_error("Invalid volume header");

    const EFI_FIRMWARE_VOLUME_HEADER* volHdr =
        reinterpret_cast<const EFI_FIRMWARE_VOLUME_HEADER*>(data);

    if (Guid(volHdr->FileSystemGuid) != Guid(EFI_FIRMWARE_FILE_SYSTEM2_GUID))
        throw std::runtime_error("Unsupported firmware file system");
    if (!volHdr->ExtHeaderOffset)
        throw std::runtime_error("Extended header not found");
    if (fileMap.size <
        volHdr->ExtHeaderOffset + sizeof(EFI_FIRMWARE_VOLUME_EXT_HEADER))
        throw std::runtime_error("Invalid extended header");

    const EFI_FIRMWARE_VOLUME_EXT_HEADER* volExtHdr =
        reinterpret_cast<const EFI_FIRMWARE_VOLUME_EXT_HEADER*>(
            reinterpret_cast<const uint8_t*>(volHdr) + volHdr->ExtHeaderOffset);

    if (Guid(volExtHdr->FvName) != Guid(Nvram::volumeGuid))
        throw std::runtime_error("Unsupported volume");

    // Unpack file

    // FFS file header is 8-byte aligned
    const uint8_t* ffsStart =
        reinterpret_cast<const uint8_t*>(volExtHdr) + volExtHdr->ExtHeaderSize;
    ffsStart += reinterpret_cast<size_t>(ffsStart) % 8;

    if (fileMap.size < ffsStart - data + sizeof(EFI_FFS_FILE_HEADER))
        throw std::runtime_error("FFS file header not found");

    const EFI_FFS_FILE_HEADER* ffsHdr =
        reinterpret_cast<const EFI_FFS_FILE_HEADER*>(ffsStart);

    if (Guid(ffsHdr->Name) != Guid(Nvram::ffsGuid))
        throw std::runtime_error("Unsupported NVRAM file system");

    // Unpack NVRAM data

    const uint8_t* nvramStart =
        reinterpret_cast<const uint8_t*>(ffsHdr) + sizeof(EFI_FFS_FILE_HEADER);
    const size_t nvramSize =
        (*reinterpret_cast<const uint32_t*>(ffsHdr->Size) & 0x00ffffff) -
        sizeof(EFI_FFS_FILE_HEADER);
    if (fileMap.size < nvramStart - data + nvramSize)
        throw std::runtime_error("Unexpected end of NVRAM file");

    return parseNvram(nvramStart, nvramSize);
}

Variables parseNvram(const uint8_t* data, size_t size)
{
    return Nvram().parse(data, size);
}

} // namespace nvram
