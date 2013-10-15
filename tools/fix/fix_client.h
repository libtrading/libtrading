enum fix_client_mode {
	FIX_CLIENT_SCRIPT,
	FIX_CLIENT_SESSION,
	FIX_CLIENT_ORDER,
};

struct fix_client_function {
	int (*fix_session_initiate)(struct fix_session_cfg *cfg, void *arg);
	enum fix_client_mode mode;
};

static int fix_client_script(struct fix_session_cfg *cfg, void *arg);
static int fix_client_session(struct fix_session_cfg *cfg, void *arg);
static int fix_client_order(struct fix_session_cfg *cfg, void *arg);
