//sportident_constants.h

#ifndef SPORTIDENT_CONSTANTS_H
#define SPORTIDENT_CONSTANTS_H

#include <QByteArray>
#include <cstdint>

namespace SportIdent {
namespace Constants {

// Константы протокола
constexpr uint8_t STX = 0x02;
constexpr uint8_t ETX = 0x03;
constexpr uint8_t ACK = 0x06;
constexpr uint8_t NAK = 0x15;
constexpr uint8_t DLE = 0x10;
constexpr uint8_t WAKEUP = 0xFF;

// Основные команды
constexpr uint8_t C_GET_SI9 = 0xEF;
constexpr uint8_t C_SI9_DET = 0xE8;
constexpr uint8_t C_SI_REM = 0xE7;
constexpr uint8_t C_SET_MS = 0xF0;
constexpr uint8_t C_GET_MS = 0xF1;
constexpr uint8_t C_SET_TIME = 0xF6;
constexpr uint8_t C_GET_TIME = 0xF7;
constexpr uint8_t C_BEEP = 0xF9;
constexpr uint8_t C_OFF = 0xF8;
constexpr uint8_t C_SET_SYS_VAL = 0x82;
constexpr uint8_t C_GET_SYS_VAL = 0x83;

// Параметры
constexpr uint8_t P_MS_DIRECT = 0x4D;    // "M" - мастер режим
constexpr uint8_t P_MS_INDIRECT = 0x53;  // "S" - slave режим

// Смещения в системных данных
constexpr uint8_t O_MODE = 0x71;
constexpr uint8_t O_STATION_CODE = 0x72;
constexpr uint8_t O_PROTO = 0x74;
constexpr uint8_t O_SERIAL_NO = 0x00;
constexpr uint8_t O_FIRMWARE = 0x05;
constexpr uint8_t O_BUILD_DATE = 0x08;
constexpr uint8_t O_MODEL_ID = 0x0B;
constexpr uint8_t O_MEM_SIZE = 0x0D;
constexpr uint8_t O_BAT_DATE = 0x15;
constexpr uint8_t O_BAT_CAP = 0x19;

// Режимы работы станции
constexpr uint8_t M_CONTROL = 0x02;
constexpr uint8_t M_START = 0x03;
constexpr uint8_t M_FINISH = 0x04;
constexpr uint8_t M_READOUT = 0x05;
constexpr uint8_t M_CLEAR = 0x07;
constexpr uint8_t M_CHECK = 0x0A;

// Константы для CRC
constexpr uint16_t CRC_POLYNOM = 0x8005;
constexpr uint16_t CRC_BITF = 0x8000;

// Максимальные размеры
constexpr int MAX_BLOCK_SIZE = 128;
constexpr int BLOCKS_SI9 = 2;
constexpr int BLOCKS_SI10 = 8;
constexpr int PUNCHES_PER_BLOCK_SI10 = 32;

// Структура данных карт
namespace CardOffsets {
    // SI9/SI10 структура (основные смещения)
    struct CardStructure {
        int cn2 = 25;
        int cn1 = 26;
        int cn0 = 27;
        int std = 12;    // Start day
        int st = 14;     // Start time
        int ftd = 16;    // Finish day
        int ft = 18;     // Finish time
        int ctd = 8;     // Check day
        int ct = 10;     // Check time
        int ltd = -1;    // Clear day (нет для SI9/10)
        int lt = -1;     // Clear time (нет для SI9/10)
        int rc = 22;     // Punch counter
        int p1 = 56;     // First punch (SI9)
        int p1_si10 = 128; // First punch (SI10)
        int pl = 4;      // Punch record length
        int pm = 50;     // Max punches
        int pt = 0;      // Punch time day
        int cn = 1;      // Control number
        int pth = 2;     // Punch time high
        int ptl = 3;     // Punch time low
        int blocks = 2;  // Number of blocks
    };
    
    constexpr CardStructure SI9 = {
        25, 26, 27,      // cn2, cn1, cn0
        12, 14,          // std, st
        16, 18,          // ftd, ft
        8, 10,           // ctd, ct
        -1, -1,          // ltd, lt (нет)
        22,              // rc
        56, 128,         // p1, p1_si10
        4, 50,           // pl, pm
        0, 1, 2, 3,      // pt, cn, pth, ptl
        2                // blocks
    };
    
    constexpr CardStructure SI10 = {
        25, 26, 27,      // cn2, cn1, cn0
        12, 14,          // std, st
        16, 18,          // ftd, ft
        8, 10,           // ctd, ct
        -1, -1,          // ltd, lt (нет)
        22,              // rc
        56, 128,         // p1, p1_si10
        4, 128,          // pl, pm (до 128 для SI10)
        0, 1, 2, 3,      // pt, cn, pth, ptl
        8                // blocks
    };
}

} // namespace Constants
} // namespace SportIdent

#endif // SPORTIDENT_CONSTANTS_H
