import struct

def process_fdt_data(hex_string):
    # Удаляем пробелы и разбиваем на байты
    data = bytes.fromhex(hex_string)
    
    # Убедимся, что длина кратна 2
    assert len(data) % 2 == 0, "Длина данных должна быть чётной (по 2 байта на short)"

    # Преобразуем в список short'ов (младший байт первым — little-endian)
    shorts = list(struct.unpack('<' + 'H' * (len(data) // 2), data))

    # Обработка — сдвиг вправо на 1 (делим на 2)
    processed = [(value >> 1) for value in shorts]

    # Упаковываем обратно в байты
    result_bytes = struct.pack('<' + 'H' * len(processed), *processed)

    # Выводим результат в hex-формате
    print(" ".join(f"{b:02x}" for b in result_bytes))

# Пример данных

hex_input = "66 01 86 01 4f 01 6a 01 50 01 6f 01"
process_fdt_data(hex_input)

