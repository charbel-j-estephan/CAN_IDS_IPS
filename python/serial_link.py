"""USB serial link to the ESP32 boards: open a board, stream packets to a sender."""
import sys
import time

ACK_EVERY = 64       # a sender writes 'A' after this many frames
MAX_INFLIGHT = 256   # frames the PC may have sent that the sender has not acked yet


def open_board(port, baud, label, ready=(b"READY",)):
    """Open a board, restart its sketch and wait for one of the `ready` marks.

    The ESP32s restart (EN pulsed through RTS) and print READY. The STM32's
    own USB port cannot be restarted that way, and Windows does not even pass
    an RTS change on to it, so for a receiver its once a second #STAT line
    counts as ready too."""
    import serial
    ser = serial.Serial(port, baud, timeout=0.1)
    # Same reset sequence as esptool: pulse EN so the sketch starts from scratch.
    ser.setDTR(False)
    ser.setRTS(True)
    time.sleep(0.1)
    ser.setRTS(False)
    seen, t0 = b"", time.time()
    while not any(r in seen for r in ready):
        if time.time() - t0 > 20:
            sys.exit("%s on %s never printed READY. Is the sketch flashed and the "
                     "baud rate 921600? If it prints 'CAN init failed', check the wiring."
                     % (label, port))
        seen += ser.read(256)
    ser.reset_input_buffer()
    print("%s ready on %s" % (label, port))
    return ser


def stream(ser, packets, label_timeout=15, name="sender"):
    """Send every packet to a sender, never more than MAX_INFLIGHT unacked."""
    n = len(packets)
    sent = acked = 0
    last_progress = time.time()
    t0 = time.time()
    while sent < n:
        waiting = ser.in_waiting
        if waiting:
            got = ser.read(waiting).count(b"A")
            if got:
                acked += got
                last_progress = time.time()
        if sent - acked * ACK_EVERY >= MAX_INFLIGHT:
            if time.time() - last_progress > label_timeout:
                sys.exit("The %s stalled. Is the receiver powered, wired to the same bus "
                         "and showing READY? Without a receiver nobody ACKs frames." % name)
            time.sleep(0.0005)
            continue
        k = min(ACK_EVERY, n - sent)
        ser.write(b"".join(packets[sent:sent + k]))
        sent += k
        if sent % 5000 < k:
            rate = sent / (time.time() - t0)
            print("  %s: sent %d / %d frames (%.0f frames/s)" % (name, sent, n, rate))
    return time.time() - t0
