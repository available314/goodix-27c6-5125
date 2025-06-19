import struct

data = bytes.fromhex("01 00")


def get_checksum(data: bytes, seed: int = 0x00, target: int = 0xAA) -> int:
    s = (seed + sum(data)) & 0xFF
    return (target - s) & 0xFF

examples = [
    ("01 00", 0x86),
]


seed = 0
for i in range(0xFF):
    result = get_checksum(data, i)
    if (result == 0x86):
        print("Seed i:", i)
        seed = i


for hex_str, expected in examples:
    data = bytes.fromhex(hex_str)
    result = get_checksum(data, seed)
    print(f"Expected: {expected:02X}, Got: {result:02X}, Match: {result == expected}")

