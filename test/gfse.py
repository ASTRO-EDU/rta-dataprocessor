import os
import sys
import socket
import struct
import math
import time
from time import sleep
import argparse
from threading import Thread
import h5py
from waveform import Waveform, int_to_twos_comp
from crc32 import crc32_fill_table, crc32

import zmq
import numpy as np


def data_to_str(data):
    msg = ''
    for b in data:
        msg += "%02X" % b
    return "[%02d] 0x%s" % (len(data), msg)


###########################################
def ds_to_wform(ds, rpID=None, runID=None, sessionID=None, configID=None):
    # Read the entire dataset into a NumPy array and force a conversion to int16 to avoid issues with out-of-bound values.
    arr = ds[:].astype(np.int16)        # arr = ds[:]

    # trx is set using the current time since the std epoch
    wform = Waveform()

    if rpID:
        wform.rpID = rpID
    else:
        wform.rpID = ds.attrs['rp_id']

    if runID:
        wform.runID = runID
    else:
        wform.runID = ds.attrs['runid']

    if sessionID:
        wform.sessionID = sessionID
    else:
        wform.sessionID = ds.attrs['sessionID']

    if configID:
        wform.configID = configID
    else:
        wform.configID = ds.attrs['configID']

    wform.timeSts = ds.attrs['TimeSts']
    wform.ppsSliceNo = ds.attrs['PPSSliceNO']
    wform.year = ds.attrs['Year']
    wform.month = ds.attrs['Month']
    wform.day = ds.attrs['Day']
    wform.hh = ds.attrs['HH']
    wform.mm = ds.attrs['mm']
    wform.ss = ds.attrs['ss']
    wform.usec = ds.attrs['usec']

    # Time stamp is represented with a C/C++ timespec structure, hence
    # the Waveform creation time since std epoch in seconds is used
    # and converted into sec/nsec
    # NOTE: sec is set one second before the trx
    t = math.modf(wform.trx)
    wform.tstmp = (int(t[1]) - 1, round(t[0] * 1e9))

    wform.eql = ds.attrs['Eql']
    wform.dec = ds.attrs['Dec']

    # The buffer is already ordered
    wform.curr_off = arr.shape[0]-1
    wform.trig_off = 0

    wform.sample_no = arr.shape[0]

    wform.dt = float(wform.dec) * 8e-9

    # First/last sample time
    wform.tstart = float(wform.tstmp[0]) + float(wform.tstmp[1]) * 1e-9
    wform.tstop = wform.tstart + wform.dt * (wform.sample_no - 1)

    wform.data = []
    wform.trigt = []
    wform.sigt = []
    wform.sigr = []
    wform.sige = []

    # Convert numpy array to a tuple
    tpl = tuple(arr.reshape(1, -1)[0])

    # Reordering
    curr_off = wform.curr_off + 1
    if curr_off > wform.trig_off:
        pre1 = tpl[0:wform.trig_off]
        pre2 = tpl[wform.trig_off:curr_off]
        post = tpl[curr_off:]
        wform.data = ()
        for val in pre2 + post + pre1:
            wform.data += (int_to_twos_comp(val, 14),)
    else:
        pre = tpl[0:curr_off]
        post1 = tpl[curr_off:wform.trig_off]
        post2 = tpl[wform.trig_off:]
        wform.data = ()
        for val in post2 + pre + post1:
            wform.data += (int_to_twos_comp(val, 14),)

    return wform
###########################################


START = 0x8D

# APID
CLASS_TC = 0x00
CLASS_TM = 0x80
CLASS_MASK = 0x80
SOURCE_MASK = 0x7F

# Sequence
GROUP_CONT = 0x0000
GROUP_FIRST = 0x4000
GROUP_LAST = 0x8000
GROUP_STAND_ALONE = 0xC000
GROUP_MASK = 0xC000
COUNT_MASK = 0x3FFF

U32_x_PACKET = 1020

SAMPLES_X_PACKET = 2*U32_x_PACKET


def create_unified_waveform_packet(wform, srcid, npkt, crc_table):
    """
    Build a unified waveform packet that includes:
      - HeaderDams (12 bytes) + Data_WaveHeader (44 bytes)
      - Followed by one data block (Data_WaveData), which comprises:
          [1-byte type, 1-byte subtype, 2 bytes spares] + waveform data
    The entire unified payload is then prepended with its total size (4 bytes).
    """
    ###############################
    # Create the unified header part.
    ###############################
    # Build Data_WaveHeader payload (44 bytes)
    pkttype   = struct.pack("<B", 0xA1)   # Packet type (0xA1)
    subtype   = struct.pack("<B", 0x01)   # Header sub-type
    spares    = struct.pack("<BB", 0, 0)   # Spare bytes
    
    sessionid = struct.pack("<H", wform.sessionID)
    configid  = struct.pack("<H", wform.configID)
    timests     = struct.pack("<B", wform.timeSts)
    ppssliceno  = struct.pack("<B", wform.ppsSliceNo)
    year        = struct.pack("<B", wform.year)
    month       = struct.pack("<B", wform.month)
    day         = struct.pack("<B", wform.day)
    hh          = struct.pack("<B", wform.hh)
    mm          = struct.pack("<B", wform.mm)
    ss          = struct.pack("<B", wform.ss)
    usec        = struct.pack("<L", wform.usec)
    tv_sec      = struct.pack("<L", wform.tstmp[0])
    tv_nsec     = struct.pack("<L", wform.tstmp[1])
    dec         = struct.pack("<L", wform.dec)
    curr_off    = struct.pack("<L", wform.curr_off)
    trig_off    = struct.pack("<L", wform.trig_off)
    sample_no   = struct.pack("<L", wform.sample_no)
    
    header_payload = (
         pkttype + subtype + spares +
         sessionid + configid +
         timests + ppssliceno + year + month +
         day + hh + mm + ss +
         usec + tv_sec + tv_nsec +
         dec + curr_off + trig_off + sample_no
    )
    assert len(header_payload) == 44, f"Data_WaveHeader payload is {len(header_payload)} bytes, not 44"

    # Build HeaderDams (12 bytes)
    start_field = struct.pack("<B", START)
    apid_field  = struct.pack("<B", CLASS_TM + srcid)
    seq_field   = struct.pack("<H", GROUP_FIRST + npkt)
    runid_field = struct.pack("<H", wform.runID)
    size_field  = struct.pack("<H", len(header_payload))  # should be 44
    crcval = 0xFFFFFFFF
    crcval = crc32(crcval, header_payload, crc_table)
    crc_field = struct.pack("<L", crcval)
    header_common = start_field + apid_field + seq_field + runid_field + size_field + crc_field
    # The unified header part is the concatenation of HeaderDams and the Data_WaveHeader payload.
    unified_header = header_common + header_payload  # 12 + 44 = 56 bytes

    ###############################
    # Create the unified data block.
    ###############################
    # We want to pack all waveform data from wform.data into a single block.
    # First, get all the 16-bit samples.
    off = 0
    nsmp = wform.sample_no  # all samples
    dataval = list(wform.data[off:off+nsmp])
    if len(dataval) % 2 != 0:
        dataval.append(0)
    # Pack each pair of 16-bit values into a single 32-bit unsigned integer (little-endian)
    u32_list = [
        (dataval[i] & 0xFFFF) | ((dataval[i+1] & 0xFFFF) << 16)
        for i in range(0, len(dataval), 2)
    ]
    waveform_data = struct.pack("<%dL" % len(u32_list), *u32_list)
    
    # Build the Data_WaveData "header" for the data block: type, subtype, spares.
    data_type  = struct.pack("<B", 0xA1)         # type (must match Data_WaveData::TYPE)
    data_subtype = struct.pack("<B", 0x02)         # subtype for waveform data
    data_spares  = struct.pack("<BB", 0, 0)
    data_field = data_type + data_subtype + data_spares + waveform_data
    
    ###############################
    # Build the unified packet.
    ###############################
    # The unified packet payload is: unified_header (56 bytes) followed by data_field.
    unified_payload = unified_header + data_field

    # Prepend a 4-byte size field (unsigned little-endian)
    total_size = len(unified_payload)
    size_prefix = struct.pack("<L", total_size)
    
    unified_packet = size_prefix + unified_payload

    return unified_packet


def wform_to_unified_packet(wform, srcid, npkt, crc_table):
    # npkt is used only for the header's sequence field.
    # Return a single packet.
    pkt = create_unified_waveform_packet(wform, srcid, npkt, crc_table)
    return npkt + 1, pkt


def create_hk(state, flags, wform_count, crc_table):
    # Payload structure
    # [0] Type (uint8) 0x03
    # [1] SubType (uint8) 0x00
    # [2] State (uint8) 0x02 = SERVICE 0x03 ACQUISITION
    # [3] Flags (uint8) 0x80 = PPS_NOK 0x40 = GPS_NOK 0x20 = TRIG_ERR (no events)
    # [4] WaveCount (uint32)

    # Build the Data_Hk payload (16 bytes)
    hk_type    = struct.pack("<B", 0x03)       # Housekeeping type: 0x03
    hk_subtype = struct.pack("<B", 0x01)       # Housekeeping subtype: 0x01
    hk_state   = struct.pack("<B", state)      # Provided state (e.g. 0x02 or 0x03)
    hk_flags   = struct.pack("<B", flags)      # Provided flags (e.g. 0x80, etc.)
    hk_waveCount = struct.pack("<I", wform_count)
    now = time.time()
    tv_sec = struct.pack("<L", int(now))
    tv_nsec = struct.pack("<L", int((now - int(now)) * 1e9))
    
    data_payload = hk_type + hk_subtype + hk_state + hk_flags + hk_waveCount + tv_sec + tv_nsec
    assert len(data_payload) == 16, f"Data_Hk payload must be 16 bytes, got {len(data_payload)}"
    
    # Compute the CRC32 on the payload using your crc32 function and crc_table.
    crc_val = crc32(0xFFFFFFFF, data_payload, crc_table) & 0xFFFFFFFF
    crc_field = struct.pack("<L", crc_val)
    
    # Build the header (12 bytes)
    start_field = struct.pack("<B", 0x8D)  # Start is 0x8D
    apid_field = struct.pack("<B", 0x80)    # CLASS_TM is 0x80 
    GROUP_STAND_ALONE = 0xC000              # For housekeeping, use GROUP_STAND_ALONE
    seq_field = struct.pack("<H", GROUP_STAND_ALONE)
    runID_field = struct.pack("<H", 0)      
    size_field = struct.pack("<H", len(data_payload))  # Should be 16
    
    header = start_field + apid_field + seq_field + runID_field + size_field + crc_field
    assert len(header) == 12, f"Header must be 12 bytes, got {len(header)}"
    
    # Combine header and payload
    packet_without_prefix = header + data_payload  # 12 + 16 = 28 bytes total
    
    # Prepend a 4-byte size prefix (size of header+payload)
    total_size = len(packet_without_prefix)
    size_prefix = struct.pack("<L", total_size)
    
    return size_prefix + packet_without_prefix


class SendThread(Thread):
    def __init__(self, pub_socket, crc_table, dname, rpid, wform_dt, hk_rel_no, pkt_delay, restart=True):
        Thread.__init__(self)
        self.pub_socket = pub_socket
        self.crc_table = crc_table
        self.dname = dname
        self.rpid = rpid
        self.wform_count = 0
        self.hk_count = 0
        self.wform_dt = wform_dt
        self.hk_rel_no = hk_rel_no
        self.pkt_delay = pkt_delay
        self.running = True
        self.restart = restart  # Whether to restart when finished

        # Performance counters
        self.packets_sent = 0
        self.start_time = time.time()  # current time in seconds

    def run(self):
        npkt = 0
        self.packets_sent = 0
        self.start_time = time.time()

        # Get all files, don't limit to just 3
        files = sorted(os.listdir(self.dname))
        if not files:
            print(f'ERROR: No files found in directory {self.dname}')
            return
        
        print(f'[GFSE] INFO: Using all {len(files)} files for streaming')

        while self.running:
            # Loop through all files continuously
            for file in files:
                if not self.running:
                    break
                fname = os.path.join(self.dname, file)
                print('[GFSE] INFO: Send: Open', fname)
                try:
                    with h5py.File(fname, 'r') as fin:
                        group = fin['waveforms']
                        for key in group:
                            if not self.running:
                                break
                            ds = group[key]
                            wform = ds_to_wform(ds)
                            npkt, unified_pkt = wform_to_unified_packet(wform, self.rpid, npkt, self.crc_table)
                            self.pub_socket.send(unified_pkt)
                            self.packets_sent += 1
                            sleep(self.pkt_delay)

                            self.wform_count += 1
                            self.hk_count += 1

                            if self.hk_count == self.hk_rel_no:
                                hk_pkt = create_hk(0x03, 0x80, self.wform_count, self.crc_table)
                                self.pub_socket.send(hk_pkt)
                                self.packets_sent += 1
                                sleep(self.pkt_delay)
                                self.hk_count = 0

                            sleep(self.wform_dt)

                            now = time.time()
                            elapsed = now - self.start_time
                            if elapsed >= 1.0:
                                rate = self.packets_sent / elapsed
                                print(f"INFO: SendThread: {rate:.1f} packets/s")
                                self.packets_sent = 0
                                self.start_time = now
                except Exception as e:
                    print('ERROR: Send: File not valid:', e)
            
            if not self.restart:
                print('[GFSE] INFO: Completed one cycle of all files. Restart is disabled, stopping...')
                self.running = False
                break
            
            print('[GFSE] INFO: Completed one cycle of all files, starting over from the beginning...')

    def stop(self):
        self.running = False

DESCRIPTION = 'DAM server emulator v1.2.0'

if __name__ == '__main__':
    # ----------------------------------
    # Parse inputs
    # ----------------------------------

    # Configure input arguments
    parser = argparse.ArgumentParser(prog='gfse', description=DESCRIPTION)
    parser.add_argument('--addr', type=str, help='The Redpitaya IP address', required=True)
    parser.add_argument('--port', type=int, help='Server port', required=True)
    parser.add_argument('--indir', type=str, help='Input directory', required=True)
    parser.add_argument('--rpid', type=int, help='Redpitaya id', required=False)
    parser.add_argument('--wform-sec', type=float, help='Number of waveform per second', required=False, default=10.0)
    parser.add_argument('--hk-sec', type=float, help='Number of hk per second', required=False, default=0.2)
    parser.add_argument('--pkt-delay-sec', type=float, help='Delay between packets in seconds', required=False, default=0.00001)
    parser.add_argument('--restart', action='store_true', help='Whether to restart when finished sending the files')

    # Parse arguments and stop in case of help
    args = parser.parse_args(sys.argv[1:])
    print(args)

    wform_dt = 1/args.wform_sec
    hk_dt = 1/args.hk_sec
    hk_rel_no = round(hk_dt / wform_dt)
    #args.pkt_delay_sec = 0 #AB

    print("[GFSE] INFO: Main: Waveform x sec %f (%f s)" % (args.wform_sec, wform_dt))
    print("[GFSE] INFO: Main: HK x sec %f (%f s, %d wform period)" % (args.hk_sec, hk_dt, hk_rel_no))
    print("[GFSE] INFO: Main: Packet delay %f s" % args.pkt_delay_sec)
    print("[GFSE] INFO: Main: Restart when finished: %s" % ("Yes" if args.restart else "No"))

    print(DESCRIPTION)

    # ----------------------------------
    # Open socket
    # ----------------------------------

    crc_table = crc32_fill_table(0x05D7B3A1)
    count = 0

    print("[GFSE] INFO: Main: Listen on ", args.port)

    # Create a ZMQ socket
    context = zmq.Context()
    pub_socket = context.socket(zmq.PUB)
    pub_socket.bind(f"tcp://{args.addr}:{args.port}")
    print("[GFSE] INFO: Main: Publisher running")


    # Create the control PULL socket for receiving control commands
    ctrl_port = args.port + 1  # control port; note: consumer will bind a PUSH here
    ctrl_socket = context.socket(zmq.PULL)
    ctrl_socket.bind(f"tcp://{args.addr}:{ctrl_port}")
    print(f"[GFSE] INFO: Control socket (PULL) connected to port {ctrl_port}")

    # Allow some time for connection if necessary
    sleep(1)

    # Initialize a poller for non-blocking control checks.
    poller = zmq.Poller()
    poller.register(ctrl_socket, zmq.POLLIN)
    
    # Flag to indicate whether acquisition (sending waveform packets) is active.
    acquisition_running = False
    send_thread = None


    while True:
        try:
            socks = dict(poller.poll(100))  # timeout 100ms
            if ctrl_socket in socks and socks[ctrl_socket] == zmq.POLLIN:
                msg = ctrl_socket.recv_string(flags=zmq.NOBLOCK)
                print(f"[GFSE] INFO: Control received: {msg}")
                if msg == "START":
                    acquisition_running = True
                    if not send_thread or not send_thread.is_alive():
                        send_thread = SendThread(pub_socket, crc_table, args.indir, args.rpid, wform_dt, hk_rel_no, args.pkt_delay_sec, args.restart)
                        send_thread.start()
                    else:
                        print("[GFSE] INFO: Send thread already running.")
                elif msg == "STOP":
                    acquisition_running = False
                    if send_thread and send_thread.is_alive():
                        print("[GFSE] INFO: Main: Stop acquisition")
                        send_thread.stop()
                        send_thread.join()
                        send_thread = None
                        print("[GFSE] INFO: Main: Acquisition stopped")
                else:
                    print("[GFSE] INFO: Unknown control command:", msg)
            else:
                if not acquisition_running:
                    print("INFO: No control message received yet, trying again...")
            sleep(1)
        except KeyboardInterrupt:
            print("[GFSE] INFO: KeyboardInterrupt caught, stopping...")
            if send_thread and send_thread.is_alive():
                send_thread.stop()
                send_thread.join()
                send_thread = None
            pub_socket.close()
            ctrl_socket.close()
            context.term()
            break

    print('End')