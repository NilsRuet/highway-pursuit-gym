import ctypes
import numpy as np
import os
import uuid
import subprocess
from multiprocessing import shared_memory
from envs._remote.highway_pursuit_data import *
from envs._remote.shared_names import *
from enum import Enum

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

class ErrorCode(Enum):
    """
    Enum representing errors that can occur during communication.

    Members (int):
        NOT_ACK: Indicates that the connection was unsuccessful (-1).
        ACK: Connection was successful, but no error was written. Most likely an unhandled crash (0).
        NATIVE_ERROR: Indicates a handled but unspecified exception server-side (1).
        CLIENT_TIMEOUT: Indicates that the server perceives the client as unresponsive. (2).
        GAME_TIMEOUT: Indicates that the game took too long to update (3).
        UNSUPPORTED_BACKBUFFER_FORMAT (int): Unsupported pixel format (4).
        UNKNOWN_ACTION: An unknown action was sent to the server (5).
        ENVIRONMENT_NOT_RESET: STEP was called while the environment was either terminated or uninitialized.
    """
    NOT_ACK = -1
    ACK = 0
    NATIVE_ERROR = 1
    CLIENT_TIMEOUT = 2
    GAME_TIMEOUT = 3
    UNSUPPORTED_BACKBUFFER_FORMAT = 4
    UNKNOWN_ACTION = 5,
    ENVIRONMENT_NOT_RESET = 6

class HighwayPursuitClient:
    """
    A client class for interacting with the Highway Pursuit server. Manages the communication between the client and the 
    game environment, including sending actions, receiving observations, and server synchronization.
    """

    SERVER_TIMEOUT = 10000 # timeout in ms

    def __init__(self, launcher_path, highway_pursuit_path, dll_path, options):
        """
        Initializes the env.

        Args:
            launcher_path (str): Path to the launcher executable.
            highway_pursuit_path (str): Path to the highway pursuit executable.
            dll_path (str): Path to the server DLL file.
            options (dict): dict with env options
                - is_real_time (bool): If highway pursuit runs in real time. Defaults to False.
                - frameskip (int): the number of frames to repeat an action for. Defaults to 4.
                - log_dir (str): Directory for storing server logs. If not provided, defaults to a 'logs' folder in the same directory as the DLL path.
        """       
        
        # App and serv dll paths
        self._launcher_path = os.path.abspath(launcher_path)
        self._highway_pursuit_path = os.path.abspath(highway_pursuit_path)
        self._dll_path = os.path.abspath(dll_path)

        # options
        self._options = options

        # Handles for shared memory sections
        self._shared_memory_handles = []

    def _create_shared_memory(self, name, size):
        """
        Creates a shared memory segment and appends it to the shared memory handles list.
        """
        
        sm = shared_memory.SharedMemory(name=name, size=size, create=True)
        self._shared_memory_handles.append(sm)
        return sm

    def create_process_and_connect(self):
        """
        Connects to and starts the server, and returns the observation shape and action count.

        Returns:
            tuple: A tuple containing:
                - observation_shape (tuple): The shape of the observation data.
                - action_count (int): The number of possible actions.
        """
        
        # Generate name for mutex and share memory sections
        self._app_resources_id = f"{str(uuid.uuid1())}-"
        # Setup the server and the data formats
        self._setup_server()
        return self.observation_shape, self.action_count
    
    def _start_process(self):
        """
        Starts the launcher process.
        """
        # Define the command and its arguments
        command = [
            self._launcher_path,
            self._highway_pursuit_path,
            self._dll_path,
            str(self._options["real_time"]),
            str(self._options["frameskip"]),
            self._options["log_dir"],
            self._app_resources_id
        ]

        # Run the command
        result = subprocess.run(command, capture_output=False, text=False)
        if(result.returncode != 0):
            raise Exception(f"HighwayPursuit launcher failed with code {result.returncode}")

    def _name_from_id(self, id):
        """
        Get the shared resource unique name from its id
        """
        return f"{self._app_resources_id}{id}"

    def _setup_server(self):
        """
        Creates the semaphores and shared memory sections for communicating with the server.
        Handles the connection protocol and returns the server info.
        """
        # Define shared resources names
        lock_server_name = self._name_from_id(server_mutex_id)
        lock_client_name = self._name_from_id(client_mutex_id)

        return_code_memory_name = self._name_from_id(return_code_memory_id)
        server_info_memory_name = self._name_from_id(server_info_memory_id)
        instruction_memory_name = self._name_from_id(instruction_memory_id)
        observation_memory_name = self._name_from_id(observation_memory_id)
        info_memory_name = self._name_from_id(info_memory_id)
        reward_memory_name = self._name_from_id(reward_memory_id)
        action_memory_name = self._name_from_id(action_memory_id) 
        termination_memory_name = self._name_from_id(termination_memory_id)

        # Create semaphores for synchronization
        # Initially no availability
        self._lock_server_pool = Semaphore(lock_server_name, initial_count=0, max_count=1) 
        self._lock_client_pool = Semaphore(lock_client_name, initial_count=0, max_count=1)

        # Create the shared memory for the return code
        # Write some value that has to be overwritten by the server to ensure it is initialized
        self._return_code_sm = self._create_shared_memory(name=return_code_memory_name, size=ctypes.sizeof(ReturnCode))
        init_return_code = bytearray(ReturnCode(ErrorCode.NOT_ACK.value))
        self._return_code_sm.buf[:len(init_return_code)] = init_return_code

        # Create the initial shared memory for retrieving server info
        self._server_info_sm = self._create_shared_memory(name=server_info_memory_name, size=ctypes.sizeof(ServerInfo))
        
        # Start the server process
        self._start_process()

        # Notify server that server info is ready to be retrieved, and wait for the operation to complete
        self._sync_wait_for_serv()

        # Retrieve the server info
        server_info: ServerInfo = ServerInfo.from_buffer_copy(self._server_info_sm.buf)
        self.observation_shape = (server_info.obs_height, server_info.obs_width, server_info.obs_channels)
        self.action_count = server_info.action_count
        
        # Create the remaining shared memory
        self._instruction_sm = self._create_shared_memory(name=instruction_memory_name, size=ctypes.sizeof(Instruction))
        self._info_sm = self._create_shared_memory(name=info_memory_name, size=ctypes.sizeof(Info))
        self._reward_sm = self._create_shared_memory(name=reward_memory_name, size=ctypes.sizeof(Reward))
        self._termination_sm = self._create_shared_memory(name=termination_memory_name, size=ctypes.sizeof(Termination))

        observation_buffer_size = np.prod(self.observation_shape).item()
        action_buffer_size = self.action_count # one byte per action
        self._observation_sm = self._create_shared_memory(name=observation_memory_name, size=observation_buffer_size)
        self._action_sm = self._create_shared_memory(name=action_memory_name, size=action_buffer_size)

        # Server will now connect to the shared memory
        self._sync_wait_for_serv()

    def reset(self):
        """
        Requests to reset the environment, waits for the server and retrieves the initial observation.

        Returns:
            tuple: A tuple containing:
                - observation (array-like): The initial observation after the reset.
                - info (dict): Additional environment information.
        """
        
        # Query
        self._write_instruction(Instruction(Instruction.RESET))
        self._sync_wait_for_serv()
        # Result
        info: Info = Info.from_buffer_copy(self._info_sm.buf)
        # The copy allow unlinking the data
        observation = self._read_observation()

        return observation, info.to_dict()

    def step(self, action):
        """
        Performs one step in the environment by sending the given action to the server, retrieves the reward and next observation.

        Args:
            action (any): The action to take in the environment.

        Returns:
            tuple: A tuple containing:
                - observation (array-like): The observation after the step.
                - reward (float): The reward received after the action.
                - terminated (bool): Whether the episode has terminated.
                - truncated (bool): Whether the episode has been truncated.
                - info (dict): Additional environment information.
        """
        self._write_action(action)
        self._write_instruction(Instruction(Instruction.STEP))
        self._sync_wait_for_serv()

        # return observation, reward, terminated, truncated, info
        observation = self._read_observation()
        reward: Reward = Reward.from_buffer_copy(self._reward_sm.buf)
        info: Info = Info.from_buffer_copy(self._info_sm.buf)
        termination: Termination = Termination.from_buffer_copy(self._termination_sm.buf)
        return observation, reward.reward, termination.terminated, termination.truncated, info.to_dict()

    def _read_observation(self):
        """
        Retrieves an observation from the shared memory buffer
        """
        array = np.ndarray(self.observation_shape, dtype=np.uint8, buffer=self._observation_sm.buf)
        array.flags.writeable = False
        return np.copy(array)[:, :, :3]

    def close(self):
        """
        Closes the environment, and cleans up shared memory resources and locks.
        This method ensures that all shared resources are properly released and unlinked.
        """
         
        # Write the close instruction to the instruction buffer
        self._write_instruction(Instruction(Instruction.CLOSE))
        self._sync_wait_for_serv()

        # At this point, the server shouldn't use any shared resource
        # Clean up everything
        for sm in self._shared_memory_handles:
            sm.close()

        for sm in self._shared_memory_handles:
            sm.unlink()

        self._lock_client_pool.close()
        self._lock_server_pool.close()

    def _write_action(self, action: np.ndarray):
        """
        Writes the given action to the shared memory buffer.
        """
        # Write an action in the appropriate buffer
        bytes = bytearray(action)
        self._action_sm.buf[:len(bytes)] = bytes

    def _write_instruction(self, instruction: Instruction):
        """
        Writes the given instruction  to the shared memory buffer.
        """
        # Write an instruction in the appropriate buffer
        bytes = bytearray(instruction)
        self._instruction_sm.buf[:len(bytes)] = bytes

    def _sync_wait_for_serv(self):
        """
        Waits for the server to process the instruction in shared memory.
        """
        self._lock_server_pool.release()
        wait_result = self._lock_client_pool.acquire(HighwayPursuitClient.SERVER_TIMEOUT)
        
        has_server_timed_out = (wait_result != 0)
        error_code = self._get_error()
        if(has_server_timed_out or error_code != ErrorCode.ACK.value):
            message = f"server error - {'TIMEOUT | ' if has_server_timed_out else ''}{ErrorCode(error_code).name}"
            raise Exception(message)

    def _get_error(self):
        """
        Retrieves the error code from the shared memory buffer.
        """
        return_code = ReturnCode.from_buffer_copy(self._return_code_sm.buf)
        return return_code.code