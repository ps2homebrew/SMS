#ifndef __VU1_H__
#define __VU1_H__

void init_vu1();
void vu1_upload_code(const u64 *code, u32 addr, u32 len);

#define EXEC_MSCAL  0
#define EXEC_MSCNT  1
#define EXEC_MSCALF 2
void vu1_exec_code(u32 addr, int mode);
void vu1_wait();
void vu1_start();

#define DB_OFF 0
#define DB_ON  1
void vu1_upload_data(const vertex *data, u32 addr, u32 len, int db_mode);
void vu1_send_chain();
void vu1_dumpmicro(u32 addr, u32 len);
void vu1_dumpdata(u32 addr, u32 len);
void vu1_dumpdata_i(u32 addr, u32 len);
void vu1_dumpregs();
void vu1_wait_dma();
void vu1_setup_db(u32 base, u32 offset);
void vu1_upload_data_copy(const vertex *data, u32 addr, u32 len, int db_mode);

#endif
