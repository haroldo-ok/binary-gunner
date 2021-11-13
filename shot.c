#include "shot.h"

const path_step default_path[] = {
	{0, -4},
	{-128, -128}
};

const path default_paths[] = {
	{4, -8, 0, default_path}
};


const shot_info player_shot_infos[PLAYER_SHOT_TYPE_COUNT] = {
	{64, 1, 45, 10, 1, default_paths}, // 0
};
