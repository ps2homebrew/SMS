#ifndef STREAMS_H
#define STREAMS_H



int streams_sh_start(void);
void streams_sh_stop(void);
//void streams_sh_update(void);

/*int stream_init(const char *name,int default_mixing_level,
		int sample_rate,
		int param,void (*callback)(int param,INT16 *buffer,int length));
*/
int stream_init_multi(int channels, int param,
		      void (*callback) (int param, int16 ** buffer,
					int length));
//void stream_update(int channel,int min_interval);     /* min_interval is in usec */

void streamupdate(int len);
void mixer_set_volume(int channel,int volume);

#endif
