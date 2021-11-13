#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lib/SMSlib.h"
#include "lib/PSGlib.h"
#include "actor.h"
#include "shot.h"
#include "map.h"
#include "score.h"
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
actor timer_label;
actor chain_label;
actor time_over;

score_display timer;
score_display score;
score_display chain;

struct ply_ctl {
	char shot_delay;
	char shot_type;
	char pressed_shot_selection;
	char color;
	char death_delay;
} ply_ctl;

char timer_delay;

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
	SMS_loadBGPalette(tileset_palette_bin);
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

	if (ply_ctl.death_delay) {
		ply_ctl.death_delay--;
	} else {
		if (joy & PORT_A_KEY_2) {
			if (!ply_ctl.shot_delay) {
				if (fire_player_shot()) {
					ply_ctl.shot_delay = player_shot_infos[ply_ctl.shot_type].firing_delay;
				}
			}
		}
		
		if (joy & PORT_A_KEY_1) {
			if (!ply_ctl.pressed_shot_selection) {
				ply_ctl.color = (ply_ctl.color + 1) & 1;
				player.base_tile = ply_ctl.color ? 6 : 2;
				ply_ctl.pressed_shot_selection = 1;			
			}
		} else {
			ply_ctl.pressed_shot_selection = 0;
		}

		if (ply_ctl.shot_delay) ply_ctl.shot_delay--;
	}
}

void draw_player() {
	if (!(ply_ctl.death_delay & 0x08)) draw_actor(&player);
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
				info->base_tile + (ply_ctl.color << 1), info->frame_count);
				
			sht->path = path->steps;
			sht->state = ply_ctl.color;
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

void update_score(actor *enm, actor *sht) {
	// Hit the wrong enemy: reset the chain.
	if (enm->state != sht->state) {
		update_score_display(&chain, 0);
	}
	
	// Shot an enemy of a different color: change the chain color
	if (enm->state != chain_label.state) {
		chain_label.state = enm->state;
		chain_label.base_tile = chain_label.state ? 186 : 180;
	}
	
	increment_score_display(&chain, 1);
	increment_score_display(&score, chain.value);
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
		enm->state = enemy_spawner.type;

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
				update_score(enm, sht);
			}
			
			if (!ply_ctl.death_delay && is_colliding_against_player(enm)) {
				enm->active = 0;
				ply_ctl.death_delay = 120;
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

void init_score() {
	init_actor(&timer_label, 16, 8, 1, 1, 178, 1);
	init_score_display(&timer, 24, 8, 236);
	//update_score_display(&timer, 60);
	update_score_display(&timer, 3);
	timer_delay = 60;
	
	init_score_display(&score, 16, 24, 236);
	init_actor(&chain_label, 16, 40, 3, 1, 180, 1);
	init_score_display(&chain, 16, 56, 236);
}

void handle_score() {
	if (timer_delay) {
		timer_delay--;
	} else {
		if (timer.value) increment_score_display(&timer, -1);
		timer_delay = 60;
	}
}

void draw_score() {
	draw_actor(&timer_label);
	draw_score_display(&timer);

	draw_score_display(&score);

	if (chain.value > 1) {
		draw_actor(&chain_label);
		draw_score_display(&chain);
	}
}

void gameplay_loop() {
	SMS_useFirstHalfTilesforSprites(1);
	SMS_setSpriteMode(SPRITEMODE_TALL);
	SMS_VDPturnOnFeature(VDPFEATURE_HIDEFIRSTCOL);

	SMS_displayOff();
	SMS_loadPSGaidencompressedTiles(sprites_tiles_psgcompr, 0);
	SMS_loadPSGaidencompressedTiles(tileset_tiles_psgcompr, 256);
	load_standard_palettes();

	init_map(level1_bin);
	draw_map_screen();

	SMS_displayOn();
	
	init_actor(&player, 116, PLAYER_BOTTOM - 16, 2, 1, 2, 1);
	player.animation_delay = 20;
	ply_ctl.shot_delay = 0;
	ply_ctl.shot_type = 0;
	ply_ctl.pressed_shot_selection = 0;
	ply_ctl.death_delay = 0;
	
	init_enemies();
	init_player_shots();
	init_score();

	while (timer.value) {	
		handle_player_input();
		handle_enemies();
		handle_player_shots();
		handle_score();
	
		SMS_initSprites();

		draw_player();
		draw_enemies();
		draw_player_shots();
		draw_score();
		
		SMS_finalizeSprites();
		SMS_waitForVBlank();
		SMS_copySpritestoSAT();
		
		// Scroll two lines per frame
		draw_map();		
		draw_map();		
	}
}

void timeover_sequence() {
	init_actor(&time_over, 107, 64, 6, 1, 116, 1);

	while (1) {
		SMS_initSprites();

		draw_actor(&time_over);
		draw_player();
		draw_enemies();
		draw_player_shots();
		draw_score();
		
		SMS_finalizeSprites();
		SMS_waitForVBlank();
		SMS_copySpritestoSAT();
		
		draw_map();		
	}
}

void main() {	
	gameplay_loop();
	timeover_sequence();
}

SMS_EMBED_SEGA_ROM_HEADER(9999,0); // code 9999 hopefully free, here this means 'homebrew'
SMS_EMBED_SDSC_HEADER(0,2, 2021,11,13, "Haroldo-OK\\2021", "Dragon Blaster",
  "A cybernetic shoot-em-up.\n"
  "Made for the SHMUP JAM 2 - Neon - https://itch.io/jam/shmup-jam-2-neon\n"
  "Built using devkitSMS & SMSlib - https://github.com/sverx/devkitSMS");
