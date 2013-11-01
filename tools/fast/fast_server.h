enum fast_server_mode {
	FAST_SERVER_SCRIPT,
	FAST_SERVER_PONG,
};

struct fast_server_arg {
	const char *script;
	const char *xml;
	int pongs;
};

struct fast_server_function {
	int (*fast_session_accept)(struct fast_session_cfg *cfg, struct fast_server_arg *arg);
	enum fast_server_mode mode;
};

static int fast_server_script(struct fast_session_cfg *cfg, struct fast_server_arg *arg);
static int fast_server_pong(struct fast_session_cfg *cfg, struct fast_server_arg *arg);
