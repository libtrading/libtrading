#define	FAST_MAX_ELEMENTS_NUMBER	32
#define	FAST_MAX_LINE_LENGTH		256

struct felem {
	struct fast_message msg;
	char buf[FAST_MAX_LINE_LENGTH];
};

struct fcontainer {
	unsigned long nr;
	unsigned long cur;
	struct felem felems[FAST_MAX_ELEMENTS_NUMBER];
};

void fcontainer_init(struct fcontainer *self,
		struct fast_field *fields, unsigned long nr_fields);
void fcontainer_free(struct fcontainer *container);
struct felem *next_elem(struct fcontainer *self);
struct felem *cur_elem(struct fcontainer *self);
struct felem *add_elem(struct fcontainer *self);
int init_elem(struct felem *elem, char *line);
struct fcontainer *fcontainer_new(void);

int script_read(FILE *stream, struct fcontainer *self);
int fmsgcmp(struct fast_message *expected, struct fast_message *actual);
void fprintmsg(FILE *stream, struct fast_message *msg);
