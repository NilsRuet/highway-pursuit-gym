import ctypes
import numpy as np
import os
import uuid
import subprocess
from multiprocessing import shared_memory
from envs._remote.highway_pursuit_data import *
from envs._remote.shared_names import *

kernel32 = ctypes.windll.kernel32

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
        return kernel32.CloseHandle(self._semaphore)

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

        self._app_resources_id = f"{str(uuid.uuid1())}-"
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
            self._app_resources_id
        ]

        # Run the command
        result = subprocess.run(command, capture_output=False, text=False)
        if(result.returncode != 0):
            raise Exception(f"HighwayPursuit launcher failed with code {result.returncode}")

        # TODO: maybe get the PID of the server back such that it can be killed in case it stops responding

    def setup_server(self):
        # Define shared resources names
        lock_server_name = f"{self._app_resources_id}{server_mutex_id}"
        lock_client_name = f"{self._app_resources_id}{client_mutex_id}"

        server_info_memory_name = f"{self._app_resources_id}{server_info_memory_id}"
        instruction_memory_name = f"{self._app_resources_id}{instruction_memory_id}"
        observation_memory_name = f"{self._app_resources_id}{observation_memory_id}"
        info_memory_name = f"{self._app_resources_id}{info_memory_id}"
        reward_memory_name = f"{self._app_resources_id}{reward_memory_id}" 
        action_memory_name = f"{self._app_resources_id}{action_memory_id}" 
        termination_memory_name = f"{self._app_resources_id}{termination_memory_id}" 

        # Create semaphores for synchronization
        # Initially no availability
        self._lock_server_pool = Semaphore(lock_server_name, initial_count=0, max_count=1) 
        self._lock_client_pool = Semaphore(lock_client_name, initial_count=0, max_count=1)

        # Create the initial shared memory for retrieving server info
        self._server_info_sm = shared_memory.SharedMemory(name=server_info_memory_name, size=ctypes.sizeof(ServerInfo), create=True)
        
        # Start the server process
        self._start_process()

        # Notify server that server info is ready to be retrieved, and wait for the operation to complete
        self._lock_server_pool.release()
        self._lock_client_pool.acquire()

        # Retrieve the server info
        server_info: ServerInfo = ServerInfo.from_buffer_copy(self._server_info_sm.buf)
        self.observation_shape = (server_info.obs_height, server_info.obs_width, server_info.obs_channels)
        self.action_count = server_info.action_count
        
        # Create the remaining shared memory
        self._instruction_sm = shared_memory.SharedMemory(name=instruction_memory_name, size=ctypes.sizeof(Instruction), create=True)
        self._info_sm = shared_memory.SharedMemory(name=info_memory_name, size=ctypes.sizeof(Info), create=True)
        self._reward_sm = shared_memory.SharedMemory(name=reward_memory_name, size=ctypes.sizeof(Reward), create=True)
        self._termination_sm = shared_memory.SharedMemory(name=termination_memory_name, size=ctypes.sizeof(Termination), create=True)

        observation_buffer_size = np.prod(self.observation_shape).item()
        action_buffer_size = self.action_count # one byte per action
        self._observation_sm = shared_memory.SharedMemory(name=observation_memory_name, size=observation_buffer_size, create=True)
        self._action_sm = shared_memory.SharedMemory(name=action_memory_name, size=action_buffer_size, create=True)

        # Release the server, it will connect to the shared memory sections
        self._lock_server_pool.release()

        # Lock the client mutex to ensure the server is in wait mode
        self._lock_client_pool.acquire()

    def reset(self):
        # Query
        self._write_instruction(Instruction(Instruction.RESET))
        self._sync_wait_for_serv()
        # Result
        info: Info = Info.from_buffer_copy(self._info_sm.buf)
        # The copy allow unlinking the data
        observation = self._read_observation()

        return observation, info

    def step(self, action):
        self._write_action(action)
        self._write_instruction(Instruction(Instruction.STEP))
        self._sync_wait_for_serv()

        # return observation, reward, terminated, truncated, info
        observation = self._read_observation()
        reward: Reward = Reward.from_buffer_copy(self._reward_sm.buf)
        info: Info = Info.from_buffer_copy(self._info_sm.buf)
        termination: Termination = Termination.from_buffer_copy(self._termination_sm.buf)
        return observation, reward, termination.terminated, termination.truncated, info

    def _read_observation(self):
        return np.copy(np.ndarray(self.observation_shape, dtype=np.uint8, buffer=self._observation_sm.buf)[:, :, :3])

    def close(self):
        # Write the close instruction to the instruction buffer
        self._write_instruction(Instruction(Instruction.CLOSE))
        self._sync_wait_for_serv()

        # At this point, the server shouldn't use any shared resource
        # Clean up everything
        self._server_info_sm.close()
        self._instruction_sm.close()
        self._observation_sm.close()
        self._info_sm.close()
        self._reward_sm.close()
        self._action_sm.close()
        self._termination_sm.close()

        self._server_info_sm.unlink()
        self._instruction_sm.unlink()
        self._observation_sm.unlink()
        self._info_sm.unlink()
        self._reward_sm.unlink()
        self._action_sm.unlink()
        self._termination_sm.unlink()

        self._lock_client_pool.close()
        self._lock_server_pool.close()

    def _write_action(self, action: np.ndarray):
        # Write an action in the appropriate buffer
        bytes = bytearray(action)
        self._action_sm.buf[:len(bytes)] = bytes

    def _write_instruction(self, instruction: Instruction):
        # Write an instruction in the appropriate buffer
        bytes = bytearray(instruction)
        self._instruction_sm.buf[:len(bytes)] = bytes

    def _sync_wait_for_serv(self):
        self._lock_server_pool.release()
        self._lock_client_pool.acquire()