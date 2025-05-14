import sys, getopt


def crc32_fill_table(poly):
    table = []
    for i in range(256):
        c = i << 24
        for j in range(8):
            if c & 0x80000000:
                c = (c << 1) ^ poly
            else:
                c = c << 1
        c &= 0xFFFFFFFF
        table.append(c)
    return table


def crc32_print_table(table):
    i = 0
    for j in range(64):
        s = " "
        for k in range(4):
            s += "0x%08X" % table[i]
            if k < 3:
                s += ", "
            i += 1
        print(s)


def crc32(crc, buff, table):
    for i in range(len(buff)):
        crc = (crc << 8) ^ table[((crc >> 24) ^ buff[i]) & 255]
        crc = crc & 0xffffffff
    return crc


if __name__ == "__main__":

    print("Compute CRC32")

    ifname = ""

    try:
        opts, args = getopt.getopt(sys.argv[1:], "i:", ["ifname"])
    except getopt.GetoptError:
        print("crc32.py -i <inputfilename>")
        sys.exit(0)

    for opt, arg in opts:
        if opt in ("-i", "--ifname"):
            ifname = arg

    table = crc32_fill_table(0x04C11DB7)
    crc = 0xFFFFFFFF

    print(" Open: %s" % ifname)

    with open(ifname, "rb") as fin:

        while True:
            data = fin.read(8)
            if data:
                crc = crc32(crc, data, table)
            else:
                break

    print("CRC32: 0x%08X" % crc)
    print("End")