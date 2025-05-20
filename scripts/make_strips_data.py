import argparse
import struct

def make_empty_error():
    packet_bytes = [
        0x22, 0x0a,
        0x13, 0xfe,
        0x77, 0xf4, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
        0x6f, 0xed
    ]

    return packet_bytes

def make_empty():
    packet_bytes = [
        0x22, 0x0a,
        0x6f, 0xed
    ]

    return packet_bytes

def make_full():
    # Build max length packet
    packet_bytes = [
        0x22, 0xaa,
    ]

    for a in range(11):
        # 256 channels with 4 per cluster
        for b in range(64):
            word = 7 # All three subsequent hits
            if b == 63:
                word |= 0x8000
            word |= a << 11
            word |= (b * 4) << 3
            packet_bytes.extend([(word >> 8) & 0xff, word & 0xff])

    packet_bytes.extend([0x6f, 0xed])
    return packet_bytes

def make_full_counters():
    packets = [
    ]

    pattern = [0x40, 0x33, 0x0f, 0xff, 0xff, 0xff, 0xf9, 0x14, 0x10]
    for a in range(11):
        pattern[0] = 0x40 | a
        pattern[6] = 0xf0 | a # ABC ID in Status
        for r in range(0x80, 0xc0):
            pattern[1] = r
            packets.append(list(pattern))

            # print(packets)
    return packets

def make_empty_counters():
    packets = [
    ]

    pattern = [0x40, 0x33, 0x00, 0x00, 0x00, 0x00, 0x09, 0x14, 0x10]
    for a in range(11):
        pattern[0] = 0x40 | a
        pattern[6] = 0xf0 | a # ABC ID in Status
        for r in range(0x80, 0xc0):
            pattern[1] = r
            packets.append(list(pattern))

            # print(packets)
    return packets

def make_hcc_hpr():
    packet_bytes = [
        0xe0, 0xf5, 0x78, 0x50, 0x07, 0x90, 0x7f
    ]
    return packet_bytes

def make_abc_hpr(a):
    packet_bytes = [
        0xd0, 0x3f, 0x07, 0x85, 0x51, 0xff, 0x0f, 0xf9, 0x14, 0x90,
    ]
    packet_bytes[0] |= a
    return packet_bytes

def make_hpr_packets():
    packets = [make_hcc_hpr()]
    packets.extend(make_abc_hpr(a) for a in range(10))
    return packets

def fill_32(pb):
    while len(pb) % 4 != 0:
        pb.append(0)

def make_data(pk_type, out_name, repetitions):
    out = open(out_name, "wb")

    if pk_type == "basic":
        multi_packet_bytes = [make_empty_error()]
    elif pk_type == "empty":
        multi_packet_bytes = [make_empty()]
    elif pk_type == "full":
        multi_packet_bytes = [make_full()]
    elif pk_type == "empty_counters":
        multi_packet_bytes = make_empty_counters()
    elif pk_type == "full_counters":
        multi_packet_bytes = make_full_counters()
    elif pk_type == "hpr":
        multi_packet_bytes = make_hpr_packets()
    else:
        raise Exception(f"Packet type '{pk_type}' not recognised")

    multi_packet_bytes = multi_packet_bytes * repetitions

    for pp in multi_packet_bytes:
        fill_32(pp)

    count = len(multi_packet_bytes)

    # Packet count
    out.write(struct.pack("I", count))

    ts = 100
    elink_id = 0

    # print(bytes(packet_bytes))
    for pb in multi_packet_bytes:
        l = len(pb)

        # Pack TS, ID, byteCount, data
        out.write(struct.pack(f"QQI{l}s", ts, elink_id, l, bytes(pb)))

        ts += 100

    out.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument('-t', '--type', dest="pkt_type", help="Packet type", default="basic")
    parser.add_argument('-o', '--output', dest="output_name", help="Output file name", default="data_file.bin")
    parser.add_argument('-r', '--repetitions', type = int, dest="repetitions", help="Repetitions", default="1")

    args = parser.parse_args()

    make_data(args.pkt_type, args.output_name, args.repetitions)
