/* run test */

#include "ptl_test.h"

ptl_handle_any_t get_handle(char *val)
{
	ptl_handle_any_t handle;

	if (!strcmp("INVALID", val))
		handle = PTL_INVALID_HANDLE;
	else if (!strcmp("NONE", val))
		handle = PTL_HANDLE_NONE;
	else
		handle = (ptl_handle_any_t)strtoul(val, NULL, 0);

	return handle;
}

ptl_handle_md_t md_get_handle(struct node_info *info, char *val)
{
	int n;

	if (sscanf(val, "[%d]", &n) == 1) {
		if (n < 0 || n >= STACK_SIZE) {
			printf("invalid md, n = %d\n", n);
			return 0xffffffffffffffffULL;
		}
		return info->md_stack[n];
	} else
		return get_handle(val);
}

ptl_handle_eq_t eq_get_handle(struct node_info *info, char *val)
{
	int n;

	if (sscanf(val, "[%d]", &n) == 1) {
		if (n < 0 || n >= STACK_SIZE) {
			printf("invalid eq, n = %d\n", n);
			return 0xffffffffffffffffULL;
		}
		return info->eq_stack[n];
	} else
		return get_handle(val);
}

ptl_handle_ct_t ct_get_handle(struct node_info *info, char *val)
{
	int n;

	if (sscanf(val, "[%d]", &n) == 1) {
		if (n < 0 || n >= STACK_SIZE) {
			printf("invalid eq, n = %d\n", n);
			return 0xffffffffffffffffULL;
		}
		return info->ct_stack[n];
	} else
		return get_handle(val);
}

long get_number(struct node_info *info, char *orig_val)
{
	char *val;
	long num;
	char *tok[4];
	char *save = NULL;
	int i;

	orig_val = strdup(orig_val);
	val = orig_val;

	if (info && val[0] == '$') {
		val++;
		i = 0;
		while((tok[i++] = strtok_r(val, ".", &save))) 
			val = 0;

		if (!strcmp("actual", tok[0])) {
			if (!strcmp("max_pt_index", tok[1])) {
				num = info->actual.max_pt_index;
			} else {
				num = 0;
			}
		} else {
			num = 0;
		}
	} else {
		num = strtol(val, NULL, 0);
	}

	free(orig_val);
	return num;
}

datatype_t get_datatype(ptl_datatype_t type, char *val)
{
	datatype_t num;

	num.u64 = 0;

	switch (type) {
	case PTL_CHAR:
		num.s8 = strtol(val, NULL, 0);
		break;
	case PTL_UCHAR:
		num.u8 = strtoul(val, NULL, 0);
		break;
	case PTL_SHORT:
		num.s16 = strtol(val, NULL, 0);
		break;
	case PTL_USHORT:
		num.u16 = strtoul(val, NULL, 0);
		break;
	case PTL_INT:
		num.s32 = strtol(val, NULL, 0);
		break;
	case PTL_UINT:
		num.u32 = strtoul(val, NULL, 0);
		break;
	case PTL_LONG:
		num.s64 = strtol(val, NULL, 0);
		break;
	case PTL_ULONG:
		num.u64 = strtoul(val, NULL, 0);
		break;
	case PTL_FLOAT:
		num.f = strtof(val, NULL);
		break;
	case PTL_DOUBLE:
		num.d = strtod(val, NULL);
		break;
	default:
		printf("invalid type in get_datatype\n");
		break;
	}

	return num;
}

int get_uid(struct node_info *info, char *val)
{
	if (!strcmp("ANY", val)) return PTL_UID_ANY;
	else return get_number(info, val);
	
}

int get_jid(struct node_info *info, char *val)
{
	if (!strcmp("ANY", val)) return PTL_JID_ANY;
	else return get_number(info, val);
	
}

int get_index(char *val, struct node_info *info)
{
	int index;

	if (!strcmp("MIN", val))
		index = 0;
	else if (!strcmp("MAX", val))
		index = info->actual.max_pt_index - 1;
	else if (!strcmp("BIG", val))
		index = info->actual.max_pt_index;
	else if (!strcmp("ANY", val))
		index = PTL_PT_ANY;
	else if (!strcmp("INVALID", val))
		index = 0x7fffffff;
	else
		index = strtol(val, NULL, 0);

	return index;
}

void *get_ptr(char *val)
{
	void *ptr;

	if (!strcmp("NULL", val))
		ptr = NULL;
	else if (!strcmp("BAD", val))
		ptr = (void *)0x0123;
	else
		ptr = (void *)strtol(val, NULL, 0);

	return ptr;
}

/*
 * push_info
 *	this routine is called in preparation for executing a new operation
 *	it creates a new parameter set that is copied from the previous one
 *	with some operation specific configuration and some defaults
 */
struct node_info *push_info(struct node_info *head, int tok)
{
	int i;
	struct node_info *info;

	info = calloc(1, sizeof *info);
	if (!info) {
		printf("unable to allocate node_info\n");
		return NULL;
	}

	*info = *head;
	info->buf_alloc = 0;

	info->next = head;
	head->prev = info;

	/* defaults */
	info->count = 1;
	info->ret = PTL_OK;
	info->type = PTL_UCHAR;

	/* If token is MD/LE/ME then allocate current largest buffer */
	switch(tok) {
	case NODE_PTL_MD:
	case NODE_PTL_MD_BIND:
	case NODE_PTL_ME:
	case NODE_PTL_ME_APPEND:
	case NODE_PTL_LE:
	case NODE_PTL_LE_APPEND:
		info->buf = calloc(1, info->actual.max_msg_size);
		if (!info) {
			printf("unable to allocate md/me/le buffer\n");
			free(info);
			return NULL;
		}
		info->buf_alloc = 1;
		break;
	default:
		break;
	}
	get_maps();

	for (i = 0; i < 8; i++) {
		info->iov[i].iov_base	= info->buf + 4096*i;
		info->iov[i].iov_len	= 4096;
	}

	info->md.start			= info->buf;
	info->md.length			= info->actual.max_msg_size;
	info->md.options		= 0;

	info->le.start			= info->buf;
	info->le.length			= info->actual.max_msg_size;
	info->le.options		= 0;

	info->me.start			= info->buf;
	info->me.length			= info->actual.max_msg_size;
	info->me.options		= 0;
	info->me.min_free		= 0;

	switch(tok) {
	case NODE_PTL_NI:
	case NODE_PTL_NI_INIT:
		info->ptr = &info->ni_handle;
		info->desired_ptr = &info->desired;
		info->actual_ptr = &info->actual;
		info->desired_map_ptr = &info->desired_map[0];
		info->actual_map_ptr = &info->actual_map[0];
		info->ni_opt = PTL_NI_MATCHING | PTL_NI_PHYSICAL;
		break;
	case NODE_PTL_GET_UID:
                info->ptr = &info->uid;
		break;
	case NODE_PTL_GET_ID:
                info->ptr = &info->id;
		break;
	case NODE_PTL_GET_JID:
                info->ptr = &info->jid;
		break;
	case NODE_PTL_PT:
	case NODE_PTL_PT_ALLOC:
		info->ptr = &info->pt_index;
		info->pt_index = 0;
		info->pt_opt = 0;
		break;
	case NODE_PTL_EQ:
	case NODE_PTL_EQ_ALLOC:
		info->ptr = &info->eq_handle;
		break;
	case NODE_PTL_EQ_GET:
		info->ptr = &info->eq_event;
		break;
	case NODE_PTL_EQ_WAIT:
		info->ptr = &info->eq_event;
		break;
	case NODE_PTL_EQ_POLL:
		info->ptr = &info->eq_event;
		info->which_ptr = &info->which;
		info->which = -1;
		info->eq_event.type = -1;
		break;
	case NODE_PTL_CT:
	case NODE_PTL_CT_ALLOC:
		info->ptr = &info->ct_handle;
		break;
	case NODE_PTL_CT_GET:
		info->ptr = &info->ct_event;
		break;
	case NODE_PTL_CT_WAIT:
		info->ptr = &info->ct_event;
		break;
	case NODE_PTL_MD:
	case NODE_PTL_MD_BIND:
		info->ptr = &info->md_handle;
		info->md.options = 0;
		break;
	case NODE_PTL_LE:
	case NODE_PTL_LE_APPEND:
		info->ptr = &info->le_handle;
		info->le.options = 0;
		break;
	case NODE_PTL_ME:
	case NODE_PTL_ME_APPEND:
		info->ptr = &info->me_handle;
		info->me.options = 0;
		break;
	case NODE_PTL_FETCH:
	case NODE_PTL_TRIG_FETCH:
		info->get_md_handle = info->md_stack[info->next_md - 1];
		info->put_md_handle = info->md_stack[info->next_md - 2];
		break;
	case NODE_PTL_SWAP:
	case NODE_PTL_TRIG_SWAP:
		info->get_md_handle = info->md_stack[info->next_md - 1];
		info->put_md_handle = info->md_stack[info->next_md - 2];
		info->ptr = &info->operand;
		info->atom_op = PTL_SWAP;
		break;
	}

	return info;
}

struct node_info *pop_node(struct node_info *info)
{
	struct node_info *head;

	head = info->next;
	head->prev = NULL;

	if (info->buf_alloc)
		free(info->buf);
	free(info);

	return head;
}

/*
 * set_data
 *	initialize a buffer with repeated numeric values of
 *	a given type
 */
int set_data(datatype_t val, void *data, int type, int length)
{
	uint8_t *p_u8;
	int8_t *p_8;
	uint16_t *p_u16;
	int16_t *p_16;
	uint32_t *p_u32;
	int32_t *p_32;
	uint64_t *p_u64;
	int64_t *p_64;
	float *p_f;
	double *p_d;
	int i;

	switch(type) {
	case PTL_CHAR:
		p_8 = data;
		for (i = 0; i < length; i++, p_8++)
			*p_8 = val.s8;
		break;
	case PTL_UCHAR:
		p_u8 = data;
		for (i = 0; i < length; i++, p_u8++)
			*p_u8 = val.u8;
		break;
	case PTL_SHORT:
		p_16 = data;
		for (i = 0; i < length/2; i++, p_16++)
			*p_16 = val.s16;
		break;
	case PTL_USHORT:
		p_u16 = data;
		for (i = 0; i < length/2; i++, p_u16++)
			*p_u16 = val.u16;
		break;
	case PTL_INT:
		p_32 = data;
		for (i = 0; i < length/4; i++, p_32++)
			*p_32 = val.s32;
		break;
	case PTL_UINT:
		p_u32 = data;
		for (i = 0; i < length/4; i++, p_u32++)
			*p_u32 = val.u32;
		break;
	case PTL_LONG:
		p_64 = data;
		for (i = 0; i < length/8; i++, p_64++)
			*p_64 = val.s64;
		break;
	case PTL_ULONG:
		p_u64 = data;
		for (i = 0; i < length/8; i++, p_u64++)
			*p_u64 = val.u64;
		break;
	case PTL_FLOAT:
		p_f = data;
		for (i = 0; i < length/4; i++, p_f++)
			*p_f = val.f;
		break;
	case PTL_DOUBLE:
		p_d = data;
		for (i = 0; i < length/8; i++, p_d++)
			*p_d = val.d;
		break;
	}

	return 0;
}

int get_attr(struct node_info *info, xmlNode *node)
{
	xmlAttr *attr;
	char *val;
	struct dict_entry *e;
	int set_md_start = 0;
	int set_md_len = 0;
	int set_md_data = 0;
	int set_le_start = 0;
	int set_le_len = 0;
	int set_le_data = 0;
	int set_me_start = 0;
	int set_me_len = 0;
	int set_me_data = 0;

	for (attr = node->properties; attr; attr = attr->next) {
		val = (char *)attr->children->content;
		e = lookup((char *)attr->name);
		if (!e) {
			printf("invalid attr: %s\n", attr->name);
			return 1;
		}

		switch (e->token) {
		case ATTR_RET:
			info->ret = get_ret(val);
			break;
		case ATTR_PTR:
			info->ptr = get_ptr(val);
			break;
		case ATTR_COUNT:
			info->count = get_number(info, val);
			break;

		/* ni */
		case ATTR_IFACE:
			info->iface = strtol(val, NULL, 0);
			break;
		case ATTR_NI_OPT:
			info->ni_opt = get_ni_opt(val);
			break;
		case ATTR_PID:
			info->pid = get_number(info, val);
			break;
		case ATTR_UID:
			info->uid = get_uid(info, val);
			break;
		case ATTR_JID:
			info->jid = get_jid(info, val);
			break;
		case ATTR_DESIRED_PTR:
			info->desired_ptr = get_ptr(val);
			break;
		case ATTR_DESIRED_MAX_ENTRIES:
			info->desired.max_entries = get_number(info, val);
			break;
		case ATTR_DESIRED_MAX_OVER:
			info->desired.max_overflow_entries = get_number(info, val);
			break;
		case ATTR_DESIRED_MAX_MDS:
			info->desired.max_mds = get_number(info, val);
			break;
		case ATTR_DESIRED_MAX_EQS:
			info->desired.max_eqs = get_number(info, val);
			break;
		case ATTR_DESIRED_MAX_CTS:
			info->desired.max_cts = get_number(info, val);
			break;
		case ATTR_DESIRED_MAX_PT_INDEX:
			info->desired.max_pt_index = get_number(info, val);
			break;
		case ATTR_DESIRED_MAX_IOVECS:
			info->desired.max_iovecs = get_number(info, val);
			break;
		case ATTR_DESIRED_MAX_LIST_SIZE:
			info->desired.max_list_size = get_number(info, val);
			break;
		case ATTR_DESIRED_MAX_MSG_SIZE:
			info->desired.max_msg_size = get_number(info, val);
			break;
		case ATTR_DESIRED_MAX_ATOMIC_SIZE:
			info->desired.max_atomic_size = get_number(info, val);
			break;
		case ATTR_ACTUAL_PTR:
			info->actual_ptr = get_ptr(val);
			break;
		case ATTR_MAP_SIZE:
			info->map_size = get_number(info, val);
			break;
		case ATTR_DESIRED_MAP_PTR:
			info->desired_map_ptr = get_ptr(val);
			break;
		case ATTR_ACTUAL_MAP_PTR:
			info->actual_map_ptr = get_ptr(val);
			break;
		case ATTR_NI_HANDLE:
			info->ni_handle = get_handle(val);
			break;

		/* pt */
		case ATTR_PT_OPT:
			info->pt_opt = get_pt_opt(val);
			break;
		case ATTR_PT_INDEX:
			info->pt_index = get_index(val, info);
			break;

		case ATTR_LIST:
			info->list = get_list(val);
			break;

		case ATTR_IOV_BASE:
			info->iov[0].iov_base = get_ptr(val);
			break;

		/* md */
		case ATTR_MD_START:
			info->md.start = get_ptr(val);
			set_md_start++;
			break;
		case ATTR_MD_LENGTH:
			info->md.length = get_number(info, val);
			set_md_len++;
			break;
		case ATTR_MD_OPT:
			info->md.options = get_md_opt(val);
			break;
		case ATTR_MD_HANDLE:
			info->md_handle = md_get_handle(info, val);
			break;
		case ATTR_MD_DATA:
			info->md_data = get_datatype(info->type, val);
			set_md_data++;
			break;

		/* le */
		case ATTR_LE_START:
			info->le.start = get_ptr(val);
			set_le_start++;
			break;
		case ATTR_LE_LENGTH:
			info->le.length = get_number(info, val);
			set_le_len++;
			break;
		case ATTR_LE_OPT:
			info->le.options = get_le_opt(val);
			break;
		case ATTR_LE_DATA:
			info->le_data = get_datatype(info->type, val);
			set_le_data++;
			break;

		/* me */
		case ATTR_ME_START:
			info->me.start = get_ptr(val);
			set_me_start++;
			break;
		case ATTR_ME_LENGTH:
			info->me.length = get_number(info, val);
			set_me_len++;
			break;
		case ATTR_ME_MATCH:
			info->me.match_bits = get_number(info, val);
			break;
		case ATTR_ME_IGNORE:
			info->me.ignore_bits = get_number(info, val);
			break;
		case ATTR_ME_OPT:
			info->me.options = get_me_opt(val);
			break;
		case ATTR_ME_MIN_FREE:
			info->me.min_free = get_number(info, val);
			break;
		case ATTR_ME_DATA:
			info->me_data = get_datatype(info->type, val);
			set_me_data++;
			break;

		case ATTR_TIME:
			info->timeout = get_number(info, val);
			break;
		case ATTR_WHICH_PTR:
			info->which_ptr = get_ptr(val);
			break;

		/* eq */
		case ATTR_EQ_COUNT:
			info->eq_count = get_number(info, val);
			break;
		case ATTR_EQ_HANDLE:
			info->eq_handle = eq_get_handle(info, val);
			break;

		/* ct */
		case ATTR_CT_HANDLE:
			info->ct_handle = ct_get_handle(info, val);
			break;
		case ATTR_CT_EVENT_SUCCESS:
			info->ct_event.success = get_number(info, val);
			break;
		case ATTR_CT_EVENT_FAILURE:
			info->ct_event.failure = get_number(info, val);
			break;
		case ATTR_CT_TEST:
			info->ct_test = get_number(info, val);
			break;

		case ATTR_ATOM_OP:
			info->atom_op = get_atom_op(val);
			break;
		case ATTR_ATOM_TYPE:
			info->type = get_atom_type(val);
			break;
		case ATTR_GET_MD_HANDLE:
			info->get_md_handle = md_get_handle(info, val);
			break;
		case ATTR_PUT_MD_HANDLE:
			info->put_md_handle = md_get_handle(info, val);
			break;
		case ATTR_USER_PTR:
			info->user_ptr = get_ptr(val);
			break;
		case ATTR_TYPE:
			info->type = get_atom_type(val);
			break;
		case ATTR_LENGTH:
			info->length = get_number(info, val);
			break;
		case ATTR_OPERAND:
			info->operand = get_datatype(info->type, val);
			break;
		case ATTR_MATCH:
			info->match = get_number(info, val);
			break;
		case ATTR_LOC_OFFSET:
			info->loc_offset = get_number(info, val);
			break;
		case ATTR_REM_OFFSET:
			info->rem_offset = get_number(info, val);
			break;
		case ATTR_ACK_REQ:
			info->ack_req = get_ack_req(val);
			break;
		case ATTR_THRESHOLD:
			info->threshold = get_number(info, val);
			break;
		case ATTR_HANDLE1:
			info->handle1 = get_handle(val);
			break;
		case ATTR_HANDLE2:
			info->handle2 = get_handle(val);
			break;
		case ATTR_EVENT_TYPE:
			info->eq_event.type = get_event_type(val);
			break;
		}
	}

	if (info->md.options & PTL_IOVEC) {
		if (!set_md_start)
			info->md.start = (void *)&info->iov[0];
		if (!set_md_len)
			info->md.length = 8;
	}

	if (info->le.options & PTL_IOVEC) {
		if (!set_le_start)
			info->le.start = (void *)&info->iov[0];
		if (!set_le_len)
			info->le.length = 8;
	}

	if (info->me.options & PTL_IOVEC) {
		if (!set_me_start)
			info->me.start = (void *)&info->iov[0];
		if (!set_me_len)
			info->me.length = 8;
	}


	if (set_md_data) {
		set_data(info->md_data, info->buf, info->type,
			info->actual.max_msg_size);
	}

	if (set_le_data) {
		set_data(info->le_data, info->buf, info->type,
			info->actual.max_msg_size);
	}

	if (set_me_data) {
		set_data(info->me_data, info->buf, info->type,
			info->actual.max_msg_size);
	}
	return 0;
}

int check_data(struct node_info *info, char *val, void *data, int type, int length)
{
	uint8_t *p_u8;
	int8_t *p_8;
	uint16_t *p_u16;
	int16_t *p_16;
	uint32_t *p_u32;
	int32_t *p_32;
	uint64_t *p_u64;
	int64_t *p_64;
	float *p_f;
	double *p_d;
	int i;
	datatype_t num;

	num = get_datatype(type, val);

	switch(type) {
	case PTL_CHAR:
		p_8 = data;
		for (i = 0; i < length; i++, p_8++)
			if (*p_8 != num.s8) {
				if (debug)
					printf("check_data char failed expected %x got %x at i = %d\n",
						num.s8, *p_8, i);
				return 1;
			}
		break;
	case PTL_UCHAR:
		p_u8 = data;
		for (i = 0; i < length; i++, p_u8++)
			if (*p_u8 != num.u8) {
				if (debug)
					printf("check_data uchar failed expected %x got %x at i = %d\n",
						num.u8, *p_u8, i);
				return 1;
			}
		break;
	case PTL_SHORT:
		p_16 = data;
		for (i = 0; i < length/2; i++, p_16++)
			if (*p_16 != num.s16) {
				if (debug)
					printf("check_data short failed expected %x got %x at i = %d\n",
						num.s16, *p_16, i);
				return 1;
			}
		break;
	case PTL_USHORT:
		p_u16 = data;
		for (i = 0; i < length/2; i++, p_u16++)
			if (*p_u16 != num.u16) {
				if (debug)
					printf("check_data ushort failed expected %x got %x at i = %d\n",
						num.u16, *p_u16, i);
				return 1;
			}
		break;
	case PTL_INT:
		p_32 = data;
		for (i = 0; i < length/4; i++, p_32++)
			if (*p_32 != num.s32) {
				if (debug)
					printf("check_data int failed expected %x got %x at i = %d\n",
						num.s32, *p_32, i);
				return 1;
			}
		break;
	case PTL_UINT:
		p_u32 = data;
		for (i = 0; i < length/4; i++, p_u32++)
			if (*p_u32 != num.u32) {
				if (debug)
					printf("check_data uint failed expected %x got %x at i = %d\n",
						num.u32, *p_u32, i);
				return 1;
			}
		break;
	case PTL_LONG:
		p_64 = data;
		for (i = 0; i < length/8; i++, p_64++)
			if (*p_64 != num.s64) {
				if (debug)
					printf("check_data long failed "
						"expected %" PRIx64 "got %"
						PRIx64 " at i = %d\n",
						num.s64, *p_64, i);
				return 1;
			}
		break;
	case PTL_ULONG:
		p_u64 = data;
		for (i = 0; i < length/8; i++, p_u64++)
			if (*p_u64 != num.u64) {
				if (debug)
					printf("check_data ulong failed "
						"expected %" PRIx64 " got %"
						PRIx64 " at i = %d\n",
						num.u64, *p_u64, i);
				return 1;
			}
		break;
	case PTL_FLOAT:
		p_f = data;
		for (i = 0; i < length/4; i++, p_f++)
			if (*p_f != num.f) {
				if (debug)
					printf("check_data float failed expected %12.0f got %12.0f at i = %d\n",
						num.f, *p_f, i);
				return 1;
			}
		break;
	case PTL_DOUBLE:
		p_d = data;
		for (i = 0; i < length/8; i++, p_d++)
			if (*p_d != num.d) {
				if (debug)
					printf("check_data double failed expected %22.20lf got %22.20lf at i = %d\n",
						num.d, *p_d, i);
				return 1;
			}
		break;
	default:
		return 1;
	}

	return 0;
}

int check_attr(struct node_info *info, xmlNode *node)
{
	xmlAttr *attr;
	char *val;
	struct dict_entry *e;
	unsigned int offset = 0;
	unsigned int length = 1;
	int type = PTL_UCHAR;

	for (attr = node->properties; attr; attr = attr->next) {
		val = (char *)attr->children->content;
		e = lookup((char *)attr->name);
		if (!e) {
			printf("invalid attr: %s\n", attr->name);
			return 1;
		}

		switch (e->token) {
		case ATTR_THREAD_ID:
			if(info->thread_id != get_number(info, val)) {
				return 1;
			}
			break;
		case ATTR_ACTUAL_MAX_ENTRIES:
			if(info->actual.max_entries != get_number(info, val)) {
				return 1;
			}
			break;
		case ATTR_ACTUAL_MAX_OVER:
			if(info->actual.max_overflow_entries != get_number(info, val)) {
				return 1;
			}
			break;
		case ATTR_ACTUAL_MAX_MDS:
			if(info->actual.max_mds != get_number(info, val)) {
				return 1;
			}
			break;
		case ATTR_ACTUAL_MAX_EQS:
			if(info->actual.max_eqs != get_number(info, val)) {
				return 1;
			}
			break;
		case ATTR_ACTUAL_MAX_CTS:
			if(info->actual.max_cts != get_number(info, val)) {
				return 1;
			}
			break;
		case ATTR_ACTUAL_MAX_IOVECS:
			if(info->actual.max_iovecs != get_number(info, val)) {
				return 1;
			}
			break;
		case ATTR_ACTUAL_MAX_LIST_SIZE:
			if(info->actual.max_list_size != get_number(info, val)) {
				return 1;
			}
			break;
		case ATTR_ACTUAL_MAX_MSG_SIZE:
			if(info->actual.max_msg_size != get_number(info, val)) {
				return 1;
			}
			break;
		case ATTR_ACTUAL_MAX_ATOMIC_SIZE:
			if(info->actual.max_atomic_size != get_number(info, val)) {
				return 1;
			}
			break;
		case ATTR_ACTUAL_MAX_PT_INDEX:
			if(info->actual.max_pt_index != get_number(info, val)) {
				return 1;
			}
			break;
		case ATTR_WHICH:
			if(info->which != get_number(info, val)) {
				return 1;
			}
			break;

		case ATTR_EVENT_TYPE:
			if (info->eq_event.type != get_event_type(val))
				return 1;
			break;
		case ATTR_EVENT_NID:
			if (info->eq_event.initiator.phys.nid != get_number(info, val))
				return 1;
			break;
		case ATTR_EVENT_PID:
			if (info->eq_event.initiator.phys.pid != get_number(info, val))
				return 1;
			break;
		case ATTR_EVENT_RANK:
			if (info->eq_event.initiator.rank != get_number(info, val))
				return 1;
			break;
		case ATTR_EVENT_PT_INDEX:
			if (info->eq_event.pt_index != get_number(info, val))
				return 1;
			break;
		case ATTR_EVENT_UID:
			if (info->eq_event.uid != get_number(info, val))
				return 1;
			break;
		case ATTR_EVENT_JID:
			if (info->eq_event.jid != get_number(info, val))
				return 1;
			break;
		case ATTR_EVENT_MATCH:
			if(info->eq_event.match_bits != get_number(info, val)) {
				return 1;
			}
			break;
		case ATTR_EVENT_RLENGTH:
			if(info->eq_event.rlength != get_number(info, val)) {
				return 1;
			}
			break;
		case ATTR_EVENT_MLENGTH: {
			ptl_size_t mlength;
			if (info->eq_event.type == PTL_EVENT_REPLY ||
			    info->eq_event.type == PTL_EVENT_SEND ||
			    info->eq_event.type == PTL_EVENT_ACK)
				mlength = info->eq_event.mlength;
			else
				mlength = info->eq_event.mlength;
			if(mlength != get_number(info, val)) {
				return 1;
			}
			break;
		}
		case ATTR_EVENT_OFFSET: {
			ptl_size_t offset;
			if (info->eq_event.type == PTL_EVENT_REPLY ||
			    info->eq_event.type == PTL_EVENT_SEND ||
			    info->eq_event.type == PTL_EVENT_ACK)
				offset = info->eq_event.remote_offset;
			else
				offset = info->eq_event.remote_offset;
			if(offset != get_number(info, val)) {
				return 1;
			}
			break;
		}
		case ATTR_EVENT_START:
			if(info->eq_event.start != get_ptr(val)) {
				return 1;
			}
			break;
		case ATTR_EVENT_USER_PTR: {
			void *user_ptr;
			if (info->eq_event.type == PTL_EVENT_REPLY ||
			    info->eq_event.type == PTL_EVENT_SEND ||
			    info->eq_event.type == PTL_EVENT_ACK)
				user_ptr = info->eq_event.user_ptr;
			else
				user_ptr = info->eq_event.user_ptr;
			if(user_ptr != get_ptr(val)) {
				return 1;
			}
			break;
		}
		case ATTR_EVENT_HDR_DATA:
			if(info->eq_event.hdr_data != get_number(info, val)) {
				return 1;
			}
			break;
		case ATTR_EVENT_NI_FAIL: {
			int ni_fail;
			if (info->eq_event.type == PTL_EVENT_REPLY ||
			    info->eq_event.type == PTL_EVENT_SEND ||
			    info->eq_event.type == PTL_EVENT_ACK)
				ni_fail = info->eq_event.ni_fail_type;
			else
				ni_fail = info->eq_event.ni_fail_type;
			if(ni_fail != get_ni_fail(val)) {
				return 1;
			}
			break;
		}
		case ATTR_EVENT_ATOM_OP:
			if(info->eq_event.atomic_operation != get_atom_op(val)) {
				return 1;
			}
			break;
		case ATTR_EVENT_ATOM_TYPE:
			if(info->eq_event.atomic_type != get_number(info, val)) {
				return 1;
			}
			break;
		case ATTR_CT_EVENT_SUCCESS:
			if(info->ct_event.success != get_number(info, val)) {
				return 1;
			}
			break;
		case ATTR_CT_EVENT_FAILURE:
			if(info->ct_event.failure != get_number(info, val)) {
				return 1;
			}
			break;
		case ATTR_LENGTH:
			length = get_number(info, val);
			break;
		case ATTR_OFFSET:
			offset = get_number(info, val);
			break;
		case ATTR_TYPE:
			type = get_atom_type(val);
			break;
		case ATTR_MD_DATA:
			if (check_data(info, val, &info->buf[offset], type, length))
				return 1;
			break;
		case ATTR_LE_DATA:
			if (check_data(info, val, &info->buf[offset], type, length))
				return 1;
			break;
		case ATTR_ME_DATA:
			if (check_data(info, val, &info->buf[offset], type, length))
				return 1;
			break;
		default:
			printf("unexpected check attribute: %s\n", e->name);
			return 1;
		}
	}

	return 0;
}

int walk_tree(struct node_info *head, xmlNode *parent);

static void *run_thread(void *arg)
{
	volatile struct thread_info *t = (volatile struct thread_info *)arg;
	xmlNode *node = t->node;
	struct node_info *info;

	do {
		sched_yield();
	} while(!t->run);

	info = malloc(sizeof(*info));
	if (!info) {
		printf("unable to allocate memory for info\n");
		t->errs = PTL_NO_SPACE;
		return NULL;
	}
	get_maps();
	*info = *t->info;
	info->thread_id = t->num;

	t->errs = walk_tree(info, node);

	free(info);

	return NULL;
}

int walk_tree(struct node_info *info, xmlNode *parent)
{
	xmlNode *node = NULL;
	int errs;
	int tot_errs = 0;
	int i;
	struct dict_entry *e;

	for (node = parent; node; node = node->next) {
		if (node->type == XML_ELEMENT_NODE) {
			errs = 0;
			e = lookup((char *)node->name);
			if (!e) {
				errs = 1;
				printf("invalid node: %s\n", node->name);
				goto done;
			}

			if (debug) printf("start node = %s\n", e->name);

			/* the following cases do not push the stack */
			switch (e->token) {
			case NODE_SET:
				errs = get_attr(info, node);
				goto done;
			case NODE_CHECK:
				errs = check_attr(info, node);
				goto done;
			case NODE_IF:
				if ((info->cond = (check_attr(info, node) == PTL_OK)))
					errs = walk_tree(info, node->children);

				goto done;
			case NODE_ELSE:
				if (!info->cond)
					errs = walk_tree(info, node->children);
				goto done;
			case NODE_DESC:
				printf("%-60s", node->children->content);
				fflush(stdout);
				goto done;
			case NODE_COMMENT:
				goto done;
			}

			info = push_info(info, e->token);
			errs = get_attr(info, node);
			if (errs)
				goto pop;

			switch (e->token) {
			case NODE_TEST:
				errs = walk_tree(info, node->children);
				break;
			case NODE_SUBTEST:
				errs = walk_tree(info, node->children);
				if (errs)
					printf("Errors = %d\n", errs);
				else
					printf("\033[1;32mPassed\033[0m\n");
					//printf("Passed\n");
				break;
			case NODE_REPEAT:
				for (i = 0; i < info->count; i++)
					errs += walk_tree(info, node->children);
				break;
			case NODE_THREADS: {
				volatile struct thread_info *t;
				info->threads = calloc(info->count, sizeof(struct thread_info));

				for (i = 0; i < info->count; i++) {
					t = &info->threads[i];
					t->num = i;
					t->node = node->children;
					t->info = info;
					pthread_create((pthread_t *)&t->thread, NULL, run_thread, (void *)t);
				}

				for (i = 0; i < info->count; i++)
					info->threads[i].run = 1;

				for (i = 0; i < info->count; i++) {
					void *ignore;
					t = &info->threads[i];
					pthread_join(t->thread, &ignore);
					errs += t->errs;
				}

				free((void *)info->threads);
				break;
			}
			case NODE_MSLEEP:
				usleep(1000*info->count);
				break;
			case NODE_DUMP_OBJECTS:
				_dump_type_counts();
				break;
			case NODE_PTL:
				errs = test_ptl_init(info);
				errs += walk_tree(info, node->children);
				errs += test_ptl_fini(info);
				break;
			case NODE_PTL_INIT:
				errs = test_ptl_init(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_FINI:
				errs = test_ptl_fini(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_NI:
				errs = test_ptl_ni_init(info);
				errs += walk_tree(info, node->children);
				errs += test_ptl_ni_fini(info);
				break;
			case NODE_PTL_NI_INIT:
				errs = test_ptl_ni_init(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_NI_FINI:
				errs = test_ptl_ni_fini(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_NI_STATUS:
				errs = test_ptl_ni_status(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_NI_HANDLE:
				errs = test_ptl_ni_handle(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_PT:
				for (i = 0; i < info->count - 1; i++) {
					errs += test_ptl_pt_alloc(info);
					info = push_info(info, e->token);
					errs += get_attr(info, node);
				}
				errs += test_ptl_pt_alloc(info);
				errs += walk_tree(info, node->children);
				errs += test_ptl_pt_free(info);
				for (i = 0; i < info->count - 1; i++) {
					info = pop_node(info);
					errs += test_ptl_pt_free(info);
				}
				break;
			case NODE_PTL_PT_ALLOC:
				errs = test_ptl_pt_alloc(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_PT_FREE:
				errs = test_ptl_pt_free(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_PT_DISABLE:
				errs = test_ptl_pt_disable(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_PT_ENABLE:
				errs = test_ptl_pt_enable(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_GET_UID:
				errs = test_ptl_get_uid(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_GET_ID:
				errs = test_ptl_get_id(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_GET_JID:
				errs = test_ptl_get_jid(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_MD:
				for (i = 0; i < info->count - 1; i++) {
					errs += test_ptl_md_bind(info);
					info = push_info(info, e->token);
					errs += get_attr(info, node);
				}
				errs += test_ptl_md_bind(info);
				errs += walk_tree(info, node->children);
				errs += test_ptl_md_release(info);
				for (i = 0; i < info->count - 1; i++) {
					info = pop_node(info);
					errs += test_ptl_md_release(info);
				}
				break;
			case NODE_PTL_MD_BIND:
				errs = test_ptl_md_bind(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_MD_RELEASE:
				errs = test_ptl_md_release(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_LE:
				for (i = 0; i < info->count - 1; i++) {
					errs += test_ptl_le_append(info);
					info = push_info(info, e->token);
					errs += get_attr(info, node);
				}
				errs += test_ptl_le_append(info);
				errs += walk_tree(info, node->children);
				errs += test_ptl_le_unlink(info);
				for (i = 0; i < info->count - 1; i++) {
					info = pop_node(info);
					errs += test_ptl_le_unlink(info);
				}
				break;
			case NODE_PTL_LE_APPEND:
				errs = test_ptl_le_append(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_LE_UNLINK:
				errs = test_ptl_le_unlink(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_ME:
				for (i = 0; i < info->count - 1; i++) {
					errs += test_ptl_me_append(info);
					info = push_info(info, e->token);
					errs += get_attr(info, node);
				}
				errs += test_ptl_me_append(info);
				errs += walk_tree(info, node->children);
				errs += test_ptl_me_unlink(info);
				for (i = 0; i < info->count - 1; i++) {
					info = pop_node(info);
					errs += test_ptl_me_unlink(info);
				}
				break;
			case NODE_PTL_ME_APPEND:
				errs = test_ptl_me_append(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_ME_UNLINK:
				errs = test_ptl_me_unlink(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_EQ:
				for (i = 0; i < info->count - 1; i++) {
					errs += test_ptl_eq_alloc(info);
					info = push_info(info, e->token);
					errs += get_attr(info, node);
				}
				errs += test_ptl_eq_alloc(info);
				errs += walk_tree(info, node->children);
				errs += test_ptl_eq_free(info);
				for (i = 0; i < info->count - 1; i++) {
					info = pop_node(info);
					errs += test_ptl_eq_free(info);
				}
				break;
			case NODE_PTL_EQ_ALLOC:
				errs = test_ptl_eq_alloc(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_EQ_FREE:
				errs = test_ptl_eq_free(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_EQ_GET:
				errs = test_ptl_eq_get(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_EQ_WAIT:
				errs = test_ptl_eq_wait(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_EQ_POLL:
				errs = test_ptl_eq_poll(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_CT:
				for (i = 0; i < info->count - 1; i++) {
					errs += test_ptl_ct_alloc(info);
					info = push_info(info, e->token);
					errs += get_attr(info, node);
				}
				errs += test_ptl_ct_alloc(info);
				errs += walk_tree(info, node->children);
				errs += test_ptl_ct_free(info);
				for (i = 0; i < info->count - 1; i++) {
					info = pop_node(info);
					errs += test_ptl_ct_free(info);
				}
				break;
			case NODE_PTL_CT_ALLOC:
				errs = test_ptl_ct_alloc(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_CT_FREE:
				errs = test_ptl_ct_free(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_CT_GET:
				errs = test_ptl_ct_get(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_CT_WAIT:
				errs = test_ptl_ct_wait(info);
				errs += walk_tree(info, node->children);
				break;
			//case NODE_PTL_CT_POLL:
				//errs = test_ptl_ct_poll(info);
				//errs += walk_tree(info, node->children);
				//break;
			case NODE_PTL_CT_SET:
				errs = test_ptl_ct_set(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_CT_INC:
				errs = test_ptl_ct_inc(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_PUT:
				errs = test_ptl_put(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_GET:
				errs = test_ptl_get(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_ATOMIC:
				errs = test_ptl_atomic(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_FETCH:
				errs = test_ptl_fetch_atomic(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_SWAP:
				errs = test_ptl_swap(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_TRIG_PUT:
				errs = test_ptl_trig_put(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_TRIG_GET:
				errs = test_ptl_trig_get(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_TRIG_ATOMIC:
				errs = test_ptl_trig_atomic(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_TRIG_FETCH:
				errs = test_ptl_trig_fetch_atomic(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_TRIG_SWAP:
				errs = test_ptl_trig_swap(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_TRIG_CT_INC:
				errs = test_ptl_trig_ct_inc(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_TRIG_CT_SET:
				errs = test_ptl_trig_ct_set(info);
				errs += walk_tree(info, node->children);
				break;
			case NODE_PTL_HANDLE_IS_EQUAL:
				errs = test_ptl_handle_is_eq(info);
				errs += walk_tree(info, node->children);
				break;
			}
pop:
			info = pop_node(info);
done:
			tot_errs += errs;
			if (debug) printf("end node = %s, errs = %d\n", e->name, errs);
		}
	}

	return tot_errs;
}

void set_default_info(struct node_info *info)
{
	memset(info, 0, sizeof(*info));

	info->count				= 1;
	info->ret				= PTL_OK;

	info->iface				= PTL_IFACE_DEFAULT;
	info->pid				= PTL_PID_ANY;
	info->ni_opt				= PTL_NI_MATCHING | PTL_NI_PHYSICAL;
	info->desired.max_entries		= 10;
	info->desired.max_overflow_entries	= 10;
	info->desired.max_mds			= 10;
	info->desired.max_cts			= 10;
	info->desired.max_eqs			= 10;
	info->desired.max_pt_index		= 10;
	info->desired.max_iovecs		= 8;
	info->desired.max_list_size		= 10;
	info->desired.max_msg_size		= 64;
	info->desired.max_atomic_size		= 64;
	info->map_size				= 10;
	info->ni_handle				= PTL_INVALID_HANDLE;

	info->timeout				= 10;	/* msec */
	info->eq_count				= 10;
	info->eq_size				= 1;
	info->eq_handle				= PTL_EQ_NONE;
	info->ct_handle				= PTL_CT_NONE;
	info->atom_op				= PTL_SUM;
	info->list				= PTL_PRIORITY_LIST;

	info->user_ptr			= NULL;

	info->length			= 1;
	info->loc_offset		= 0;
	info->ack_req			= PTL_NO_ACK_REQ;
}

void run_doc(xmlDocPtr doc)
{
	int errs;
	struct node_info *info;
	xmlNode *root_element;

	info = malloc(sizeof(*info));
	if (!info) {
		printf("unable to allocate memory for node info\n");
		return;
	}
	get_maps();

	set_default_info(info);

	root_element = xmlDocGetRootElement(doc);

	errs = walk_tree(info, root_element);

	free(info);

	printf("	Total Errors %d\n\n", errs);
}
