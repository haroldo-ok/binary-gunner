#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lib/SMSlib.h"
#include "lib/PSGlib.h"
#include "actor.h"
#include "shot.h"
#include "map.h"
#include "data.h"

#define PLAYER_TOP (4)
#define PLAYER_LEFT (8)
#define PLAYER_RIGHT (256 - 16)
#define PLAYER_BOTTOM (SCREEN_H - 16)
#define PLAYER_SPEED (3)	

#define PLAYER_SHOT_SPEED (4)
#define PLAYER_SHOT_MAX (8)
#define FOR_EACH_PLAYER_SHOT(sht) sht = player_shots; for (char shot_index = PLAYER_SHOT_MAX; shot_index; shot_index--, sht++)

#define ENEMY_MAX (3)
#define FOR_EACH_ENEMY(enm) enm = enemies; for (char enemy_index = ENEMY_MAX; enemy_index; enemy_index--, enm++)
#define ENEMY_PATH_MAX (4)

actor player;
actor player_shots[PLAYER_SHOT_MAX];
actor enemies[ENEMY_MAX];

struct ply_ctl {
	char shot_delay;
	char shot_type;
} ply_ctl;

struct enemy_spawner {
	char type;
	char x;
	char flags;
	char delay;
	char next;
	path_step *path;
	char all_dead;
} enemy_spawner;

const path_step *enemy_paths[ENEMY_PATH_MAX] = {
	(path_step *) path1_path, 
	(path_step *) path2_path,
	(path_step *) path3_path,
	(path_step *) path4_path
};

void load_standard_palettes() {
	SMS_loadBGPalette(sprites_palette_bin);
	SMS_loadSpritePalette(sprites_palette_bin);
	SMS_setSpritePaletteColor(0, 0);
}

char fire_player_shot();

void handle_player_input() {
	static unsigned char joy;	
	joy = SMS_getKeysStatus();

	if (joy & PORT_A_KEY_LEFT) {
		if (player.x > PLAYER_LEFT) player.x -= PLAYER_SPEED;
	} else if (joy & PORT_A_KEY_RIGHT) {
		if (player.x < PLAYER_RIGHT) player.x += PLAYER_SPEED;
	}

	if (joy & PORT_A_KEY_UP) {
		if (player.y > PLAYER_TOP) player.y -= PLAYER_SPEED;
	} else if (joy & PORT_A_KEY_DOWN) {
		if (player.y < PLAYER_BOTTOM) player.y += PLAYER_SPEED;
	}

	if (joy & PORT_A_KEY_2) {
		if (!ply_ctl.shot_delay) {
			if (fire_player_shot()) {
				ply_ctl.shot_delay = player_shot_infos[ply_ctl.shot_type].firing_delay;
			}
		}
	}
	
	if (ply_ctl.shot_delay) ply_ctl.shot_delay--;
}

void init_player_shots() {
	static actor *sht;
	
	FOR_EACH_PLAYER_SHOT(sht) {
		sht->active = 0;
	}
}

void handle_player_shots() {
	static actor *sht;
	
	FOR_EACH_PLAYER_SHOT(sht) {
		if (sht->active) {
			move_actor(sht);
			if (sht->y < 0) sht->active = 0;
			if (sht->state == 1 && !sht->state_timer) sht->active = 0;
		}
	}
}

void draw_player_shots() {
	static actor *sht;
	
	FOR_EACH_PLAYER_SHOT(sht) {
		draw_actor(sht);
	}
}

char fire_player_shot() {
	static actor *sht;
	static char shots_to_fire, fired;
	static shot_info *info;
	static path *path;
	
	info = player_shot_infos + ply_ctl.shot_type;
	path = info->paths;
	shots_to_fire = info->length;
	fired = 0;
	
	FOR_EACH_PLAYER_SHOT(sht) {
		if (!sht->active) {
			init_actor(sht, 
				player.x + path->x, player.y + path->y, 
				1, 1, 
				info->base_tile, info->frame_count);
				
			sht->path = path->steps;
			sht->state = 1;
			sht->state_timer = info->life_time;
						
			// Fired something
			fired = 1;
			path++;
			shots_to_fire--;
			if (!shots_to_fire)	return 1;
		}
	}

	// Didn't fire anything
	return fired;
}

actor *check_collision_against_shots(actor *_act) {
	static actor *act, *sht;
	static int act_x, act_y;
	static int sht_x, sht_y;
	
	act = _act;
	act_x = act->x;
	act_y = act->y;
	FOR_EACH_PLAYER_SHOT(sht) {
		if (sht->active) {
			sht_x = sht->x;
			sht_y = sht->y;
			if (sht_x > act_x - 8 && sht_x < act_x + 16 &&
				sht_y > act_y - 16 && sht_y < act_y + 16) {
				return sht;
			}
		}		
	}	
	
	return 0;
}

char is_colliding_against_player(actor *_act) {
	static actor *act;
	static int act_x, act_y;
	
	act = _act;
	act_x = act->x;
	act_y = act->y;
	
	if (player.x > act_x - 12 && player.x < act_x + 12 &&
		player.y > act_y - 12 && player.y < act_y + 12) {
		return 1;
	}
	
	return 0;
}

void init_enemies() {
	static actor *enm;

	enemy_spawner.x = 0;	
	enemy_spawner.delay = 0;
	enemy_spawner.next = 0;
	
	FOR_EACH_ENEMY(enm) {
		enm->active = 0;
	}
}

void handle_enemies() {
	static actor *enm, *sht;	
	
	if (enemy_spawner.delay) {
		enemy_spawner.delay--;
	} else if (enemy_spawner.next != ENEMY_MAX) {
		if (!enemy_spawner.x) {
			enemy_spawner.type = rand() & 1;
			enemy_spawner.x = 8 + rand() % 124;
			enemy_spawner.flags = 0;
			enemy_spawner.path = enemy_paths[rand() % ENEMY_PATH_MAX];
			if (rand() & 1) {
				enemy_spawner.x += 124;
				enemy_spawner.flags |= PATH_FLIP_X;
			}
		}
		
		enm = enemies + enemy_spawner.next;
		
		init_actor(enm, enemy_spawner.x, 0, 2, 1, enemy_spawner.type ? 132 : 128, 1);
		enm->path_flags = enemy_spawner.flags;
		enm->path = enemy_spawner.path;

		enemy_spawner.delay = 10;
		enemy_spawner.next++;
	}
	
	enemy_spawner.all_dead = 1;
	FOR_EACH_ENEMY(enm) {
		move_actor(enm);
		
		if (enm->x < -32 || enm->x > 287 || enm->y < -16 || enm->y > 192) {
			enm->active = 0;
		}

		if (enm->active) {
			sht = check_collision_against_shots(enm);
			if (sht) {
				sht->active = 0;
				enm->active = 0;
			}
			
			if (is_colliding_against_player(enm)) {
				enm->active = 0;				
			}
		}
		
		if (enm->active) enemy_spawner.all_dead = 0;
	}	
	
	if (enemy_spawner.all_dead) {
		enemy_spawner.x = 0;
		enemy_spawner.next = 0;
	}
}

void draw_enemies() {
	static actor *enm;
	
	FOR_EACH_ENEMY(enm) {
		draw_actor(enm);
	}
}

void main() {	
	SMS_useFirstHalfTilesforSprites(1);
	SMS_setSpriteMode(SPRITEMODE_TALL);
	SMS_VDPturnOnFeature(VDPFEATURE_HIDEFIRSTCOL);

	SMS_displayOff();
	SMS_loadPSGaidencompressedTiles(sprites_tiles_psgcompr, 0);
	load_standard_palettes();

	SMS_displayOn();
	
	init_actor(&player, 116, PLAYER_BOTTOM - 16, 2, 1, 2, 1);
	player.animation_delay = 20;
	ply_ctl.shot_delay = 0;
	ply_ctl.shot_type = 0;
	
	init_enemies();
	init_player_shots();

	while (1) {	
		handle_player_input();
		handle_enemies();
		handle_player_shots();
	
		SMS_initSprites();

		draw_actor(&player);
		draw_enemies();
		draw_player_shots();		
		
		SMS_finalizeSprites();
		SMS_waitForVBlank();
		SMS_copySpritestoSAT();
	}
}

SMS_EMBED_SEGA_ROM_HEADER(9999,0); // code 9999 hopefully free, here this means 'homebrew'
SMS_EMBED_SDSC_HEADER(0,1, 2021,11,07, "Haroldo-OK\\2021", "Dragon Blaster",
  "A cybernetic shoot-em-up.\n"
  "Made for the SHMUP JAM 2 - Neon - https://itch.io/jam/shmup-jam-2-neon\n"
  "Built using devkitSMS & SMSlib - https://github.com/sverx/devkitSMS");
