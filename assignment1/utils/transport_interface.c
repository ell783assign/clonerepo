#include "transport_interface.h"

int32_t send_data(int32_t dest_fd, int32_t length, unsigned char *buffer)
{
    
}
int32_t receive_response(int32_response);
int32_t receive_data(uint32_t *length, char **data);
int32_t process_get_or_put(uint32_t is_put, unsigned char *file_name);
int32_t process_exec(unsigned char *command_string);

