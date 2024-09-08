import ctypes
import numpy as np
import uuid
import subprocess
from multiprocessing import shared_memory
from highway_pursuit_data import *

def generate_unique_name():
    return str(uuid.uuid1())

class NamedMutex():
    def __init__(self, name):
        self.mutex = ctypes.windll.kernel32.OpenMutexA(1, 0, name.encode('utf-8'))

    def wait(self):
        ctypes.windll.kernel32.WaitForSingleObject(self.mutex, -1)

    def release(self):
        ctypes.windll.kernel32.ReleaseMutex(self.mutex)

class HighwayPursuitClient:
    def __init__(self, launcher_path, highway_pursuit_path, is_real_time = False):
        self.launcher_path = launcher_path
        self.highway_pursuit_path = highway_pursuit_path
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
        self.lock_server_name = generate_unique_name()
        self.lock_client_name = generate_unique_name()

        self.server_info_memory_name = generate_unique_name()
        self.instruction_memory_name = generate_unique_name()
        self.observation_memory_name = generate_unique_name()
        self.info_memory_name = generate_unique_name()
        self.reward_memory_name = generate_unique_name()

        # Start the process
        self._start_process()

        # TODO: how to ensure the process is initialized before connecting mutex/shared memory?

        # Connect mutex, shared memory buffers
        self.lock_server_mutex = NamedMutex(self.lock_server_name)
        self.lock_client_mutex = NamedMutex(self.lock_client_name)

        # This does NOT create the shared memory sections, those are created from C#
        # As such this is not the client's responsibility to unlink them
        self.server_info_sm = shared_memory.SharedMemory(name= self.server_info_memory_name)
        self.instruction_sm = shared_memory.SharedMemory(name=self.instruction_memory_name)
        self.observation_sm = shared_memory.SharedMemory(name=self.observation_memory_name)
        self.info_sm = shared_memory.SharedMemory(name=self.info_memory_name)
        self.reward_sm = shared_memory.SharedMemory(name=self.reward_memory_name)

        # Get server info
        # TODO: should server info be an instruction? or sent by default upon connection?
        self.lock_client_mutex.wait()
        server_info: ServerInfo = ServerInfo.from_buffer_copy(self.server_info_sm.buf)
        self.lock_server_mutex.release()

        return (server_info.obs_width, server_info.obs_height, server_info.obs_channels), server_info.action_count
    
    def _start_process(self):
        # Define the command and its arguments
        command = [
            self.launcher_path,
            self.highway_pursuit_path,
            self.is_real_time,
            self.lock_server_name,
            self.lock_client_name,
            self.server_info_memory_name,
            self.instruction_memory_name,
            self.observation_memory_name,
            self.info_memory_name,
            self.reward_memory_name
        ]
        # Run the command
        self.server_process = subprocess.Popen(command, capture_output=False, text=False)

    def reset(self):
        # return observation, info
        pass
        

    def step(self):
        # return observation, reward, terminated, truncated, info
        pass

    def close(self):
        # Wait for server to be ready
        self.lock_client_mutex.wait()
        # TODO: Write the close instruction

        # Close access to the shared memory sections, the server is responsible for unlinking them
        self.server_info_sm.close()
        self.instruction_sm.close()
        self.observation_sm.close()
        self.info_sm.close()
        self.reward_sm.close()

        # Release the server mutex
        self.lock_server_mutex.release()

        