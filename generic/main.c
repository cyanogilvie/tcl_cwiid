#include "tclstuff.h"
#include <tcl.h>
#include <inttypes.h>
#include <cwiid.h>
#include <bluetooth/bluetooth.h>

#define NS	"cwiid::"

static int g_testmode = 0;
static Tcl_ThreadDataKey	thread_mesg_callbacks;
static Tcl_ThreadDataKey	thread_static_keys;
typedef int method_handler(cwiid_wiimote_t* handle, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[]);

// Static string Tcl_Objs <<<
struct static_keys {
	int			initialized;
	Tcl_Obj*	type;
	Tcl_Obj*	status;
	Tcl_Obj*	ext_type;
	Tcl_Obj*	none;
	Tcl_Obj*	nunchuk;
	Tcl_Obj*	classic;
	Tcl_Obj*	balance;
	Tcl_Obj*	motionplus;
	Tcl_Obj*	unknown;
	Tcl_Obj*	bdaddr;
	Tcl_Obj*	btclass;
	Tcl_Obj*	name;
	Tcl_Obj*	sec;
	Tcl_Obj*	nsec;
	Tcl_Obj*	mesgs;
	Tcl_Obj*	battery;
	Tcl_Obj*	btn;
	Tcl_Obj*	buttons;
	Tcl_Obj*	acc;
	Tcl_Obj*	stick;
	Tcl_Obj*	l_stick;
	Tcl_Obj*	r_stick;
	Tcl_Obj*	l;
	Tcl_Obj*	r;
	Tcl_Obj*	right_top;
	Tcl_Obj*	right_bottom;
	Tcl_Obj*	left_top;
	Tcl_Obj*	left_bottom;
	Tcl_Obj*	x;
	Tcl_Obj*	y;
	Tcl_Obj*	z;
	Tcl_Obj*	phi;
	Tcl_Obj*	theta;
	Tcl_Obj*	psi;
	Tcl_Obj*	angle_rate;
	Tcl_Obj*	low_speed;
	Tcl_Obj*	error;
	Tcl_Obj*	disconnect;
	Tcl_Obj*	comm;
	Tcl_Obj*	ir;
	Tcl_Obj*	sources;
};
//>>>
static void init_static_keys(struct static_keys* keys) //<<<
{
#define MAKE_KEY(name, str) \
	Tcl_IncrRefCount((keys->name = Tcl_NewStringObj(str, -1)));

	MAKE_KEY(type, "type");
	MAKE_KEY(status, "status");
	MAKE_KEY(ext_type, "ext_type");
	MAKE_KEY(none, "none");
	MAKE_KEY(nunchuk, "nunchuk");
	MAKE_KEY(classic, "classic");
	MAKE_KEY(balance, "balance");
	MAKE_KEY(motionplus, "motionplus");
	MAKE_KEY(unknown, "unknown");
	MAKE_KEY(bdaddr, "bdaddr");
	MAKE_KEY(btclass, "btclass");
	MAKE_KEY(name, "name");
	MAKE_KEY(sec, "sec");
	MAKE_KEY(nsec, "nsec");
	MAKE_KEY(mesgs, "mesgs");
	MAKE_KEY(battery, "battery");
	MAKE_KEY(btn, "btn");
	MAKE_KEY(buttons, "buttons");
	MAKE_KEY(acc, "acc");
	MAKE_KEY(stick, "stick");
	MAKE_KEY(l_stick, "l_stick");
	MAKE_KEY(r_stick, "r_stick");
	MAKE_KEY(l, "l");
	MAKE_KEY(r, "r");
	MAKE_KEY(right_top, "right_top");
	MAKE_KEY(right_bottom, "right_bottom");
	MAKE_KEY(left_top, "left_top");
	MAKE_KEY(left_bottom, "left_bottom");
	MAKE_KEY(x, "x");
	MAKE_KEY(y, "y");
	MAKE_KEY(z, "z");
	MAKE_KEY(phi, "phi");
	MAKE_KEY(theta, "theta");
	MAKE_KEY(psi, "psi");
	MAKE_KEY(angle_rate, "angle_rate");
	MAKE_KEY(low_speed, "low_speed");
	MAKE_KEY(error, "error");
	MAKE_KEY(disconnect, "disconnect");
	MAKE_KEY(comm, "comm");
	MAKE_KEY(ir, "ir");
	MAKE_KEY(sources, "sources");
	keys->initialized = 1;
}

//>>>

/*
#define EVAL(args ...) _eval(args, NULL)
static int _eval(Tcl_Interp* interp, int flags, ...) //<<<
{
	Tcl_Obj*	cmd = Tcl_NewObj();
	Tcl_Obj*	obj;
	int			res;
	va_list		ap;

	va_start(ap, flags);
	obj = va_arg(ap, Tcl_Obj*);
	while (obj) {
		TEST_OK(Tcl_ListObjAppendElement(interp, cmd, obj));
		obj = va_arg(ap, Tcl_Obj*);
	}
	va_end(ap);

	Tcl_IncrRefCount(cmd);
	res = Tcl_EvalObjEx(interp, cmd, flags || TCL_EVAL_DIRECT);
	Tcl_DecrRefCount(cmd);
	return res;
}

//>>>
*/

static int Get_bdaddrFromObj(interp, obj, bdaddr) //<<<
	Tcl_Interp*		interp;
	Tcl_Obj*		obj;
	bdaddr_t*		bdaddr;
{
	if (str2ba(Tcl_GetString(obj), bdaddr) != 0)
		THROW_ERROR("Expecting a bluetooth address like 00:21:BD:26:FC:47, got: \"",
				Tcl_GetString(obj), "\"");
	return TCL_OK;
}

//>>>
static int Get_flagsFromObj(interp, obj, flags) //<<<
	Tcl_Interp*		interp;
	Tcl_Obj*		obj;
	int*			flags;
{
	int			oc, i, index;
	Tcl_Obj**	ov;
	static const char* flag_names[] = {
		"MESG_IFC",
		"CONTINUOUS",
		"REPEAT_BTN",
		"NONBLOCK",
		"MOTIONPLUS",
		NULL
	};
	int map[] = {
		CWIID_FLAG_MESG_IFC,
		CWIID_FLAG_CONTINUOUS,
		CWIID_FLAG_REPEAT_BTN,
		CWIID_FLAG_NONBLOCK,
		CWIID_FLAG_MOTIONPLUS
	};

	TEST_OK(Tcl_ListObjGetElements(interp, obj, &oc, &ov));

	*flags = 0;
	for (i=0; i<oc; i++) {
		TEST_OK(Tcl_GetIndexFromObj(interp, ov[i], flag_names, "flag", TCL_EXACT, &index));
		*flags |= map[index];
	}

	return TCL_OK;
}

//>>>
static void close_handle(ClientData cdata) //<<<
{
	cwiid_wiimote_t*	handle = (cwiid_wiimote_t*)cdata;
	if (cwiid_close(handle) != 0) {
		fprintf(stderr, "Error closing cwiid handle\n");
	} else if (g_testmode) {
		fprintf(stderr, "Closed cwiid handle\n");
	}
}

//>>>
static int method_get_id(cwiid_wiimote_t* handle, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[]) //<<<
{
	CHECK_ARGS(1, "get_id");

	Tcl_SetObjResult(interp, Tcl_NewIntObj(cwiid_get_id(handle)));

	return TCL_OK;
}

//>>>
static int method_enable(cwiid_wiimote_t* handle, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[]) //<<<
{
	int		flags;

	CHECK_ARGS(2, "enable flags");

	TEST_OK(Get_flagsFromObj(interp, objv[2], &flags));

	if (cwiid_enable(handle, flags) != 0)
		THROW_ERROR("Error setting flags");

	return TCL_OK;
}

//>>>
static int method_disable(cwiid_wiimote_t* handle, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[]) //<<<
{
	int flags;

	CHECK_ARGS(2, "disable flags");

	TEST_OK(Get_flagsFromObj(interp, objv[2], &flags));

	if (cwiid_disable(handle, flags) != 0)
		THROW_ERROR("Error clearing flags");

	return TCL_OK;
}

//>>>
static Tcl_Obj* decode_mesg(union cwiid_mesg* mesg) //<<<
{
	Tcl_Obj*	m = Tcl_NewObj();
	Tcl_Obj*	v;
	int			i;
	struct static_keys* keys = Tcl_GetThreadData(&thread_static_keys, sizeof(*keys));

	switch (mesg->type) {
		case CWIID_MESG_STATUS:
			Tcl_DictObjPut(NULL, m, keys->type, keys->status);
			Tcl_DictObjPut(NULL, m, keys->battery, Tcl_NewIntObj(mesg->status_mesg.battery));
			switch (mesg->status_mesg.ext_type) {
				case CWIID_EXT_NONE:       v = keys->none;       break;
				case CWIID_EXT_NUNCHUK:    v = keys->nunchuk;    break;
				case CWIID_EXT_CLASSIC:    v = keys->classic;    break;
				case CWIID_EXT_BALANCE:    v = keys->balance;    break;
				case CWIID_EXT_MOTIONPLUS: v = keys->motionplus; break;
				default:                   v = keys->unknown;    break;
			}
			Tcl_DictObjPut(NULL, m, keys->ext_type, v);
			break;

		case CWIID_MESG_BTN:
			Tcl_DictObjPut(NULL, m, keys->type, keys->btn);
			Tcl_DictObjPut(NULL, m, keys->buttons, Tcl_NewIntObj(mesg->btn_mesg.buttons));
			break;

		case CWIID_MESG_ACC:
			Tcl_DictObjPut(NULL, m, keys->type, keys->acc);
			Tcl_DictObjPut(NULL, m, keys->x, Tcl_NewIntObj(mesg->acc_mesg.acc[CWIID_X]));
			Tcl_DictObjPut(NULL, m, keys->y, Tcl_NewIntObj(mesg->acc_mesg.acc[CWIID_Y]));
			Tcl_DictObjPut(NULL, m, keys->z, Tcl_NewIntObj(mesg->acc_mesg.acc[CWIID_Z]));
			break;

		case CWIID_MESG_IR:
			Tcl_DictObjPut(NULL, m, keys->type, keys->ir);
			v = Tcl_NewObj();
			for (i = 0; i < CWIID_IR_SRC_COUNT; i++) {
				if (mesg->ir_mesg.src[i].valid) {
					Tcl_ListObjAppendElement(NULL, v, Tcl_NewIntObj(i));
					Tcl_ListObjAppendElement(NULL, v, Tcl_NewIntObj(mesg->ir_mesg.src[i].pos[CWIID_X]));
					Tcl_ListObjAppendElement(NULL, v, Tcl_NewIntObj(mesg->ir_mesg.src[i].pos[CWIID_Y]));
				}
			}
			Tcl_DictObjPut(NULL, m, keys->sources, v);
			break;

		case CWIID_MESG_NUNCHUK:
			Tcl_DictObjPut(NULL, m, keys->type, keys->nunchuk);
			Tcl_DictObjPut(NULL, m, keys->buttons, Tcl_NewIntObj(mesg->nunchuk_mesg.buttons));
			v = Tcl_NewObj();
			Tcl_DictObjPut(NULL, v, keys->x, Tcl_NewIntObj(mesg->nunchuk_mesg.stick[CWIID_X]));
			Tcl_DictObjPut(NULL, v, keys->y, Tcl_NewIntObj(mesg->nunchuk_mesg.stick[CWIID_Y]));
			Tcl_DictObjPut(NULL, m, keys->stick, v);
			v = Tcl_NewObj();
			Tcl_DictObjPut(NULL, v, keys->x, Tcl_NewIntObj(mesg->nunchuk_mesg.stick[CWIID_X]));
			Tcl_DictObjPut(NULL, v, keys->y, Tcl_NewIntObj(mesg->nunchuk_mesg.stick[CWIID_Y]));
			Tcl_DictObjPut(NULL, v, keys->z, Tcl_NewIntObj(mesg->nunchuk_mesg.stick[CWIID_Z]));
			Tcl_DictObjPut(NULL, m, keys->acc, v);
			break;

		case CWIID_MESG_CLASSIC:
			Tcl_DictObjPut(NULL, m, keys->type, keys->classic);
			Tcl_DictObjPut(NULL, m, keys->buttons, Tcl_NewIntObj(mesg->classic_mesg.buttons));
			v = Tcl_NewObj();
			Tcl_DictObjPut(NULL, v, keys->x, Tcl_NewIntObj(mesg->classic_mesg.l_stick[CWIID_X]));
			Tcl_DictObjPut(NULL, v, keys->y, Tcl_NewIntObj(mesg->classic_mesg.l_stick[CWIID_Y]));
			Tcl_DictObjPut(NULL, m, keys->l_stick, v);
			v = Tcl_NewObj();
			Tcl_DictObjPut(NULL, v, keys->x, Tcl_NewIntObj(mesg->classic_mesg.r_stick[CWIID_X]));
			Tcl_DictObjPut(NULL, v, keys->y, Tcl_NewIntObj(mesg->classic_mesg.r_stick[CWIID_Y]));
			Tcl_DictObjPut(NULL, m, keys->r_stick, v);
			Tcl_DictObjPut(NULL, m, keys->l, Tcl_NewIntObj(mesg->classic_mesg.l));
			Tcl_DictObjPut(NULL, m, keys->r, Tcl_NewIntObj(mesg->classic_mesg.r));
			break;

		case CWIID_MESG_BALANCE:
			Tcl_DictObjPut(NULL, m, keys->type, keys->balance);
			Tcl_DictObjPut(NULL, m, keys->right_top, Tcl_NewIntObj(mesg->balance_mesg.right_top));
			Tcl_DictObjPut(NULL, m, keys->right_bottom, Tcl_NewIntObj(mesg->balance_mesg.right_bottom));
			Tcl_DictObjPut(NULL, m, keys->left_top, Tcl_NewIntObj(mesg->balance_mesg.left_top));
			Tcl_DictObjPut(NULL, m, keys->left_bottom, Tcl_NewIntObj(mesg->balance_mesg.left_bottom));
			break;

		case CWIID_MESG_MOTIONPLUS:
			Tcl_DictObjPut(NULL, m, keys->type, keys->motionplus);
			v = Tcl_NewObj();
			Tcl_DictObjPut(NULL, v, keys->phi, Tcl_NewIntObj(mesg->motionplus_mesg.angle_rate[CWIID_PHI]));
			Tcl_DictObjPut(NULL, v, keys->theta, Tcl_NewIntObj(mesg->motionplus_mesg.angle_rate[CWIID_THETA]));
			Tcl_DictObjPut(NULL, v, keys->psi, Tcl_NewIntObj(mesg->motionplus_mesg.angle_rate[CWIID_PSI]));
			Tcl_DictObjPut(NULL, m, keys->angle_rate, v);
			v = Tcl_NewObj();
			/*
			Tcl_DictObjPut(NULL, v, keys->x, Tcl_NewIntObj(mesg->motionplus_mesg.low_speed[CWIID_X]));
			Tcl_DictObjPut(NULL, v, keys->y, Tcl_NewIntObj(mesg->motionplus_mesg.low_speed[CWIID_Y]));
			Tcl_DictObjPut(NULL, v, keys->z, Tcl_NewIntObj(mesg->motionplus_mesg.low_speed[CWIID_Z]));
			Tcl_DictObjPut(NULL, m, keys->low_speed, v);
			*/
			break;

		case CWIID_MESG_ERROR:
			Tcl_DictObjPut(NULL, m, keys->type, keys->error);
			switch (mesg->error_mesg.error) {
				case CWIID_ERROR_NONE:       v = keys->none;       break;
				case CWIID_ERROR_DISCONNECT: v = keys->disconnect; break;
				case CWIID_ERROR_COMM:       v = keys->comm;       break;
				default:                     v = keys->unknown;    break;
			}
			Tcl_DictObjPut(NULL, m, keys->error, v);
			break;

		default:
			Tcl_DictObjPut(NULL, m, keys->type, keys->unknown);
			break;
	}

	return m;
}

//>>>
static void _mesg_handler(cwiid_wiimote_t* handle, int mesg_count, union cwiid_mesg mesg[], struct timespec* timestamp) //<<<
{
	int				i;
	Tcl_HashTable*	mesg_callbacks;
	Tcl_Obj*		mesg_list;
	Tcl_HashEntry*	entry;
	Tcl_Obj*		info = Tcl_NewObj();
	struct static_keys*	keys = Tcl_GetThreadData(&thread_static_keys, sizeof(*keys));

	mesg_callbacks = Tcl_GetThreadData(&thread_mesg_callbacks, sizeof(*mesg_callbacks));
	entry = Tcl_FindHashEntry(mesg_callbacks, handle);
	if (entry == NULL) return;

	mesg_list = Tcl_NewObj();
	for (i=0; i<mesg_count; i++) {
		Tcl_ListObjAppendElement(NULL, mesg_list, decode_mesg(&mesg[i]));
	}

	Tcl_DictObjPut(NULL, info, keys->sec, Tcl_NewWideIntObj(timestamp->tv_sec));
	Tcl_DictObjPut(NULL, info, keys->nsec, Tcl_NewWideIntObj(timestamp->tv_nsec));
	Tcl_DictObjPut(NULL, info, keys->mesgs, mesg_list);

	fprintf(stderr, "_mesg_handler: %s\n", Tcl_GetString(info));
	// TODO: queue info mesg somehow
}

//>>>
static int method_set_mesg_callback(cwiid_wiimote_t* handle, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[]) //<<<
{
	int				len, new, res;
	Tcl_HashTable*	mesg_callbacks;
	Tcl_HashEntry*	entry;
	Tcl_Obj*		prev_cb = NULL;

	CHECK_ARGS(2, "set_mesg_callback cb");

	mesg_callbacks = Tcl_GetThreadData(&thread_mesg_callbacks, sizeof(*mesg_callbacks));

	entry = Tcl_CreateHashEntry(mesg_callbacks, handle, &new);
	if (!new) {
		prev_cb = (Tcl_Obj*)Tcl_GetHashValue(entry);
		Tcl_DecrRefCount(prev_cb);
		prev_cb = NULL;
	}

	Tcl_GetStringFromObj(objv[2], &len);
	if (len == 0) {
		// Remove callback
		res = cwiid_set_mesg_callback(handle, NULL);
	} else {
		// Set callback
		Tcl_IncrRefCount(objv[2]);
		Tcl_SetHashValue(entry, objv[2]);
		res = cwiid_set_mesg_callback(handle, _mesg_handler);
	}

	if (res != 0)
		THROW_ERROR("Error setting callback");

	return TCL_OK;
}

//>>>
static int method_get_mesg(cwiid_wiimote_t* handle, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[]) //<<<
{
	int					mesg_count, i;
	union cwiid_mesg*	mesg;
	struct timespec		timestamp;
	Tcl_Obj*			mesg_list;
	Tcl_Obj*			info;
	struct static_keys*	keys = Tcl_GetThreadData(&thread_static_keys, sizeof(*keys));

	CHECK_ARGS(1, "get_mesg");

	if (cwiid_get_mesg(handle, &mesg_count, &mesg, &timestamp))
		THROW_ERROR("Error polling for mesg");

	mesg_list = Tcl_NewObj();
	for (i=0; i<mesg_count; i++) {
		Tcl_ListObjAppendElement(NULL, mesg_list, decode_mesg(&mesg[i]));
	}

	info = Tcl_NewObj();
	Tcl_DictObjPut(NULL, info, keys->sec, Tcl_NewWideIntObj(timestamp.tv_sec));
	Tcl_DictObjPut(NULL, info, keys->nsec, Tcl_NewWideIntObj(timestamp.tv_nsec));
	Tcl_DictObjPut(NULL, info, keys->mesgs, mesg_list);

	Tcl_SetObjResult(interp, info);

	return TCL_OK;
}

//>>>
static int method_set_rpt_mode(cwiid_wiimote_t* handle, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[]) //<<<
{
	int			i, index, oc, mode=0;
	Tcl_Obj**	ov;
	const char*	mode_names[] = {
		"STATUS",
		"BTN",
		"ACC",
		"IR",
		"NUNCHUK",
		"CLASSIC",
		"BALANCE",
		"MOTIONPLUS",
		"EXT",
		NULL
	};
	int modes[] = {
		CWIID_RPT_STATUS,
		CWIID_RPT_BTN,
		CWIID_RPT_ACC,
		CWIID_RPT_IR,
		CWIID_RPT_NUNCHUK,
		CWIID_RPT_CLASSIC,
		CWIID_RPT_BALANCE,
		CWIID_RPT_MOTIONPLUS,
		CWIID_RPT_EXT
	};

	CHECK_ARGS(2, "set_rpt_mode rpt_modes");

	TEST_OK(Tcl_ListObjGetElements(interp, objv[2], &oc, &ov));
	for (i=0; i<oc; i++) {
		TEST_OK(Tcl_GetIndexFromObj(interp, ov[i], mode_names, "mode", TCL_EXACT, &index));
		mode |= modes[index];
	}

	if (cwiid_set_rpt_mode(handle, mode) != 0)
		THROW_ERROR("Error setting report mode");

	return TCL_OK;
}

//>>>
static int method_not_implemented(cwiid_wiimote_t* handle, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[]) //<<<
{
	THROW_ERROR("Not implemented yet");
}

//>>>
static int handle_cmd(cdata, interp, objc, objv) //<<<
	ClientData		cdata;
	Tcl_Interp*		interp;
	int				objc;
	Tcl_Obj *const	objv[];
{
	cwiid_wiimote_t*	handle = (cwiid_wiimote_t*)cdata;
	int		method;
	const char* methods[] = {
		"get_id",
		"set_data",
		"get_data",
		"enable",
		"disable",
		"set_mesg_callback",
		"get_mesg",
		"get_state",
		"get_acc_cal",
		"get_balance_cal",
		"command",
		"set_rpt",
		"request_status",
		"set_led",
		"set_rumble",
		"set_rpt_mode",
		"read",
		"write",
		NULL
	};
	method_handler*	methodtable[] = {
		method_get_id,
		method_not_implemented /* method_set_data */,
		method_not_implemented /* method_get_data */,
		method_enable,
		method_disable,
		method_set_mesg_callback,
		method_get_mesg,
		method_not_implemented /* method_get_state */,
		method_not_implemented /* method_get_acc_cal */,
		method_not_implemented /* method_balance_cal */,
		method_not_implemented /* method_command */,
		method_not_implemented /* method_set_rpt */,
		method_not_implemented /* method_request_status */,
		method_not_implemented /* method_set_led */,
		method_not_implemented /* method_set_rumble */,
		method_set_rpt_mode,
		method_not_implemented /* method_read */,
		method_not_implemented /* method_write */
	};

	if (objc < 2)
		CHECK_ARGS(1, "method ?args ...?");

	TEST_OK(Tcl_GetIndexFromObj(interp, objv[1], methods, "method", TCL_EXACT,
				&method));

	// Poor man's ensemble
	return (methodtable[method])(handle, interp, objc, objv);
}

//>>>
static int glue_find_wiimote(cdata, interp, objc, objv) //<<<
	ClientData		cdata;
	Tcl_Interp*		interp;
	int				objc;
	Tcl_Obj *const	objv[];
{
	bdaddr_t		bdaddr;
	int				timeout, res;

	CHECK_ARGS(2, "bdaddr timeout");

	TEST_OK(Get_bdaddrFromObj(interp, objv[1], &bdaddr));
	TEST_OK(Tcl_GetIntFromObj(interp, objv[2], &timeout));

	res = cwiid_find_wiimote(&bdaddr, timeout);
	// TODO: figure out how this works

	Tcl_SetObjResult(interp, Tcl_NewIntObj(res));

	return TCL_OK;
}

//>>>
static int glue_list_wiimotes(cdata, interp, objc, objv) //<<<
	ClientData		cdata;
	Tcl_Interp*		interp;
	int				objc;
	Tcl_Obj *const	objv[];
{
	int						i, count;
	struct cwiid_bdinfo*	bdinfo;
	Tcl_Obj*				res;

	CHECK_ARGS(0, "");

	res = Tcl_NewObj();

	count = cwiid_get_bdinfo_array(-1, 2, -1, &bdinfo, 0);
	for (i=0; i<count; i++) {
		char	ba_str[18];
		ba2str(&bdinfo[i].bdaddr, ba_str);
		TEST_OK(Tcl_ListObjAppendElement(interp, res, Tcl_NewStringObj(ba_str, -1)));
	}

	Tcl_SetObjResult(interp, res);
	return TCL_OK;
}

//>>>
static int Get_bdinfoflagsFromObj(Tcl_Interp* interp, Tcl_Obj* obj, int* flags) //<<<
{
	int			oc, i, index;
	Tcl_Obj**	ov;
	static const char* flag_names[] = {
		"BT_NO_WIIMOTE_FILTER",
		NULL
	};
	int map[] = {
		BT_NO_WIIMOTE_FILTER
	};

	TEST_OK(Tcl_ListObjGetElements(interp, obj, &oc, &ov));

	flags = 0;
	for (i=0; i<oc; i++) {
		TEST_OK(Tcl_GetIndexFromObj(interp, ov[1], flag_names, "flag", TCL_EXACT,
					&index));
		*flags |= map[index];
	}

	return TCL_OK;
}

//>>>
static int glue_get_bdinfo_array(cdata, interp, objc, objv) //<<<
	ClientData		cdata;
	Tcl_Interp*		interp;
	int				objc;
	Tcl_Obj *const	objv[];
{
	int						i, count, dev_id=-1, timeout=2, flags=0;
	struct cwiid_bdinfo*	bdinfo;
	Tcl_Obj*				res;
	Tcl_Obj*				dev;
	struct static_keys*		keys = Tcl_GetThreadData(&thread_static_keys, sizeof(*keys));

	if (objc < 1 || objc > 4)
		CHECK_ARGS(3, "?dev_id? ?timeout? ?flags?");

	if (objc > 1)
		TEST_OK(Tcl_GetIntFromObj(interp, objv[1], &dev_id));
	if (objc > 2)
		TEST_OK(Tcl_GetIntFromObj(interp, objv[2], &timeout));
	if (objc > 3)
		TEST_OK(Get_bdinfoflagsFromObj(interp, objv[3], &flags));

	if (timeout < 0)
		THROW_ERROR("timeout cannot be negative");

	res = Tcl_NewObj();

	count = cwiid_get_bdinfo_array(-1, 2, -1, &bdinfo, flags);
	for (i=0; i<count; i++) {
		char	ba_str[18];
		ba2str(&bdinfo[i].bdaddr, ba_str);

		dev = Tcl_NewObj();
		TEST_OK(Tcl_DictObjPut(interp, dev, keys->bdaddr, Tcl_NewStringObj(ba_str, -1)));
		TEST_OK(Tcl_DictObjPut(interp, dev, keys->btclass,
					Tcl_ObjPrintf("0x%02X%02X%02X",
						bdinfo[i].btclass[2],
						bdinfo[i].btclass[1],
						bdinfo[i].btclass[0]
					)
		));
		TEST_OK(Tcl_DictObjPut(interp, dev, keys->name, Tcl_NewStringObj(bdinfo[i].name, -1)));

		TEST_OK(Tcl_ListObjAppendElement(interp, res, dev));
	}

	Tcl_SetObjResult(interp, res);
	return TCL_OK;
}

//>>>
static int glue_open(cdata, interp, objc, objv) //<<<
	ClientData		cdata;
	Tcl_Interp*		interp;
	int				objc;
	Tcl_Obj *const	objv[];
{
	static int			handle_seq = 0;
	char				cmd[32];
	cwiid_wiimote_t*	handle;
	int					flags = 0;
	bdaddr_t			bdaddr;
	int					timeout;

	if (objc < 2 || objc > 4)
		CHECK_ARGS(2, "bdaddr ?flags? ?timeout?");

	TEST_OK(Get_bdaddrFromObj(interp, objv[1], &bdaddr));
	if (objc > 2)
		TEST_OK(Get_flagsFromObj(interp, objv[2], &flags));

	if (objc > 3) {
		TEST_OK(Tcl_GetIntFromObj(interp, objv[3], &timeout));
		handle = cwiid_open_timeout(&bdaddr, flags, timeout);
	} else {
		handle = cwiid_open(&bdaddr, flags);
	}

	if (handle == NULL) {
		Tcl_SetErrorCode(interp, "CWIID", "OPEN", NULL);
		THROW_ERROR("Could not open connection");
	}

	sprintf(cmd, "cwiid%d", handle_seq++);
	Tcl_CreateObjCommand(interp, cmd, handle_cmd, handle, close_handle);
	Tcl_SetObjResult(interp, Tcl_NewStringObj(cmd, -1));
	return TCL_OK;
}

//>>>
static int glue_testmode(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[]) //<<<
{
	// Internal: set the test mode (used by the unit tests)
	CHECK_ARGS(1, "enabled");
	TEST_OK(Tcl_GetBooleanFromObj(interp, objv[1], &g_testmode));
	return TCL_OK;
}

//>>>
int Cwiid_Init(Tcl_Interp* interp) //<<<
{
	Tcl_HashTable*	mesg_callbacks;
	struct static_keys*	keys;

	if (Tcl_InitStubs(interp, "8.6", 0) == NULL) return TCL_ERROR;

	mesg_callbacks = Tcl_GetThreadData(&thread_mesg_callbacks, sizeof(*mesg_callbacks));
	keys = Tcl_GetThreadData(&thread_static_keys, sizeof(*keys));
	if (!keys->initialized) {
		init_static_keys(keys);
		Tcl_InitHashTable(mesg_callbacks, TCL_ONE_WORD_KEYS);
	}

	NEW_CMD(NS "find_wiimote", glue_find_wiimote);
	NEW_CMD(NS "list_wiimotes", glue_list_wiimotes);
	NEW_CMD(NS "get_bdinfo_array", glue_get_bdinfo_array);
	NEW_CMD(NS "open", glue_open);

	NEW_CMD(NS "_testmode", glue_testmode);

	TEST_OK(Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION));

	return TCL_OK;
}

//>>>

// vim: foldmethod=marker foldmarker=<<<,>>> ts=4 shiftwidth=4
