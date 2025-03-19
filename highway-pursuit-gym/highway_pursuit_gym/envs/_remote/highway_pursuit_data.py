import ctypes

class ServerInfo(ctypes.Structure):
    _fields_ = (
        ('obs_height', ctypes.c_uint),
        ('obs_width', ctypes.c_uint),
        ('obs_channels', ctypes.c_uint),
        ('action_count', ctypes.c_uint)
    )
    
class Instruction(ctypes.Structure):
    RESET_NEW_LIFE = 1
    RESET_NEW_GAME = 2
    STEP = 3
    CLOSE = 0xFF

    _fields_ = (
        ('instruction', ctypes.c_byte),
    )

class Info(ctypes.Structure):
    _fields_ = (
        ('tps', ctypes.c_float),
        ('memory', ctypes.c_float),
        ('server_time', ctypes.c_float),
        ('game_time', ctypes.c_float),
    )

    def to_dict(self):
        return {
                "tps": self.tps,
                "memory_usage": self.memory,
                "server_time": self.server_time,
                "game_time": self.game_time
            }

class Reward(ctypes.Structure):
    _fields_ = (
        ('reward', ctypes.c_float),
    )

class Termination(ctypes.Structure):
    _fields_ = (
        ('terminated', ctypes.c_byte),
        ('truncated', ctypes.c_byte)
    )

class ReturnCode(ctypes.Structure):
    _fields_ = (
        ('code', ctypes.c_byte),
    )

# Observation struct is not included because its size is dynamic based on the server config