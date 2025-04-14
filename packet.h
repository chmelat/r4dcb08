/*  Declare of global functions */
extern int received_packet (int fd, PACKET *p_RP, int mode);
extern int send_packet (int fd, PACKET *p_SP);
extern void form_packet (uint8_t addr, uint8_t inst, uint8_t *p_data,
                  int len, PACKET *p_Packet);
extern void print_packet(PACKET *p_P);
