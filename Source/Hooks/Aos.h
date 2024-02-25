#include <windows.h>

extern int client_base;

enum KV6TYPE {
	SPADE_3 = 0,
	BLOCK_3 = 4,
	SEMI_3 = 8,
	SMG_3 = 12,
	SHOTGUN_3 = 16,
	GRENADE_3 = 20,

	SPADE = 24,
	BLOCK = 28,
	SEMI = 32,
	SMG = 36,
	SHOTGUN = 40,
	GRENADE = 44,

	PLAYERHEAD_B = 48,
	PLAYERTORSO_B = 52,
	PLAYERTORSOC_B = 56,
	PLAYERARMS_B = 60,
	PLAYERLEG_B = 64,
	PLAYERLEGC_B = 68,
	PLAYERDEAD_B = 72,

	PLAYERHEAD_G = 76,
	PLAYERTORSO_G = 80,
	PLAYERTORSOC_G = 84,
	PLAYERARMS_G = 88,
	PLAYERLEG_G = 92,
	PLAYERLEGC_G = 96,
	PLAYERDEAD_G = 100,

	INTEL_B = 104,
	INTEL_G = 108,
	CP_B = 112,
	CP_G = 116,
	CP_N = 120,
	SEMITRACER = 124,
	SMGTRACER = 128,
	SHOTGUNTRACER = 132,
	SEMICASING = 136,
	SMGCASING = 140,
	SHOTGUNCASING = 144
};

void load_kv6_files();
void load_world_objects();
void load_player_team_skin(int player_id);
void load_player_weapon_skin(int player_id);
void load_player_skins();
void load_image_files();
void spectator_movement_hook();
void handle_spectate_input();