#include "tclstuff.h"
#include <tcl.h>
#include <cwiid.h>
#include <inttypes.h>
#include <stdlib.h>

#define NS	"cwiid::"

static int g_testmode = 0;
typedef int method_handler(cwiid_wiimote_t* handle, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[]);

#define EVAL(args) _eval(args, NULL)
static int _eval(Tcl_Interp* interp, int flags, Tcl_Obj* obj, ...) //<<<
{
	Tcl_Obj*	cmd = Tcl_NewObj();

	while (obj)
		TEST_OK(Tcl_ListObjAppendElement(interp, cmd, obj++));

	Tcl_IncrRefCount(cmd);
	res = Tcl_EvalObjEx(interp, cmd, flags || TCL_EVAL_DIRECT);
	Tcl_DecrRefCount(cmd);
	return res;
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

#define NOT_IMPLEMENTED(method) static int method(cwiid_wiimote_t* handle, Tcl_Interp* interp, int objc, Tcl_Obj *const objv) { THROW_ERROR("Not implemented yet"); }

NOT_IMPLEMENTED(method_set_data);
NOT_IMPLEMENTED(method_get_data);
NOT_IMPLEMENTED(method_enable);
NOT_IMPLEMENTED(method_disable);
NOT_IMPLEMENTED(method_set_mesg_callback);
NOT_IMPLEMENTED(method_get_mesg);
NOT_IMPLEMENTED(method_get_state);
NOT_IMPLEMENTED(method_get_acc_cal);
NOT_IMPLEMENTED(method_balance_cal);
NOT_IMPLEMENTED(method_command);
NOT_IMPLEMENTED(method_set_rpt);
NOT_IMPLEMENTED(method_request_status);
NOT_IMPLEMENTED(method_set_led);
NOT_IMPLEMENTED(method_set_rumble);
NOT_IMPLEMENTED(method_set_rpt_mode);
NOT_IMPLEMENTED(method_read);
NOT_IMPLEMENTED(method_write);

static int handle_cmd(cdata, interp, objc, objv) //<<<
	ClientData		cdata;
	Tcl_Interp*		interp;
	int				objc;
	Tcl_Obj *const	objv[];
{
	cwiid_wiimote_t*	handle = (cwiid_wiimote_t*)cdata;
	int		method;
	const char* methods = {
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
	method_handler	methodtable = [
		method_get_id,
		method_set_data,
		method_get_data,
		method_enable,
		method_disable,
		method_set_mesg_callback,
		method_get_mesg,
		method_get_state,
		method_get_acc_cal,
		method_balance_cal,
		method_command,
		method_set_rpt,
		method_request_status,
		method_set_led,
		method_set_rumble,
		method_set_rpt_mode,
		method_read,
		method_write
	];

	if (objc < 2)
		CHECK_ARGS(1, "method ?args ...?");

	TEST_OK(Tcl_GetIndexFromObj(interp, objv[1], methods, "method", TCL_EXACT,
				&method));

	// Poor man's ensemble
	return (methodtable[method])(handle, interp, objc, objv);
}

//>>>
static int Get_bdaddrFromObj(interp, obj, bdaddr) //<<<
	Tcl_Interp*		interp;
	Tcl_Obj*		obj;
	bdaddr_t*		bdaddr;
{
	int			objc, i;
	Tcl_Obj*	objv[];
	long int	part;
	const char*	str;
	char*		endptr;
	int			len;

	TEST_OK(EVAL(
		Tcl_NewStringObj("split", -1), obj, Tcl_NewStringObj(":", 1)
	));

	TEST_OK(Tcl_ListObjGetElements(interp, Tcl_GetObjResult(interp), &objc, &objv));

	if (objc != 6)
		goto badaddr;

	for (i=0; i<6; i++) {
		len = Tcl_GetStringFromObj(objv[i], &len);
		if (len == 0)
			goto badpart;

		part = strtol(str, &endptr, 16);
		if (endptr - str != len || part < 0 || part > 255)
			goto badpart;

		bdaddr->b[i] = (uint8_t)part;
	}

	return TCL_OK;

badaddr:
	THROW_ERROR("Expecting a bluetooth address like 00:21:BD:26:FC:47, got: \"", Tcl_GetString(objv[1]), "\"");

badpart:
	THROW_ERROR("Invalid address part: \"", str, "\"");
}

//>>>
static int Get_flagsFromObj(interp, obj, flags) //<<<
	Tcl_Interp*		interp;
	Tcl_Obj*		obj;
	inti*			flags;
{
	int			oc, i, index;
	Tcl_Obj**	ov;
	static const char* flags[] = {
		"CWIID_FLAG_MESG_IFC",
		"CWIID_FLAG_CONTINUOUS",
		"CWIID_FLAG_REPEAT_BTN",
		"CWIID_FLAG_NONBLOCK",
		NULL
	};
	int map[] = {
		CWIID_FLAG_MESG_IFC,
		CWIID_FLAG_CONTINUOUS,
		CWIID_FLAG_REPEAT_BTN,
		CWIID_FLAG_NONBLOCK
	};

	TEST_OK(Tcl_ListObjGetElements(interp, obj, &oc, &ov));

	flags = 0;
	for (i=0; i<oc; i++) {
		TEST_OK(Tcl_GetIndexFromObj(interp, objv[1], flags, "flag", TCL_EXACT,
					&index));
		*flags |= map[index];
	}

	return TCL_OK;
}

//>>>
static int glue_find_wiimote(cdata, interp, objc, objv) //<<<
{
	bdaddr_t		bdaddr;
	int				timeout, res;

	CHECK_ARGS(2, "bdaddr timeout");

	TEST_OK(Get_bdaddrFromObj(interp, objv[1], &bdaddr));
	TEST_OK(Get_IntFromObj(interp, objv[2], &timeout));

	res = cwiid_find_wiimote(&bdaddr, timeout);
	// TODO: figure out how this works

	Tcl_SetObjResult(interp, Tcl_NewIntObj(res));

	return TCL_OK;
}

//>>>
static int glue_list_wiimotes(cdata, interp, objc, objv) //<<<
{
	int						i, count;
	struct cwiid_bdinfo*	bdinfo;
	Tcl_Obj*				res;

	CHECK_ARGS(0, "");

	res = Tcl_NewObj();

	count = cwiid_get_bdinfo_array(-1, 2, -1, &bdinfo, flags);
	for (i=0; i<count; i++) {
		char	ba_str[18];
		ba2str(bdinfo[i].bdaddr, ba_str);
		TEST_OK(Tcl_ListObjAppendElement(interp, res, Tcl_NewStringObj(ba_str, -1)));
	}
	return TCL_OK;
}

//>>>
static int Get_bdinfoflagsFromObj(Tcl_Interp* interp, Tcl_Obj* obj, int* flags) //<<<
{
	int			oc, i, index;
	Tcl_Obj**	ov;
	static const char* flags[] = {
		"BT_NO_WIIMOTE_FILTER",
		NULL
	};
	int map[] = {
		BT_NO_WIIMOTE_FILTER
	};

	TEST_OK(Tcl_ListObjGetElements(interp, obj, &oc, &ov));

	flags = 0;
	for (i=0; i<oc; i++) {
		TEST_OK(Tcl_GetIndexFromObj(interp, objv[1], flags, "flag", TCL_EXACT,
					&index));
		*flags |= map[index];
	}

	return TCL_OK;
}

//>>>
static int glue_get_bdinfo_array(cdata, interp, objc, objv) //<<<
{
	int						i, count, dev_id=-1, timeout=2, flags=0;
	struct cwiid_bdinfo*	bdinfo;
	Tcl_Obj*				res;
	Tcl_Obj*				dev;
	Tcl_Obj*				key_bdaddr = Tcl_NewStringObj("bdaddr", -1);
	Tcl_Obj*				key_btclass = Tcl_NewStringObj("btclass", -1);
	Tcl_Obj*				key_name = Tcl_NewStringObj("name", -1);

	if (objv < 1 || objc > 4)
		CHECK_ARGS(3, "?dev_id? ?timeout? ?flags?");

	if (objc > 1)
		TEST_OK(Tcl_GetIntFromObj(interp, objv[1], &dev_id));
	if (objc > 2)
		TEST_OK(Tcl_GetIndexFromObj(interp, objv[2], &timeout));
	if (objc > 3)
		TEST_OK(Get_bdinfoflagsFromObj(interp, objv[3], &flags));

	if (timeout < 0)
		THROW_ERROR("timeout cannot be negative");

	res = Tcl_NewObj();

	count = cwiid_get_bdinfo_array(-1, 2, -1, &bdinfo, flags);
	for (i=0; i<count; i++) {
		char	ba_str[18];
		ba2str(bdinfo[i].bdaddr, ba_str);

		dev = Tcl_NewObj();
		TEST_OK(Tcl_DictObjPut(interp, dev, key_bdaddr, Tcl_NewStringObj(ba_str, -1)));
		TEST_OK(Tcl_DictObjPut(interp, dev, key_btclass,
					Tcl_ObjPrintf("0x%02X%02X%02X",
						bdinfo[i].btclass[2],
						bdinfo[i].btclass[1],
						bdinfo[i].btclass[0]
					)
		));
		TEST_OK(Tcl_DictObjPut(interp, dev, key_name, Tcl_NewStringObj(bdinfo[i].name, -1)));

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
	char*				cmd[32];
	cwiid_wiimote_t*	handle;
	int					flags = 0;
	bdaddr_t			bdaddr;

	if (objc < 2 || objc > 3)
		CHECK_ARGS(2, "bdaddr ?flags?");

	TEST_OK(Get_bdaddrFromObj(interp, objv[1], &bdaddr));
	if (objc > 2)
		TEST_OK(Get_flagsFromObj(interp, objv[2], &flags));

	handle = cwiid_open(&bdaddr, flags);
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
	if (Tcl_InitStubs(interp, "8.6", 0) == NULL) return TCL_ERROR;

	NEW_CMD(NS "find_wiimote", glue_find_wiimote);
	NEW_CMD(NS "list_wiimotes", glue_list_wiimotes);
	NEW_CMD(NS "get_bdinfo_array", glue_get_bdinfo_array);
	NEW_CMD(NS "open", glue_open);

	NEW_CMD(NS "_testmode", glue_testmode);

	return TCL_OK;
}

//>>>

// vim: foldmethod=marker foldmarker=<<<,>>> ts=4 shiftwidth=4
