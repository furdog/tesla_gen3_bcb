#include "canary_log_reader.h"

#include <assert.h>
#include <stdio.h>

void canary_print_header()
{
	printf(";CANARY V2.3\n;TIME_us.d  ID       FL L DATA\n");
}

void canary_print_frame(struct canary_log_reader *self)
{
	int i;
			
	printf("%011u %08X %02X %i",
	       self->_frame.timestamp_us, self->_frame.id,
	       self->_frame.flags, self->_frame.len);
	       
	for (i = 0; i < self->_frame.len; i++)
		printf(" %02X", self->_frame.data[i]);
	printf("\n");
}

int main()
{
	int c;
	struct canary_log_reader c_inst;

	FILE *file = fopen("gbt_working_sequence.txt", "r");

	assert(file);
	
	canary_log_reader_init(&c_inst);

	canary_print_header();
	
	c = getc(file);
	while (!feof(file)) {
		enum canary_log_reader_event ev =
					    canary_log_reader_putc(&c_inst, c);
		if (ev ==
		    CANARY_LOG_READER_EVENT_FRAME_READY) {
			canary_print_frame(&c_inst);
		}

		if (ev == CANARY_LOG_READER_EVENT_ERROR) {
			printf("err, state: %i, flags: %i\n",
				c_inst._estate, c_inst._eflags); fflush(0);
		}

		c = getc(file);
	}

	printf("FINISHED, TOTAL_FRAMES: %llu\n", c_inst._total_frames);

	assert(c_inst._total_frames == 3898);

	return 0;
}
