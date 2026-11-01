"""Exact length of a classic CAN frame on the wire, stuff bits included.

The Car Hacking dataset stamps each frame when it ENDS. To replay a frame so it
ends at its original time, the sender has to start it one frame length
earlier, and that length depends on the ID and data through bit stuffing.
"""


def _crc15(bits):
    crc = 0
    for b in bits:
        nxt = b ^ ((crc >> 14) & 1)
        crc = (crc << 1) & 0x7FFF
        if nxt:
            crc ^= 0x4599
    return crc


def frame_bits(can_id, dlc, data):
    """Bits from start of frame to the end of the end-of-frame field, for an
    11 bit ID data frame. Stuffing covers start of frame up to the CRC."""
    n = min(dlc, 8)
    bits = [0]                                              # start of frame
    bits += [(can_id >> (10 - i)) & 1 for i in range(11)]   # identifier
    bits += [0, 0, 0]                                       # RTR, IDE, r0
    bits += [(dlc >> (3 - i)) & 1 for i in range(4)]        # DLC
    for byte in list(data)[:n]:
        bits += [(byte >> (7 - i)) & 1 for i in range(8)]
    crc = _crc15(bits)
    bits += [(crc >> (14 - i)) & 1 for i in range(15)]

    stuffed, run, last = 0, 0, None
    for b in bits:
        if b == last:
            run += 1
        else:
            run, last = 1, b
        if run == 5:                     # five equal bits: the controller inserts the opposite one
            stuffed += 1
            last, run = 1 - b, 1
    return len(bits) + stuffed + 1 + 2 + 7   # CRC delimiter, ACK slot + delimiter, end of frame


def frame_us(can_id, dlc, data, bitrate=500000):
    return frame_bits(can_id, dlc, data) * 1e6 / bitrate
