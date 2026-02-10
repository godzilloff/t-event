# Константы алгоритма (из SI Programmers Manual)
CRC_BITF = 0x8000      # Маска старшего бита (бит 15)
CRC_POLYNOM = 0x8005   # Полином CRC-16

def _to_int(s):
    """Преобразует байтовую строку в целое число (big-endian)"""
    value = 0
    # Переворачиваем для обработки от младшего байта к старшему
    for offset, c in enumerate(s[::-1]):
        value += c << (offset * 8)
    return value

def _to_str(i, length):
    """Преобразует целое число в байтовую строку (big-endian)"""
    return i.to_bytes(length, "big")

def _crc(s, verbose=True):
    """
    Вычисляет CRC-16 по алгоритму из SI Programmers Manual
    
    Args:
        s: байтовая строка данных
        verbose: если True - выводить пошаговую отладку
    """
    def two_chars(data):
        """Генератор, разбивающий данные на 2-байтные блоки (с дополнением нулями при нечётной длине)"""
        if len(data) == 0:
            return
        
        # Дополняем до чётной длины
        if len(data) % 2 == 0:
            data += b"\x00\x00"
        else:
            data += b"\x00"
        
        for i in range(0, len(data), 2):
            yield data[i:i+2]
    
    if len(s) < 1:
        return b"\x00\x00"
    
    # Инициализация: первые 2 байта данных = начальное значение CRC
    crc = _to_int(s[0:2])
    if verbose:
        print(f"\n=== НАЧАЛО ВЫЧИСЛЕНИЯ CRC ===")
        print(f"Входные данные (hex): {s.hex().upper()}")
        print(f"Первые 2 байта: {s[0:2].hex().upper()} -> начальное CRC = 0x{crc:04X}")
        print(f"Оставшиеся данные: {s[2:].hex().upper() if len(s)>2 else 'нет'}")
        print("-" * 70)
    
    block_num = 0
    for c in two_chars(s[2:]):
        block_num += 1
        val = _to_int(c)
        if verbose:
            print(f"\n>>> Обработка блока #{block_num}: 0x{val:04X} (байты: {c.hex().upper()})")
            print(f"    Начальное состояние: CRC=0x{crc:04X}, VAL=0x{val:04X}")
            print(f"    {'Шаг':<5} {'CRC (bin)':<20} {'VAL (bin)':<20} {'Действие'}")
            print(f"    {'-'*66}")
        
        for j in range(16):
            # Сохраняем состояние перед операцией
            crc_before = crc
            val_before = val
            msb_crc = (crc & CRC_BITF) != 0
            msb_val = (val & CRC_BITF) != 0
            
            if msb_crc:
                # Ветка 1: старший бит CRC = 1
                crc = (crc << 1) & 0x1FFFF  # Сдвиг с сохранением 17 бит для переноса
                if msb_val:
                    crc |= 1  # Добавляем перенос из VAL
                crc ^= CRC_POLYNOM
                action = "CRC↑1 → XOR 0x8005"
            else:
                # Ветка 2: старший бит CRC = 0
                crc = (crc << 1) & 0x1FFFF
                if msb_val:
                    crc |= 1
                action = "CRC↑1 (без XOR)"
            
            val = (val << 1) & 0xFFFF  # Сдвигаем VAL, сохраняя 16 бит
            
            if verbose:
                crc_bin = format(crc_before >> 8, '08b') + ' ' + format(crc_before & 0xFF, '08b')
                val_bin = format(val_before >> 8, '08b') + ' ' + format(val_before & 0xFF, '08b')
                print(f"    {j:<5} {crc_bin} {val_bin} {action}")
                if j == 15:  # Последний шаг блока
                    print(f"    Итог блока: CRC=0x{crc & 0xFFFF:04X}")
    
    # Обрезаем до 16 бит и преобразуем в байты (big-endian)
    crc &= 0xFFFF
    result = _to_str(crc, 2)
    
    if verbose:
        print("-" * 70)
        print(f"Финальное CRC (16 бит): 0x{crc:04X}")
        print(f"Результат в байтах: {result.hex().upper()}")
        print("=" * 70)
    
    return result

def _crc_check(s, crc):
    """Проверяет правильность CRC"""
    computed = _crc(s, verbose=False)
    return computed == crc

# ================= ТЕСТЫ =================
if __name__ == "__main__":
    print("="*70)
    print("ДЕМОНСТРАЦИЯ АЛГОРИТМА CRC-16 (SportIdent)")
    print("="*70)
    
    # # Тест 1: EF0100 -> E209
    # print("\nТЕСТ 1: Данные EF0100 → ожидаемый CRC E209")
    # data1 = bytes.fromhex("EF0100")
    # expected1 = bytes.fromhex("E209")
    # result1 = _crc(data1, verbose=True)
    # print(f"\nПроверка: {'✓ УСПЕШНО' if result1 == expected1 else '✗ ОШИБКА'}")
    # print(f"Вычислено: {result1.hex().upper()}, Ожидаемо: {expected1.hex().upper()}")
    
    # # Тест 2: 000F0F6AF775 -> 7EA6
    # print("\n\nТЕСТ 2: Данные 000F0F6AF775 → ожидаемый CRC 7EA6")
    # data2 = bytes.fromhex("000F0F6AF775")
    # expected2 = bytes.fromhex("7EA6")
    # result2 = _crc(data2, verbose=True)
    # print(f"\nПроверка: {'✓ УСПЕШНО' if result2 == expected2 else '✗ ОШИБКА'}")
    # print(f"Вычислено: {result2.hex().upper()}, Ожидаемо: {expected2.hex().upper()}")


    # # Тест 3: EF0104 -> E609
    # print("\nТЕСТ 3: Данные EF0104 → ожидаемый CRC E609")
    # data1 = bytes.fromhex("EF0104")
    # expected1 = bytes.fromhex("E609")
    # result1 = _crc(data1, verbose=True)
    # print(f"\nПроверка: {'✓ УСПЕШНО' if result1 == expected1 else '✗ ОШИБКА'}")
    # print(f"Вычислено: {result1.hex().upper()}, Ожидаемо: {expected1.hex().upper()}")

    # # Тест 4: E8 06 00 0F 0F 7A 78 58 → ожидаемый CRC 5D 02
    # print("\n\nТЕСТ 4: Данные E8 06 00 0F 0F 7A 78 58 → ожидаемый CRC 5D 02")
    # data2 = bytes.fromhex("E806000F0F7A7858")
    # expected2 = bytes.fromhex("5D02")
    # result2 = _crc(data2)
    # print(f"\nПроверка: {'✓ УСПЕШНО' if result2 == expected2 else '✗ ОШИБКА'}")
    # print(f"Вычислено: {result2.hex().upper()}, Ожидаемо: {expected2.hex().upper()}")

    # # Тест 5: 
    # print("\n\nТЕСТ 5: Данные 06 00 0F 0F 7A 78 58 → ожидаемый CRC 5D 02")
    # data2 = bytes.fromhex("06000F0F7A7858")
    # expected2 = bytes.fromhex("5D02")
    # result2 = _crc(data2)
    # print(f"\nПроверка: {'✓ УСПЕШНО' if result2 == expected2 else '✗ ОШИБКА'}")
    # print(f"Вычислено: {result2.hex().upper()}, Ожидаемо: {expected2.hex().upper()}")


    # # Тест 6: 
    # print("\n\nТЕСТ 6: Данные 00 0F 0F 7A 78 58 → ожидаемый CRC 5D 02")
    # data2 = bytes.fromhex("EF83000F000DB3DD9AEAEAEAEA0B045A2B0B035A3F0B016A540797082C0F6DF1D2040FFF4B373230353333303B4B6F73696C6F7620496C79613B4D3B31393838303631363B4D545543493B676F647A696C6C6F6666406D61696C2E72753B2B37393035353135343838303B4D6F733F6F773B4D6F733F6F773B3134313032313B5275737369")
    # expected2 = bytes.fromhex("997F")
    # result2 = _crc(data2)
    # print(f"\nПроверка: {'✓ УСПЕШНО' if result2 == expected2 else '✗ ОШИБКА'}")
    # print(f"Вычислено: {result2.hex().upper()}, Ожидаемо: {expected2.hex().upper()}")

    # Тест 5: 
    print("\n\nТЕСТ 5:")
    data2 = bytes.fromhex("E806000F0F6DF1D2")
    expected2 = bytes.fromhex("6918")
    result2 = _crc(data2)
    print(f"\nПроверка: {'✓ УСПЕШНО' if result2 == expected2 else '✗ ОШИБКА'}")
    print(f"Вычислено: {result2.hex().upper()}, Ожидаемо: {expected2.hex().upper()}")