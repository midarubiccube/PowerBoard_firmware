#pragma once

#include <cstdint>

enum class DataType : uint8_t {
    COMMON_COMAND = 0x01,
    POWERBOARD_COMANND = 0x02,
    BLCD_COMANND = 0x03,
    MOTORBOARDC_COMAND = 0x04,
    // Add other data types as needed
};


union ID {
    uint16_t id;
    struct {
        uint16_t board_num : 4;
        DataType data_type : 4;
        uint16_t priority : 3;
    } fields;
};


#pragma pack(push, 1)
struct PWRPacket {
    bool pwrstatus;
    uint8_t ledstatus;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct PWRXPacket {
    float current;
    float battery1_voltage;
    float battery2_voltage;
    float output_voltage;
};
#pragma pack(pop)