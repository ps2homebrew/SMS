
// 0xFABE
OP_0xFABE:
{
	u32 adr, res;
	u32 src, dst;

	neogeo_exit();
}
RET(0)

// 0xFABF
OP_0xFABF:
{
	u32 adr, res;
	u32 src, dst;

	img_display = 1;
	cdrom_load_files();
}
RET(0)

// 0xFAC0
OP_0xFAC0:
{
	u32 adr, res;
	u32 src, dst;

	img_display = 0;
	cdrom_load_files();
}
RET(0)

// 0xFAC1
OP_0xFAC1:
{
	u32 adr, res;
	u32 src, dst;

	neogeo_upload();
}
RET(0)

// 0xFAC2
OP_0xFAC2:
{
	u32 adr, res;
	u32 src, dst;

	neogeo_prio_switch();
}
RET(0)

// 0xFAC3
OP_0xFAC3:
{
	u32 adr, res;
	u32 src, dst;

	neogeo_cdda_control();
}
RET(0)
