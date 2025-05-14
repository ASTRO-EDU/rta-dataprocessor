import struct
import time
import numpy as np

# RedPitaya ADC is 14 bit, raw samples are returned in 2's complement
# In general N-bit two's complement number spans [−2^(N−1), 2^(N−1) − 1],
# hence N = 14 and range is [-8192, 8191]
# Raw data representation is
# 0x0000 =      0 = 0
# 0x0001 =      1 = 1
# 0x0002 =      2 = 2
# ...
# 0x1FFD =   8189 = 8189
# 0x1FFE =   8190 = 8190
# 0x1FFF =   8191 = 8191
# 0x2000 =   8192 = -8192
# 0x2001 =   8193 = -8191
# 0x2002 =   8194 = -8190
# ...
# 0x3FFD =  16381 = -3
# 0x3FFE =  16382 = -2
# 0x3FFF =  16383 = -1

def twos_comp_to_int(val, bits):
    if (val & (1 << (bits - 1))) != 0: # if sign bit is set e.g., 8bit: 128-255
        val = val - (1 << bits)        # compute negative value
    return val                         # return positive value as is


###########################################
def int_to_twos_comp(val, bits):
    """
    Convert an integer to its two's complement representation in 'bits' bits.
    This function masks the value to 'bits' bits and then, if the sign bit is set,
    adjusts the value to represent it as a negative number in Python.
    """
    # Force the value to be a native Python int
    val = int(val)
    mask = (1 << bits) - 1  # For 14 bits, mask = 0x3FFF (16383)
    # Keep only the lower 'bits' bits
    val = val & mask        
    sign_bit = 1 << (bits - 1)
    if val & sign_bit:
        val -= (1 << bits)
    return val
###########################################
"""
def int_to_twos_comp(val, bits):
    if val < 0:
        val = val + (1 << bits)
    return val

"""


def hex_to_str(data):
    str = ''
    for b in data:
        str += '%02X' % b
    return str


class Waveform:
    """GAMMA_FLASH Waveform handling"""

    def __init__(self,trx=None,rpID=None,runID=None):

        # Receive time in secs past January 1, 1970, 00:00:00 (UTC)
        if trx is None:
            self.trx = time.time()
        else:
            self.trx = trx

        # Operation identification
        self.rpID = rpID
        self.runID = runID
        self.sessionID = None
        self.configID = None

        # Absolute time
        self.timeSts = None
        self.ppsSliceNo = None
        self.year = None
        self.month = None
        self.day = None
        self.hh = None
        self.mm = None
        self.ss = None
        self.usec = None

        # Waveform info
        self.tstmp = None
        self.eql = None
        self.dec = None
        self.curr_off = None
        self.trig_off = None
        self.sample_no = None

        # Derived data
        self.tstart = None
        self.tstop = None
        self.dt = None
        self.data = None
        self.trigt = None
        self.sigt = None
        self.sigr = None
        self.sige = None

    def __str__(self):
        ts = time.gmtime(self.trx)
        str1 = time.strftime("%Y-%m-%dT%H_%M_%S", ts)
        sns = math.modf(tm)
        usec = round(sns[0] * 1e6)
        str2 = '%06d' % usec
        return str1 + '.' + str2


    def load_header(self, fname):
        with open(fname, "rb") as f:
            self.unpack_header(f)

    def load(self, fname):
        with open(fname, "rb") as f:
            self.unpack_header(f)
            self.unpack_data(f)

    def save(self, fname):
        with open(fname, "wb") as f:
            self.pack_header(f)
            self.pack_data(f)

    def unpack_header(self, f):
        self.runID = struct.unpack("<L", f.read(4))  # run ID
        self.sessionID = struct.unpack("<L", f.read(4))  # session ID
        self.configID = struct.unpack("<L", f.read(4))  # configuration ID
        self.tstmp = struct.unpack("<LL", f.read(8))  # timespec sec.nsec
        dummy = struct.unpack("<B", f.read(1))[0]  # dummy
        self.eql = struct.unpack("<B", f.read(1))[0]  # euqualization
        self.dec = struct.unpack("<H", f.read(2))[0]  # decimation
        self.curr_off = struct.unpack("<L", f.read(4))[0]  # current off (end of the acquired data)
        self.trig_off = struct.unpack("<L", f.read(4))[0]  # trigger off
        self.sample_no = struct.unpack("<L", f.read(4))[0]
        self.compute_time()

    def pack_header(self, f):
        raw = struct.pack("<L", self.runID)
        raw = struct.pack("<L", self.sessionID)
        raw = struct.pack("<L", self.configID)
        raw += struct.pack("<LL", self.tstmp[0], self.tstmp[1])
        raw += struct.pack("<B", 0)
        raw += struct.pack("<B", self.eql)
        raw += struct.pack("<H", self.dec)
        raw += struct.pack("<L", self.curr_off)
        raw += struct.pack("<L", self.trig_off)
        raw += struct.pack("<L", self.sample_no)
        f.write(raw)

    def unpack_data(self,f):
        self.data = struct.unpack("<%dH" % self.sample_no, f.read(2*self.sample_no))
        self.compute_sig()

    def pack_data(self, f):
        raw = struct.pack("<%dH" % self.sample_no, *self.data)
        f.write(raw)

    def read_header(self, raw):

        # Unpack header data

        # Skip type/subtype/spare0/spare1
        off = 4

        sz = 2
        self.sessionID = struct.unpack("<H", raw[off:off + sz])[0]
        off += sz

        sz = 2
        self.configID = struct.unpack("<H", raw[off:off + sz])[0]
        off += sz

        sz = 1
        self.timeSts = struct.unpack("<B", raw[off:off + sz])[0]
        off += sz

        sz = 1
        self.ppsSliceNo = struct.unpack("<B", raw[off:off + sz])[0]
        off += sz

        sz = 1
        self.year = struct.unpack("<B", raw[off:off + sz])[0]
        off += sz

        sz = 1
        self.month = struct.unpack("<B", raw[off:off + sz])[0]
        off += sz

        sz = 1
        self.day = struct.unpack("<B", raw[off:off + sz])[0]
        off += sz

        sz = 1
        self.hh = struct.unpack("<B", raw[off:off + sz])[0]
        off += sz

        sz = 1
        self.mm = struct.unpack("<B", raw[off:off + sz])[0]
        off += sz

        sz = 1
        self.ss = struct.unpack("<B", raw[off:off + sz])[0]
        off += sz

        sz = 4
        self.usec = struct.unpack("<L", raw[off:off + sz])[0]
        off += sz

        sz = 8
        self.tstmp = struct.unpack("<LL", raw[off:off+sz])  # timespec sec.nsec
        off += sz

        sz = 1
        #dummy = struct.unpack("<B", raw[off:off+sz])[0]  # dummy
        off += sz

        sz = 1
        self.eql = struct.unpack("<B", raw[off:off+sz])[0]  # equalization
        off += sz

        sz = 2
        self.dec = struct.unpack("<H", raw[off:off+sz])[0]  # decimation
        off += sz

        sz = 4
        self.curr_off = struct.unpack("<L", raw[off:off+sz])[0]  # current off (end of the acquired data)
        off += sz

        sz = 4
        self.trig_off = struct.unpack("<L", raw[off:off+sz])[0]  # trigger off
        off += sz

        sz = 4
        self.sample_no = struct.unpack("<L", raw[off:off+sz])[0]
        self.sample_to_read = self.sample_no
        self.data = []

        self.compute_time()

    def read_data(self, raw):

        #print("Sample to read %8d" % self.sample_to_read)

        # Number of samples in this block of data
        n = (len(raw)-4)/2

        # Unpack waveform data
        self.data += struct.unpack("<%dH" % n, raw[4:])

        self.sample_to_read -= n
        if self.sample_to_read == 0:
            self.compute_sig() # This is not needed to produce DL0
            return True
        else:
            return False

    def compute_time(self):

        # Sampling time
        self.dt = float(self.dec) * 8e-9

        #print(self.tstmp)

        # Time of the first and last samples
        self.tstart = float(self.tstmp[0]) + float(self.tstmp[1]) * 1e-9
        self.tstop = self.tstart + self.dt * (self.sample_no - 1)

    def compute_sig(self):

        # Reordering
        curr_off = self.curr_off + 1
        if curr_off > self.trig_off:
            post_trig = self.data[self.trig_off:curr_off]
            pre_trig = self.data[curr_off:] + self.data[0:self.trig_off]
        else:
            post_trig = self.data[self.trig_off:] + self.data[0:curr_off]
            pre_trig = self.data[curr_off:self.trig_off]

        self.trigt = len(pre_trig) * self.dt

        self.sigt = np.zeros(self.sample_no)
        self.sigr = np.zeros(self.sample_no)
        count = 0
        for raw in pre_trig + post_trig:
            self.sigt[count] = count * self.dt
            self.sigr[count] = twos_comp_to_int(raw, 14)
            count += 1

        if self.eql == 1:
            self.sige = self.sigr * 2.0 / 16384
        else:
            self.sige = self.sigr * 40.0 / 16384

    def print(self):

        if self.rpID:
            print("RedPitaya ID: %02d" % self.rpID)

        print("      Run ID: %08d" % self.runID)
        print("  Session ID: %08d" % self.sessionID)
        print("  Config. ID: %08d" % self.configID)

        print(" Time status: %02X" % self.timeSts)

        if self.timeSts == 0:
            print("        Year: %02d" % self.year)
            print("       Month: %02d" % self.month)
            print("         Day: %02d" % self.day)
            print("       Hours: %02d" % self.hh)
            print("     Minutes: %02d" % self.mm)
            print("     Seconds: %02d" % self.ss)
            print("Microseconds: %06d" % self.usec)

        print("  Start time: %22.9f [s]" % self.tstart)
        print("   Stop time: %22.9f [s]" % self.tstop)
        print("Equalization:    %6d," % self.eql)
        print("  Decimation:    %6d, %11.9f [s]" % (self.dec, self.dt))
        print("Current off.: %22d" % self.curr_off)
        print("Trigger off.: %22d" % self.trig_off)
        print("  Sample no.: %22d" % self.sample_no)

        if self.trig_off > self.curr_off:
            pre_trig_sz = self.trig_off - self.curr_off
            post_trig_sz = self.sample_no - pre_trig_sz
        else:
            post_trig_sz = self.curr_off - self.trig_off
            pre_trig_sz = self.sample_no -  post_trig_sz

        print(" Pre trig sz: %22d" % pre_trig_sz)
        print("Post trig sz: %22d" % post_trig_sz)


if __name__ == "__main__":

    print("Load GF binary data")
    
    import matplotlib.pyplot as plt
    from matplotlib import rcParams

    wform = Waveform()
    wform.load(fname)

    wform.print()

    rcParams['font.family'] = 'arial'

    fig, ax = plt.subplots(figsize=(12, 4))
    ax.plot(wform.sigt, wform.sige)
    ax.axvline(x=wform.trigt, color="black", linestyle=(0, (5, 5)))
    ax.set_xlabel("Time [s]")
    ax.set_ylabel("Signal [V]")
    plt.grid(True)

    fig = plt.figure(figsize=(12, 4))
    ax = fig.add_gridspec(1, 4)
    ax1 = fig.add_subplot(ax[0, 0:3])
    ax1.plot(wform.sigt, wform.sige)
    ax1.axvline(x=wform.trigt, color="black", linestyle=(0, (5, 5)))
    ax1.set_xlabel("Time [s]")
    ax1.set_ylabel("Signal [V]")
    plt.grid(True)

    ax2 = fig.add_subplot(ax[3])
    cols = ['Report']
    #cells = [["Sample period %5.3e [s]" % wform.dt],["Pre-trigger %5.3e [s]" % wform.trigt],["Post-trigger %5.3e [s]" % (wform.tstop - wform.tstart + wform.dt - wform.trigt)], ["Total time %5.3e [s]" % (wform.tstop - wform.tstart +  wform.dt)], ["Min. %10.6f [V]" % np.min(wform.sige)], ["Max. %10.6f [V]" % np.max(wform.sige)]]
    cells = [["Sample period", "%5.3e" % wform.dt, "[s]"], ["Pre-trigger", "%5.3e" % wform.trigt, "[s]"],
             ["Post-trigger", "%5.3e" % (wform.tstop - wform.tstart + wform.dt - wform.trigt), "[s]"],
             ["Total time", "%5.3e" % (wform.tstop - wform.tstart + wform.dt), "[s]"],
             ["Min.", "%10.6f" % np.min(wform.sige), "[V]"], ["Max.", "%10.6f" % np.max(wform.sige), "[V]"],["Trigger level", "%10.6f" % np.interp(wform.trigt, wform.sigt, wform.sige), "[V]"]]

    ax2.axis('tight')
    ax2.axis('off')
    table = ax2.table(cellText=cells, colWidths = [0.5,0.5,0.1], loc='upper center',edges='open')
    table.auto_set_font_size(False)
    table.set_fontsize(10)

    #plt.savefig("test.png")

    plt.show()
