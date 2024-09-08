import ctypes

class ServerInfo(ctypes.Structure):
    _fields_ = (
        ('obs_width', ctypes.c_uint),
        ('obs_height', ctypes.c_uint),
        ('obs_channels', ctypes.c_uint),
        ('action_count', ctypes.c_uint)
    )
    
class Instruction(ctypes.Structure):
    RESET = 1
    STEP = 2
    CLOSE = 3

    _fields_ = (
        ('instruction', ctypes.c_byte),
    )

class Info(ctypes.Structure):
    _fields_ = (
        ('tps', ctypes.c_float),
        ('memory', ctypes.c_float),
    )

class Reward(ctypes.Structure):
    _fields_ = (
        ('reward', ctypes.c_uint),
    )

# Observation struct is not included because its size is dynamic based on the server config