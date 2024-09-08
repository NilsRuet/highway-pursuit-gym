import ctypes
import numpy as np
import os
import uuid
import subprocess
from multiprocessing import shared_memory
from envs._remote.highway_pursuit_data import *

kernel32 = ctypes.windll.kernel32
global_mutex_prefix = "Global\\"

def generate_unique_name():
    return str(uuid.uuid1())

class Semaphore():
    INFINITE = 0xFFFFFFFF

    def __init__(self, name, initial_count, max_count):
        self._semaphore = kernel32.CreateSemaphoreW(
            None,
            initial_count,
            max_count,
            ctypes.c_wchar_p(name)
        )

    def acquire(self, timeout=INFINITE):
        return ctypes.windll.kernel32.WaitForSingleObject(self._semaphore, timeout)

    def release(self):
        return kernel32.ReleaseSemaphore(self._semaphore, 1, None)
    
    def close(self):
        return kernel32.CloseHandle(self._mutex)

class HighwayPursuitClient:
    def __init__(self, launcher_path, highway_pursuit_path, dll_path, is_real_time = False):
        self.launcher_path = os.path.abspath(launcher_path)
        self.highway_pursuit_path = os.path.abspath(highway_pursuit_path)
        self.dll_path = os.path.abspath(dll_path)
        self.is_real_time = is_real_time

    def create_process_and_connect(self):
        """
        Create the highway pursuit server process and connects to it.
        
        Parameters:
        realTime (bool): if highway pursuit should run in real time (60fps), or in fast forward mode
        
        Returns:
        tuple: observation shape
        int: action count
        """
        # Generate name for mutex and share memory sections
        self.lock_server_name = f"{global_mutex_prefix}{generate_unique_name()}"
        self.lock_client_name = f"{global_mutex_prefix}{generate_unique_name()}"

        self.server_info_memory_name = generate_unique_name()
        self.instruction_memory_name = generate_unique_name()
        self.observation_memory_name = generate_unique_name()
        self.info_memory_name = generate_unique_name()
        self.reward_memory_name = generate_unique_name()

        # Setup the server and the data formats
        self.setup_server()
        return self.observation_shape, self.action_count
    
    def _start_process(self):
        # Define the command and its arguments
        command = [
            self.launcher_path,
            self.highway_pursuit_path,
            self.dll_path,
            str(self.is_real_time),
            self.lock_server_name,
            self.lock_client_name,
            self.server_info_memory_name,
            self.instruction_memory_name,
            self.observation_memory_name,
            self.info_memory_name,
            self.reward_memory_name
        ]

        # Run the command
        result = subprocess.run(command, capture_output=False, text=False)
        # TODO: handle error codes
        # TODO: maybe get the PID of the server back such that it can be killed in case it stops responding
        if(result.returncode != 0):
            raise Exception(f"HighwayPursuit launcher failed with code {result.returncode}")

    def setup_server(self):
        # Create semaphores for synchronization

        # Initially no availability
        self.lock_server_pool = Semaphore(self.lock_server_name, initial_count=0, max_count=1) 
        self.lock_client_pool = Semaphore(self.lock_client_name, initial_count=0, max_count=1)

        # Create the initial shared memory for retrieving server info
        self.server_info_sm = shared_memory.SharedMemory(name= self.server_info_memory_name, size=ctypes.sizeof(ServerInfo), create=True)
        
        # Start the server process
        self._start_process()
        # Notify server that server info is ready to be retrieved, and wait for the operation to complete

        self.lock_server_pool.release()
        self.lock_client_pool.acquire()

        # Retrieve the server info
        server_info: ServerInfo = ServerInfo.from_buffer_copy(self.server_info_sm.buf)
        self.observation_shape = (server_info.obs_width, server_info.obs_height, server_info.obs_channels)
        self.action_count = server_info.action_count
        
        # Create the remaining shared memory
        self.instruction_sm = shared_memory.SharedMemory(name=self.instruction_memory_name, size=ctypes.sizeof(Instruction), create=True)
        self.info_sm = shared_memory.SharedMemory(name=self.info_memory_name, size=ctypes.sizeof(Info), create=True)
        self.reward_sm = shared_memory.SharedMemory(name=self.reward_memory_name, size=ctypes.sizeof(Reward), create=True)

        observation_buffer_size = np.prod(self.observation_shape).item()
        self.observation_sm = shared_memory.SharedMemory(name=self.observation_memory_name, size=observation_buffer_size, create=True)

        # Release the server, it will connect to the shared memory sections
        self.lock_server_pool.release()

        # Lock the client mutex to ensure the server is in wait mode
        self.lock_client_pool.acquire()

    def _write_instruction_unsafe(self, instruction: Instruction):
        """This writes an instruction in the appropriate shared memory, without synchronization"""
        bytes = bytearray(instruction)
        self.instruction_sm.buf[:len(bytes)] = bytes

    def reset(self):
        # TODO:
        # return observation, info
        return None, None

    def step(self, action):
        # TODO:
        # return observation, reward, terminated, truncated, info
        return None, None, True, True, None

    def close(self):
        # Write the close instruction to the instruction buffer
        self._write_instruction_unsafe(Instruction(Instruction.CLOSE))
        self.lock_server_pool.release()

        # Wait for server to acknowledge
        self.lock_client_pool.acquire()

        # At this point, the server shouldn't use any shared resource
        # Clean up everything
        self.server_info_sm.close()
        self.instruction_sm.close()
        self.observation_sm.close()
        self.info_sm.close()
        self.reward_sm.close()

        self.server_info_sm.unlink()
        self.instruction_sm.unlink()
        self.observation_sm.unlink()
        self.info_sm.unlink()
        self.reward_sm.unlink()

        self.lock_client_pool.close()
        self.server_mutex.close()
