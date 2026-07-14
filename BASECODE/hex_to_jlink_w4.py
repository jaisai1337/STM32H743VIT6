#!/usr/bin/env python3
import sys
from intelhex import IntelHex

if len(sys.argv) != 3:
    print("Usage: python3 hex_to_jlink_w4.py input.hex output.jlink")
    sys.exit(1)

input_hex = sys.argv[1]
output_jlink = sys.argv[2]

print("Reading:", input_hex)
ih = IntelHex(input_hex)
if ih.maxaddr() < ih.minaddr():
    print("⚠️ No valid data found in HEX file!")
    sys.exit(1)

min_addr = ih.minaddr()
max_addr = ih.maxaddr()
print("Address range: 0x%08X - 0x%08X" % (min_addr, max_addr))

# Convert all data to a dictionary for faster access
data = ih.todict()

count = 0
with open(output_jlink, "w") as out:
    start_addr = min_addr - (min_addr % 4)
    end_addr = max_addr + 1
    for addr in range(start_addr, end_addr, 4):
        b0 = data.get(addr, 0xFF)
        b1 = data.get(addr + 1, 0xFF)
        b2 = data.get(addr + 2, 0xFF)
        b3 = data.get(addr + 3, 0xFF)
        value = (b3 << 24) | (b2 << 16) | (b1 << 8) | b0
        out.write("w4 0x%08X 0x%08X\n" % (addr, value))
        count += 1
    
    # Append final J-Link commands
    out.write("\nr\n")
    out.write("g\n")
    out.write("qc\n")
print("Done. %d w4 commands written to %s" % (count, output_jlink))