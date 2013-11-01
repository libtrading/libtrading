enum fast_client_mode {
	FAST_CLIENT_SCRIPT,
};

struct fast_client_arg {
	const char *script;
	const char *xml;
};

struct fast_client_function {
	int (*fast_session_initiate)(struct fast_session_cfg *cfg, struct fast_client_arg *arg);
	enum fast_client_mode mode;
};

static int fast_client_script(struct fast_session_cfg *cfg, struct fast_client_arg *arg);
