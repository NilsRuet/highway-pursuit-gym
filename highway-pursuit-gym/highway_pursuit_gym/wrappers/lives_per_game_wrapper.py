import gymnasium as gym

class LivesPerGameWrapper(gym.Wrapper):
    DEFAULT_LIVES = 3

    def __init__(self, env, lives = DEFAULT_LIVES):
        """
        Wrapper limiting the amount of times the player can respawn in the game, before starting a new one.
        Helps making the env less difficult by replaying the first biomes more often, and limiting ennemy quantity.
        Args:
            env (gym.Env): The environment to wrap.
            lives (int): The amount of retries per game.
        """
        super(LivesPerGameWrapper, self).__init__(env)
        self._max_lives = lives
        self._current_lives = lives

    def reset(self, seed=None, options: dict = None):
        # Allow respawning while some lives are left
        if(self._current_lives > 0):
            self._current_lives -= 1
            new_game = False
        # Otherwise start a new game
        else:
            self._current_lives = self._max_lives
            new_game = True

        # Create the options dict if not provided
        if(options == None):
            options = { "new_game" : new_game }
        # Override new_game if not provided in the options, or if not set to True
        elif(not options.get("new_game", False)):
            options["new_game"] = new_game

        return self.env.reset(seed=seed, options=options)