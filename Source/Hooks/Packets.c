#include <Packets.h>
#include <Voxlap.h>
#include <Rendering.h>
#include <Menu.h>
#include <Presence.h>
#include <Aos.h>

ENetPacket* PacketBuffer;
ENetPeer* peer;

extern struct ItemMultitext* LoggerMultitext;

void send_packet(void* packet, size_t size) {
	asm volatile(
		"mov %0, %%esi\n\t"
		"push $0x1\n\t"
		"push %2\n\t"
		"push %1\n\t"
		"mov %%esi, %%eax\n\t"
		"add $0x3e5e0, %%eax\n\t"
		"call *%%eax\n\t"
		"add $0xc, %%esp\n\t"

		"push %%eax\n\t"
		"add $0x38690, %%esi\n\t"
		"call *%%esi\n\t"
		"add $0x4, %%esp"
	:: "r" (client_base), "r" (packet), "g" (size));
}

void send_msg(char *msg) {
	struct packet_chat* msgp = malloc(sizeof(struct packet_chat));
	msgp->packet_id = 17;
	msgp->player_id = *(uint8_t*)(client_base+0x13B1CF0);
	msgp->chat_type = 0;

	strncpy(msgp->msg, msg, strlen(msg));
	msgp->msg[strlen(msg)] = '\0';

	send_packet(msgp, sizeof(msgp)+strlen(msg));
}

void send_ext_info() {
	struct packet_ext_info* extinfop = malloc(sizeof(struct packet_ext_info));
	extinfop->packet_id = 60;
	extinfop->length = 1;

	struct packet_extension* extC = malloc(sizeof(struct packet_extension));
	extC->ext_id = 193;
	extC->version = 1;

	extinfop->packet = *extC;

	send_packet(extinfop, sizeof(extinfop));

	free(extinfop);
	free(extC);
}

void send_client_info() {
	struct packet_client_info* packetV = malloc(sizeof(struct packet_client_info));
	packetV->packet_id = 34;
	packetV->identifier = 68;
	packetV->version_major = 0;
	packetV->version_minor = 1;
	packetV->version_revision = 0;

	HMODULE ntdll = GetModuleHandle("ntdll.dll");

	char *a = "Windows";
	if (ntdll) {
		if (GetProcAddress(ntdll, "wine_get_version"))
			a = "Linux"; // fuck you macos users muahahah
	}

	strncpy(packetV->os, a, strlen(a));

	send_packet(packetV, sizeof(packetV)+strlen(a)+1);

	free(packetV);
}

void send_handshake_back(int challenge) {
	struct packet_handshake_back* packetHandBack = malloc(sizeof(struct packet_handshake_back));
	packetHandBack->packet_id = 32;
	packetHandBack->challenge = challenge;

	send_packet(packetHandBack, sizeof(packetHandBack)+1);
	free(packetHandBack);
}

int packet_handler() {
	//if (PacketBuffer->data[0] != 2)
		//printf("%i\n", PacketBuffer->data[0]);

	// if enabled it will jump to the end of the aos packet handler
	// so we can "override" behaviours
	int skip = 0;
	switch(PacketBuffer->data[0]) {
		case 12:
			{
				trigger_player_count_validation();
				break;
			}
		case 13:
			{
				struct packet_block_action* block_action = (struct packet_block_action*)PacketBuffer->data;

				if (block_action->action_type == 0) {
					long block = getcube(block_action->x, block_action->y, block_action->z);

					if (block != 0) {
						long color = *(long*)(client_base+0x7ce8c+(block_action->player_id*936));

						*(long*)block = color|0x7f000000;
						skip = 1;
					}
				}
			}
			break;
		case 15:
			{
				struct packet_state_data* state_data = (struct packet_state_data*)PacketBuffer->data;
				get_server_info(1, state_data->game_mode_id);
				break;
			}
		case 17:
			{
				uint8_t* buf = (uint8_t*)malloc(PacketBuffer->dataLength*sizeof(uint8_t));
				memcpy(buf, PacketBuffer->data, PacketBuffer->dataLength*sizeof(uint8_t));
				struct packet_chat* p = (struct packet_chat*)buf;

				add_new_text(LoggerMultitext, p->msg);
				if (p->chat_type > 2)
					add_custom_message(p->chat_type, p->msg);

				free(buf);
			}
			break;
		case 18:
			{
				// server doesn't send us the player left for some reason so client
				// gets confused and guess what, players dont get connect, so lets do like
				// all other clients and clean the whole array
				for (int i = 0; i < 32; i++) {
					memset((void*)(client_base+0x7cb70+(i*0x3a8)), 0, 0x3a8);
				}
			}
			break;
		case 20:
			{
				decrement_player_count();
				break;
			}
		case 31:
			{
				struct packet_handshake_back* fds = (struct packet_handshake_back*)PacketBuffer->data;
				send_handshake_back(fds->challenge);
			}
			break;
		case 33:
			send_client_info();
			break;
		case 60:
			send_ext_info();
			break;
	}

	int leaveoffset = client_base+0x343ed;
	if (skip == 1)
		leaveoffset = client_base+0x355f2;

	return leaveoffset;
}

// when handling map and state data, it uses a different packet handler
__declspec(naked) void map_packet_hook() {
	asm volatile("movl %%ebx, %0": "=r" (PacketBuffer));

	packet_handler();

	asm volatile(
		"movl %0, %%esi\n\t"
		"mov %1, %%ecx\n\t"
		"movl 8(%%ebx), %%edx\n\t"
		"movb (%%edx), %%al\n\t"
		"jmp *%%ecx"::"r"(PacketBuffer), "r"(client_base+0x33b17));
}

__declspec(naked) void after_packet_hook() {
	asm volatile("movl %%esi, %0": "=r" (PacketBuffer));

	if (PacketBuffer->data[0] == 13 || PacketBuffer->data[0] == 14)
		update_minimap();

	asm volatile(
		"push %1\n\t"
		"push %0\n\t"
		"pop %%esi\n\t"
		"pop %%edi\n\t"
		"jmp *%%edi"
		::"r"(PacketBuffer), "r"(client_base+0x35614));

}

__declspec(naked) void packet_hook() {
	asm volatile("movl %%esi, %0": "=r" (PacketBuffer));
	asm volatile("movl 40(%%esp), %0": "=r" (peer));

	int leaveoffset = packet_handler();

	asm volatile(
		"movl %0, %%esi\n\t"
		"mov %1, %%ecx\n\t"
		"movl 8(%%esi), %%edx\n\t"
		"movb (%%edx), %%al\n\t"
		"sub %%ebx, %%ebx\n\t" //fix register mess issue with pointers
		"jmp *%%ecx"::"r"(PacketBuffer), "r"(leaveoffset));
}