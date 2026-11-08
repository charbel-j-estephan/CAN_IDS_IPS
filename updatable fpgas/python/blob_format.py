"""The model blob: one binary file holding a whole model. Full description in
docs/BLOB_FORMAT.md. Every security field exists from day one; Phase 1 leaves
some of them unused.

    offset  size
    0       64    header
    64      16    feature map
    80      96    node counts, one byte per (tree, level): 8 trees x 12 levels
    176     272   self test vectors, 8 x 34 bytes
    448     256   key table bitmap, bit i = byte i >> 3, bit i & 7
    704     5*S   the S split nodes in order tree, level, index
    704+5S  4     CRC-32 of every byte before it
    708+5S  16    MAC of every byte before it (zero until signed)

All multi byte integers are little endian. A node is 36 bits, packed into 5
bytes, low bits first: right (8), left (8), threshold (16), feature (4), then
4 reserved bits. left and right are each {is_leaf, 7 bit index or class vote}.
"""
import struct
import zlib

import numpy as np

MAGIC = 0x4D464943          # "CIFM" in file order
FORMAT_VERSION = 1

N_TREES_MAX = 8
DEPTH_MAX = 12
N_FEAT_MAX = 16
N_CLASS_MAX = 8
NODES_PER_LEVEL_MAX = 128
THRESH_MAX = 0xFFFF
N_SELFTEST_MAX = 8

HEADER_SIZE = 64
OFF_FEATURE_MAP = 64
OFF_COUNTS = 80
OFF_SELFTEST = 176
SELFTEST_SIZE = 34
OFF_KEY_TABLE = 448
OFF_NODES = 704
NODE_SIZE = 5
TRAILER_SIZE = 20           # CRC 4 + MAC 16
OVERHEAD = OFF_NODES + TRAILER_SIZE

SLOT_BYTES = 24 * 1024      # one flash slot: UFM pages 0-2 of a sector
MAX_SPLITS = (SLOT_BYTES - OVERHEAD) // NODE_SIZE

DEVICE_ANY = 0xFFFFFFFF     # accepted from the boot ROM only, never over SPI
PROTOCOL_CAN = 1

FLAG_ENCRYPTED = 0x0001     # reserved for Phase D

LEAF = 0x80

HEADER_FMT = "<IHHIIIIHBBBBBBBBBB I12s12s"
assert struct.calcsize(HEADER_FMT) == HEADER_SIZE


def pack_node(feature, threshold, left, right):
    v = right | (left << 8) | (threshold << 16) | (feature << 32)
    return v.to_bytes(NODE_SIZE, "little")


def unpack_node(raw):
    v = int.from_bytes(raw, "little")
    return (v >> 32) & 0xF, (v >> 16) & 0xFFFF, (v >> 8) & 0xFF, v & 0xFF


def build(trees, n_features, n_classes, feat_bytes, known_keys, selftests,
          model_id, version, device_id=DEVICE_ANY, protocol=PROTOCOL_CAN,
          rule=(True, 0, 2), feature_map=None, flags=0, key_id=0, iv=b"\0" * 12):
    """Make a blob. trees is a list (one per tree) of lists (one per level) of
    (feature, threshold, left, right). selftests is a list of (features, vote)."""
    if not 1 <= len(trees) <= N_TREES_MAX:
        raise ValueError("%d trees: at most %d" % (len(trees), N_TREES_MAX))
    if len(selftests) > N_SELFTEST_MAX:
        raise ValueError("at most %d self test vectors" % N_SELFTEST_MAX)

    counts = bytearray(N_TREES_MAX * DEPTH_MAX)
    nodes = bytearray()
    depth = 0
    n_splits = 0
    for t, levels in enumerate(trees):
        if len(levels) > DEPTH_MAX:
            raise ValueError("tree %d has %d levels: at most %d" % (t, len(levels), DEPTH_MAX))
        depth = max(depth, len(levels))
        for l, level in enumerate(levels):
            if len(level) > NODES_PER_LEVEL_MAX:
                raise ValueError("tree %d level %d has %d nodes: at most %d"
                                 % (t, l, len(level), NODES_PER_LEVEL_MAX))
            counts[t * DEPTH_MAX + l] = len(level)
            n_splits += len(level)
            for feature, threshold, left, right in level:
                nodes += pack_node(feature, threshold, left, right)
    if n_splits > MAX_SPLITS:
        raise ValueError("%d split nodes: a %d byte flash slot holds %d"
                         % (n_splits, SLOT_BYTES, MAX_SPLITS))

    if feature_map is None:
        feature_map = [i if i < n_features else 0xFF for i in range(N_FEAT_MAX)]

    table = bytearray(256)
    for k in known_keys:
        table[int(k) >> 3] |= 1 << (int(k) & 7)

    tests = bytearray(SELFTEST_SIZE * N_SELFTEST_MAX)
    for i, (features, vote) in enumerate(selftests):
        off = i * SELFTEST_SIZE
        struct.pack_into("<16H", tests, off, *[int(x) for x in features])
        tests[off + 32] = vote

    total = OVERHEAD + len(nodes)
    rule_en, rule_from, rule_to = rule
    header = struct.pack(HEADER_FMT, MAGIC, FORMAT_VERSION, flags, total, model_id,
                         version, device_id, protocol, len(trees), depth, n_features,
                         n_classes, feat_bytes, 1 if rule_en else 0, rule_from, rule_to,
                         len(selftests), 0, key_id, iv, b"\0" * 12)
    body = header + bytes(feature_map) + bytes(counts) + bytes(tests) + bytes(table) + bytes(nodes)
    return body + struct.pack("<I", zlib.crc32(body)) + b"\0" * 16


def parse(blob):
    """Check a blob and return its fields. Raises ValueError on any fault."""
    if len(blob) < OVERHEAD:
        raise ValueError("too short")
    f = struct.unpack_from(HEADER_FMT, blob)
    h = dict(zip(("magic", "format", "flags", "total_len", "model_id", "version", "device_id",
                  "protocol", "n_trees", "depth", "n_features", "n_classes", "feat_bytes",
                  "rule_en", "rule_from", "rule_to", "n_selftest", "reserved0", "key_id", "iv",
                  "reserved1"), f))
    if h["magic"] != MAGIC:
        raise ValueError("bad magic")
    if h["format"] != FORMAT_VERSION:
        raise ValueError("bad format version")
    if h["total_len"] != len(blob):
        raise ValueError("length %d, header says %d" % (len(blob), h["total_len"]))
    body, crc = blob[:-TRAILER_SIZE], struct.unpack_from("<I", blob, len(blob) - TRAILER_SIZE)[0]
    if zlib.crc32(body) != crc:
        raise ValueError("bad CRC")
    h["crc"] = crc
    h["mac"] = bytes(blob[-16:])
    counts = blob[OFF_COUNTS:OFF_COUNTS + N_TREES_MAX * DEPTH_MAX]
    off = OFF_NODES
    trees = []
    for t in range(N_TREES_MAX):
        levels = []
        for l in range(DEPTH_MAX):
            n = counts[t * DEPTH_MAX + l]
            levels.append([unpack_node(blob[off + i * NODE_SIZE:off + (i + 1) * NODE_SIZE])
                           for i in range(n)])
            off += n * NODE_SIZE
        trees.append(levels)
    if off != len(blob) - TRAILER_SIZE:
        raise ValueError("node counts do not match the length")
    h["trees"] = trees[:h["n_trees"]]
    h["key_table"] = bytes(blob[OFF_KEY_TABLE:OFF_KEY_TABLE + 256])
    h["selftests"] = []
    for i in range(h["n_selftest"]):
        o = OFF_SELFTEST + i * SELFTEST_SIZE
        h["selftests"].append((struct.unpack_from("<16H", blob, o), blob[o + 32]))
    return h


def forest_votes(trees, X, n_classes):
    """What the hardware does, in Python. X has one row per frame and one column
    per feature slot. Returns the class of each frame (ties to the lowest)."""
    X = np.asarray(X)
    rows = np.arange(len(X))
    votes = np.zeros((len(X), n_classes), dtype=np.int32)
    for levels in trees:
        idx = np.zeros(len(X), dtype=np.int32)
        done = np.zeros(len(X), dtype=bool)
        val = np.zeros(len(X), dtype=np.int32)
        for level in levels:
            if not level:
                break
            a = np.array(level, dtype=np.int64)
            pending = np.flatnonzero(~done)
            if len(pending) == 0:
                break
            n = a[idx[pending]]
            x = X[pending, n[:, 0]]
            child = np.where(x <= n[:, 1], n[:, 2], n[:, 3])
            leaf = (child & LEAF) != 0
            val[pending[leaf]] = child[leaf] & 0x7F
            done[pending[leaf]] = True
            idx[pending[~leaf]] = child[~leaf] & 0x7F
        if not done.all():
            raise ValueError("a frame fell off the bottom of a tree")
        votes[rows, val] += 1
    return np.argmax(votes, axis=1)
