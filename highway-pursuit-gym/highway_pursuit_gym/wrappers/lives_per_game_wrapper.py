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
        self._max_lives = max(lives, 1) # Max lives can't be 0 or less
        self._respawns_left = self._max_lives

    def reset(self, seed=None, options: dict = None):
        # Allow respawning while some lives are left
        if(self._respawns_left > 0):
            self._respawns_left -= 1
            new_game = False
        # Otherwise start a new game
        else:
            # -1 because the current reset call counts as a respawn
            self._respawns_left = self._max_lives - 1
            new_game = True

        # Create the options dict if not provided
        if(options == None):
            options = { "new_game" : new_game }
        # Override new_game if not provided in the options, or if not set to True
        elif(not options.get("new_game", False)):
            options["new_game"] = new_game

        return self.env.reset(seed=seed, options=options)