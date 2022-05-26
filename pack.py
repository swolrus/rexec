def pack_header(mkind, mlength, data):
    if data:
        return struct.pack("III", mkind, mlength, data)
    else:
        return struct.pack("II", mkind, mlength)


# Unpack header byte values
def unpack_header(header, data):
    if data:
        return struct.unpack("III", header)
    else:
        return struct.unpack("II", header)
